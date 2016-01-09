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
 *
 * Usage:
 *      texture -i [input_file] -o [output_file] -h [height] -w [weight] -s [scale] -d [direction] -m [patch_mode]
 *              -t [iteration]
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

int max_row = 500;
int max_col = 500;

/**
 * Texture generation function: the input and output matrix must be allocated with a valid dimension before calling
 * this function
 */
void generate(Mat& input, Mat& output, int iteration, float scaling_factor, float dir, Patch_Mode patch_mode) {

    int height = output.rows;
    int width = output.cols;

    int sample_height = input.rows;
    int sample_width = input.cols;

    // write directly on the output

    for (int row = 0; row < sample_height; row++)
        for (int col = 0; col < sample_width; col++)
            output.at<Vec3b>(row, col) = input.at<Vec3b>(row, col);

    // loop in order to cover the whole image

    Montage montage(max_row, max_col);
    montage.add_photo(input);
    montage.reset();
    montage.assemble(0,0,0);
    montage.show();
    waitKey(0);

    char key;
    int count  = 1;
    while(iteration--){
        int row = rand() % (max_row);
        int col = rand() % (max_col);
        float distance = float(row + col * dir)/sqrt(1.0 + dir * dir);
        float resize_factor = scaling_factor / (scaling_factor + distance);
        if (scaling_factor == 0)
            resize_factor = 1;

        Size size(int(input.cols * resize_factor), int(input.rows * resize_factor));
        Mat temp;
        resize(input,temp,size);

        cv::Point2f pc(temp.cols/2., temp.rows/2.);
        cv::Mat r = cv::getRotationMatrix2D(pc, -45, 1.0);
        cv::warpAffine(temp, temp, r, temp.size());

        montage.add_photo(temp);
        montage.assemble(count++,row,col);
        //waitKey();
        //cin >> key;
        //if (key == 'q')
          //  break;
    }
    montage.show();
    waitKey(0);
}

/*
 * Main function parses the parameters, allocates the memory and calls the corresponding function
 */
int main(int argc, char** argv) {

    // reading the parameters

    /*string input_file;
    string output_file;
    int height = 0;
    int width = 0;
    float scale = 1;
    float direction = 0;
    Patch_Mode patch_mode = Random;
    int iteration;

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
            default:
                return EXIT_FAILURE;
        }

    if (input_file == "" || output_file == "" || height == 0 || width == 0)
        return EXIT_FAILURE;

    // allocate the memory and load the image

    Mat input = imread(input_file, IMREAD_COLOR);
    Mat output(height, width, CV_8UC3);

    imshow(input_file, input);
    waitKey(0);

    // call the function

    generate(input, output, iteration, scale, direction, patch_mode);

    // show/save the result

    imshow(output_file, output);
    waitKey(0);*/
    string inputfile = "samples/bean.jpg";
    Mat input = imread(inputfile);
    if (input.rows >= 500)
        resize(input,input,Size(input.rows/10, input.cols/10));
    Mat output(input.rows, input.cols, CV_8UC3);
    generate(input,output,70,0,1,Patch_Mode(Random));

    return EXIT_SUCCESS;
}