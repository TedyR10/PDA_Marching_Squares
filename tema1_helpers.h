// Copyright Theodor-Ioan Rolea 2023
#ifndef TEMA1_HELPERS_H
#define TEMA1_HELPERS_H
#include <pthread.h>

struct thread_args {
	int id;
	int P;
	pthread_barrier_t *barrier;
    ppm_image *image;
    int step_x;
    int step_y;
    int sigma;
    char *filename;
    ppm_image **contour_map;
    ppm_image *scaled_image;
    unsigned char **grid;
};

#endif