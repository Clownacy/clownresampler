/*
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

/*
This demonstrates use of clownresampler's low-level API.

The low-level API is ideal for when the entirety of the input data is available
at once, whereas the high-level API is ideal for when the input data is
streamed piece by piece.
*/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MA_NO_ENGINE
#define MA_NO_NODE_GRAPH
#define MA_API static
#include "libraries/miniaudio.h" /* v0.11.9 */

#define DR_MP3_IMPLEMENTATION
#define DR_MP3_ONLY_MP3
#define DRMP3_API static
#include "libraries/dr_mp3.h" /* v0.6.33 */

#define CLOWNRESAMPLER_IMPLEMENTATION
#define CLOWNRESAMPLER_STATIC
#define CLOWNRESAMPLER_NO_HIGH_LEVEL_API /* We do not need the high-level API. */
#include "../clownresampler.h"

static ClownResampler_Precomputed precomputed;
static ClownResampler_LowLevel_State resampler;
static unsigned int total_channels;
static drmp3_int16 *resampler_input_buffer;
static size_t resampler_input_buffer_total_frames;
static size_t resampler_input_buffer_frames_remaining;

typedef struct ResamplerCallbackData
{
	ma_int16 *output_pointer;
	ma_uint32 output_buffer_frames_remaining;
} ResamplerCallbackData;

static cc_bool ResamplerOutputCallback(void *user_data, const cc_s32f *frame, cc_u8f total_samples)
{
	ResamplerCallbackData* const callback_data = (ResamplerCallbackData*)user_data;

	cc_u8f i;

	/* Output the frame. */
	for (i = 0; i < total_samples; ++i)
	{
		cc_s32f sample;

		sample = frame[i];

		/* Clamp the sample to 16-bit. */
		if (sample > 0x7FFF)
			sample = 0x7FFF;
		else if (sample < -0x7FFF)
			sample = -0x7FFF;

		/* Push the sample to the output buffer. */
		*callback_data->output_pointer++ = (ma_int16)sample;
	}

	/* Signal whether there is more room in the output buffer. */
	return --callback_data->output_buffer_frames_remaining != 0;
}

static void AudioCallback(ma_device *device, void *output, const void *input, ma_uint32 frame_count)
{
	ResamplerCallbackData callback_data;

	(void)device;
	(void)input;

	callback_data.output_pointer = (ma_int16*)output;
	callback_data.output_buffer_frames_remaining = frame_count;

	/* Resample the decoded audio data. */
	ClownResampler_LowLevel_Resample(&resampler, &precomputed, &resampler_input_buffer[(resampler_input_buffer_total_frames - resampler_input_buffer_frames_remaining) * total_channels], &resampler_input_buffer_frames_remaining, ResamplerOutputCallback, &callback_data);

	/* If there are no more samples left, then fill the remaining space in the buffer with 0. */
	memset(callback_data.output_pointer, 0, callback_data.output_buffer_frames_remaining * total_channels * sizeof(ma_int16));
}

int main(int argc, char **argv)
{
	int exit_code;
	drmp3 mp3_decoder;

	exit_code = EXIT_FAILURE;

	if (argc < 2)
	{
		fputs("Pass the path to an MP3 file as an argument.\n", stderr);
	}
	else
	{
		if (!drmp3_init_file(&mp3_decoder, argv[1], NULL))
		{
			fputs("Failed to initialise MP3 decoder.\n", stderr);
		}
		else
		{
			/******************************/
			/* Initialise audio playback. */
			/******************************/
			ma_device_config miniaudio_config;
			ma_device miniaudio_device;

			miniaudio_config = ma_device_config_init(ma_device_type_playback);
			miniaudio_config.playback.format   = ma_format_s16;
			miniaudio_config.playback.channels = mp3_decoder.channels;
			miniaudio_config.sampleRate        = 0; /* Use whatever sample rate the playback device wants. */
			miniaudio_config.dataCallback      = AudioCallback;
			miniaudio_config.pUserData         = NULL;

			if (ma_device_init(NULL, &miniaudio_config, &miniaudio_device) != MA_SUCCESS)
			{
				drmp3_uninit(&mp3_decoder);
				fputs("Failed to initialise playback device.\n", stderr);
			}
			else
			{
				/*****************************************/
				/* Finished initialising audio playback. */
				/*****************************************/

				const size_t size_of_frame = mp3_decoder.channels * sizeof(drmp3_int16);

				size_t total_mp3_pcm_frames;

				total_mp3_pcm_frames = drmp3_get_pcm_frame_count(&mp3_decoder);
				total_channels = mp3_decoder.channels;

				/* Inform the user of the input and output sample rates. */
				fprintf(stderr, "MP3 Sample Rate: %lu\n", (unsigned long)mp3_decoder.sampleRate);
				fprintf(stderr, "Playback Sample Rate: %lu\n", (unsigned long)miniaudio_device.sampleRate);
				fflush(stderr);

				/******************************/
				/* Initialise clownresampler. */
				/******************************/

				/* Precompute the Lanczos kernel. */
				ClownResampler_Precompute(&precomputed);

				/* Create a resampler that converts from the sample rate of the MP3 to the sample rate of the playback device. */
				/* The low-pass filter is set to 44100Hz since that should allow all human-perceivable frequencies through. */
				ClownResampler_LowLevel_Init(&resampler, mp3_decoder.channels, mp3_decoder.sampleRate, miniaudio_device.sampleRate, 44100);

				/*****************************************/
				/* Finished initialising clownresampler. */
				/*****************************************/

				/*****************************************/
				/* Set up clownresampler's input buffer. */
				/*****************************************/

				/* Create a buffer to hold the decoded PCM data. */
				/* clownresampler's low-level API requires that this buffer have padding at its beginning and end. */
				resampler_input_buffer = (drmp3_int16*)malloc((resampler.lowest_level.integer_stretched_kernel_radius * 2 + total_mp3_pcm_frames) * size_of_frame);

				if (resampler_input_buffer == NULL)
				{
					drmp3_uninit(&mp3_decoder);
					fputs("Failed to allocate memory for resampler input buffer.\n", stderr);
				}
				else
				{
					/* Set the padding samples at the start to 0. */
					memset(&resampler_input_buffer[0], 0, resampler.lowest_level.integer_stretched_kernel_radius * size_of_frame);

					/* Decode the MP3 to the input buffer. */
					drmp3_read_pcm_frames_s16(&mp3_decoder, total_mp3_pcm_frames, &resampler_input_buffer[resampler.lowest_level.integer_stretched_kernel_radius * total_channels]);
					drmp3_uninit(&mp3_decoder);

					/* Set the padding samples at the end to 0. */
					memset(&resampler_input_buffer[(resampler.lowest_level.integer_stretched_kernel_radius + total_mp3_pcm_frames) * total_channels], 0, resampler.lowest_level.integer_stretched_kernel_radius * size_of_frame);

					/* Initialise some variables that will be used by the audio callback. */
					resampler_input_buffer_total_frames = resampler_input_buffer_frames_remaining = total_mp3_pcm_frames;

					/*****************************************************/
					/* Finished setting up the resampler's input buffer. */
					/*****************************************************/

					/* Begin playback. */
					ma_device_start(&miniaudio_device);

					/* Wait for input from the user before terminating the program. */
					fgetc(stdin);

					ma_device_stop(&miniaudio_device);

					free(resampler_input_buffer);

					exit_code = EXIT_SUCCESS;
				}

				ma_device_uninit(&miniaudio_device);
			}
		}
	}

	return exit_code;
}
