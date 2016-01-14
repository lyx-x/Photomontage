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
 *      s: scaling factor in float ((0, 0) is set to 1, (a, d * a) is set to scale ^ a)
 *      d: scaling direction in tangent form (d = delta_y / delta_x)
 *      m: patch finding mode (0 for Random placement, 1 for Entire patch matching, or 2 for Sub-patch matching)
 *      t: number of iterations
 *      r: rotation range
 *
 * Usage:
 *      texture -i [input_file] -o [output_file] -h [height] -w [weight] -s [scale] -d [direction] -m [patch_mode]
 *              -t [iteration] -r [rotation_range]
 *
 * Example:
 *      texture -i samples/floor.jpg -o results/floor.jpg -h 256 -w 256
 *
 * N.B: when choose a small scaling factor, make sure the iteration is large enough to cover
 */

#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include "montage.h"

using namespace std;
using namespace cv;

enum Patch_Mode {Random, Entire, Sub_Match}; // only the random method has been implemented

/**
 * Texture generation function: the input and output matrix must be allocated with a valid dimension before calling
 * this function
 */
void generate(Mat& input, Mat& output, int iteration, float scaling_factor, float dir, Patch_Mode patch_mode = Random, int range = 0) {

    int height = output.rows;
    int width = output.cols;

    // prepare the nap

    Montage montage(height, width, height / 3, width / 3);
    montage.add_photo(input);
    montage.reset();
    montage.assemble(0, 0, 0); // add the first image

    // loop in order to cover the whole image

    int count  = 1;
    while (iteration--) {
        int row = rand() % (height + height / 3 * 2); // random point on the whole nap
        int col = rand() % (width + width / 3 * 2);
        int rotation = (range > 0) ? rand() % range : 0; // add random rotation

        float distance = float((row + col * dir) / sqrt(1.0 + dir * dir));
        float resize_factor = pow(scaling_factor,(distance/height));
        if (scaling_factor == 0)
            resize_factor = 1;

        Size size(int(input.cols * resize_factor), int(input.rows * resize_factor));
        Mat tmp;
        resize(input, tmp, size);

        // rotate the image

        Point2f pc(tmp.cols / 2.0f, tmp.rows / 2.0f);
        Mat r = getRotationMatrix2D(pc, rotation, 1.0);
        warpAffine(tmp, tmp, r, tmp.size());

        montage.add_photo(tmp);
        montage.assemble(count++, row, col);
    }

    montage.save_output(output);
    // montage.save_mask("results/mask.jpg");

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
    float scale = 0;
    float direction = 1;
    Patch_Mode patch_mode = Random;
    int iteration = 0;
    int range = 0;

    for (int i = 1; i < argc; i++)
        switch (argv[i][1]) {
            case 'i': // input file
                input_file = argv[++i];
                break;
            case 'o': // output file
                output_file = argv[++i];
                break;
            case 'h':
                height = atoi(argv[++i]);
                break;
            case 'w':
                width = atoi(argv[++i]);
                break;
            case 's':
                scale = float(atof(argv[++i]));
                break;
            case 'd':
                direction = float(atof(argv[++i]));
                break;
            case 'm':
                patch_mode = Patch_Mode(atoi(argv[++i]));
                break;
            case 't':
                iteration = atoi(argv[++i]);
                break;
            case 'r':
                range = atoi(argv[++i]);
                break;
            default:
                return EXIT_FAILURE;
        }

    if (input_file == "" || output_file == "" || height == 0 || width == 0)
        return EXIT_FAILURE;

    // allocate the memory and load the image

    Mat input = imread(input_file, IMREAD_COLOR);
    Mat output(height, width, CV_8UC3);

    imshow(input_file, input);

    // call the function

    generate(input, output, iteration, scale, direction, patch_mode, range);

    // show/save the result

    imwrite(output_file, output);

    imshow(output_file, output);
    waitKey(0);

    return EXIT_SUCCESS;
}