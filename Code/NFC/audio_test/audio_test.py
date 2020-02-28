from pysndfx import AudioEffectsChain
from playsound import playsound
import audioread

infile = 'snuggle_puppy.wav'
outfile = 'snuggle_puppy_processed.wav'

with audioread.audio_open(infile) as f:
    totalsec = f.duration
print(totalsec)

fx = (
    AudioEffectsChain()
    .pitch(0)
    #.reverb()
    #.phaser(0.6, 0.66, 3, 0.6, 2, True)
    #.chorus(0.5, 0.9, [[50, 0.4, 0.25, 2, 't'], [60, 0.32, 0.4, 2.3, 't'], [40, 0.3, 0.3, 1.3, 's']])
    #.overdrive(20, 20)
    #.bend([['.35', '180', '2'], ['2', '740', '1'], ['4', '-500', '2'], ['0', '-520', '1']])
    .bend([['0', '520', str(totalsec/4)], ['4', '-520', str(totalsec/4)]])

)

# Apply phaser and reverb directly to an audio file.
fx(infile)

#playsound(outfile)
