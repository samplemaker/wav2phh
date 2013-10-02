/** \file pulse_height_hist.c
 * \brief Create a pulse height histogramm from a wav file
 *
 * \author Copyright (C) 2012 samplemaker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * @{
 */


/*-----------------------------------------------------------------------------
 * Includes
 *-----------------------------------------------------------------------------
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "math_util.h"

/* for windows crosscompiling one has to link against libsnd locally */
#if defined (__MINGW32__)
  #include "sndfile.h"
#else
  #include <sndfile.h>
#endif


/*-----------------------------------------------------------------------------
 * Defines
 *-----------------------------------------------------------------------------
 */

//#define PRINT_VERBOSE 1

/* number of bins in the histogram */
#define HIST_RESOLUTION 1024

/* buffer size for buffer holding the upsample pulse. the buffer size must be
   to hold the pulse plus some samples around after upsampling. note that the 
   pulse width is limited by the glitch filter */
#define PEAKBUF_MAX 1024

/* process 1048576 byte blocks: 131072 doubles */
#define NUM_LOW  131072

/* number of samples to be copied CPY_BUFFER = 2 * CPY_BUFFER_2.
 * (8192 bytes copy buffer for 2 * 512 samples)
 *
 * trigger threshold = NUM_LOW + CPY_BUFFER_2.
 * for 2 * 512 copy buffer size: 512 past-doubles + 512 future-doubles */
#define CPY_BUFFER_2 512

/*graph display *(data->stream+4)@6 */


/*-----------------------------------------------------------------------------
 * Types
 *-----------------------------------------------------------------------------
 */

/* Input data stream control for asynchrounus input data handling
 *
 * note: (numthresh ~ numlow + 0.5*numhigh) to provide 0.5*numhigh into
 *       the past and into the future. low can be as high as necessary to 
 *       improve runtime
 */
typedef struct {
  /* number of floats to be removed from buffer if buffer is rearranged */
  uint32_t num_low;
  /* number of floats to be shifted from the end to the begin of the buffer */
  uint32_t num_high;
  /* low + hgh = toal buffer size */
  uint32_t num_tot;
  /* number of floats processed in the buffer until the buffer is be rearranged */
  uint32_t num_thresh;
  /* number floats already read from input buffer */
  uint32_t num_read;
  uint32_t total_frames;
  uint32_t samplerate;
  SNDFILE *file_in;
  double *stream;
} data_t;


typedef struct {
  /* rel. trigger threshold detecting a peak */
  double trig_thresh;
  /* peaks with less than these sample points are cancelled */
  uint32_t min_glitch_filter;
  /* max_glitch_filter ~ invk*PEAKBUF_MAX */
  uint32_t max_glitch_filter;
  /* take some extra sample before rising edge and use for interpolation */
  uint8_t num_past;
} pulse_t;


typedef struct {
  double diff_thresh;
  double rel_thresh;
  double act_value;
} baseline_t;


/*-----------------------------------------------------------------------------
 * Globals
 *-----------------------------------------------------------------------------
 */

static data_t data;

/* default is no software gain */
static double soft_gain = 1.0;

/** Data stream handling
 *
 *  For the interpolation algorithm we have to provide access into the past
 *  but also into the future. Therefore a intermediate buffer is used.
 *  Once a certain portion of data from the buffer is processed the buffer
 *  section is rearranged and new data is loaded into the intermediate buffer.
 *
 *  The buffer size is num_low + num_high
 *  If more then num_thresh bytes are already processed
 *  copy num_high bytes starting at position num_low to the beginning of the
 *  buffer and fill up with fresh num_low bytes of fresh data at the position
 *  num_high.
 *
 *  *m is a pointer locating to the current position in the local buffer.
 *  *l holds the current position in the input stream (wav).
 */
