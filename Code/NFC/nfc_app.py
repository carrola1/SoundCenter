#!/usr/bin/python3

from subprocess import PIPE, Popen
import smbus
import RPi.GPIO as GPIO

app_path = '/home/pi/Documents/GitHub/SoundCenter/Code/NFC/'
audio_path = '/home/pi/Documents/GitHub/SoundCenter/Audio/'

song_dict = {'25b057c9': 'red.wav',
             '85c4546a': 'green.wav'} 

led_pin = 13
GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)
GPIO.setup(led_pin, GPIO.OUT)
GPIO.output(led_pin, GPIO.LOW)

i2cbus = smbus.SMBus(1)
i2cbus.write_byte_data(0x44, 0x0D, 0xFF)  # set gpio expander inputs to have pull-ups

while(1):
  cmd = app_path + 'nfc-poll'
  stdout, stderr = Popen(cmd, stdout=PIPE, stderr=PIPE).communicate()
  uid = stdout.hex()
  print(uid)

  infile = song_dict.get(uid)

  totalsec = 14 #pre-program this for each wav file

  if (infile is not None): 

    infile = audio_path + infile

    GPIO.output(led_pin, GPIO.HIGH)

    pitch = (i2cbus.read_byte_data(0x44, 0x0F))^255 - 1

    # 0-14 values
    chorus = 0
    bend = 0

    # Translate Switch Inputs
    pitch_param = ['pitch', str((pitch-8)*100)]

    # chorus(gain-in, gain-out, [delay, decay, speed, depth, sine/triangle])
    chorus_param = ['chorus', '0.5', '0.9']
    for ii in range(0, max(int(chorus/3), 1)):
        if (ii % 2 == 0):
            chorus_param = chorus_param + ['{:.2f}'.format(x) for x in [(ii+1)*20, 0.3+0.03*(ii-2), 0.25+0.05*(ii-2), 1.6+0.2*(ii-2)]] + ['-t']
        else:
            chorus_param = chorus_param + ['{:.2f}'.format(x) for x in [(ii+1)*20, 0.3+0.03*(ii-2), 0.25+0.05*(ii-2), 2]] + ['-s']

    # bend([delay, freq_shift, duration])
    num_bends = max(int((bend+1)/2), 1) # up to 8 bends
    bend_duration = (totalsec - 0.1)/num_bends/2
    bend_param = ['gain', '-5', 'bend', '-f', '80', '-o', '4']
    for ii in range(0, num_bends):
        if (ii % 2 == 0):
            bend_param = bend_param + [','.join(['{:.2f}'.format(x) for x in [(ii+1)*bend_duration/2, 300*(ii+1), bend_duration]])]
            ','.join(bend_param)
        else:
            bend_param = bend_param + [','.join(['{:.2f}'.format(x) for x in [(ii+1)*bend_duration/2, -300*(ii+1), bend_duration]])]

    cmd = ['play', infile]
    if ((pitch < 7) | (pitch > 8)):
        cmd = cmd + pitch_param
    if (chorus > 0):
        cmd = cmd + chorus_param
    if (bend > 0):
        cmd = cmd + bend_param
    stdout, stderr = Popen(cmd, stdout=PIPE, stderr=PIPE).communicate()

    GPIO.output(led_pin, GPIO.LOW)
