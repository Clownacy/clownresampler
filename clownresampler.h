/*
clownresampler

This is a single-file library for resampling audio. It is written in C89 and
dual-licenced under the terms of The Unlicence and the Zero-Clause BSD licence.
In particular, this library implements a windowed-sinc resampler, using a
Lanczos window.

https://github.com/Clownacy/clownresampler
*/

/*
This software is available under two licences - choose whichever you prefer:


Licence 1 - The Unlicense

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non - commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>


Licence 2 - Zero-Clause BSD

Copyright (c) 2022 Clownacy

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef CLOWNRESAMPLER_GUARD_MISC
#define CLOWNRESAMPLER_GUARD_MISC


/* Configuration */

/* Define 'CLOWNRESAMPLER_STATIC' to limit the visibility of public functions. */
/* Alternatively, define 'CLOWNRESAMPLER_API' to control the qualifiers applied to the public functions. */
#ifndef CLOWNRESAMPLER_API
 #ifdef CLOWNRESAMPLER_STATIC
  #define CLOWNRESAMPLER_API static
 #else
  #define CLOWNRESAMPLER_API
 #endif
#endif

/* Controls the number of 'lobes' of the windowed sinc function.
   A higher number results in better audio, but is more expensive. */
#ifndef CLOWNRESAMPLER_KERNEL_RADIUS
#define CLOWNRESAMPLER_KERNEL_RADIUS 3
#endif

/* How many samples to render per lobe for the pre-computed Lanczos kernel.
   Higher numbers produce a higher-quality Lanczos kernel, but cause it to take
   up more memory and cache. */
#ifndef CLOWNRESAMPLER_KERNEL_RESOLUTION
#define CLOWNRESAMPLER_KERNEL_RESOLUTION 0x400 /* 1024 samples per lobe should be more than good enough */
#endif

/* The maximum number of channels supported by the resampler.
   This will likely be removed in the future. */
#ifndef CLOWNRESAMPLER_MAXIMUM_CHANNELS
#define CLOWNRESAMPLER_MAXIMUM_CHANNELS 16 /* As stb_vorbis says, this should be enough for pretty much everyone. */
#endif


/* Header */

#include <stddef.h>

typedef struct ClownResampler_Precomputed
{
	long lanczos_kernel_table[CLOWNRESAMPLER_KERNEL_RADIUS * 2 * CLOWNRESAMPLER_KERNEL_RESOLUTION];
} ClownResampler_Precomputed;

typedef struct ClownResampler_LowLevel_State
{
	unsigned int channels;
	size_t position_integer;
	unsigned long position_fractional;            /* 16.16 fixed point. */
	unsigned long increment;                      /* 16.16 fixed point. */
	long sample_normaliser;                       /* 17.15 fixed point. */
	size_t stretched_kernel_radius;               /* 16.16 fixed point. */
	size_t integer_stretched_kernel_radius;
	size_t stretched_kernel_radius_delta;         /* 16.16 fixed point. */
	size_t kernel_step_size;
} ClownResampler_LowLevel_State;

typedef struct ClownResampler_HighLevel_State
{
	ClownResampler_LowLevel_State low_level;

	short input_buffer[0x1000];
	short *input_buffer_start;
	short *input_buffer_end;
} ClownResampler_HighLevel_State;

#endif /* CLOWNRESAMPLER_GUARD_MISC */

#if !defined(CLOWNRESAMPLER_STATIC) || defined(CLOWNRESAMPLER_IMPLEMENTATION)

#ifndef CLOWNRESAMPLER_GUARD_FUNCTION_DECLARATIONS
#define CLOWNRESAMPLER_GUARD_FUNCTION_DECLARATIONS

