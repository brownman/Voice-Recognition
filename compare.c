#include <limits.h>
#include <stdio.h>
#include <sndfile.h>
#include <stdlib.h>
#include <string.h>
#include <plplot/plplot.h>
#include <math.h>
#include <fftw3.h>

struct fft_average {
	double xavg;
	double yavg;
};

struct fft_average *do_fft(double *in, int max)
{
	fftw_complex *out;
	double *fft_in;
	fftw_plan p;
	int i;
	double xmax = 0, xmin = 0, ymin = 0, ymax = 0;
	double average = 0;
	struct fft_average *avg;

	avg = malloc(sizeof(struct fft_average));
	if (!avg)
		exit(1);
	fft_in = (double *)fftw_malloc(sizeof(double) * max);
	out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * max);

	p = fftw_plan_dft_r2c_1d(max, fft_in, out, FFTW_PRESERVE_INPUT);
	for (i = 0; i < max; i++)
		fft_in[i] = in[i];

//	memcpy(fft_in, in, sizeof(double) * max);
	fftw_execute(p);

	for (i = 0; i < max; i++) {
		double *d;

		d = out[i];
		if (d[0] > xmax)
			xmax = d[0];
		else if (d[0] < xmin)
			xmin = d[0];
		if (d[1] > ymax)
			ymax = d[1];
		else if (d[1] < ymin)
			ymin = d[1];
	}

	plsdev("xwin");
	plinit();
	plenv(-1, 1, -1, 1, 0, 0);
	for (i = 0; i < max; i++) {
		PLFLT x;
		PLFLT y;
		double *d;

		d = out[i];
		if (d[0] < 0) {
			x = d[0] / -xmin;
			avg->xavg += (d[0] / -xmin);
			if (i > 0)
				avg->xavg /= 2;
		} else {
			x = d[0] / xmax;
			avg->xavg += (d[0] / xmax);
			if (i > 0)
				avg->xavg /= 2;
		}

		if (d[1] < 0) {
			y = d[1] / -ymin;
			avg->yavg += (d[1] / -ymin);
			if (i > 0)
				avg->yavg /= 2;
		} else {
			y = d[1] / ymax;
			avg->yavg += (d[1] / ymax);
			if (i > 0)
				avg->yavg /= 2;
		}

		plpoin(1, &x, &y, 1);
	}
	plend();

	fftw_destroy_plan(p);
	fftw_free(fft_in);
	fftw_free(out);

	return avg;
}

void normalize_wave(SNDFILE *file, double *in, int samples, int channels)
{
	int i, c;
	int min = 0, max = 0;
	int *input;
	int num = 0;
	int thresh = samples / 100;

	input = malloc(sizeof(int) * channels);
	if (!input) {
		printf("Error allocating input buffer\n");
		sf_close(file);
		exit(1);
	}

	for (i = 0; i < samples; i++) {
		sf_count_t ret;

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

		in[i] = input[0];
	}
	
	printf("Normalizing\n");
	for (i = 0; i < samples; i++) {
		if (in[i] > 0)
			in[i] /= max;
		else if (in[i] < 0)
			in[i] /= (-min);

		if (in[i] < 0.25f && in[i] > -0.25) {
			num++;
		} else {
			num = 0;
		}

		if (num == thresh) {
			for (c = i; c > i - thresh; c--)
				in[c] = -2;
		} else if (num > thresh) {
			in[i] = -2;
		}
	}

	free(input);
}

int shift_wave(double *input, int samples)
{
	int num = 0;
	int start = -1;
	int thresh = (samples * 5) / 100;
	int i = 0;
	int c = 0;
	int max;

	for (i = 0; i < samples; i++) {
		if (input[i] == -2) {
			num = 0;
			if (start != -1)
				break;
			continue;
		}
		num++;
		if (num == thresh) {
			int b;
			start = i;
			for (b = i - thresh + 1; b <= i; b++) {
				input[c] = input[b];
				c++;
			}
		} else if (num > thresh) {
			input[c] = input[i];
			c++;
		}
	}

	max = c - 1;

	for (; c < samples; c++)
		input[c] = -2;

	return max;
}

