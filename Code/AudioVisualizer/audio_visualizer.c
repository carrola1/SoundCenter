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
    #include <math.h>
    #include <unistd.h>
    #include <signal.h>
    #include "portaudio.h"
    #include "kiss_fftr.h"
    #include "led-matrix-c.h"
    #include "pigpio.h"
    
    #define SAMPLE_RATE  (44100)
    #define FRAMES_PER_BUFFER (2048)
    #define NFFT (1024)
    #define NUM_MATRIX_BINS (32)
    #define MA_FILT_LEN (5)
    #define GAIN_ADJ_MUSIC (50)
    #define GAIN_ADJ_NORM (15)
    
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
        PaError             err = paNoError;
        PaStream*           stream;
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
        float mag[NUM_MATRIX_BINS];
        kiss_fftr_cfg cfg = kiss_fftr_alloc( NFFT ,0 ,0,0);

        /*******************************************************************/
        // Initialize and configure moving average
        /*******************************************************************/
        float mag_sum[NUM_MATRIX_BINS];
        float mag_filt[NUM_MATRIX_BINS];
        for (int j=0; j<NUM_MATRIX_BINS; j++) {
          mag_sum[j] = 0.0;
        }

        float mag_fifo[MA_FILT_LEN][NUM_MATRIX_BINS];
        for (int i=0; i<MA_FILT_LEN; i++) {
          for (int j=0; j<NUM_MATRIX_BINS; j++) {
            mag_fifo[i][j] = 0.0;
          }
        }

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
        int max_ind;

        /*******************************************************************/
        // Initizalize GPIO
        /*******************************************************************/
        gpioInitialise();
        gpioSetMode(25, PI_INPUT);
        //gpioSetPullUpDown(25, PI_PUD_UP);

        /*******************************************************************/
        // Main loop
        /*******************************************************************/
        while(1) {
            if (gpioRead(25) == 0) {
                for (int frame = 0; frame < 10000; frame++) {
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
                    max_ind = 2;
                    // Get real-sided magnitude
                    for (int j=0; j<NUM_MATRIX_BINS; j++) {
                        mag[j] = (float)(20.0*log10(bufout[j+2].r*bufout[j+2].r + bufout[j+2].i*bufout[j+2].i));
                        if (mag[j] > max_val) {
                            max_val = mag[j];
                            max_ind = j;
                        }
                        mag_sum[j] = mag_sum[j] + mag[j] - mag_fifo[MA_FILT_LEN-1][j];
                        mag_filt[j] = mag_sum[j]/(float)MA_FILT_LEN;
                    }

                    // Shift moving average fifo
                    for (int i=MA_FILT_LEN-1; i>0; i--) {
                      for (int j=0; j<NUM_MATRIX_BINS; j++) {
                        mag_fifo[i][j] = mag_fifo[i-1][j];
                      }
                    }
                    for (int j=0; j<NUM_MATRIX_BINS; j++) {
                      mag_fifo[0][j] = mag[j];
                    }

                    //printf(f, "Frame %i: \tMax Freq = %i Hz\tMax value = %f\n", frame, max_ind*SAMPLE_RATE/2/NFFT*2, max_val);
                    
                    // Normalize
                    float mag_adj;
                    if (gpioRead(25) == 0) {
                        mag_adj = (float)GAIN_ADJ_MUSIC;
                    } else {
                        mag_adj = (float)GAIN_ADJ_NORM;
                    }
                    for (int j=0; j<NUM_MATRIX_BINS; j++) {
                        mag_filt[j] = mag_filt[j] - mag_adj;
                    }

                    /* Update matrix. ------------------------------------------- */
                    for (y = 0; y < height; ++y) {
                        for (x = 0; x < width; x+=MATRIX_BIN_WIDTH) {
                            if (mag_filt[x/MATRIX_BIN_WIDTH] >= y) {
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

                /* Shut off matrix after loop. ---------------------------------- */
                for (y = 0; y < height; ++y) {
                    for (x = 0; x < width; ++x) {
                        led_canvas_set_pixel(offscreen_canvas, x, 31-y, 0, 0, 0);
                    }
                }
                offscreen_canvas = led_matrix_swap_on_vsync(matrix, offscreen_canvas);
            }
        }
        
        err = Pa_CloseStream( stream );
        if( err != paNoError ) goto done;
        
        free(cfg);
        free(buf); free(bufout); free(audio_data);
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
