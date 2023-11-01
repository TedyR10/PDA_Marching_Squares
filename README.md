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

## Algorithm Steps

1. **Initialize Contour Map**:
   - Create a map that associates binary configurations with corresponding
   contour images. These contour images are stored in the `./contours` directory.

2. **Rescale Image** (if needed):
   - Optionally rescale the input image using bicubic interpolation.

3. **Sample Grid**:
   - Create a grid of binary values based on pixel color comparisons with a
   specified threshold (sigma).

4. **March Squares**:
   - Determine the type of contour for each subgrid and update the original
   image with contour pixels.

5. **Write Output**:
   - Write the resulting image with contours to the specified output file.

6. **Free Resources**:
   - Release memory and resources used during processing.