inline static
void stream_handler(uint32_t *m, uint32_t *l, data_t *data){

  if (*m >= data->num_thresh){
    /* copy the upper part (num_high bytes starting at position num_low) of
       the data_stream buffer to the beginning of the data stream */
    memcpy(data->stream,
           data->stream + data->num_low,
           data->num_high * sizeof(double));
    /* one complete buffer already loaded during init */
    uint32_t frames_to_read = data->total_frames - data->num_read;
    /* if there is still enough wav data to fill up the intermediate buffer */
    if (frames_to_read > data->num_low){
      /* fill the data_stream with new samples excluding the copied area */
      sf_read_double (data->file_in,
                      data->stream + data->num_high,
                      data->num_low);
      if (soft_gain != 1.0){
        for (uint32_t i = 0; i < data->num_low; i++){
           (data->stream + data->num_high)[i] = soft_gain * (data->stream + data->num_high)[i];}
      }

      data->num_read += data->num_low;

      /* align to the correct position: m for reading the local buffer */
      *m = *m - data->num_low;
      /* align to the correct position: (l + m) position in global buffer */
      *l += data->num_low;

    }
    else{
      /* if the wav file is almost completely processed load the rest */
      sf_read_double (data->file_in,
                      data->stream + data->num_high,
                      frames_to_read);
      if (soft_gain != 1.0){
        for (uint32_t i = 0; i < frames_to_read; i++){
           (data->stream + data->num_high)[i] = soft_gain * (data->stream + data->num_high)[i];}
      }

      data->num_read += frames_to_read;

      *m = *m - data->num_low;
      *l += data->num_low;

    }
    printf("#frame: %d -> %0.1f%% completed\r", *l,
            100.0*(double)(*l)/(double)(data->total_frames));
    fflush(stdout);
  }
}

/** Initialization of the data buffer
 *
 *  Creates the intermediate sample buffer and fill the buffer for
 *  the first time.
 */
inline static
void stream_init(char *wav_file, uint32_t num_low, uint32_t num_high_2,
                 data_t *data){

  data->num_low = num_low;
  data->num_high = 2 * num_high_2;
  data->num_tot = data->num_high + data->num_low;
  data->num_thresh = num_low + num_high_2;

  SF_INFO sfinfo;
  data->file_in = sf_open(wav_file, SFM_READ, &sfinfo);

  if (data->file_in == NULL){
    printf ("error: could not open wav !\n\r");
    exit(1);
  }
  else{
    printf("opened device %s\n\r", wav_file);
  }
  printf("frames: %d samplerate: %d channels: %d\n\r", (int)sfinfo.frames,
         sfinfo.samplerate, sfinfo.channels);

  data->samplerate = sfinfo.samplerate;

  data->total_frames = sfinfo.frames;
  if (data->total_frames <= data->num_tot){
    printf("can only proceed data streams bigger than one low buffer size !\n\r");
    exit(1);
  }
  data->stream = (double *) malloc(sizeof(double) * data->num_tot);

  if (data->stream == NULL){
    printf("error: allocating memory for the data stream !\n\r");
    exit(1);
  }
  sf_read_double (data->file_in, data->stream, data->num_tot);
  if (soft_gain != 1.0){
    for (uint32_t i = 0; i < data->num_tot; i++){data->stream[i] = soft_gain * data->stream[i];}
  }

  data->num_read = data->num_tot;
}


inline static
void stream_exit(data_t *data){

  free(data->stream);
  sf_close(data->file_in);
}


