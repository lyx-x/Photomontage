/*
 * Texture program
 *
 * This program generates a texture image from a smaller sample image. It uses the Graph Cut technique described in
 * a 2003 paper "Graphcut Textures: Image and Video Synthesis Using Graph Cut".
 *
 * Parameters:
 *      i: path to the sample png image (with a smaller dimension)
 *      o: path to the output png image
 *      h: height of output image
 *      w: width of output image
 *      r: rotation range (from -r to r) in degree
 *      s: scaling factor in float ((0, 0) is set to 1, (a, d * a) is set to scale ^ a)
 *      d: scaling direction in tangent form (d = delta_y / delta_x)
 *
 * Usage:
 *      texture -i [input_file] -o [output_file] -h [height] -w [weight] -r [rot_range] -s [scale] -d [direction]
 */

#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;

/**
 * Texture generation function: the input and output matrix must be allocated with a valid dimension before calling
 * this function
 */
void generate(Mat& input, Mat& output, int iteration,  int rot_range, int scaling_factor, int dir) {
    // loop several time to get a good result
    while(iteration--) {
        // get a small piece of input file (eg. 32 * 32)

        // put the piece on a temporary matrix and overlay it on the previous output matrix (with rotation and scaling)

        // find the minimum cut inside the overlay area (eg. 4 or 8 in width)

        // generate new output matrix with the cut value

    }
}

/*
 * Main function parses the parameters, allocates the memory and calls the corresponding function
 */
int main(int argc, char** argv) {

    // reading the parameters

    string input_file;
    string output_file;
    int height = 0;
    int width = 0;
    int rotation_range = 0;
    float scale = 1;
    float direction = 0;

    for (int i = 1; i < argc; i++)
        switch(argv[i][1]) {
            case 'h':
                height = atoi(argv[++i]);
                break;
            case 'w':
                width = atoi(argv[++i]);
                break;
            case 'r':
                rotation_range = atoi(argv[++i]);
                break;
            case 's':
                scale = float(atof(argv[++i]));
                break;
            case 'd':
                direction = float(atof(argv[++i]));
                break;
            case 'i': // input file
                input_file = argv[++i];
                break;
            case 'o': // output file
                output_file = argv[++i];
                break;
            default:
                return EXIT_FAILURE;
        }

    if (input_file == "" || output_file == "" || height == 0 || width == 0)
        return EXIT_FAILURE;

    // allocate the memory and load the image



    // call the function

    // show/save the result

    return EXIT_SUCCESS;
}