**Name: Theodor-Ioan Rolea**

**Group: 333CA**

# HW 1 APD

### Description:

* This project implements the Marching Squares algorithm for contour detection
on an input image. The program reads an input image in PPM format and performs
contour detection using parallel processing. The Marching Squares algorithm is
used to identify and extract contours from a given image. It works by sampling
the image, creating a grid of binary values, and then determining the type of
contour for each subgrid. The code parallelizes these steps for efficient
processing, achieving a speed up of over **350%**.
***

## Algorithm Steps & Parallelization:

1. **Initialize Contour Map**:
   - Create a map that associates binary configurations with corresponding
   contour images. Split the for loop into multiple ranges and assign each
   thread a range to process.

2. **Rescale Image** (if needed):
   - Optionally rescale the input image using bicubic interpolation. Done the
   same way as in the previous step, by splitting the for loop into multiple
   ranges and assigning each thread a range to process. I employed a
   barrier to ensure that all threads finish processing before continuing.

3. **Sample Grid**:
   - Create a grid of binary values based on pixel color comparisons with a
   specified threshold (sigma). Firstly, efficiently allocated memory for the
   grid the same way, with start and end. After all the threads have finished,
   start grid sampling by splitting the for loop into multiple ranges and
   assigning each thread a range to process. Do the same for the outer edges &
   wait for all threads to finish.

4. **March Squares**:
   - Determine the type of contour for each subgrid and update the original
   image with contour pixels. Same as above, split the for loop into multiple
   ranges and then waited for all the threads to finish by using a barrier.

5. **Write Output**:
   - Write the resulting image to the specified output file.

6. **Free Resources**:
   - Free memory and resources used during processing.

***

## Final thoughts:

* I really enjoyed working on this project. I learned a lot about parallel
programming and how to efficiently parallelize code. I am glad that I got to
use what I learned in the APD course and labs and apply it to the project.