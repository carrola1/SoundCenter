/*
     * $Id$
     *
     * This program uses the PortAudio Portable Audio Library.
     * For more information see: http://www.portaudio.com
     * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
     *
     * Permission is hereby granted, free of charge, to any person obtaining
     * a copy of this software and associated documentation files
     * (the "Software"), to deal in the Software without restriction,
     * including without limitation the rights to use, copy, modify, merge,
     * publish, distribute, sublicense, and/or sell copies of the Software,
     * and to permit persons to whom the Software is furnished to do so,
     * subject to the following conditions:
     *
     * The above copyright notice and this permission notice shall be
     * included in all copies or substantial portions of the Software.
     *
     * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
     * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
     * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
     * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
     * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
     * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
     * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
     */
    
    /*
     * The text above constitutes the entire PortAudio license; however, 
     * the PortAudio community also makes the following non-binding requests:
     *
     * Any person wishing to distribute modifications to the Software is
     * requested to send the modifications to the original developer so that
     * they can be incorporated into the canonical version. It is also 
     * requested that these non-binding requests be included along with the 
     * license above.
     */
    
    #include <stdio.h>
    #include <stdlib.h>
    #include "portaudio.h"
    #include "kiss_fftr.h"
    
    /* #define SAMPLE_RATE  (17932) // Test failure to open with this value. */
    #define SAMPLE_RATE  (44100)
    #define FRAMES_PER_BUFFER (512)
    #define NUM_SECONDS     (0.2)
    /* #define DITHER_FLAG     (paDitherOff) */
    #define DITHER_FLAG     (0) 
    
    #define WRITE_TO_FILE   (0)

    #define NFFT           (256)
    
    /* Select sample format. */
    #if 1
    #define PA_SAMPLE_TYPE  paFloat32
    typedef float SAMPLE;
    #define SAMPLE_SILENCE  (0.0f)
    #define PRINTF_S_FORMAT "%.8f"
    #elif 1
    #define PA_SAMPLE_TYPE  paInt16
    typedef short SAMPLE;
    #define SAMPLE_SILENCE  (0)
    #define PRINTF_S_FORMAT "%d"
    #elif 0
    #define PA_SAMPLE_TYPE  paInt8
    typedef char SAMPLE;
    #define SAMPLE_SILENCE  (0)
    #define PRINTF_S_FORMAT "%d"
    #else
    #define PA_SAMPLE_TYPE  paUInt8
    typedef unsigned char SAMPLE;
    #define SAMPLE_SILENCE  (128)
    #define PRINTF_S_FORMAT "%d"
    #endif
    
    typedef struct
    {
        int          frameIndex;  /* Index into sample array. */
        int          maxFrameIndex;
        SAMPLE      *recordedSamples;
    }
    paTestData;
    
    /* This routine will be called by the PortAudio engine when audio is needed.
    ** It may be called at interrupt level on some machines so don't do anything
    ** that could mess up the system like calling malloc() or free().
    */
    static int recordCallback( const void *inputBuffer, void *outputBuffer,
                               unsigned long framesPerBuffer,
                               const PaStreamCallbackTimeInfo* timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void *userData )
    {
        paTestData *data = (paTestData*)userData;
        const SAMPLE *rptr = (const SAMPLE*)inputBuffer;
        SAMPLE *wptr = &data->recordedSamples[data->frameIndex];
        long framesToCalc;
        long i;
        int finished;
        unsigned long framesLeft = data->maxFrameIndex - data->frameIndex;
    
        (void) outputBuffer; /* Prevent unused variable warnings. */
        (void) timeInfo;
        (void) statusFlags;
        (void) userData;
    
        if( framesLeft < framesPerBuffer )
        {
            framesToCalc = framesLeft;
            finished = paComplete;
        }
        else
        {
            framesToCalc = framesPerBuffer;
            finished = paContinue;
        }
    
        if( inputBuffer == NULL )
        {
            for( i=0; i<framesToCalc; i++ )
            {
                *wptr++ = SAMPLE_SILENCE;  /* left */
            }
        }
        else
        {
            for( i=0; i<framesToCalc; i++ )
            {
                *wptr++ = *rptr++;  /* left */
            }
        }
        data->frameIndex += framesToCalc;
        return finished;
    }
   
    /*******************************************************************/
    int main(void);
    int main(void)
    {
        PaStreamParameters  inputParameters;
        PaStream*           stream;
        PaError             err = paNoError;
        paTestData          data;
        int                 i;
        int                 totalFrames;
        int                 numSamples;
        int                 numBytes;
    
        //printf("patest_record.c\n"); fflush(stdout);
    
        data.maxFrameIndex = totalFrames = NUM_SECONDS * SAMPLE_RATE; /* Record for a few seconds. */
        data.frameIndex = 0;
        numSamples = totalFrames;
        numBytes = numSamples * sizeof(SAMPLE);
        data.recordedSamples = (SAMPLE *) malloc( numBytes ); /* From now on, recordedSamples is initialised. */
        if( data.recordedSamples == NULL )
        {
            printf("Could not allocate record array.\n");
            goto done;
        }
        for( i=0; i<numSamples; i++ ) data.recordedSamples[i] = 0;
    
        err = Pa_Initialize();
        if( err != paNoError ) goto done;

        int numDevices = Pa_GetDeviceCount();
        //fprintf(stderr,"Num devices=%i\n", numDevices);
        inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
        if (inputParameters.device == paNoDevice) {
            fprintf(stderr,"Error: No default input device.\n");
            goto done;
        }
        inputParameters.channelCount = 1;
        inputParameters.sampleFormat = PA_SAMPLE_TYPE;
        inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
        inputParameters.hostApiSpecificStreamInfo = NULL;
    
        /* Record some audio. -------------------------------------------- */
        err = Pa_OpenStream(
                    &stream,
                    &inputParameters,
                    NULL,                  /* &outputParameters, */
                    SAMPLE_RATE,
                    FRAMES_PER_BUFFER,
                    paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                    recordCallback,
                    &data );
        if( err != paNoError ) goto done;
    
        err = Pa_StartStream( stream );
        if( err != paNoError ) goto done;
        //printf("\n=== Now recording!! Please speak into the microphone. ===\n"); fflush(stdout);
    
        while( ( err = Pa_IsStreamActive( stream ) ) == 1 )
        {
            Pa_Sleep(10);
            //printf("index = %d\n", data.frameIndex ); fflush(stdout);
        }
        if( err < 0 ) goto done;
    
        err = Pa_CloseStream( stream );
        if( err != paNoError ) goto done;

        //Take FFT
        kiss_fft_scalar * buf;
        kiss_fft_cpx * bufout;
        buf=(kiss_fft_scalar*)malloc(NFFT);
        bufout=(kiss_fft_cpx*)malloc(NFFT*2);
        float * mag;
        mag = (float*)malloc(NFFT/2+1);
        int max_val = 0;
        int max_ind = 0;

        for (int i=0; i<NFFT; i++) {
            buf[i] = data.recordedSamples[i]*10.0;
        }

        kiss_fftr_cfg cfg = kiss_fftr_alloc( NFFT ,0 ,0,0);
        kiss_fftr(cfg, buf, bufout);

        // Get real-sided magnitude
        for (int i=0; i<NFFT/2+1; i++) {
            mag[i] = bufout[i].r*bufout[i].r + bufout[i].i*bufout[i].i;
            if (mag[i] > max_val) {
                max_val = mag[i];
                max_ind = i;
            }
        }
        printf("Max Freq = %i Hz\n", max_ind*SAMPLE_RATE/2/NFFT*2);
        
        free(cfg);
        free(buf); free(bufout); free(mag);
   
        /* Write recorded data to a file. */
        #if WRITE_TO_FILE
            {
                FILE  *fid;
                fid = fopen("recorded.raw", "wb");
                if( fid == NULL )
                {
                    printf("Could not open file.");
                }
                else
                {
                    fwrite( data.recordedSamples, sizeof(SAMPLE), totalFrames, fid );
                    fclose( fid );
                    printf("Wrote data to 'recorded.raw'\n");
                }
            }
        #endif
   
   done:
       Pa_Terminate();
       if( data.recordedSamples )       /* Sure it is NULL or valid. */
           free( data.recordedSamples );
       if( err != paNoError )
       {
           fprintf( stderr, "An error occured while using the portaudio stream\n" );
           fprintf( stderr, "Error number: %d\n", err );
           fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
           err = 1;          /* Always return 0 or 1, but no other return codes. */
       }
       return err;
   }