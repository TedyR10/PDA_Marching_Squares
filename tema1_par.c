// Copyright Theodor-Ioan Rolea 2023
#define _POSIX_C_SOURCE 200112 // flag for barriers
#include "helpers.h"
#include "tema1_helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define CONTOUR_CONFIG_COUNT    16
#define FILENAME_MAX_SIZE       50
#define STEP                    8
#define SIGMA                   200
#define RESCALE_X               2048
#define RESCALE_Y               2048
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#define CLAMP(v, min, max) if(v < min) { v = min; } else if(v > max) { v = max; }

// Creates a map between the binary configuration (e.g. 0110_2) and the corresponding pixels
// that need to be set on the output image. An array is used for this map since the keys are
// binary numbers in 0-15. Contour images are located in the './contours' directory.
void init_contour_map(struct thread_args *args) {

    // Parallelize contour map
    int start = args->id * CONTOUR_CONFIG_COUNT / args->P;
    int end = MIN((args->id + 1) * CONTOUR_CONFIG_COUNT / args->P, CONTOUR_CONFIG_COUNT);

    for (int i = start; i < end; i++) {
        char filename[FILENAME_MAX_SIZE];
        sprintf(filename, "./contours/%d.ppm", i);
        args->contour_map[i] = read_ppm(filename);
    }
}

// Updates a particular section of an image with the corresponding contour pixels.
// Used to create the complete contour image.
void update_image(ppm_image *image, ppm_image *contour, int x, int y) {

    for (int i = 0; i < contour->x; i++) {
        for (int j = 0; j < contour->y; j++) {
            int contour_pixel_index = contour->x * i + j;
            int image_pixel_index = (x + i) * image->y + y + j;

            image->data[image_pixel_index].red = contour->data[contour_pixel_index].red;
            image->data[image_pixel_index].green = contour->data[contour_pixel_index].green;
            image->data[image_pixel_index].blue = contour->data[contour_pixel_index].blue;
        }
    }
}

// Corresponds to step 1 of the marching squares algorithm, which focuses on sampling the image.
// Builds a p x q grid of points with values which can be either 0 or 1, depending on how the
// pixel values compare to the `sigma` reference value. The points are taken at equal distances
// in the original image, based on the `step_x` and `step_y` arguments.
void sample_grid(struct thread_args *args) {
    int sigma = args->sigma;
    int step_x = args->step_x;
    int step_y = args->step_y;

    int p = args->scaled_image->x / step_x;
    int q = args->scaled_image->y / step_y;

    int start = args->id * p / args->P;
    int end = MIN((args->id + 1) * p / args->P, p);

    int start2 = args->id * q / args->P;
    int end2 = MIN((args->id + 1) * q / args->P, q);

    // Efficiently allocate memory for the grid
    for (int i = start; i <= end; i++) {
        args->grid[i] = (unsigned char *)malloc((q + 1) * sizeof(unsigned char));
        if (!args->grid[i]) {
            fprintf(stderr, "Unable to allocate memory\n");
            exit(1);
        }
    }

    pthread_barrier_wait(args->barrier);

    // Parallelize grid sampling
    for (int i = 0; i < p; i++) {
        for (int j = start2; j < end2; j++) {
            ppm_pixel curr_pixel = args->scaled_image->data[i * step_x * args->scaled_image->y + j * step_y];

            unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

            if (curr_color > sigma) {
                args->grid[i][j] = 0;
            } else {
                args->grid[i][j] = 1;
            }
        }
    }
    args->grid[p][q] = 0;

    // last sample points have no neighbors below / to the right, so we use pixels on the
    // last row / column of the input image for them
    for (int i = start; i < end; i++) {
        ppm_pixel curr_pixel = args->scaled_image->data[i * step_x * args->scaled_image->y + args->scaled_image->x - 1];

        unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

        if (curr_color > sigma) {
            args->grid[i][q] = 0;
        } else {
            args->grid[i][q] = 1;
        }
    }

    for (int j = start2; j < end2; j++) {
        ppm_pixel curr_pixel = args->scaled_image->data[(args->scaled_image->x - 1) * args->scaled_image->y + j * step_y];

        unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

        if (curr_color > sigma) {
            args->grid[p][j] = 0;
        } else {
            args->grid[p][j] = 1;
        }
    }

    pthread_barrier_wait(args->barrier);
}

// Corresponds to step 2 of the marching squares algorithm, which focuses on identifying the
// type of contour which corresponds to each subgrid. It determines the binary value of each
// sample fragment of the original image and replaces the pixels in the original image with
// the pixels of the corresponding contour image accordingly.
void march(struct thread_args *args) {
    int step_x = args->step_x;
    int step_y = args->step_y;
    int p = args->scaled_image->x / step_x;
    int q = args->scaled_image->y / step_y;

    // Parallelize marching
    int start = args->id * q / args->P;
    int end = MIN((args->id + 1) * q / args->P, q);

    for (int i = 0; i < p; i++) {
        for (int j = start; j < end; j++) {
            unsigned char k = 8 * args->grid[i][j] + 4 * args->grid[i][j + 1] + 2 * args->grid[i + 1][j + 1] + 1 * args->grid[i + 1][j];
            update_image(args->scaled_image, args->contour_map[k], i * step_x, j * step_y);
        }
    }

	pthread_barrier_wait(args->barrier);
}

