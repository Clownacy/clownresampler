/* Public-domain C89 single-file library for resampling audio.
   Made by Clownacy. */

#ifndef CLOWNRESAMPLER_H
#define CLOWNRESAMPLER_H


/* Configuration */

/* Define this as 'static' to limit the visibility of functions. */
#ifndef CLOWNRESAMPLER_API 
#define CLOWNRESAMPLER_API
#endif

/* Controls the number of 'lobes' of the windowed sinc function.
   A higher number results in better audio, but is more expensive. */
#ifndef CLOWNRESAMPLER_KERNEL_RADIUS
#define CLOWNRESAMPLER_KERNEL_RADIUS 3
#endif

#ifndef CLOWNRESAMPLER_KERNEL_RESOLUTION
#define CLOWNRESAMPLER_KERNEL_RESOLUTION 0x400 /* 1024 */
#endif

#ifndef CLOWNRESAMPLER_MAXIMUM_CHANNELS
#define CLOWNRESAMPLER_MAXIMUM_CHANNELS 2
#endif

/* Header */

#include <stddef.h>

/* For 16.16. This is good because it reduces multiplcations and divisions to mere bit-shifts. */
#define CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE 0x10000
#define CLOWNRESAMPLER_TO_FIXED_POINT_FROM_RATIO(a, b) (CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE * (a) / (b))
#define CLOWNRESAMPLER_TO_FIXED_POINT_FROM_INTEGER(x) ((x) * CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE)
#define CLOWNRESAMPLER_TO_INTEGER_FROM_FIXED_POINT_FLOOR(x) ((x) / CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE)
#define CLOWNRESAMPLER_TO_INTEGER_FROM_FIXED_POINT_ROUND(x) (((x) + (CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE / 2)) / CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE)
#define CLOWNRESAMPLER_TO_INTEGER_FROM_FIXED_POINT_CEIL(x) (((x) + (CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE - 1)) / CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE)
#define CLOWNRESAMPLER_FIXED_POINT_MULTIPLY(a, b) ((a) * (b) / CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE)


/* Low-level API.
   This API has lower overhead, but is more difficult to use, requiring that
   audio be pre-processed before resampling. */

typedef struct ClownResampler_LowLevel_State
{
	unsigned int channels;
	size_t position_integer;
	unsigned long position_fractional;      /* 16.16 fixed point */
	unsigned long increment;                /* 16.16 fixed point */
	size_t stretched_kernel_radius;         /* 16.16 fixed point */
	size_t integer_stretched_kernel_radius;
	size_t stretched_kernel_radius_delta;   /* 16.16 fixed point */
	size_t inverse_kernel_scale;            /* 16.16 fixed point */
	size_t kernel_step_size;
} ClownResampler_LowLevel_State;

/* Initialises a low-level resampler. This function must be called before the
   state is passed to any other functions. By default, it will have a
   resampling ratio of 1.0 (that is, the output sample rate will be the same as
   the input). */
CLOWNRESAMPLER_API void ClownResampler_LowLevel_Init(ClownResampler_LowLevel_State *resampler, unsigned int channels);

/* Sets the ratio of the resampler. For example, if the input sample rate is
   48000Hz, then a ratio of 0.5 will cause the output sample rate to be
   24000Hz. */
CLOWNRESAMPLER_API void ClownResampler_LowLevel_SetResamplingRatio(ClownResampler_LowLevel_State *resampler, unsigned long input_sample_rate, unsigned long output_sample_rate);

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
CLOWNRESAMPLER_API void ClownResampler_LowLevel_Resample(ClownResampler_LowLevel_State *resampler, const short *input_buffer, size_t *total_input_frames, short *output_buffer, size_t *total_output_frames);


/* High-level API.
   This API has more overhead, but is easier to use. */

typedef struct ClownResampler_HighLevel_State
{
	ClownResampler_LowLevel_State low_level;

	short input_buffer[0x1000];
	short *input_buffer_start;
	short *input_buffer_end;
} ClownResampler_HighLevel_State;

/* Initialises a high-level resampler. This function must be called before the
   state is passed to any other functions. This function also sets the ratio of
   the resampler. For example, if the input sample rate is 48000Hz, then a
   ratio of 0.5 will cause the output sample rate to be 24000Hz. */
CLOWNRESAMPLER_API void ClownResampler_HighLevel_Init(ClownResampler_HighLevel_State *resampler, unsigned int channels, unsigned long input_sample_rate, unsigned long output_sample_rate);

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
CLOWNRESAMPLER_API size_t ClownResampler_HighLevel_Resample(ClownResampler_HighLevel_State *resampler, short *output_buffer, size_t total_output_frames, size_t(*pull_callback)(void *user_data, short *buffer, size_t buffer_size), void *user_data);


/* Implementation */

