#!/usr/bin/python3

from subprocess import PIPE, Popen
import smbus
import RPi.GPIO as GPIO

app_path = '/home/pi/Documents/GitHub/SoundCenter/Code/NFC/'
audio_path = '/home/pi/Documents/GitHub/SoundCenter/Audio/'

song_dict = {'b5cc57c9': '01_Blues_Clues.wav',
             '85c4546a': '02_Bubble_Guppies.wav',
             '055f57c9': '03_Paw_Patrol.wav',
             '8bcc3e7e': '04_Pete_The_Cat.wav',
             '15f557c9': '05_Storybots.wav',
             '35a657c9': '06_Elsa.wav',
             'e57457c9': '07_Elmo.wav',
             'e56a57c9': '08_Snuggle_Puppy.wav'}

# Setup GPIO
GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)

# Setup audio wake output
wake_pin = 10
GPIO.setup(wake_pin, GPIO.OUT)
GPIO.output(wake_pin, GPIO.HIGH)

# Setup bend rotary switch
echo_sw_pins = [4, 17, 27, 22]
for pin in echo_sw_pins:
    GPIO.setup(pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)

# Setup I2C bus and GPIO Expander
i2cbus = smbus.SMBus(1)
i2cbus.write_byte_data(0x44, 0x0D, 0xFF)  # set gpio expander inputs to have pull-ups

uid = None

while(1):
  cmd = app_path + 'nfc-poll'
  stdout, stderr = Popen(cmd, stdout=PIPE, stderr=PIPE).communicate()
  uid_new = stdout.hex()
  print(uid_new)

  if (uid != uid_new):
    infile = song_dict.get(uid_new)
  else:
    infile = None
  uid = uid_new

  if (infile is not None): 

    infile = audio_path + infile

    GPIO.output(wake_pin, GPIO.LOW)

    exp_switches = i2cbus.read_byte_data(0x44, 0x0F)
    chorus = (exp_switches >> 4)^0x0F - 1
    pitch = (exp_switches & 0x0F)^0x0F - 1
    echo = 0
    pow2 = 0
    for pin in echo_sw_pins:
        echo = echo + (GPIO.input(pin)^1)*2**pow2
        pow2 = pow2 + 1
    echo = echo - 1

    # Translate Switch Inputs
    pitch_param = ['pitch', str((pitch-8)*100)]

    # chorus(gain-in, gain-out, [delay, decay, speed, depth, sine/triangle])
    chorus_param = ['chorus', '0.5', '0.9']
    for ii in range(0, max(int(chorus/3), 1)):
        if (ii % 2 == 0):
            chorus_param = chorus_param + ['{:.2f}'.format(x) for x in [(ii+1)*20, 0.3+0.03*(ii-2), 0.25+0.05*(ii-2), 1.6+0.2*(ii-2)]] + ['-t']
        else:
            chorus_param = chorus_param + ['{:.2f}'.format(x) for x in [(ii+1)*20, 0.3+0.03*(ii-2), 0.25+0.05*(ii-2), 2]] + ['-s']

    # echo(gain-in, gain-out, [delay, decay])
    echo_param = ['echo', '0.9', '0.95']
    if (echo < 9):
        echo_param = echo_param + ['{:.2f}'.format(x) for x in [echo*100, 0.4]]
    else:
        echo_param = echo_param + ['{:.2f}'.format(x) for x in [800, 0.4]]
        echo_param = echo_param + ['{:.2f}'.format(x) for x in [echo*100, 0.3]]

    cmd = ['play', infile, 'gain', '-15']
    if ((pitch < 7) | (pitch > 8)):
        cmd = cmd + pitch_param
    if (chorus > 0):
        cmd = cmd + chorus_param
    if (echo > 0):
        cmd = cmd + echo_param
    stdout, stderr = Popen(cmd, stdout=PIPE, stderr=PIPE).communicate()

    GPIO.output(wake_pin, GPIO.HIGH)