// Calls `free` method on the utilized resources.
void free_resources(ppm_image *image, ppm_image **contour_map, unsigned char **grid, int step_x) {
    for (int i = 0; i < CONTOUR_CONFIG_COUNT; i++) {
        free(contour_map[i]->data);
        free(contour_map[i]);
    }
    free(contour_map);

    for (int i = 0; i <= image->x / step_x; i++) {
        free(grid[i]);
    }
    free(grid);

    free(image->data);
    free(image);
}

ppm_image *rescale_image(struct thread_args *args) {
    uint8_t sample[3];

    // Parallelize rescaling
    int start = args->id * (args->scaled_image->y / args->P);
    int end = MIN((args->id + 1) * (args->scaled_image->y / args->P), args->scaled_image->y);

    // use bicubic interpolation for scaling
    for (int i = 0; i < args->scaled_image->x; i++) {
        for (int j = start; j < end; j++) {
            float u = (float)i / (float)(args->scaled_image->x - 1);
            float v = (float)j / (float)(args->scaled_image->y - 1);
            sample_bicubic(args->image, u, v, sample);

            args->scaled_image->data[i * args->scaled_image->y + j].red = sample[0];
            args->scaled_image->data[i * args->scaled_image->y + j].green = sample[1];
            args->scaled_image->data[i * args->scaled_image->y + j].blue = sample[2];
        }
    }

    pthread_barrier_wait(args->barrier);

    return args->scaled_image;
}

void *f(void *arg) {
    struct thread_args *args = (struct thread_args *)arg;

    // 0. Initialize contour map
    init_contour_map(args);

    // 1. Rescale the image if needed
    if (args->scaled_image != args->image) {
        args->scaled_image = rescale_image(args);
    }

    // 2. Sample the grid
    sample_grid(args);

    // 3. March the squares
    march(args);

    // 4. Write output
    write_ppm(args->scaled_image, args->filename);

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: ./tema1 <in_file> <out_file> <P>\n");
        return 1;
    }

    int i, r;
	void *status;
    int P = atoi(argv[3]);

    ppm_image *image = read_ppm(argv[1]);
    int step_x = STEP;
    int step_y = STEP;

    // Initialize threads and thread arguments
    pthread_t threads[P];
	struct thread_args *args = malloc(P * sizeof(struct thread_args));
    
    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, P);

    // Map allocation
    ppm_image **map = (ppm_image **)malloc(CONTOUR_CONFIG_COUNT * sizeof(ppm_image *));
    if (!map) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }

    // Rescaling

    // Internal flag to check whether the image has to be rescaled or not
    int rescale = 0;
    // we only rescale downwards
    if (image->x <= RESCALE_X && image->y <= RESCALE_Y) {
        rescale = 1;
        args->scaled_image = image;
        args->image = image;
    } else {
        // alloc memory for image
        ppm_image *new_image = (ppm_image *)malloc(sizeof(ppm_image));
        if (!new_image) {
            fprintf(stderr, "Unable to allocate memory\n");
            exit(1);
        }
        new_image->x = RESCALE_X;
        new_image->y = RESCALE_Y;

        new_image->data = (ppm_pixel*)malloc(new_image->x * new_image->y * sizeof(ppm_pixel));
        if (!new_image) {
            fprintf(stderr, "Unable to allocate memory\n");
            exit(1);
        }
        args->scaled_image = new_image;
		args->image = image;
    }

    // Grid allocation

    // Check whether the image has to be rescaled or not
    int p, q;
    if (args->scaled_image != image) {
        p = RESCALE_X / step_x;
        q = RESCALE_Y / step_y;
    } else {
        p = image->x / step_x;
        q = image->y / step_y;
    }

    unsigned char **grid = (unsigned char **)malloc((p + 1) * sizeof(unsigned char*));
    if (!grid) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }

    // Start threads
    for (i = 0; i < P; i++) {
        args[i].id = i;
        args[i].P = P;
        args[i].barrier = &barrier;
        args[i].image = image;
        args[i].step_x = step_x;
        args[i].step_y = step_y;
        args[i].sigma = SIGMA;
        args[i].filename = argv[2];
        args[i].grid = grid;
        args[i].contour_map = map;
        if (rescale == 1) {
            args[i].scaled_image = image;
        } else {
            args[i].scaled_image = args->scaled_image;
        }

		r = pthread_create(&threads[i], NULL, f, &args[i]);

		if (r) {
			printf("Error creating thread id %d\n", i);
			exit(-1);
		}
	}

    // Wait for threads to finish
    for (i = 0; i < P; i++) {
		r = pthread_join(threads[i], &status);

		if (r) {
			printf("Error joining thread id %d\n", i);
			exit(-1);
		}
	}

    // Free memory for image
	if (args->scaled_image != args->image) {
		free(args->image->data);
		free(args->image);
	}
    
    // Free other resources
    free_resources(args->scaled_image, args->contour_map, args->grid, args->step_x);
	
    free(args);
   
	pthread_barrier_destroy(&barrier);

    return 0;
}