inline static
void do_histogram(uint32_t *histogram,
                  uint16_t k,
                  uint16_t window_size2,
                  data_t *data,
                  pulse_t *pulse,
                  baseline_t *baseline){

  double peak_buffer[PEAKBUF_MAX];
  uint32_t dead_begin = window_size2 + pulse->num_past;
  uint32_t dead_end = data->total_frames - pulse->max_glitch_filter;
  uint32_t m = dead_begin;
  uint32_t l = 0;
  while ((l + m) < dead_end){

     stream_handler(&m, &l, data);

     /* extract baseline and calculate moving average */
     const double delta = data->stream[m] - data->stream[m + 1];
     if ((fabs(delta) < baseline->diff_thresh) &&
         (data->stream[m] < baseline->rel_thresh)){
       baseline->act_value = moving_average(data->stream[m]);
     }

     /* rising edge above a trigger threshold is found */
     if ((data->stream[m] < data->stream[m + 1]) &&
         ((data->stream[m + 1] - baseline->act_value) > pulse->trig_thresh)){
       /* get the pulse start position plus some extra samples in the past */
       const uint32_t start = m - pulse->num_past;
       /* until peak is reached */
       while ((data->stream[m] < data->stream[m + 1])){
         m ++;
         if (m >= data->num_tot){
           printf("error: input buffer size to small (%d) !\n\r",m);
           exit(1);
         }
       }
       /* get stop position := start+2*(peakpos-start) */
       const uint32_t stop = m + m - start;
       if (stop >= data->num_tot){
         printf("error: input buffer size to small (%d) !\n\r",stop);
         exit(1);
       }
       const uint32_t num_src = stop - start;
       /* the pulse width without extra samples (past & future) is */
       uint32_t pulse_width = num_src - pulse->num_past - pulse->num_past;
       /* skip glitches */
       if ((pulse_width > pulse->min_glitch_filter) &&
           (pulse_width < pulse->max_glitch_filter)) {
         m = stop;
         uint16_t num_dst = k * (num_src - 1);
         /* apply cardinal series (upsampling) */
         upsample(data->stream, peak_buffer, start, stop);
         /* get the peak maximum and minimum */
         double search_max = -1.0;
         double search_min = 1.0;
         for (uint16_t n = 0; n < num_dst; n ++){
            if (search_max < peak_buffer[n]) {
              search_max = peak_buffer[n];
            }
            if (search_min > peak_buffer[n]) {
              search_min = peak_buffer[n];
            }
         }
         /* cancel pile up: output max - min
          * note: in noisy environments it might be better to trust in
          * the baseline:
            search_max = search_max - baseline->act_value;
          */
         search_max = search_max - search_min;
         /* count the peak value into a pulse height histogram */
         /* if we compare linux vs. windows (mingw) histogram results they are somewhat different due
            to rounding issues. an extra float cast is spent to get the results identical */
         const int16_t index = (int16_t)(d2i((float)(HIST_RESOLUTION * search_max)));
         if ((index < HIST_RESOLUTION) && (index >= 0)){
            histogram[index] ++;
         }
         #ifdef PRINT_VERBOSE
         printf("event at timestamp: %f[sec]\n\r",
                0.5*(float)(stop+start)/(float)(data->samplerate));
         printf("start: %d (%0.3fms); stop: %d (%0.3fms); width: %d (%0.1fus); height: %0.3f; baseline %0.4f\n\r",
                start, 1000.0*(float)(start)/(float)(data->samplerate),
                stop, 1000.0*(float)(stop)/(float)(data->samplerate),
                pulse_width, 1000000.0*(float)(pulse_width)/(float)(data->samplerate),
                search_max, baseline->act_value);
         printf("press <enter> to print raw data dump ...\n\r");
         getchar();
         printf("source:\n\r");
         for (uint32_t a = start; a < stop;a++){
            printf("%f\n\r", data->stream[a]);
         }
         printf("destinty:\n\r");
         for (uint32_t a = 0; a < num_dst;a++){
            printf("%f\n\r", peak_buffer[a]);
         }
         printf("press <enter> to continue ...\n\r");
         getchar();
         #endif
       /* E.O. glitch */
       }
       else{
         m ++;
       }
     }
     else{
       m ++;
     }
  }
}


