/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2012 - Hans-Kristian Arntzen
 * 
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <audio/audio_resampler.h>
#include <retro_inline.h>

#define PHASE_BITS 8
#define SUBPHASE_BITS 15

#define PHASES (1 << PHASE_BITS)
#define PHASES_SHIFT (SUBPHASE_BITS)
#define SUBPHASES (1 << SUBPHASE_BITS)
#define SUBPHASES_SHIFT 0
#define SUBPHASES_MASK ((1 << SUBPHASE_BITS) - 1)
#define PHASES_WRAP (1 << (PHASE_BITS + SUBPHASE_BITS))
#define FRAMES_SHIFT (PHASE_BITS + SUBPHASE_BITS)

#define SIDELOBES 8
#define TAPS (SIDELOBES * 2)
#define CUTOFF 1.0

#define PHASE_INDEX 0
#define DELTA_INDEX 1

typedef struct rarch_sinc_fixed_resampler
{
   int16_t phase_table[PHASES][2][TAPS];
   int16_t buffer_l[2 * TAPS];
   int16_t buffer_r[2 * TAPS];

   unsigned ptr;
   uint32_t time;
} rarch_sinc_fixed_resampler_t;

static INLINE double __sinc(double val)
{
   if (fabs(val) < 0.00001)
      return 1.0;
   return sin(val) / val;
}

static INLINE double lanzcos(double index)
{
   return __sinc(index);
}

static void init_sinc_table(rarch_sinc_fixed_resampler_t *resamp)
{
   unsigned i, j;

   /* Sinc phases: [..., p + 3, p + 2, p + 1, p + 0, p - 1, p - 2, p - 3, p - 4, ...] */
   for (i = 0; i < PHASES; i++)
   {
      for (j = 0; j < TAPS; j++)
      {
         double p = (double)i / PHASES;
         double sinc_phase = M_PI * (p + (SIDELOBES - 1 - j));

         float val = CUTOFF * __sinc(CUTOFF * sinc_phase) * lanzcos(sinc_phase / SIDELOBES);
         resamp->phase_table[i][PHASE_INDEX][j] = (int16_t)(val * 0x7fff);
      }
   }

   /* Optimize linear interpolation. */
   for (i = 0; i < PHASES - 1; i++)
   {
      for (j = 0; j < TAPS; j++)
      {
         resamp->phase_table[i][DELTA_INDEX][j] =
            (resamp->phase_table[i + 1][PHASE_INDEX][j] - resamp->phase_table[i][PHASE_INDEX][j]);
      }
   }

   /* Interpolation between [PHASES - 1] => [PHASES]  */
   for (j = 0; j < TAPS; j++)
   {
      double p = 1.0;
      double sinc_phase = M_PI * (p + (SIDELOBES - 1 - j));
      double phase = CUTOFF * __sinc(CUTOFF * sinc_phase) * lanzcos(sinc_phase / SIDELOBES);

      int16_t result = 0x7fff * phase - resamp->phase_table[PHASES - 1][PHASE_INDEX][j];

      resamp->phase_table[PHASES - 1][DELTA_INDEX][j] = result;
   }
}

/* No memalign() for us on Win32 ... */
static void *aligned_alloc__(size_t boundary, size_t size)
{
   void *ptr = malloc(boundary + size + sizeof(uintptr_t));
   if (!ptr)
      return NULL;

   uintptr_t addr = ((uintptr_t)ptr + sizeof(uintptr_t) + boundary) & ~(boundary - 1);
   void **place   = (void**)addr;
   place[-1]      = ptr;

   return (void*)addr;
}

static void aligned_free__(void *ptr)
{
   void **p = (void**)ptr;
   free(p[-1]);
}

static void *resampler_sinc_fixed_new(const struct resampler_config *config,
      double bandwidth_mod, enum resampler_quality quality,
      resampler_simd_mask_t mask)
{
   rarch_sinc_fixed_resampler_t *re = (rarch_sinc_fixed_resampler_t*)aligned_alloc__(16, sizeof(*re));
   if (!re)
      return NULL;

   memset(re, 0, sizeof(*re));

   init_sinc_table(re);

   return re;
}

static INLINE int16_t sinc_fixed_saturate(int32_t val)
{
   if (val > 0x7fff)
      return 0x7fff;
   else if (val < -0x8000)
      return -0x8000;
   return val;
}

static void process_sinc_fixed(rarch_sinc_fixed_resampler_t *resamp,
      int16_t *out_buffer)
{
   unsigned i;
   int32_t sum_l = 0;
   int32_t sum_r = 0;
   const int16_t *buffer_l = resamp->buffer_l + resamp->ptr;
   const int16_t *buffer_r = resamp->buffer_r + resamp->ptr;

   unsigned phase = resamp->time >> PHASES_SHIFT;
   unsigned delta = (resamp->time >> SUBPHASES_SHIFT) & SUBPHASES_MASK;

   const int16_t *phase_table = resamp->phase_table[phase][PHASE_INDEX];
   const int16_t *delta_table = resamp->phase_table[phase][DELTA_INDEX];

   for (i = 0; i < TAPS; i++)
   {
      int16_t sinc_val = phase_table[i] + ((delta * delta_table[i] + 0x4000) >> 15);
      sum_l           += (buffer_l[i] * sinc_val + 0x4000) >> 15;
      sum_r           += (buffer_r[i] * sinc_val + 0x4000) >> 15;
   }

   out_buffer[0]       = sinc_fixed_saturate(sum_l);
   out_buffer[1]       = sinc_fixed_saturate(sum_r);
}

static void resampler_sinc_fixed_process(void *re_,
      struct resampler_data *data)
{
   rarch_sinc_fixed_resampler_t *re = (rarch_sinc_fixed_resampler_t*)re_;
   uint32_t ratio = PHASES_WRAP / data->ratio;

   const int16_t *input  = data->data_in;
   int16_t *output       = data->data_out;
   size_t frames         = data->input_frames;
   size_t out_frames     = 0;

   while (frames)
   {
      process_sinc_fixed(re, output);
      output += 2;
      out_frames++;

      re->time += ratio;
      while (re->time >= PHASES_WRAP)
      {
         re->buffer_l[re->ptr + TAPS] = re->buffer_l[re->ptr] = *input++;
         re->buffer_r[re->ptr + TAPS] = re->buffer_r[re->ptr] = *input++;
         re->ptr = (re->ptr + 1) & (TAPS - 1);

         re->time -= PHASES_WRAP;
         frames--;
      }
   }

   data->output_frames = out_frames;
}

static void resampler_sinc_fixed_free(void *data)
{
   rarch_sinc_fixed_resampler_t *resamp = (rarch_sinc_fixed_resampler_t*)data;
   aligned_free__(resamp);
}

retro_resampler_t sinc_resampler = {
   resampler_sinc_fixed_new,
   resampler_sinc_fixed_process,
   resampler_sinc_fixed_free,
   RESAMPLER_API_VERSION,
   "sinc",
   "sinc"
};