#ifdef CLOWNRESAMPLER_IMPLEMENTATION

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

#define CLOWNRESAMPLER_COUNT_OF(x) (sizeof(x) / sizeof(*(x)))
#define CLOWNRESAMPLER_MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLOWNRESAMPLER_MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLOWNRESAMPLER_CLAMP(x, min, max) (CLOWNRESAMPLER_MIN((max), CLOWNRESAMPLER_MAX((min), (x))))

static float ClownResampler_LanczosKernel(float x)
{
	const float kernel_radius = (float)CLOWNRESAMPLER_KERNEL_RADIUS;

	//assert(x != 0.0f);
	if (x == 0.0f)
		return 1.0f;

	assert(fabsf(x) <= kernel_radius);
	/*if (fabsf(x) > kernel_radius)
		return 0.0f;*/

	const float x_times_pi = x * 3.1415926535897932384626433832795028841971693993751058209749445923078164062862089986280348253421170679f; /* 100 digits should be good enough */
	const float x_times_pi_divided_by_radius = x_times_pi / kernel_radius;

	return (sinf(x_times_pi) * sinf(x_times_pi_divided_by_radius)) / (x_times_pi * x_times_pi_divided_by_radius);
}

static long ClownResampler_LanczosKernelTable[CLOWNRESAMPLER_KERNEL_RADIUS * 2 * CLOWNRESAMPLER_KERNEL_RESOLUTION];

/* Low-level API */

CLOWNRESAMPLER_API void ClownResampler_LowLevel_Init(ClownResampler_LowLevel_State *resampler, unsigned int channels)
{
	/* TODO - We really should just return here */
	assert(channels <= CLOWNRESAMPLER_MAXIMUM_CHANNELS);

	for (size_t i = 0; i < CLOWNRESAMPLER_COUNT_OF(ClownResampler_LanczosKernelTable); ++i)
		ClownResampler_LanczosKernelTable[i] = (long)((float)CLOWNRESAMPLER_FIXED_POINT_FRACTIONAL_SIZE * ClownResampler_LanczosKernel(((float)i / (float)CLOWNRESAMPLER_COUNT_OF(ClownResampler_LanczosKernelTable) * 2.0f - 1.0f) * (float)CLOWNRESAMPLER_KERNEL_RADIUS));

	resampler->channels = channels;
	resampler->position_integer = 0;
	resampler->position_fractional = 0;
	ClownResampler_LowLevel_SetResamplingRatio(resampler, 1, 1); /* A nice sane default */
}

CLOWNRESAMPLER_API void ClownResampler_LowLevel_SetResamplingRatio(ClownResampler_LowLevel_State *resampler, unsigned long input_sample_rate, unsigned long output_sample_rate)
{
	const unsigned long ratio = CLOWNRESAMPLER_TO_FIXED_POINT_FROM_RATIO(input_sample_rate, output_sample_rate);
	const unsigned long inverse_ratio = CLOWNRESAMPLER_TO_FIXED_POINT_FROM_RATIO(output_sample_rate, input_sample_rate);

	/* TODO - Freak-out if the ratio is so high that the kernel radius would exceed the size of the input buffer */
	const unsigned long kernel_scale = CLOWNRESAMPLER_MAX(ratio, CLOWNRESAMPLER_TO_FIXED_POINT_FROM_INTEGER(1));

	resampler->increment = ratio;
	resampler->stretched_kernel_radius = CLOWNRESAMPLER_KERNEL_RADIUS * kernel_scale;
	resampler->integer_stretched_kernel_radius = CLOWNRESAMPLER_TO_INTEGER_FROM_FIXED_POINT_CEIL(resampler->stretched_kernel_radius);
	resampler->stretched_kernel_radius_delta = CLOWNRESAMPLER_TO_FIXED_POINT_FROM_INTEGER(resampler->integer_stretched_kernel_radius) - resampler->stretched_kernel_radius;
	assert(resampler->stretched_kernel_radius_delta < CLOWNRESAMPLER_TO_FIXED_POINT_FROM_INTEGER(1));
	resampler->inverse_kernel_scale = CLOWNRESAMPLER_KERNEL_RESOLUTION * inverse_ratio;
	resampler->kernel_step_size = CLOWNRESAMPLER_TO_INTEGER_FROM_FIXED_POINT_FLOOR(resampler->inverse_kernel_scale);
}

