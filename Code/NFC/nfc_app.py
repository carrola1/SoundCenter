#!/usr/bin/python3

import audioread
from subprocess import PIPE, Popen
import shlex

cmd = '/home/pi/Documents/GitHub/SoundCenter/Code/NFC/libnfc/libnfc-1.7.0/examples/nfc-poll'
stdout, stderr = Popen(cmd, stdout=PIPE, stderr=PIPE).communicate()
print(stdout)

infile = 'snuggle_puppy.wav'

with audioread.audio_open(infile) as f:
    totalsec = f.duration

# 0-15 values
pitch = 12
chorus = 0
bend = 0

# Translate Switch Inputs
pitch_param = (pitch-8)*100

# chorus(gain-in, gain-out, [delay, decay, speed, depth, sine/triangle])
chorus_param = [0]*max(int(chorus/3), 1)
for ii in range(0, max(int(chorus/3), 1)):
    if (ii % 2 == 0):
        chorus_param[ii] = [(ii+1)*20, 0.3+0.03*(ii-2), 0.25+0.05*(ii-2), 1.6+0.2*(ii-2), 't']
    else:
        chorus_param[ii] = [(ii+1)*20, 0.3+0.03*(ii-2), 0.25+0.05*(ii-2), 2, 's']

# bend([delay, freq_shift, duration])
num_bends = max(int((bend+1)/2), 1) # up to 8 bends
bend_duration = (totalsec - 0.1)/num_bends/2
bend_param = [0]*num_bends
for ii in range(0, num_bends):
    if (ii % 2 == 0):
        bend_param[ii] = [(ii+1)*bend_duration/2, 300*(ii+1), bend_duration]
    else:
        bend_param[ii] = [(ii+1)*bend_duration/2, -300*(ii+1), bend_duration]
    bend_param[ii] = [str(jj) for jj in bend_param[ii]]

if (chorus > 0):
    fx.chorus(0.5, 0.9, chorus_param)
if (overdrive > 0):
    fx.overdrive(overdrive_param)
    fx.vol(-overdrive_param/2, 'dB')
if (bend > 0):
    fx.bend(bend_param)

cmd = shlex.split('play -N -V1 snuggle_puppy.wav -t f32 -r 44100 -c 2 - pitch 400 82 14.68 12',posix=False)
stdout, stderr = Popen(cmd, stdout=PIPE, stderr=PIPE).communicate()