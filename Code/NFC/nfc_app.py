#!/usr/bin/python3

from subprocess import PIPE, Popen

cmd = '/home/pi/Documents/GitHub/SoundCenter/Code/NFC/libnfc/libnfc-1.7.0/examples/nfc-poll'
stdout, stderr = Popen(cmd, stdout=PIPE, stderr=PIPE).communicate()
print(stdout)

infile = 'snuggle_puppy.wav'

totalsec = 14 #pre-program this for each wav file

if (stdout == "15  b0  57  c9"):
  # 0-15 values
  pitch = 8
  chorus = 0
  bend = 15

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

  cmd = ['play', 'snuggle_puppy.wav']
  if ((pitch < 7) | (pitch > 8)):
      cmd = cmd + pitch_param
  if (chorus > 0):
      cmd = cmd + chorus_param
  if (bend > 0):
      cmd = cmd + bend_param
  print(cmd)
  stdout, stderr = Popen(cmd, stdout=PIPE, stderr=PIPE).communicate()
else:
  print("wrong ID")
