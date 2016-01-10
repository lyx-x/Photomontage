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
 * N.B: always use the power to 2 for dimension to simplify the calculation
 */

#include <iostream>
#include <opencv2/imgproc/imgproc.hpp>
#include "montage.h"

using namespace std;
using namespace cv;

enum Patch_Mode {Random, Entire, Sub_Match};

/**
 * Texture generation function: the input and output matrix must be allocated with a valid dimension before calling
 * this function
 */
void generate(Mat& input, Mat& output, int iteration, float scaling_factor, float dir, Patch_Mode patch_mode = Random, int range = 0) {

    int height = output.rows;
    int width = output.cols;

    // loop in order to cover the whole image

    Montage montage(height, width);
    montage.add_photo(input);
    montage.reset();
    montage.assemble(0, 0, 0); // add the first image

    int count  = 1;
    while(iteration--) {
        int row = rand() % height;
        int col = rand() % width;
        int rotation = (range > 0) ? rand() % range : 0; // add random rotation

        float distance = float((row + col * dir) / sqrt(1.0 + dir * dir));
        float resize_factor = scaling_factor / (scaling_factor + distance);
        if (scaling_factor == 0)
            resize_factor = 1;

        Size size(int(input.cols * resize_factor), int(input.rows * resize_factor));
        Mat temp;
        resize(input,temp,size);

        cv::Point2f pc(temp.cols / 2.0f, temp.rows / 2.0f);
        cv::Mat r = cv::getRotationMatrix2D(pc, rotation, 1.0);
        cv::warpAffine(temp, temp, r, temp.size());

        montage.add_photo(temp);
        montage.assemble(count++,row,col);
    }

    montage.save_output(output);
    //montage.show();
    montage.save_mask("results/mask.jpg");

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
    Mat output(height + 2 * input.rows / 3, width + 2 * input.cols / 3, CV_8UC3);

    imshow(input_file, input);

    // call the function

    generate(input, output, iteration, scale, direction, patch_mode, range);

    // show/save the result

    Rect rect(input.cols / 3, input.rows / 3, width - input.cols / 3 - 1, height - input.rows / 3 - 1);
    Mat output_nap = output(rect);
    imwrite(output_file, output_nap);

    imshow(output_file, output_nap);
    waitKey(0);

    return EXIT_SUCCESS;
}