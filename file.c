#include <limits.h>
#include <stdio.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <plplot/plplot.h>
#include <fftw3.h>
#include <math.h>

int main(int argc, char **argv)
{
	SNDFILE *file;
	SF_INFO info;
	float seconds;
	int *input;
	int i;
	float min, max = 0, mean = 0, total_mean = 0;
	int skipped = 0;
	int skip = 0;
	int delay = 0;
	float thresh = 0;
	float start, end;
	double *in;
	fftw_complex *out;
	fftw_plan p;
	int samples;

	memset(&info, 0, sizeof(SF_INFO));

	file = sf_open("untitled.wav", SFM_READ, &info);
	if (!file) {
		printf("Error opening wave file\n");
		exit(1);
	}

	seconds = (float)info.frames/(float)info.samplerate;
	delay = info.samplerate / 8;

	printf("Opened untitled.wav\n");
	printf("frames: %d\n", info.frames);
	printf("samplerate: %d\n", info.samplerate);
	printf("length (in seconds): %f\n", seconds);
	printf("channels: %d\n", info.channels);
	printf("format: %x\n", info.format);
	printf("sections: %d\n", info.sections);
	printf("seekable: %d\n", info.seekable);

//	plsdev("xwin");
//	plinit();

//	plenv(0.0f, 0.3f, INT_MIN, INT_MAX, 0, 0);

	input = malloc(sizeof(int) * info.channels);
	if (!input) {
		printf("Error allocating input buffer\n");
		sf_close(file);
		exit(1);
	}

	start = 1.8f * (float)info.samplerate;
	end = 2.1f * (float)info.samplerate;

	samples = end - start;
	in = (double *)fftw_malloc(sizeof(double) * samples);
	out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * samples);
	p = fftw_plan_dft_r2c_1d(samples, in, out, 0);

	printf("start is %f, end is %f\n", start, end);

	sf_seek(file, (sf_count_t)start, SEEK_SET);

	for (i = 0; i < samples; i++) {
		sf_count_t ret;
		PLFLT time = (float)i / (float)info.samplerate;
		PLFLT x;

		ret = sf_readf_int(file, input, 1);
		if (ret <= 0) {
			printf("No more frames to read\n");
			break;
		}

		if (!i)
			min = input[0];

		if (min > input[0])
			min = input[0];

		if (max < input[0])
			max = input[0];

		total_mean += input[0];


		x = input[0];
		in[i] = x;
//		plpoin(1, &time, &x, 1);
	}

//	plend();

	plsdev("xwin");
	plinit();
	plenv(0.0f, ((float)samples / 2) + 1, 0, INT_MAX / 3, 0, 0);
	fftw_execute(p);

	for (i = 0; i < samples; i++) {
		double magnitude;
		double *d;
		PLFLT x = i;
		PLFLT y;

		d = out[i];
		magnitude = hypot(d[0] / 1000.0f, d[1] / 1000.0f);
		if (magnitude == 0)
			continue;
		printf("d[0]=%f, d[1]=%f, magnitude=%f\n", d[0], d[1], magnitude);
		y = magnitude;
		plpoin(1, &x, &y, 1);
	}

	plend();
	total_mean /= info.frames;

	printf("Min: %f\n", min);
	printf("Max: %f\n", max);
	printf("Mean: %f\n", mean);
	printf("Skipped: %d\n", skipped);
	printf("Total mean: %f\n", total_mean);
	printf("Thresh: %f\n", thresh);

	fftw_destroy_plan(p);
	fftw_free(in);
	fftw_free(out);
	free(input);
	sf_close(file);

	return 0;
}
