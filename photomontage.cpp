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
 * Ex:
 *      montage -i 2 photos/left.jpg photos/right.jpg -o results/montage.jpg -h 384 -w 512
 *
 * Usage of control window:
 *      Move the trackbar to control the position
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

Montage montage(600,1024); // the paint zone

vector<Mat> photos; // list of photos
vector<set<pair<int,int>>> constraints; // list of constraints for each image
vector<int> photo_index; // list of consecutive numbers for mouse control
vector<string> input_files; // name of photos
int *value_row, *value_col; // ralative position of each image

const int range = 5; // use small circle instead of a single pixel for control

/*
 * Method that combines all photos
 */
void assemble() {
    montage.reset();
    for(int i = 0; i < photos.size(); i++)
        montage.assemble(i, value_row[i], value_col[i], &constraints[i]);
    montage.show();
    montage.save_mask("results/mask_montage.jpg");
}

/*
 * Stardard callback for mouse event, add or remove constraints
 */
void on_mouse(int event, int x, int y, int, void* p) {
    int* index = (int*)p; // get index of the photo
    set<pair<int,int>>* constraint = &constraints[*index];
    switch (event) {
        case EVENT_LBUTTONDOWN:
            // add constraints
            for (int i = 0 - range; i <= range; i++)
                for (int j = 0 - range; j <= range; j++) {
                    int _x = x + i;
                    int _y = y + j;
                    if (i * i + j * j <= range * range) // a small circle
                        if (_x >= 0 && _x < photos[*index].cols && _y >= 0 && _y <= photos[*index].rows)
                            constraint->insert(make_pair(_y, _x));
                }
            break;
        case EVENT_RBUTTONDOWN:
            // remove constraints
            constraint->clear();
            break;
        default:
            return;
    }

    // redraw the image with constraints
    Mat tmp = photos[*index].clone();
    for (auto px : *constraint)
        tmp.at<Vec3b>(px.first, px.second) = Vec3b(0, 255, 0); // mark the constraints in green
    imshow(input_files[*index] + to_string(*index), tmp);

    assemble();

}

/*
 * Callback for moving the picture, the value of trackbar has already been changed, no need to do anythhing
 */
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

    int extra_height = height / 6; // extra space to manipulate the nap
    int extra_width = width / 6;

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

    // use global variables for relative position

    value_row = new int[num_files];
    value_col = new int[num_files];

    Mat output(height, width, CV_8UC3);
    montage = Montage(height, width, extra_height, extra_width);

    // add control panels

    for (int i = 0; i < num_files; i++) {
        namedWindow(input_files[i] + to_string(i), CV_GUI_NORMAL);
        imshow(input_files[i] + to_string(i), photos[i]);
        montage.add_photo(photos[i]);
        // set random position at first
        value_row[i] = rand() % (height + extra_height * 2 - photos[i].rows);
        value_col[i] = rand() % (width + extra_width * 2 - photos[i].cols);
        // add position control
        createTrackbar("Row", input_files[i] + to_string(i), value_row + i, height + extra_height * 2 - photos[i].rows - 1, on_trackbar);
        createTrackbar("Col", input_files[i] + to_string(i), value_col + i, width + extra_width * 2 - photos[i].cols - 1, on_trackbar);
        // add mouse callback
        setMouseCallback(input_files[i] + to_string(i), on_mouse, &photo_index[i]);
    }

    assemble();
    waitKey(0);

    // retrieve the result

    montage.save_output(output);

    imwrite(output_file, output);

    imshow(output_file, output);
    waitKey(0);

    return EXIT_SUCCESS;
}