int
main(int argc, char ** argv)
{

  pulse_t pulse = {
    0.015, /* trigger threshold */
    2,     /* glitch filter: minimum samplepoints per pulse */
    10,    /* glitch filter: max */
    5      /* samples taken from the past */
  };

  baseline_t baseline = {
    0.005, /* differential threshold */
    0.01,  /* absolute threshold */
    0.0
  };

  uint8_t have_wav_file = 0;
  bool exit_on_error = false;
  uint16_t a = 1;
  while (a < argc){
     if ((!strcmp(argv[a], "-f")) &&  (a + 1 < argc)){
       have_wav_file = a + 1;
       a += 2;
     }
       else if ((!strcmp(argv[a], "-p")) && (a + 4 < argc)){
              pulse.trig_thresh = atof(argv[a + 1]);
              pulse.min_glitch_filter = atoi(argv[a + 2]);
              pulse.max_glitch_filter = atoi(argv[a + 3]);
              pulse.num_past = atoi(argv[a + 4]);
              a += 5;
            }
            else if ((!strcmp(argv[a], "-b")) && (a + 2 < argc)){
                   baseline.diff_thresh = atof(argv[a + 1]);
	           baseline.rel_thresh = atof(argv[a + 2]);
                   a += 3;
                 }
                 else if ((!strcmp(argv[a], "-m")) && (a + 1 < argc)){
                        soft_gain = atof(argv[a + 1]);
                        a += 2;
                      }
	              else{
                        exit_on_error = true;
	                a = argc;
                      }
  }

  if ((have_wav_file == 0) || (exit_on_error == true)){
    printf("\n\r  SYNOPSIS\n\r");
    printf("         pulse_height_hist -f wavfile [-p PULSESTRUCTURE] [-b BASELINESTRUCTURE] [-m SOFTGAIN]\n\r\n\r");
    printf("  DESCRIPTION\n\r");
    printf("         wav to pulse height histogram converter (wav_mca_demonstrator)\n\r\n\r");
    printf("  OPTIONS\n\r");
    printf("         -f wavfile\n\r");
    printf("                Specify the input file (min. 96kHz uncompressed audio file)\n\r\n\r");
    printf("         -m SOFTGAIN\n\r");
    printf("                Specify a software gain for signals with poor magnitude (default is 1.0)\n\r\n\r");
    printf("         -p PULSESTRUCTURE\n\r");
    printf("                Overwrites default pulse pattern settings:\n\r");
    printf("                -p trigthresh glitchmin glitchmax samplesfrompast\n\r\n\r");
    printf("         -b BASELINESTRUCTURE\n\r");
    printf("                Overwrites default baseline pattern settings:\n\r");
    printf("                -b diffthresh absthresh\n\r\n\r");
    printf("  EXAMPLES\n\r");
    printf("         ./pulse_height_hist -f wavfile.wav\n\r");
    printf("         ./pulse_height_hist -m 2.0 -f wavfile.wav\n\r");
    printf("         ./pulse_height_hist -f wavfile.wav -b 0.005 0.01 -p 0.015 2 10 5\n\r\n\r");
    printf("  NOTES\n\r");
    printf("         once finished you may print the output data via gnuplot:\n\r");
    printf("         ./pltHist.pl [HISTOGRAM FILE]\n\r\n\r");
    printf("  AUTHOR\n\r");
    printf("         https://github.com/samplemaker\n\r\n\r");
    exit(0);
  }

  /* const char *hist_file = argv[2]; */
  const char *hist_file = "_hist_output_.csv";
  FILE *file_out = fopen (hist_file, "w");
  if (file_out == NULL){
    printf("error: could not open histogramm output !\n\r");
    exit(1);
  }
  else{
    printf("opened default output \"%s\"\n\r", hist_file);
  }

  uint32_t histogram[HIST_RESOLUTION];
  memset(histogram, 0, sizeof(histogram[0]) * HIST_RESOLUTION);

  stream_init(argv[have_wav_file], NUM_LOW, CPY_BUFFER_2, &data);
  moving_average_init(20);

  uint16_t interpolation = 7;
  uint16_t window_size2 = 15;
  sinc_table_init(interpolation, window_size2);

  do_histogram(histogram, interpolation, window_size2, &data, &pulse, &baseline);

  printf("writing histogramm ... ");
  char out_str[256];
  rewind(file_out);

  sprintf(out_str, "Channel;Counts\n");
  const uint8_t n_length = strlen(out_str);
  fwrite(out_str, 1, n_length, file_out);

  for (uint16_t i = 0; i < HIST_RESOLUTION; i++){
    sprintf(out_str, "%d;%d\n", i, histogram[i]);
    const uint8_t n_length = strlen(out_str);
    fwrite(out_str, 1, n_length, file_out);
  }
  fclose(file_out);
  printf("done.\n\r");

  sinc_table_exit();
  moving_average_exit();
  stream_exit(&data);

  printf("\n\rfilter settings:\n\r");
  printf("  pulse trigger threshold      %0.3f\n\r", pulse.trig_thresh);
  printf("  pulse glitch filter (min)    %d\n\r", pulse.min_glitch_filter);
  printf("  pulse glitch filter (max)    %d\n\r", pulse.max_glitch_filter);
  printf("  taken from past              %d\n\r", pulse.num_past);
  printf("  baseline diff. threshold     %0.3f\n\r", baseline.diff_thresh);
  printf("  baseline trigger threshold   %0.3f\n\r", baseline.rel_thresh);
  printf("  moving average               %d\n\r", 20);
  printf("  upsampling                   %d\n\r", interpolation);
  printf("  software gain                %0.1f\n\r", soft_gain);
  printf("  window size                  %d\n\r", 2*window_size2);
  printf("  method                       cardinal series\n\r");
  printf("V01\n\r");
}
