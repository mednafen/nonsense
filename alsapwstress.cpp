// g++ -Wall -O2 -o alsapwstress alsapwstress.cpp -lasound -lm && ./alsapwstress
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#undef NDEBUG
#include <assert.h>

#include <poll.h>
#include <sys/time.h>
#include <time.h>
#include <alsa/asoundlib.h>
#include <unistd.h>

typedef signed char int8;
typedef unsigned char uint8;

typedef int16_t int16;
typedef uint16_t uint16;

typedef int32_t int32;
typedef uint32_t uint32;

typedef int64_t int64;
typedef uint64_t uint64;

static snd_pcm_t *alsa_pcm;

static uint64 GetTime(void)
{
 struct timespec tp;
 uint64 ret;

 if(clock_gettime(CLOCK_MONOTONIC, &tp) == -1)
 {
  assert(0);
 }

 ret = tp.tv_sec * 1000 * 1000 + tp.tv_nsec / 1000;

 return ret;
}

static void SpinWait(uint64 us)
{
 uint64 st = GetTime();
 static volatile double v;
 unsigned i;

 v = 0;
 i = 0;

 while(GetTime() < (st + us))
 {
  v += sin(i + v);
  i = (i + 1) & 2047;
 }
}

static void ALSA_Write(const int16* data, uint32 len)
{
 while(len > 0)
 {
  int towrite = len;
  int written = 0;

  do
  {
   written = snd_pcm_writei(alsa_pcm, data, towrite);

   assert(written <= towrite);

   if(written <= 0)
   {
    switch(snd_pcm_state(alsa_pcm))
    {
     default: puts("UNKNOWN STATE");

	      if(written < 0)
	       written = towrite;
	      break;

     case SND_PCM_STATE_OPEN:
     case SND_PCM_STATE_DISCONNECTED:
		puts("OPEN/DISCONNECTED");
		written = towrite;
		break;

     case SND_PCM_STATE_XRUN:
		puts("XRUN");
		snd_pcm_prepare(alsa_pcm);
		break;

     case SND_PCM_STATE_SETUP:
		puts("SETUP");
		snd_pcm_prepare(alsa_pcm);
		break;

     case SND_PCM_STATE_SUSPENDED:
		puts("SUSPENDED");
		snd_pcm_prepare(alsa_pcm);
		break;
    }
   }
   else if(written < towrite)
    printf("written(%u) < towrite(%u)!\n", written, towrite);
  } while(written <= 0);

  data += written;
  len -= written;

  if(snd_pcm_state(alsa_pcm) == SND_PCM_STATE_PREPARED)
   snd_pcm_start(alsa_pcm);
 }
}

static void ALSA_Close(void)
{
 if(alsa_pcm)
 {
  snd_pcm_close(alsa_pcm);
  alsa_pcm = nullptr;
 }
}

#define ALSA_INIT_CLEANUP	\
         if(hw_params) { snd_pcm_hw_params_free(hw_params); hw_params = nullptr; }	\
         if(alsa_pcm) { snd_pcm_close(alsa_pcm); alsa_pcm = nullptr; }

#define ALSA_TRY(func) { 	\
	int error; 	\
	error = func; 	\
	if(error < 0) 	\
	{ 		\
	 printf("ALSA Error: %s %s\n", #func, snd_strerror(error)); 	\
	 ALSA_INIT_CLEANUP	\
	 return false; 		\
	} }

static const uint32 desired_ps = 64;
static const uint32 desired_bs = 16384;
static const unsigned desired_rate = 384000;


static bool ALSA_Open(const char *id)
{
 snd_pcm_hw_params_t* hw_params = nullptr;
 snd_pcm_sw_params_t* sw_params = nullptr;
 unsigned rate = desired_rate;
 unsigned channels = 1;

 ALSA_TRY(snd_pcm_open(&alsa_pcm, "default", SND_PCM_STREAM_PLAYBACK, 0));
 ALSA_TRY(snd_pcm_hw_params_malloc(&hw_params));
 ALSA_TRY(snd_pcm_sw_params_malloc(&sw_params));

 ALSA_TRY(snd_pcm_hw_params_any(alsa_pcm, hw_params));
 ALSA_TRY(snd_pcm_hw_params_set_periods_integer(alsa_pcm, hw_params));

 ALSA_TRY(snd_pcm_hw_params_set_access(alsa_pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED));

 ALSA_TRY(snd_pcm_hw_params_set_format(alsa_pcm, hw_params, SND_PCM_FORMAT_S16));

 ALSA_TRY(snd_pcm_hw_params_set_rate_near(alsa_pcm, hw_params, &rate, 0));

 //
 // Set number of channels
 //
 ALSA_TRY(snd_pcm_hw_params_set_channels_near(alsa_pcm, hw_params, &channels));

 {
  snd_pcm_uframes_t tmpps = desired_ps;
  int dir = 0;

  snd_pcm_hw_params_set_period_size_near(alsa_pcm, hw_params, &tmpps, &dir);
 }

 snd_pcm_uframes_t tmp_uft = desired_bs;
 ALSA_TRY(snd_pcm_hw_params_set_buffer_size_near(alsa_pcm, hw_params, &tmp_uft));

 ALSA_TRY(snd_pcm_hw_params(alsa_pcm, hw_params));
 snd_pcm_uframes_t buffer_size, period_size, avail_min;
 unsigned int periods;

 ALSA_TRY(snd_pcm_hw_params_get_period_size(hw_params, &period_size, NULL));
 ALSA_TRY(snd_pcm_hw_params_get_periods(hw_params, &periods, NULL));
 snd_pcm_hw_params_free(hw_params);

 ALSA_TRY(snd_pcm_sw_params_current(alsa_pcm, sw_params));

 ALSA_TRY(snd_pcm_sw_params_get_avail_min(sw_params, &avail_min));

 ALSA_TRY(snd_pcm_sw_params(alsa_pcm, sw_params));
 snd_pcm_sw_params_free(sw_params);

 buffer_size = period_size * periods;

 printf("Rate: %u\n", rate);
 printf("Channels: %u\n", channels);
 printf("Buffer size: %llu (%.3f ms)\n", (unsigned long long)buffer_size, 1e3 * buffer_size / rate);
 printf("Period size: %llu (%.3f ms)\n", (unsigned long long)period_size, 1e3 * period_size / rate);
 printf("Avail min: %llu (%.3f ms)\n", (unsigned long long)avail_min, 1e3 * avail_min / rate);

 assert(rate == desired_rate);
 assert(channels == 1);
 assert(buffer_size == desired_bs);
 assert(period_size == desired_ps);
 assert(avail_min == period_size);

 ALSA_TRY(snd_pcm_prepare(alsa_pcm));

 return true;
}

int main(int argc, char* argv[])
{
 int16 tmp[desired_ps * 2];
 const size_t tmp_count = sizeof(tmp) / sizeof(tmp[0]);

 if(!ALSA_Open("default"))
  return -1;

 srand(0xDEADBEEF);

 for(unsigned i = 0; i < tmp_count; i++)
  tmp[i] = ((i >= tmp_count / 2) ? 2048 : -2048);

 for(;;)
 {
  SpinWait((rand() >> 16) & 511);

  {
   const uint64 st = GetTime();
   uint32 elapsed_ms;

   ALSA_Write(tmp, tmp_count);

   elapsed_ms = (GetTime() - st) / 1000;
   if(elapsed_ms >= 10)
    printf("ALSA_Write() took %u ms to write %.3f ms samples!\n", elapsed_ms, 1e3 * tmp_count / desired_rate);
  }
 }

 ALSA_Close();

 return 0;
}