int find_max(double *input, int samples)
{
	double peak = 0;
	int i = 0;
	int c = 0;
	int num_peaks = 0;

	for (i = 0; i < samples; i++) {
		if (input[i] == -2)
			continue;

		if (input[i] < 0.2 && input[i] > -0.2)
			continue;

		if (input[i] < 0) {
			if (peak < input[i]) {
				input[c] = peak;
				c++;
			} else {
				peak = input[i];
			}
		} else {
			if (peak > input[i]) {
				input[c] = peak;
				c++;
			} else {
				peak = input[i];
			}
		}
		/*
		if (input[i] > 0.1 || input[i] < -0.1) {
			input[c] = input[i];
			c++;
		}
		*/
	}

	num_peaks = c - 1;

	for (; c < samples; c++)
		input[c] = -2;

	return num_peaks;
}

int main(int argc, char **argv)
{
	SNDFILE *file1, *file2;
	SF_INFO info1, info2;
	double *in1, *in2;
	int min_frames, i;
	int like = 0, unlike = 0;
	int total = 0;
	int max1, max2;
	struct fft_average *avg;

	memset(&info1, 0, sizeof(SF_INFO));
	memset(&info2, 0, sizeof(SF_INFO));

	if (argc < 3) {
		printf("compare file1 file2");
		return 1;
	}

	file1 = sf_open(argv[1], SFM_READ, &info1);
	if (!file1) {
		printf("Error opening wave file %s\n", argv[1]);
		exit(1);
	}

	file2 = sf_open(argv[2], SFM_READ, &info2);
	if (!file2) {
		printf("Error opening wave file %s\n", argv[2]);
		sf_close(file2);
		exit(1);
	}

/*
	seconds = (float)info1.frames/(float)info1.samplerate;
	printf("Opened %s\n", argv[1]);
	printf("frames: %d\n", info1.frames);
	printf("samplerate: %d\n", info1.samplerate);
	printf("length (in seconds): %f\n", seconds);
	printf("channels: %d\n", info1.channels);
	printf("format: %x\n", info1.format);
	printf("sections: %d\n", info1.sections);
	printf("seekable: %d\n", info1.seekable);
*/

	in1 = (double *)malloc(sizeof(double) * info1.frames);
	if (!in1) {
		printf("Failed to allocate input array\n");
		sf_close(file1);
		sf_close(file2);
		return 1;
	}

	in2 = (double *)malloc(sizeof(double) * info2.frames);
	if (!in2) {
		printf("Failed to allocate input array\n");
		free(in1);
		sf_close(file1);
		sf_close(file2);
		return 1;
	}

	normalize_wave(file1, in1, info1.frames, info1.channels);
	normalize_wave(file2, in2, info2.frames, info2.channels);
	max1 = shift_wave(in1, info1.frames);
	max2 = shift_wave(in2, info2.frames);
//	max1 = find_max(in1, info1.frames);
//	max2 = find_max(in2, info2.frames);
/*
	shift_wave(in1, info1.frames);
	shift_wave(in2, info2.frames);
*/
	plsdev("xwin");
	plinit();
	plenv(0.0f, info1.frames, -1, 1, 0, 0);
	for (i = 0; i < info1.frames; i++) {
		PLFLT x;
		PLFLT sample = i;
		if (in1[i] == -2)
			continue;
		x = in1[i];
		plpoin(1, &sample, &x, 1);		
	}
	plend();
	
	plsdev("xwin");
	plinit();
	plenv(0.0f, info2.frames, -1, 1, 0, 0);
	for (i = 0; i < info2.frames; i++) {
		PLFLT x;
		PLFLT sample = i;

		if (in2[i] == -2)
			continue;
		x = in2[i];
		plpoin(1, &sample, &x, 1);		
	}
	plend();

	avg = do_fft(in1, max1);
	printf("yavg = %f, xavg=%f\n", avg->yavg, avg->xavg);
	free(avg);
	avg = do_fft(in2, max2);
	printf("yavg = %f, xavg=%f\n", avg->yavg, avg->xavg);
	free(avg);
	min_frames = (info1.frames < info2.frames) ? info1.frames : info2.frames;

	for (i = 0; i < min_frames; i++) {
		double diff;

		if (in1[i] == -2 || in2[i] == -2)
			break;

		if (in1[i] > in2[i])
			diff = in1[i] - in2[i];
		else
			diff = in2[i] - in1[i];

		if (diff <= 0.05)
			like++;
		else
			unlike++;
		total++;
	}

	printf("Minimum frames: %d\n", min_frames);
	printf("Total compares: %d\n", total);
	printf("Like: %d\n", like);
	printf("Unlike: %d\n", unlike);
	printf("Percentage alike: %f\n", ((double)like / (double)total) * 100);
	printf("Max1: %d\n", max1);
	printf("Max2: %d\n", max2);

	free(in1);
	free(in2);
	sf_close(file1);
	sf_close(file2);

	return 0;
}
