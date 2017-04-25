#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <time.h>

#define LENGTH 1024
#define uchar unsigned char

enum errorCode {
	USAGE = 1,
	OPENING,
	READING,
	FORMAT,
	PTHREAD,
	TIME,
	PJOIN
};

struct argument {
	uchar **imageR;
	uchar **imageG;
	uchar **imageB;
	int start;
	int end;
	int height;
	int width;
	uchar **res;
};

void *Sobel(void *arg);

void ntoa(uchar *buff, double number, int n)
{
	int i = 0;
	int j = 0;
	int point;
	unsigned long high = (unsigned long)number;
	double low = number - (unsigned long)number;

	while (high > 0 && i < n) {
		buff[i] = high%10 + '0';
		high = high/10;
		i++;
	}
	point = i;
	for (j = 0; j < i/2; j++) {
		uchar temp;

		temp = buff[j];
		buff[j] = buff[i-j-1];
		buff[i-j-1] = temp;
	}
	if (number < 1) {
		buff[0] = '0';
		point++;
	}
	buff[point] = 0;
	if (low > 0) {
		uchar lowstr[LENGTH];

		i = 0;
		buff[point] = '.';
		buff[point+1] = 0;
		while (low > 0 && i+point < n) {
			low = low*10.0;
			lowstr[i] = (char)low + '0';
			low = low - (unsigned long)low;
			i++;
		}
		lowstr[i] = 0;
		strcat(buff, lowstr);
	}
}

void operror(char code)
{
	char str[LENGTH] = {0};

	switch (code) {
	case 1:
		strcpy(str, "Usage: <image.ppm> <num of threads>\n");
		break;
	case 2:
		strcpy(str, "Error: opening file\n");
		break;
	case 3:
		strcpy(str, "Error: reading file\n");
		break;
	case 4:
		strcpy(str, "Error: format\n");
		break;
	case 5:
		strcpy(str, "Error: pthread create\n");
		break;
	case 6:
		strcpy(str, "Error: timer\n");
		break;
	case 7:
		strcpy(str, "Error: cannot join thread\n");
		break;
	default:
		strcpy(str, "Some error occured\n");
	}
	write(2, str, strlen(str));
	exit(code);
}

int max3(int a1, int a2, int a3)
{
	if (a1 >= a2 && a1 >= a3)
		return a1;
	if (a2 >= a1 && a2 >= a3)
		return a2;
	if (a3 >= a1 && a3 >= a2)
		return a3;
	return 0;
}

int parseImage(char *name, int *width, int *height)
{
	uchar buff[LENGTH] = {0};
	int nread;
	int ppm;
	int i = 0;
	int sizes;
	int w = 0;
	int h = 0;
	int offset = 0;

	ppm = open(name, O_RDONLY);
	if (ppm == -1)
		operror(OPENING);
	nread = read(ppm, buff, 3);
	if (nread < 3)
		operror(READING);
	if (buff[0] != 'P' || buff[1] != '6')
		operror(FORMAT);
	nread = read(ppm, buff, LENGTH);
	if (nread == 0)
		operror(READING);
	close(ppm);
	i = 0;
	offset = 3;
	while (buff[i] == '#') {
		while (buff[i] != 0x0A)
			i++;
		i++;
	}
	sizes = 3;
	while (sizes > 0) {
		while (buff[i] != 0x0A && buff[i] != 0x20) {
			if (sizes == 3)
				w = w*10 + (buff[i] - '0');
			if (sizes == 2)
				h = h*10 + (buff[i] - '0');
			if (sizes == 1)
				;
			i++;
		}
		sizes--;
		i++;
	}
	offset += i;
	*height = h;
	*width = w;
	return offset;
}

void loadImage(uchar **imageR, uchar **imageG, uchar **imageB,
				char *name, int width, int height, int offset)
{
	uchar buff[LENGTH] = {0};
	int nread;
	int x = 0;
	int y = 0;
	int ppm;
	int i = 0;

	ppm = open(name, O_RDONLY);
	if (ppm == -1)
		operror(OPENING);
	nread = read(ppm, buff, LENGTH);
	if (nread == 0)
		operror(READING);
	i = offset;
	while (nread != 0) {
		imageR[y][x] = buff[i];
		i++;
		if (i >= nread) {
			nread = read(ppm, buff, LENGTH);
			if (nread == -1)
				operror(READING);
			i = 0;
		}
		imageG[y][x] = buff[i];
		i++;
		if (i >= nread) {
			nread = read(ppm, buff, LENGTH);
			if (nread == -1)
				operror(READING);
			i = 0;
		}
		imageB[y][x] = buff[i];
		x++;
		if (x >= width) {
			x = 0;
			y++;
			if (y >= height)
				break;
		}
		i++;
		if (i >= nread) {
			nread = read(ppm, buff, LENGTH);
		if (nread == -1)
			operror(READING);
		i = 0;
		}
	}
	close(ppm);
}

