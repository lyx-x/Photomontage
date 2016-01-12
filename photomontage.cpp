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
 */

#include <iostream>
#include <map>
#include <set>
#include <opencv2/imgproc/imgproc.hpp>

#include "montage.h"

//#include "graph.h"

using namespace std;
using namespace cv;

Montage montage(600,1024);

vector<Mat> photos;
vector<set<pair<int,int>>> constraints;
int *value_row, *value_col;

const int range = 5; // use small square instead of a single pixel for control

void assemble() {
    montage.reset();
    for(int i = 0; i < photos.size(); i++)
        montage.assemble(i, value_row[i], value_col[i]);
    montage.show();
}

void on_mouse(int event, int x, int y, int flag, void* p) {
    set<pair<int,int>> *constraint = (set<pair<int,int>>*)p;
    switch (event) {
        case EVENT_LBUTTONDOWN:
            cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
            for (int i = 0 - range; i <= range; i++)
                for (int j = 0 - range; j <= range; j++) {
                    
                }
            constraint->insert(make_pair(x, y));
            break;
        case EVENT_RBUTTONDOWN:
            cout << "Right button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
            break;
        default:
            return;
    }
    assemble();
}

void on_trackbar(int, void*){
    assemble();
}

int main(int argc, char** argv) {

    // initialization

    vector<string> input_files;
    string output_file;
    int height = 480;
    int width = 800;
    int extra_height = 0; // extra space to manipulate the nap
    int extra_width = 0;
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

    // preparation

    for (int i = 0; i < num_files; i++) {
        Mat img = imread(input_files[i], CV_LOAD_IMAGE_COLOR);
        photos.push_back(img);
        extra_height = max(extra_height, img.rows);
        extra_width = max(extra_width, img.cols);
        // add new set of constraints
        set<pair<int,int>> constraint;
        constraints.push_back(constraint);
    }

    value_row = new int[num_files];
    value_col = new int[num_files];

    Mat output;
    montage = Montage(width + extra_width, height + extra_height);

    for (int i = 0; i < num_files; i++) {
        namedWindow(input_files[i]);
        imshow(input_files[i], photos[i]);
        montage.add_photo(photos[i]);
        // set random position at first
        value_row[i] = rand() % (height + extra_height - photos[i].rows);
        value_col[i] = rand() % (width + extra_width - photos[i].cols);
        // add position control
        createTrackbar("Row", input_files[i], value_row + i, height + extra_height - photos[i].rows - 1, on_trackbar);
        createTrackbar("Col", input_files[i], value_col + i, width + extra_width - photos[i].cols - 1, on_trackbar);
        // add mouse callback
        constraints[i].insert(make_pair(i,i));
        setMouseCallback(input_files[i], on_mouse, &constraints[i]);
    }

    assemble();
    waitKey(0);

    return EXIT_SUCCESS;
}