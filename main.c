#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>

#define LENGTH 1024
#define uchar unsigned char

enum errorCode {
	USAGE = 1,
	OPENING,
	READING,
	FORMAT,
	PTHREAD
};

struct argument {
	uchar** imageR;
	uchar** imageG;
	uchar** imageB;
	int start;
	int end;
	int height;
	int width;
	uchar** res;
};

void *Sobel(void *arg);

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
		default:
			strcpy(str, "Some error occured\n");
	}
	write(2, str, strlen(str));
	exit(code);
}

int max3(int a1, int a2, int a3)
{
	if (a1 >= a2 && a1 >= a3) return a1;
	if (a2 >= a1 && a2 >= a3) return a2;
	if (a3 >= a1 && a3 >= a2) return a3;
	return 0;
}

int parseImage(char* name, int* width, int* height)
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

void loadImage(uchar** imageR, uchar** imageG, uchar** imageB,
				char* name, int width, int height, int offset)
{
	uchar buff[LENGTH] = {0};
	int nread;
	int x = 0;
	int y = 0;
	int ppm;
    int i = 0;
	int j = 0;
	
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

void* Sobel(void *arg)
{
//uchar** imageR, uchar** imageG, uchar** imageB,
//			int start, int end, int height, int width, uchar** res

	int i = 0;
	int j = 0;
	int Gxr, Gxg, Gxb;
	int Gyr, Gyg, Gyb;
	int fr, fb, fg, f;
	struct argument* a;

	a = (struct argument*)arg;
printf("b\n");
	if (a->start == 0)
		a->start++;
	if (a->end == a->height)
		a->end--;
	for (i = a->start; i < a->end; i++) {
		for (j = 1; j < a->width-1; j++) {
			Gxr = a->imageR[i+1][j-1] + 2*a->imageR[i+1][j] + a->imageR[i+1][j+1] -
				 (a->imageR[i-1][j-1] + 2*a->imageR[i-1][j] + a->imageR[i-1][j+1]);
			Gyr = a->imageR[i-1][j+1] + 2*a->imageR[i][j+1] + a->imageR[i+1][j+1] -
				 (a->imageR[i-1][j-1] + 2*a->imageR[i][j-1] + a->imageR[i+1][j-1]);

			Gxg = a->imageG[i+1][j-1] + 2*a->imageG[i+1][j] + a->imageG[i+1][j+1] -
				 (a->imageG[i-1][j-1] + 2*a->imageG[i-1][j] + a->imageG[i-1][j+1]);
			Gyg = a->imageG[i-1][j+1] + 2*a->imageG[i][j+1] + a->imageG[i+1][j+1] -
				 (a->imageG[i-1][j-1] + 2*a->imageG[i][j-1] + a->imageG[i+1][j-1]);

			Gxb = a->imageB[i+1][j-1] + 2*a->imageB[i+1][j] + a->imageB[i+1][j+1] -
				 (a->imageB[i-1][j-1] + 2*a->imageB[i-1][j] + a->imageB[i-1][j+1]);
			Gyb = a->imageB[i-1][j+1] + 2*a->imageB[i][j+1] + a->imageB[i+1][j+1] -
				 (a->imageB[i-1][j-1] + 2*a->imageB[i][j-1] + a->imageB[i+1][j-1]);

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
//printf("aaa\n");
	}
	pthread_exit(0);
}

int main(int argc, char* argv[])
{
	char str[LENGTH] = {0};
	uchar buff[LENGTH] = {0};
	int width = 0;
	int height = 0;
	int ppm;
    int i = 0;
	int j = 0;
	int offset = 0;
	uchar** imageR;
	uchar** imageG;
	uchar** imageB;
	uchar** res;
	pthread_t* threads;
	int numthreads = 0;
	struct argument* arguments;

	if (argc != 3)
		operror(USAGE);
	
	offset = parseImage(argv[1], &width, &height);

	imageR = (uchar**)malloc(height*sizeof(uchar*));
	for (j = 0; j < height; j++)
		imageR[j] = (uchar*)malloc(width*sizeof(uchar));
	
	imageG = (uchar**)malloc(height*sizeof(uchar*));
	for (j = 0; j < height; j++)
		imageG[j] = (uchar*)malloc(width*sizeof(uchar));

	imageB = (uchar**)malloc(height*sizeof(uchar*));
	for (j = 0; j < height; j++)
		imageB[j] = (uchar*)malloc(width*sizeof(uchar));

	loadImage(imageR, imageG, imageB, argv[1], width, height, offset);


	res = (uchar**)calloc(height, sizeof(uchar*));
	for (j = 0; j < height; j++)
		res[j] = (uchar*)calloc(width, sizeof(uchar));
	i = 0;
	while (i < strlen(argv[2])) {
		numthreads *= 10;
		numthreads += argv[2][i] - '0';
		i++;
	}

printf("%i\n", numthreads);
printf("%i\n", width);
printf("%i\n", height);
	
	threads = (pthread_t*)malloc(numthreads*sizeof(pthread_t));
	arguments = (struct argument*)malloc(sizeof(struct argument));

	arguments->imageR = imageR;
	arguments->imageG = imageG;
	arguments->imageB = imageB;
	arguments->height = height;
	arguments->width = width;
	arguments->res = res;

printf("c\n");

	for (i = 0; i < numthreads; i++) {
		arguments->start = i*(height/numthreads);
		arguments->end = arguments->start + height/numthreads;
printf("st %i end %i\n", arguments->start, arguments->end);

		j = pthread_create(threads+i, NULL, Sobel, (void *)arguments);
		if (j != 0)
			operror(PTHREAD);
printf("e\n");

	}

	for (i = 0; i < numthreads; i++)
		pthread_join(threads[i], NULL);

//	sleep(1);
	//Sobel(imageR, imageG, imageB, 0, height, height, width, res);
printf("ff\n");
	ppm = creat("outn3.ppm", 0664);
	snprintf(buff, 12,"%d",width);
	write(ppm, "P5\n", 3);
	write(ppm, buff, strlen(buff));
	write(ppm, " ", 1);
	snprintf(buff, 12,"%d",height);
	write(ppm, buff, strlen(buff));
	write(ppm, "\n255\n", 5);

	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			write(ppm, res[i] + j, 1);
		}
	}

	return 0;
}