#ifdef __cplusplus
extern "C" {
#endif


/* Common API.
   This API is used for both the low-level and high-level APIs. */

/* Precomputes some data to improve the performance of the resampler.
   Multiple resamplers can use the same 'ClownResampler_Precomputed'.
   The output of this function is always the same, so if you want to avoid
   calling this function, then you could dump the contents of the struct and
   then insert a const 'ClownResampler_Precomputed' in your source code. */
CLOWNRESAMPLER_API void ClownResampler_Precompute(ClownResampler_Precomputed *precomputed);



/* Low-level API.
   This API has lower overhead, but is more difficult to use, requiring that
   audio be pre-processed before resampling.
   Do NOT mix low-level API calls with high-level API calls for the same
   resampler! */


/* Initialises a low-level resampler. This function must be called before the
   state is passed to any other functions. The input and output sample rates
   don't actually have to match the sample rates being used - they just need to
   provide the ratio between the two (for example, 1 and 2 works just as well
   as 22050 and 44100). Remember that a sample rate is double the frequency.
   The 'channels' parameter must not be larger than
   CLOWNRESAMPLER_MAXIMUM_CHANNELS. */
CLOWNRESAMPLER_API void ClownResampler_LowLevel_Init(ClownResampler_LowLevel_State *resampler, unsigned int channels, unsigned long input_sample_rate, unsigned long output_sample_rate, unsigned long low_pass_filter_sample_rate);

/* Adjusts properties of the resampler. The input and output sample rates don't
   actually have to match the sample rates being used - they just need to
   provide the ratio between the two (for example, 1 and 2 works just as well
   as 22050 and 44100). Remember that a sample rate is double the frequency.*/
CLOWNRESAMPLER_API void ClownResampler_LowLevel_Adjust(ClownResampler_LowLevel_State *resampler, unsigned long input_sample_rate, unsigned long output_sample_rate, unsigned long low_pass_filter_sample_rate);

/* Resamples (pre-processed) audio. The 'total_input_frames' and
   'total_output_frames' parameters measure the size of their respective
   buffers in frames, not samples or bytes.

   The input buffer must be specially pre-processed, so that it is padded with
   extra frames at the beginning and end. This is needed as the resampler will
   unavoidably read past the beginning and the end of the audio data. The
   specific number of frames needed at the beginning and end can be found in
   the 'resampler->integer_stretched_kernel_radius' variable. If the audio you
   are resampling is a chunk of a larger piece of audio, then the 'padding' at
   the beginning and end must be the frames from before and after said chunk
   of audio, otherwise these frames should just be 0. Note that these padding
   frames must not be counted by the 'total_input_frames' parameter.

   After this function returns, the 'total_input_frames' parameter will
   contain the number of frames in the input buffer that were not processed.
   Likewise, the 'total_output_frames' parameter will contain the number of
   frames in the output buffer that were not filled with resampled audio data.
*/
CLOWNRESAMPLER_API void ClownResampler_LowLevel_Resample(ClownResampler_LowLevel_State *resampler, const ClownResampler_Precomputed *precomputed, const short *input_buffer, size_t *total_input_frames, short *output_buffer, size_t *total_output_frames);



/* High-level API.
   This API has more overhead, but is easier to use.
   Do NOT mix high-level API calls with low-level API calls for the same
   resampler! */


/* Initialises a high-level resampler. This function must be called before the
   state is passed to any other functions. The input and output sample rates
   don't actually have to match the sample rates being used - they just need to
   provide the ratio between the two (for example, 1 and 2 works just as well
   as 22050 and 44100). Remember that a sample rate is double the frequency.
   The 'channels' parameter must not be larger than
   CLOWNRESAMPLER_MAXIMUM_CHANNELS. */
CLOWNRESAMPLER_API void ClownResampler_HighLevel_Init(ClownResampler_HighLevel_State *resampler, unsigned int channels, unsigned long input_sample_rate, unsigned long output_sample_rate, unsigned long low_pass_filter_sample_rate);

/* Resamples audio. This function returns when either the output buffer is
   full, or the input callback stops providing frames.

   This function returns the number of frames that were written to the output
   buffer.

   The parameters are as follows:

   'resampler'

   A pointer to a state struct that was previously initialised with the
   'ClownResampler_HighLevel_Init' function.


   'output_buffer'

   A pointer to a buffer which the resampled audio will be written to.
   The size of the audio buffer will be specified by the 'total_output_frames'
   variable.


   'total_output_frames'

   The size of the buffer specified by the 'output_buffer' parameter. The size
   is measured in frames, not samples or bytes.


   'pull_callback'

   A callback for retrieving frames of the input audio. The callback must
   write frames to the buffer pointed to by the 'buffer' parameter. The
   'buffer_size' parameter specifies the maximum number of frames that can be
   written to the buffer. The callback must return the number of frames that
   were written to the buffer. If the callback returns 0, then this function
   terminates. The 'user_data' parameter is the same as the 'user_data'
   parameter of this function.


   'user_data'
   An arbitrary pointer that is passed to the 'pull_callback' function. */
CLOWNRESAMPLER_API size_t ClownResampler_HighLevel_Resample(ClownResampler_HighLevel_State *resampler, const ClownResampler_Precomputed *precomputed, short *output_buffer, size_t total_output_frames, size_t(*pull_callback)(void *user_data, short *buffer, size_t buffer_size), void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* CLOWNRESAMPLER_GUARD_FUNCTION_DECLARATIONS */

#endif /* !defined(CLOWNRESAMPLER_STATIC) || defined(CLOWNRESAMPLER_IMPLEMENTATION) */


/* Implementation */

#ifdef CLOWNRESAMPLER_IMPLEMENTATION

#ifndef CLOWNRESAMPLER_GUARD_FUNCTION_DEFINITIONS
#define CLOWNRESAMPLER_GUARD_FUNCTION_DEFINITIONS

/* These can be used to provide your own C standard library functions. */
#ifndef CLOWNRESAMPLER_ASSERT
#include <assert.h>
#define CLOWNRESAMPLER_ASSERT assert
#endif

#ifndef CLOWNRESAMPLER_FABS
#include <math.h>
#define CLOWNRESAMPLER_FABS fabs
#endif

#ifndef CLOWNRESAMPLER_SIN
#include <math.h>
#define CLOWNRESAMPLER_SIN sin
#endif

#ifndef CLOWNRESAMPLER_MEMSET
#include <string.h>
#define CLOWNRESAMPLER_MEMSET memset
#endif

#ifndef CLOWNRESAMPLER_MEMMOVE
#include <string.h>
#define CLOWNRESAMPLER_MEMMOVE memmove
#endif

#include <stddef.h>

#define CLOWNRESAMPLER_COUNT_OF(x) (sizeof(x) / sizeof(*(x)))
#define CLOWNRESAMPLER_MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLOWNRESAMPLER_MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLOWNRESAMPLER_CLAMP(min, max, x) (CLOWNRESAMPLER_MAX((min), CLOWNRESAMPLER_MIN((max), (x))))

#define CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE (1 << 16) /* For 16.16. This is good because it reduces multiplications and divisions to mere bit-shifts. */
#define CLOWNRESAMPLER_TO_FIXED_POINT_FROM_INTEGER(x) ((x) * CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE)
#define CLOWNRESAMPLER_TO_INTEGER_FROM_FIXED_POINT_FLOOR(x) ((x) / CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE)
#define CLOWNRESAMPLER_TO_INTEGER_FROM_FIXED_POINT_ROUND(x) (((x) + (CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE / 2)) / CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE)
#define CLOWNRESAMPLER_TO_INTEGER_FROM_FIXED_POINT_CEIL(x) (((x) + (CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE - 1)) / CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE)
#define CLOWNRESAMPLER_FIXED_POINT_MULTIPLY(a, b) ((a) * (b) / CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE)

static double ClownResampler_LanczosKernel(double x)
{
	const double kernel_radius = (double)CLOWNRESAMPLER_KERNEL_RADIUS;

	const double x_times_pi = x * 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679; /* 100 digits should be good enough. */
	const double x_times_pi_divided_by_radius = x_times_pi / kernel_radius;

	/*CLOWNRESAMPLER_ASSERT(x != 0.0);*/
	if (x == 0.0)
		return 1.0;

	CLOWNRESAMPLER_ASSERT(CLOWNRESAMPLER_FABS(x) <= kernel_radius);
	/*if (CLOWNRESAMPLER_FABS(x) > kernel_radius)
		return 0.0f*/

	return (CLOWNRESAMPLER_SIN(x_times_pi) * CLOWNRESAMPLER_SIN(x_times_pi_divided_by_radius)) / (x_times_pi * x_times_pi_divided_by_radius);
}


/* Common API */

CLOWNRESAMPLER_API void ClownResampler_Precompute(ClownResampler_Precomputed *precomputed)
{
	size_t i;

	for (i = 0; i < CLOWNRESAMPLER_COUNT_OF(precomputed->lanczos_kernel_table); ++i)
		precomputed->lanczos_kernel_table[i] = (long)CLOWNRESAMPLER_TO_FIXED_POINT_FROM_INTEGER(ClownResampler_LanczosKernel(((double)i / (double)CLOWNRESAMPLER_COUNT_OF(precomputed->lanczos_kernel_table) * 2.0 - 1.0) * (double)CLOWNRESAMPLER_KERNEL_RADIUS));
}


/* Low-Level API */

CLOWNRESAMPLER_API void ClownResampler_LowLevel_Init(ClownResampler_LowLevel_State *resampler, unsigned int channels, unsigned long input_sample_rate, unsigned long output_sample_rate, unsigned long low_pass_filter_sample_rate)
{
	/* TODO - We really should just return here. */
	CLOWNRESAMPLER_ASSERT(channels <= CLOWNRESAMPLER_MAXIMUM_CHANNELS);

	resampler->channels = channels;
	resampler->position_integer = 0;
	resampler->position_fractional = 0;
	ClownResampler_LowLevel_Adjust(resampler, input_sample_rate, output_sample_rate, low_pass_filter_sample_rate);
}

static unsigned long ClownResampler_CalculateRatio(unsigned long a, unsigned long b)
{
	/* HAHAHA, I NEVER THOUGHT LONG DIVISION WOULD ACTUALLY COME IN HANDY! */
	unsigned long upper, middle, lower, result;

	/* A hack to prevent crashes when either sample rate is 0. Effectively causes playback to freeze. */
	if (a == 0 || b == 0)
		return 0;

	/* As well as splitting the number into chunks of CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE
	   size, this sneakily also multiplies it by CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE. */
	upper = a / CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE;
	middle = a % CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE;
	lower = 0;

	/* Perform long division. */
	middle |= upper % b * CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE;
	upper /= b;

	lower |= middle % b * CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE;
	middle /= b;

	/*even_lower |= lower % b * CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE;*/ /* Nothing to feed the remainder into... */
	lower /= b;

	/* Merge the chunks back together. */
	result = 0;
	result += upper * (CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE * 2);
	result += middle * (CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE * 1);
	result += lower;

	return result;
}

CLOWNRESAMPLER_API void ClownResampler_LowLevel_Adjust(ClownResampler_LowLevel_State *resampler, unsigned long input_sample_rate, unsigned long output_sample_rate, unsigned long low_pass_filter_sample_rate)
{
	const unsigned long output_to_input_ratio = ClownResampler_CalculateRatio(input_sample_rate, output_sample_rate);

	/* Determine the kernel scale. This is used to apply a low-pass filter. Not only is this something that the user may
	   explicitly request, but it's needed when downsampling to avoid artefacts. */
	/* Note that we do not ever want the kernel to be squished, but rather only stretched. */
	/* TODO - Freak-out if the ratio is so high that the kernel radius would exceed the size of the input buffer. */
	const unsigned long actual_low_pass_sample_rate = CLOWNRESAMPLER_MIN(input_sample_rate, CLOWNRESAMPLER_MIN(output_sample_rate, low_pass_filter_sample_rate));
	const unsigned long kernel_scale = ClownResampler_CalculateRatio(input_sample_rate, actual_low_pass_sample_rate);
	const unsigned long inverse_kernel_scale = ClownResampler_CalculateRatio(actual_low_pass_sample_rate, input_sample_rate);

	resampler->increment = output_to_input_ratio;
	resampler->stretched_kernel_radius = CLOWNRESAMPLER_KERNEL_RADIUS * kernel_scale;
	resampler->integer_stretched_kernel_radius = CLOWNRESAMPLER_TO_INTEGER_FROM_FIXED_POINT_CEIL(resampler->stretched_kernel_radius);
	resampler->stretched_kernel_radius_delta = CLOWNRESAMPLER_TO_FIXED_POINT_FROM_INTEGER(resampler->integer_stretched_kernel_radius) - resampler->stretched_kernel_radius;
	CLOWNRESAMPLER_ASSERT(resampler->stretched_kernel_radius_delta < CLOWNRESAMPLER_TO_FIXED_POINT_FROM_INTEGER(1));
	resampler->kernel_step_size = CLOWNRESAMPLER_FIXED_POINT_MULTIPLY(CLOWNRESAMPLER_KERNEL_RESOLUTION, inverse_kernel_scale);

	/* The wider the kernel, the greater the number of taps, the louder the sample. */
	/* Note that the scale is cast to 'long' here. This is to prevent samples from being promoted to
	   'unsigned long' later on, which breaks their sign-extension. Also note that we convert from
	   16.16 to 17.15 here. */
	resampler->sample_normaliser = (long)(inverse_kernel_scale >> (16 - 15));
}

CLOWNRESAMPLER_API void ClownResampler_LowLevel_Resample(ClownResampler_LowLevel_State *resampler, const ClownResampler_Precomputed *precomputed, const short *input_buffer, size_t *total_input_frames, short *output_buffer, size_t *total_output_frames)
{
	short *output_buffer_pointer = output_buffer;
	short *output_buffer_end = output_buffer + *total_output_frames * resampler->channels;

	for (;;)
	{
		/* Check if we've reached the end of the input buffer. */
		if (resampler->position_integer >= *total_input_frames)
		{
			resampler->position_integer -= *total_input_frames;
			*total_input_frames = 0;
			break;
		}
		/* Check if we've reached the end of the output buffer. */
		else if (output_buffer_pointer == output_buffer_end)
		{
			*total_input_frames -= resampler->position_integer;
			resampler->position_integer = 0;
			break;
		}
		else
		{
			unsigned int current_channel;
			size_t sample_index, kernel_index;

			long samples[CLOWNRESAMPLER_MAXIMUM_CHANNELS] = {0}; /* Sample accumulators. */

			/* Calculate the bounds of the kernel convolution. */
			const size_t min_relative = CLOWNRESAMPLER_TO_INTEGER_FROM_FIXED_POINT_CEIL(resampler->position_fractional + resampler->stretched_kernel_radius_delta);
			const size_t min = (resampler->position_integer + min_relative) * resampler->channels;
			const size_t max = (resampler->position_integer + resampler->integer_stretched_kernel_radius + CLOWNRESAMPLER_TO_INTEGER_FROM_FIXED_POINT_FLOOR(resampler->position_fractional + resampler->stretched_kernel_radius)) * resampler->channels;

			/* Yes, I know this line is freaking insane.
			   It's essentially a simplified and fixed-point version of this:
			   const size_t kernel_start = (size_t)(resampler->kernel_step_size * ((float)(min / resampler->channels) - resampler->position_if_it_were_a_float)); */
			const size_t kernel_start = CLOWNRESAMPLER_FIXED_POINT_MULTIPLY(resampler->kernel_step_size, (CLOWNRESAMPLER_TO_FIXED_POINT_FROM_INTEGER(min_relative) - resampler->position_fractional));

			CLOWNRESAMPLER_ASSERT(min < (*total_input_frames + resampler->integer_stretched_kernel_radius * 2) * resampler->channels);
			CLOWNRESAMPLER_ASSERT(max < (*total_input_frames + resampler->integer_stretched_kernel_radius * 2) * resampler->channels);

			for (sample_index = min, kernel_index = kernel_start; sample_index < max; sample_index += resampler->channels, kernel_index += resampler->kernel_step_size)
			{
				long kernel_value;

				CLOWNRESAMPLER_ASSERT(kernel_index < CLOWNRESAMPLER_COUNT_OF(precomputed->lanczos_kernel_table));

				/* The distance between the frames being output and the frames being read is the parameter to the Lanczos kernel. */
				kernel_value = precomputed->lanczos_kernel_table[kernel_index];

				/* Modulate the samples with the kernel and add them to the accumulators. */
				for (current_channel = 0; current_channel < resampler->channels; ++current_channel)
					samples[current_channel] += CLOWNRESAMPLER_FIXED_POINT_MULTIPLY((long)input_buffer[sample_index + current_channel], kernel_value);
			}

			/* Normalise, clamp, and output the samples. */
			for (current_channel = 0; current_channel < resampler->channels; ++current_channel)
			{
				const long sample = samples[current_channel];

				/* Normalise. */
				/* Note that we use a 17.15 version of CLOWNRESAMPLER_FIXED_POINT_MULTIPLY here.
				   This is because, if we used a 16.16 normaliser, then there's a chance that the result
				   of the multiplication would overflow, causing popping. */
				const long normalised_sample = (sample * resampler->sample_normaliser) / (1 << 15);

				/* Clamp. */
				/* Ideally this wouldn't be needed, but aliasing and/or rounding error in the Lanczos kernel necessitate it. */
				const long clamped_sample = CLOWNRESAMPLER_CLAMP(-0x7FFF, 0x7FFF, normalised_sample);

				/* Output. */
				*output_buffer_pointer++ = (short)clamped_sample;
			}

			/* Increment input buffer position. */
			resampler->position_fractional += resampler->increment;
			resampler->position_integer += CLOWNRESAMPLER_TO_INTEGER_FROM_FIXED_POINT_FLOOR(resampler->position_fractional);
			resampler->position_fractional %= CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE;
		}
	}

	/* Make 'total_output_frames' reflect how much space there is left in the output buffer. */
	*total_output_frames -= (output_buffer_pointer - output_buffer) / resampler->channels;
}


/* High-Level API */

CLOWNRESAMPLER_API void ClownResampler_HighLevel_Init(ClownResampler_HighLevel_State *resampler, unsigned int channels, unsigned long input_sample_rate, unsigned long output_sample_rate, unsigned long low_pass_filter_sample_rate)
{
	ClownResampler_LowLevel_Init(&resampler->low_level, channels, input_sample_rate, output_sample_rate, low_pass_filter_sample_rate);

	/* Blank the width of the kernel's diameter to zero, since there won't be previous data to occupy it yet. */
	CLOWNRESAMPLER_MEMSET(resampler->input_buffer, 0, resampler->low_level.integer_stretched_kernel_radius * resampler->low_level.channels * 2 * sizeof(*resampler->input_buffer));

	/* Initialise the pointers to point to the middle of the first (and newly-initialised) kernel. */
	resampler->input_buffer_start = resampler->input_buffer_end = resampler->input_buffer + resampler->low_level.integer_stretched_kernel_radius * resampler->low_level.channels;
}

CLOWNRESAMPLER_API size_t ClownResampler_HighLevel_Resample(ClownResampler_HighLevel_State *resampler, const ClownResampler_Precomputed *precomputed, short *output_buffer, size_t total_output_frames, size_t(*pull_callback)(void *user_data, short *buffer, size_t buffer_size), void *user_data)
{
	short *output_buffer_start = output_buffer;
	short *output_buffer_end = output_buffer_start + total_output_frames * resampler->low_level.channels;

	/* If we've run out of room in the output buffer, quit. */
	while (output_buffer_start != output_buffer_end)
	{
		const size_t radius_in_samples = resampler->low_level.integer_stretched_kernel_radius * resampler->low_level.channels;

		size_t input_frames;
		size_t output_frames;

		/* If the input buffer is empty, refill it. */
		if (resampler->input_buffer_start == resampler->input_buffer_end)
		{
			/* It's hard to explain this step-by-step, but essentially there's a trick we do here:
			   in order to avoid the resampler reading frames outside of the buffer, we have 'deadzones'
			   at each end of the buffer. When a new batch of frames is needed, the second deadzone is
			   copied over the first one, and the second is overwritten by the end of the new frames. */
			const size_t double_radius_in_samples = radius_in_samples * 2;

			/* Move the end of the last batch of data to the start of the buffer */
			/* (memcpy won't work here since the copy may overlap). */
			CLOWNRESAMPLER_MEMMOVE(resampler->input_buffer, resampler->input_buffer_end - radius_in_samples, double_radius_in_samples * sizeof(*resampler->input_buffer));

			/* Obtain input frames (note that the new frames start after the frames we just copied). */
			resampler->input_buffer_start = resampler->input_buffer + radius_in_samples;
			resampler->input_buffer_end = resampler->input_buffer_start + pull_callback(user_data, resampler->input_buffer + double_radius_in_samples, (CLOWNRESAMPLER_COUNT_OF(resampler->input_buffer) - double_radius_in_samples) / resampler->low_level.channels) * resampler->low_level.channels;

			/* If the callback returns 0, then we must have reached the end of the input data, so quit. */
			if (resampler->input_buffer_start == resampler->input_buffer_end)
				break;
		}

		/* Call the actual resampler. */
		input_frames = (resampler->input_buffer_end - resampler->input_buffer_start) / resampler->low_level.channels;
		output_frames = (output_buffer_end - output_buffer_start) / resampler->low_level.channels;
		ClownResampler_LowLevel_Resample(&resampler->low_level, precomputed, resampler->input_buffer_start - radius_in_samples, &input_frames, output_buffer_start, &output_frames);

		/* Increment input and output pointers. */
		resampler->input_buffer_start = resampler->input_buffer_end - input_frames * resampler->low_level.channels;
		output_buffer_start = output_buffer_end - output_frames * resampler->low_level.channels;
	}

	return total_output_frames - (output_buffer_end - output_buffer_start) / resampler->low_level.channels;
}

#endif /* CLOWNRESAMPLER_GUARD_FUNCTION_DEFINITIONS */

#endif /* CLOWNRESAMPLER_IMPLEMENTATION */
