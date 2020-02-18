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
    #include <unistd.h>
    #include "portaudio.h"
    #include "kiss_fftr.h"
    #include "led-matrix-c.h"
    
    #define SAMPLE_RATE  (44100)
    #define FRAMES_PER_BUFFER (2048)
    #define NFFT           (1024)
    #define NUM_MATRIX_BINS (32)
    
    /* Select sample format. */
    #define PA_SAMPLE_TYPE  paFloat32
    typedef float SAMPLE;
    
    /*******************************************************************/
    int main(void);
    int main(void)
    {
        /*******************************************************************/
        // Initialize and open PortAudio stream
        /*******************************************************************/
        PaStreamParameters  inputParameters;
        PaStream*           stream;
        PaError             err = paNoError;
        int                 numSamples;
        int                 numBytes;
    
        numSamples = FRAMES_PER_BUFFER;
        numBytes = numSamples * sizeof(SAMPLE);

        SAMPLE* audio_data;
        audio_data = (SAMPLE *) malloc(numBytes); 

        fclose(stderr);
        err = Pa_Initialize();
        if( err != paNoError ) goto done;
        inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
        inputParameters.channelCount = 1;
        inputParameters.sampleFormat = PA_SAMPLE_TYPE;
        inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultLowInputLatency;
        inputParameters.hostApiSpecificStreamInfo = NULL;

        err = Pa_OpenStream(
                    &stream,
                    &inputParameters,
                    NULL,                  /* &outputParameters, */
                    SAMPLE_RATE,
                    FRAMES_PER_BUFFER,
                    paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                    NULL,
                    NULL );
        if( err != paNoError ) goto done;
        freopen("CON", "w", stderr);

        /*******************************************************************/
        // Initialize and configure FFT
        /*******************************************************************/
        kiss_fft_scalar * buf;
        kiss_fft_cpx * bufout;
        buf=(kiss_fft_scalar*)malloc(NFFT*sizeof(SAMPLE));
        bufout=(kiss_fft_cpx*)malloc(NFFT*sizeof(SAMPLE)*2);
        float * mag;
        mag = (float*)malloc(NUM_MATRIX_BINS*sizeof(SAMPLE)/2);
        kiss_fftr_cfg cfg = kiss_fftr_alloc( NFFT ,0 ,0,0);
        
        /*******************************************************************/
        // Initialize RGB Matrix
        /*******************************************************************/
        struct RGBLedMatrixOptions options;
        struct RGBLedMatrix *matrix;
        struct LedCanvas *offscreen_canvas;
        int width, height;
        int x, y;

        memset(&options, 0, sizeof(options));
        options.rows = 32;
        options.cols = 64;
        options.chain_length = 1;

        matrix = led_matrix_create_from_options(&options, NULL, NULL);

        offscreen_canvas = led_matrix_create_offscreen_canvas(matrix);

        led_canvas_get_size(offscreen_canvas, &width, &height);
        int MATRIX_BIN_WIDTH = width/NUM_MATRIX_BINS;
        
        for (y = 0; y < height; ++y) {
            for (x = 0; x < width; ++x) {
              led_canvas_set_pixel(offscreen_canvas, x, y, 0, 0, 0);
            }
        }
        offscreen_canvas = led_matrix_swap_on_vsync(matrix, offscreen_canvas);

        float max_val;
        //int max_ind;

        /*******************************************************************/
        // Main loop
        /*******************************************************************/
        while(1) {
            
            /* Record some audio. ---------------------------------------- */
            err = Pa_StartStream( stream );
            if( err != paNoError ) goto done;
        
            err = Pa_ReadStream (stream, audio_data, FRAMES_PER_BUFFER);
            if( err != paNoError ) goto done;
            
            err = Pa_StopStream( stream );
            if( err != paNoError ) goto done;

            /* Take FFT. ------------------------------------------------- */
            for (int j=0; j<NFFT; j++) {
                buf[j] = audio_data[j]*10.0;
            }
            kiss_fftr(cfg, buf, bufout);
            
            /* Find max FFT bin and normalize----------------------------- */
            max_val = 0.001;
            //max_ind = 2;
            // Get real-sided magnitude
            for (int j=0; j<NUM_MATRIX_BINS; j++) {
                mag[j] = bufout[j+2].r*bufout[j+2].r + bufout[j+2].i*bufout[j+2].i;
                if (mag[j] > max_val) {
                    max_val = mag[j];
                    //max_ind = j;
                }
            }
            //printf("Frame %i: \tMax Freq = %i Hz\tMax value = %f\n", frame, max_ind*SAMPLE_RATE/2/NFFT*2, max_val);
            
            // Normalize
            if (max_val > 20) {
                for (int j=0; j<NFFT/2+1; j++) {
                    mag[j] = mag[j]/max_val*30.0;
                }
            } else {
                for (int j=0; j<NFFT/2+1; j++) {
                    mag[j] = 0;
                }
            }

            /* Update matrix. ------------------------------------------- */
            for (y = 0; y < height; ++y) {
                for (x = 0; x < width; x+=MATRIX_BIN_WIDTH) {
                  if (mag[x/MATRIX_BIN_WIDTH] >= y) {
                    for (int k = 0; k < MATRIX_BIN_WIDTH; ++k) {
                        if (y < 12) {
                            led_canvas_set_pixel(offscreen_canvas, x+k, 31-y, 1, 1, 100);
                        } else if (y < 22) {
                            led_canvas_set_pixel(offscreen_canvas, x+k, 31-y, 1, 100, 1);
                        } else if (y < 26) {
                            led_canvas_set_pixel(offscreen_canvas, x+k, 31-y, 80, 40, 1);
                        } else {
                            led_canvas_set_pixel(offscreen_canvas, x+k, 31-y, 50, 50, 1);
                        }
                    }
                  } else {
                    for (int k = 0; k < MATRIX_BIN_WIDTH; ++k) {
                        led_canvas_set_pixel(offscreen_canvas, x+k, 31-y, 0, 0, 0);
                    }
                  }
                }
            }
            offscreen_canvas = led_matrix_swap_on_vsync(matrix, offscreen_canvas);
            
        }
        
        err = Pa_CloseStream( stream );
        if( err != paNoError ) goto done;
        
        free(cfg);
        free(buf); free(bufout); free(mag); free(audio_data);
        led_matrix_delete(matrix);
        printf("Done!\n");
   
    done:
        Pa_Terminate();
        if( err != paNoError )
        {
            fprintf( stderr, "An error occured while using the portaudio stream\n" );
            fprintf( stderr, "Error number: %d\n", err );
            fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
            err = 1;          /* Always return 0 or 1, but no other return codes. */
        }
        return err;
   }