void *Sobel(void *arg)
{
	int i = 0;
	int j = 0;
	int Gxr, Gxg, Gxb;
	int Gyr, Gyg, Gyb;
	int fr, fb, fg, f;
	struct argument *a;

	a = (struct argument *)arg;
	if (a->start == 0)
		a->start++;
	if (a->end == a->height)
		a->end--;
	for (i = a->start; i < a->end; i++) {
		for (j = 1; j < a->width-1; j++) {
			Gxr = a->imageR[i+1][j-1] + 2*a->imageR[i+1][j]
				+ a->imageR[i+1][j+1] -
				 (a->imageR[i-1][j-1] + 2*a->imageR[i-1][j]
				+ a->imageR[i-1][j+1]);
			Gyr = a->imageR[i-1][j+1] + 2*a->imageR[i][j+1]
				+ a->imageR[i+1][j+1] -
				 (a->imageR[i-1][j-1] + 2*a->imageR[i][j-1]
				+ a->imageR[i+1][j-1]);

			Gxg = a->imageG[i+1][j-1] + 2*a->imageG[i+1][j]
				+ a->imageG[i+1][j+1] -
				 (a->imageG[i-1][j-1] + 2*a->imageG[i-1][j]
				+ a->imageG[i-1][j+1]);
			Gyg = a->imageG[i-1][j+1] + 2*a->imageG[i][j+1]
				+ a->imageG[i+1][j+1] -
				 (a->imageG[i-1][j-1] + 2*a->imageG[i][j-1]
				+ a->imageG[i+1][j-1]);

			Gxb = a->imageB[i+1][j-1] + 2*a->imageB[i+1][j]
				+ a->imageB[i+1][j+1] -
				 (a->imageB[i-1][j-1] + 2*a->imageB[i-1][j]
				+ a->imageB[i-1][j+1]);
			Gyb = a->imageB[i-1][j+1] + 2*a->imageB[i][j+1]
				+ a->imageB[i+1][j+1] -
				 (a->imageB[i-1][j-1] + 2*a->imageB[i][j-1]
				+ a->imageB[i+1][j-1]);

			fr = sqrt(Gxr*Gxr+Gyr*Gyr);
			fg = sqrt(Gxg*Gxg+Gyg*Gyg);
			fb = sqrt(Gxb*Gxb+Gyb*Gyb);
			f = max3(fr, fg, fb)/1.5;
			if (f > 180)
				a->res[i][j] = 255;
			else if (f < 20)
				a->res[i][j] = 0;
			else
				a->res[i][j] = f;
		}
	}
	pthread_exit(0);
}

int main(int argc, char *argv[])
{
	uchar buff[LENGTH] = {0};
	int width = 0;
	int height = 0;
	int ppm;
	int i = 0;
	int j = 0;
	int offset = 0;
	uchar **imageR;
	uchar **imageG;
	uchar **imageB;
	uchar **res;
	pthread_t *threads;
	int numthreads = 0;
	struct argument *arguments;
	struct timespec start;
	struct timespec end;
	double elapsed;

	if (argc != 3)
		operror(USAGE);

	offset = parseImage(argv[1], &width, &height);

	imageR = (uchar **)malloc(height*sizeof(uchar *));
	for (j = 0; j < height; j++)
		imageR[j] = (uchar *)malloc(width*sizeof(uchar));

	imageG = (uchar **)malloc(height*sizeof(uchar *));
	for (j = 0; j < height; j++)
		imageG[j] = (uchar *)malloc(width*sizeof(uchar));

	imageB = (uchar **)malloc(height*sizeof(uchar *));
	for (j = 0; j < height; j++)
		imageB[j] = (uchar *)malloc(width*sizeof(uchar));

	loadImage(imageR, imageG, imageB, argv[1], width, height, offset);

	res = (uchar **)calloc(height, sizeof(uchar *));
	for (j = 0; j < height; j++)
		res[j] = (uchar *)calloc(width, sizeof(uchar));
	i = 0;
	while (i < strlen(argv[2])) {
		numthreads *= 10;
		numthreads += argv[2][i] - '0';
		i++;
	}

	threads = (pthread_t *)malloc(numthreads*sizeof(pthread_t));
	arguments = (struct argument *)malloc(numthreads*sizeof(struct argument));

	j = clock_gettime(CLOCK_REALTIME, &start);
	if (j != 0)
		operror(TIME);

	for (i = 0; i < numthreads; i++) {
		arguments[i].imageR = imageR;
		arguments[i].imageG = imageG;
		arguments[i].imageB = imageB;
		arguments[i].height = height;
		arguments[i].width = width;
		arguments[i].res = res;
		arguments[i].start = i*(height/numthreads);
		arguments[i].end = arguments[i].start + height/numthreads;

		j = pthread_create(threads+i, NULL, Sobel, (void *)(arguments+i));
		if (j != 0)
			operror(PTHREAD);
	}

	for (i = 0; i < numthreads; i++)
		if (pthread_join(threads[i], NULL) != 0)
			operror(PJOIN);

	j = clock_gettime(CLOCK_REALTIME, &end);
	if (j != 0)
		operror(TIME);

	elapsed = end.tv_sec - start.tv_sec;
	elapsed += (end.tv_nsec - start.tv_nsec) / 1000000000.0;

	ppm = open("time.txt", O_WRONLY | O_CREAT | O_APPEND, 0664);
	if (ppm == 0)
		operror(OPENING);
	ntoa(buff, numthreads, 10);
	write(ppm, buff, strlen(buff));
	write(ppm, " ", 1);
	ntoa(buff, elapsed, 10);
	write(ppm, buff, strlen(buff));
	write(ppm, "\n", 1);
	close(ppm);

	ppm = creat("outn3.ppm", 0664);
	write(ppm, "P5\n", 3);
	ntoa(buff, width, LENGTH);
	write(ppm, buff, strlen(buff));
	write(ppm, " ", 1);
	ntoa(buff, height, LENGTH);
	write(ppm, buff, strlen(buff));
	write(ppm, "\n255\n", 5);

	for (i = 0; i < height; i++)
		write(ppm, res[i], width);
	close(ppm);

	for (i = 0; i < height; i++) {
		free(imageR[i]);
		free(imageG[i]);
		free(imageB[i]);
		free(res[i]);
	}
	free(threads);
	free(arguments);

	return 0;
}
