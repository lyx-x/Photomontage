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
 *      t: number of iterations
 *      a: size of small sample
 *      e: size of overlap width
 *
 * Usage:
 *      texture -i [input_file] -o [output_file] -h [height] -w [weight] -r [rot_range] -s [scale] -d [direction]
 *              -t [iteration] -a [size] -e [overlap]
 *
 * Example:
 *      texture -i samples/floor.jpg -o results/floor.jpg -h 256 -w 256 -a 64 -e 16
 */

#include <iostream>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

/**
 * Texture generation function: the input and output matrix must be allocated with a valid dimension before calling
 * this function
 */
void generate(Mat& input, Mat& output, int size, int overlap, int iteration, int rot_range, float scaling_factor, float dir) {

    // loop several time to get a good result

    int height = output.rows;
    int width = output.cols;

    int sample_height = input.rows;
    int sample_width = input.cols;

    int extra = size - overlap;

    // use a larger image to avoid border control

    Mat temp(height + 2 * extra, width + 2 * extra, CV_8UC3);



    while(iteration--) {

        // get a small piece of input file (eg. 32 * 32 or size * size)

        // put the piece on a temporary matrix and overlay it on the previous output matrix (with rotation and scaling)

        // find the minimum cut inside the overlay area (eg. 8 or overlap in width)

        // generate new output matrix with the cut value

    }

    // return only a part of the large texture file

    output = temp(Range(extra, height + extra), Range(extra, width + extra)).clone();

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
    int iteration = 0;
    int size = 32;
    int overlap = 8;

    for (int i = 1; i < argc; i++)
        switch (argv[i][1]) {
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
            case 't':
                iteration = atoi(argv[++i]);
                break;
            case 'a':
                size = atoi(argv[++i]);
                break;
            case 'e':
                overlap = atoi(argv[++i]);
                break;
            default:
                return EXIT_FAILURE;
        }

    if (input_file == "" || output_file == "" || height == 0 || width == 0)
        return EXIT_FAILURE;

    // allocate the memory and load the image

    Mat input = imread(input_file, IMREAD_COLOR);
    Mat output;

    imshow(input_file, input);
    waitKey(0);

    // call the function

    generate(input, output, size, overlap, iteration, rotation_range, scale, direction);

    // show/save the result

    imshow(output_file, output);
    waitKey(0);

    return EXIT_SUCCESS;
}