CLOWNRESAMPLER_API void ClownResampler_LowLevel_Resample(ClownResampler_LowLevel_State *resampler, const short *input_buffer, size_t *total_input_frames, short *output_buffer, size_t *total_output_frames)
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
		else if (output_buffer_pointer >= output_buffer_end)
		{
			*total_input_frames -= resampler->position_integer;
			resampler->position_integer = 0;
			break;
		}
		else
		{
			unsigned int current_channel;
			size_t sample_index, kernel_index;

			long samples[CLOWNRESAMPLER_MAXIMUM_CHANNELS] = {0}; /* Sample accumulators */

		#ifndef NDEBUG
			long accumulator = 0;
		#endif

			/* Calculate the bounds of the kernel convolution. */
			const size_t min_relative = CLOWNRESAMPLER_TO_INTEGER_FROM_FIXED_POINT_CEIL(resampler->position_fractional + resampler->stretched_kernel_radius_delta);
			const size_t min = resampler->position_integer + min_relative;
			const size_t max = resampler->position_integer + resampler->integer_stretched_kernel_radius + CLOWNRESAMPLER_TO_INTEGER_FROM_FIXED_POINT_FLOOR(resampler->position_fractional + resampler->stretched_kernel_radius);

			/* Yes, I know this line is freaking insane.
			   It's essentially a simplified and fixed-point version of this:
			   const size_t kernel_start = (size_t)((float)min - resampler->position_if_it_were_a_float) * resampler->inverse_kernel_scale_if_it_were_a_float; */
			const size_t kernel_start = CLOWNRESAMPLER_TO_INTEGER_FROM_FIXED_POINT_FLOOR(CLOWNRESAMPLER_FIXED_POINT_MULTIPLY(CLOWNRESAMPLER_TO_FIXED_POINT_FROM_INTEGER(min_relative) - resampler->position_fractional, resampler->inverse_kernel_scale));

			assert(min < *total_input_frames + resampler->integer_stretched_kernel_radius * 2);
			assert(max < *total_input_frames + resampler->integer_stretched_kernel_radius * 2);

			for (sample_index = min, kernel_index = kernel_start; sample_index < max; ++sample_index, kernel_index += resampler->kernel_step_size)
			{
				/* The distance between the frames being output and the frames being read are the parameter to the Lanczos kernel. */
				assert(kernel_index < CLOWNRESAMPLER_COUNT_OF(ClownResampler_LanczosKernelTable));

				const long kernel_value = ClownResampler_LanczosKernelTable[kernel_index];

			#ifndef NDEBUG
				accumulator += kernel_value;
			#endif

				/* Modulate the samples with the kernel and add it to the accumulator. */
				for (current_channel = 0; current_channel < resampler->channels; ++current_channel)
					samples[current_channel] += CLOWNRESAMPLER_FIXED_POINT_MULTIPLY((long)input_buffer[sample_index * resampler->channels + current_channel], kernel_value);
			}

			assert(accumulator <= 0x10000);

			/* Perform clamping and output samples. */
			for (current_channel = 0; current_channel < resampler->channels; ++current_channel)
			{
				assert(samples[current_channel] >= -0x8000 && samples[current_channel] <= 0x7FFF);
				/**output_buffer_pointer++ = (short)CLOWNRESAMPLER_CLAMP(samples[current_channel], -0x8000, 0x7FFF);*/
				*output_buffer_pointer++ = (short)samples[current_channel];
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


/* High-level API */

CLOWNRESAMPLER_API void ClownResampler_HighLevel_Init(ClownResampler_HighLevel_State *resampler, unsigned int channels, unsigned long input_sample_rate, unsigned long output_sample_rate)
{
	ClownResampler_LowLevel_Init(&resampler->low_level, channels);
	ClownResampler_LowLevel_SetResamplingRatio(&resampler->low_level, input_sample_rate, output_sample_rate);

	memset(resampler->input_buffer, 0, resampler->low_level.integer_stretched_kernel_radius * 2 * sizeof(*resampler->input_buffer));
	resampler->input_buffer_start = resampler->input_buffer_end = resampler->input_buffer + resampler->low_level.integer_stretched_kernel_radius;
}

CLOWNRESAMPLER_API size_t ClownResampler_HighLevel_Resample(ClownResampler_HighLevel_State *resampler, short *output_buffer, size_t total_output_frames, size_t(*pull_callback)(void *user_data, short *buffer, size_t buffer_size), void *user_data)
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
			memmove(resampler->input_buffer, resampler->input_buffer_end - radius_in_samples, double_radius_in_samples * sizeof(*resampler->input_buffer));

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
		ClownResampler_LowLevel_Resample(&resampler->low_level, resampler->input_buffer_start - radius_in_samples, &input_frames, output_buffer_start, &output_frames);

		/* Increment input and output pointers. */
		resampler->input_buffer_start = resampler->input_buffer_end - input_frames * resampler->low_level.channels;
		output_buffer_start = output_buffer_end - output_frames * resampler->low_level.channels;
	}

	return total_output_frames - (output_buffer_end - output_buffer_start) / resampler->low_level.channels;
}

#endif /* CLOWNRESAMPLER_IMPLEMENTATION */

#endif /* CLOWNRESAMPLER_H */
