#include <limits.h>
#include <stdio.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <plplot/plplot.h>
#include <fftw3.h>
#include <math.h>

struct frequency {
	double magnitude;
	int sample;
};

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
	struct frequency f[100];
	int num_frequencies = 0;
	double current = 0, last = 0;
	int num;
	int fsamples = 0;
	int sample;
	double ymin = 0, ymax = 0, xmin = 0, xmax = 0;

	memset(&info, 0, sizeof(SF_INFO));
	memset(&f, 0, sizeof(struct frequency) * 100);

	file = sf_open(argv[1], SFM_READ, &info);
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

	plsdev("xwin");
	plinit();

	plenv(0.0f, seconds, -1, 1, 0, 0);

	input = malloc(sizeof(int) * info.channels);
	if (!input) {
		printf("Error allocating input buffer\n");
		sf_close(file);
		exit(1);
	}

	start = 0;
	end = info.frames;
//	start = 1.8f * (float)info.samplerate;
//	end = 2.1f * (float)info.samplerate;

	samples = end - start;
	in = (double *)fftw_malloc(sizeof(double) * samples);
	out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * samples);
//	p = fftw_plan_dft_r2c_1d(samples, in, out, FFTW_PRESERVE_INPUT);

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


//		x = (double)input[0] / INT_MAX;
		in[i] = input[0];
//		plpoin(1, &time, &x, 1);
	}


	for (i = 0; i < samples; i++) {
		PLFLT time = (float)i / (float)info.samplerate;
		PLFLT x;
		if (in[i] > 0)
			in[i] /= max;
		else if (in[i] < 0)
			in[i] /= (-min);
		x = in[i];
		plpoin(1, &time, &x, 1);
	}

	plend();
/*
	fftw_execute(p);
	num = 0;
	for (i = 0; i < samples; i++) {
		double magnitude;
		double *d;
		PLFLT x;
		PLFLT y;

		d = out[i];
		magnitude = hypot(d[0], d[1]);
		x = d[0];
		if (magnitude < 10) {
			num++;
			continue;
		}
		if (d[0] > xmax)
			xmax = d[0];
		if (d[0] < xmin)
			xmin = d[0];
		if (d[1] > ymax)
			ymax = d[1];
		if (d[1] < ymin)
			ymin = d[1];
		if (magnitude < current && num > 100) {
			f[num_frequencies].magnitude = current;
			f[num_frequencies].sample = sample;
			num_frequencies++;
			num = 0;
			current = 0;
			sample = 0;
		} else if (magnitude < current) {
			num++;
		} else if (magnitude > current) {
			sample = i;
			current = magnitude;
		}
/*
		if (last && last == current && last > magnitude) {
			f[num_frequencies].magnitude = current;
			f[num_frequencies].sample = i - 1;
			num_frequencies++;
		} else if (last < magnitude) {
			current = last;
		}
		last = magnitude;
//
//		printf("current=%f, last=%f, magnitude=%f\n", current, last, magnitude);
//		printf("d[0]=%f, d[1]=%f, magnitude=%f\n", d[0], d[1], magnitude);
		y = d[1];
//		plpoin(1, &x, &y, 1);
	}

	plsdev("xwin");
	plinit();
	plenv(0, samples, xmin, xmax, 0, 0);
	for (i = 0; i < samples; i++) {
		PLFLT x;
		PLFLT y;
		double *d;

		d = out[i];
		x = i;
		y = d[0];
		plpoin(1, &x, &y, 1);
	}
	plend();
	total_mean /= info.frames;

	for (i = 0; i < num_frequencies; i++)
		printf("Peak %d: magnitude=%f, sample=%d\n",
		       i, f[i].magnitude, f[i].sample);
*/
	printf("Num frequencies: %d\n", num_frequencies);
	printf("Min: %f\n", min);
	printf("Max: %f\n", max);
	printf("Mean: %f\n", mean);
	printf("Skipped: %d\n", skipped);
	printf("Total mean: %f\n", total_mean);
	printf("Thresh: %f\n", thresh);

//	fftw_destroy_plan(p);
	fftw_free(in);
	fftw_free(out);
	free(input);
	sf_close(file);

	return 0;
}
