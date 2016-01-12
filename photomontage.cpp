/*
 * Photomontage program
 *
 * This program assembles two photos manually. User needs to enter the desired position of two photos in order to
 * see the result.
 *
 * Parameters:
 *      i: number of input images and their name
 *      o: path to the output image
 *      h: height of output image
 *      w: width of output image
 *
 * Usage:
 *      montage -i [number_of_photos] [photo_1] .. [photo_n] -o [output_file] -h [height] -w [weight]
 *
 * Usage of control window:
 *      Deplace the trackbar to control the position
 *      Left click to setup constraint with small circle
 *      Right click to clean all existing constraints of that image
 *
 */

#include <iostream>
#include <map>
#include <set>
#include <opencv2/imgproc/imgproc.hpp>

#include "montage.h"

using namespace std;
using namespace cv;

Montage montage(600,1024);

vector<Mat> photos;
vector<set<pair<int,int>>> constraints;
vector<int> photo_index;
vector<string> input_files;
int *value_row, *value_col;

const int range = 5; // use small circle instead of a single pixel for control

void assemble() {
    montage.reset();
    for(int i = 0; i < photos.size(); i++)
        montage.assemble(i, value_row[i], value_col[i], &constraints[i]);
    montage.show();
}

void on_mouse(int event, int x, int y, int flag, void* p) {
    int* index = (int*)p;
    set<pair<int,int>>* constraint = &constraints[*index];
    switch (event) {
        case EVENT_LBUTTONDOWN:
            for (int i = 0 - range; i <= range; i++)
                for (int j = 0 - range; j <= range; j++) {
                    int _x = x + i;
                    int _y = y + j;
                    if (i * i + j * j <= range * range)
                        if (_x >= 0 && _x < photos[*index].cols && _y >= 0 && _y <= photos[*index].rows)
                            constraint->insert(make_pair(_y, _x));
                }
            break;
        case EVENT_RBUTTONDOWN:
            constraint->clear();
            break;
        default:
            return;
    }
    Mat tmp = photos[*index].clone();
    for (auto px : *constraint)
        tmp.at<Vec3b>(px.first, px.second) = Vec3b(0, 255, 0);
    imshow(input_files[*index], tmp);
    assemble();
}

void on_trackbar(int, void*){
    assemble();
}

int main(int argc, char** argv) {

    // initialization

    string output_file;
    int height = 480;
    int width = 800;
    int num_files = 0;

    for (int i = 1; i < argc; i++)
        switch (argv[i][1]) {
            case 'i': // input files
                num_files = atoi(argv[++i]);
                // read the file name one by one
                for (int _i = 0; _i < num_files; _i++)
                    input_files.push_back(argv[++i]);
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
            default:
                return EXIT_FAILURE;
        }

    int extra_height = height / 4; // extra space to manipulate the nap
    int extra_width = width / 4;

    // preparation

    photo_index.clear();
    for (int i = 0; i < num_files; i++) {
        photo_index.push_back(i);
        Mat img = imread(input_files[i], CV_LOAD_IMAGE_COLOR);
        photos.push_back(img);
        height = max(height, img.rows);
        width = max(width, img.cols);
        // add new set of constraints
        set<pair<int,int>> constraint;
        constraints.push_back(constraint);
    }

    value_row = new int[num_files];
    value_col = new int[num_files];

    Mat output;
    montage = Montage(height + extra_height, width + extra_width);

    for (int i = 0; i < num_files; i++) {
        namedWindow(input_files[i], CV_GUI_NORMAL);
        imshow(input_files[i], photos[i]);
        montage.add_photo(photos[i]);
        // set random position at first
        value_row[i] = rand() % (height + extra_height - photos[i].rows);
        value_col[i] = rand() % (width + extra_width - photos[i].cols);
        // add position control
        createTrackbar("Row", input_files[i], value_row + i, height + extra_height - photos[i].rows - 1, on_trackbar);
        createTrackbar("Col", input_files[i], value_col + i, width + extra_width - photos[i].cols - 1, on_trackbar);
        // add mouse callback
        setMouseCallback(input_files[i], on_mouse, &photo_index[i]);
    }

    assemble();
    waitKey(0);

    return EXIT_SUCCESS;
}