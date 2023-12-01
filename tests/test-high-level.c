/*
Copyright (c) 2022-2023 Clownacy

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

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define DR_FLAC_IMPLEMENTATION
#define DR_FLAC_NO_OGG
#define DRFLAC_API static
#include "dr_flac.h"

#define CLOWNRESAMPLER_IMPLEMENTATION
#define CLOWNRESAMPLER_STATIC
#include "../clownresampler.h"

static ClownResampler_Precomputed precomputed;
static ClownResampler_HighLevel_State resampler;
static drflac *flac_decoder;

static size_t ResamplerInputCallback(void *user_data, cc_s16l *buffer, size_t total_frames)
{
	(void)user_data;

	/* Obtain samples from the FLAC file. */
	return drflac_read_pcm_frames_s16(flac_decoder, total_frames, buffer);
}

static cc_bool ResamplerOutputCallback(void *user_data, const cc_s32f *frame, cc_u8f total_samples)
{
	FILE* const output_file = (FILE*)user_data;

	cc_u8f i;

	/* Output the frame. */
	for (i = 0; i < total_samples; ++i)
	{
		unsigned int j;
		unsigned char bytes[4];

		for (j = 0; j < 4; ++j)
			bytes[j] = (frame[i] >> (8 * j)) & 0xFF;

		fwrite(bytes, 1, sizeof(bytes), output_file);
	}

	return cc_true;
}

int main(int argc, char **argv)
{
	int exit_code;

	exit_code = EXIT_FAILURE;

	if (argc < 6)
	{
		fprintf(stderr, "Usage: %s [path to input file] [path to output file] [input sample rate] [output sample rate] [low-pass filter sample rate]\n", argv[0]);
	}
	else
	{
		char *input_sample_rate_end;
		char *output_sample_rate_end;
		char *low_pass_sample_rate_end;

		const unsigned long input_sample_rate = strtoul(argv[3], &input_sample_rate_end, 0);
		const unsigned long output_sample_rate = strtoul(argv[4], &output_sample_rate_end, 0);
		const unsigned long low_pass_sample_rate = strtoul(argv[5], &low_pass_sample_rate_end, 0);

		if (input_sample_rate_end < argv[3] + strlen(argv[3])
		 || output_sample_rate_end < argv[4] + strlen(argv[4])
		 || low_pass_sample_rate_end < argv[5] + strlen(argv[5])
		)
		{
			fputs("Sample rate arguments were invalid.\n", stderr);
		}
		else
		{
			FILE* const output_file = fopen(argv[2], "wb");

			if (output_file == NULL)
			{
				fputs("Failed to open output file for writing.\n", stderr);
			}
			else
			{
				flac_decoder = drflac_open_file(argv[1], NULL);

				if (flac_decoder == NULL)
				{
					fputs("Failed to initialise FLAC decoder.\n", stderr);
				}
				else
				{
					/* Inform the user of the input and output sample rates. */
					fprintf(stderr, "FLAC Sample Rate: %lu\n", (unsigned long)flac_decoder->sampleRate);
					fflush(stderr);

					/******************************/
					/* Initialise clownresampler. */
					/******************************/

					/* Precompute the Lanczos kernel. */
					ClownResampler_Precompute(&precomputed);

					/* Create a resampler that converts from the sample rate of the FLAC file to the sample rate of the playback device. */
					ClownResampler_HighLevel_Init(&resampler, flac_decoder->channels, input_sample_rate, output_sample_rate, low_pass_sample_rate);

					/*****************************************/
					/* Finished initialising clownresampler. */
					/*****************************************/

					/* Resample the decoded audio data. */
					ClownResampler_HighLevel_Resample(&resampler, &precomputed, ResamplerInputCallback, ResamplerOutputCallback, output_file);
					ClownResampler_HighLevel_ResampleEnd(&resampler, &precomputed, ResamplerOutputCallback, output_file);

					drflac_close(flac_decoder);

					exit_code = EXIT_SUCCESS;
				}

				fclose(output_file);
			}
		}
	}

	return exit_code;
}
