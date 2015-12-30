/*
 * Photomontage program
 *
 * This program assembles two photos manually. User needs to enter the desired position of two photos in order to
 * see the result.
 *
 */

#include <iostream>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

vector<Mat> photos;

int* value_x;
int* value_y;
Mat nap;
Mat mask;

void init() {
    for (int row = 0; row < nap.rows; row++)
        for (int col = 0; col < nap.cols; col++) {
            nap.at<Vec3b>(row, col) = Vec3b(0, 0, 0);
            mask.at<uchar>(row, col) = -1;
        }
}

/*
 * Assemble two photos, the existing image is described with a mask matrix indicating to witch image belongs each pixel,
 * it will be partly rewritten by the new image
 *
 * Params:
 *      (global) photos: list of images
 *      (global) nap: old image
 *      (global) mask: matrix of indices, mask[i,j] = k only if photos[k][i,j] = existing[i,j], the default value is -1
 *      (global) offset_x: offset position in x of the new patch relatively to the left-top of the old image
 *      (global) offset_y: offset position in y
 *      index_new: index of the new patch
 *
 */
void assemble(int index_new) {

}

void assemble() {

    // empty the old image
    init();

    // put the first photo
    for (int row = 0; row < photos[0].rows; row++)
        for (int col = 0; col < photos[0].cols; col++) {
            nap.at<Vec3b>(row + value_x[0], col + value_y[0]) = photos[0].at<Vec3b>(row, col);
            mask.at<uchar>(row + value_x[0], col + value_y[0]) = 0;
        }

    // add the other patch
    for (int i = 1; i < photos.size(); i++)
        assemble(i);

}

void track(int, void*) {
    assemble();
    imshow("Image", nap);
}

int main() {

    int max_height = 600;
    int max_width = 1024;

    // read arguments, to be replaced

    vector<string> files;

    files.push_back("samples/bean.jpg");
    files.push_back("samples/floor.jpg");

    // read images

    for (auto s: files) {
        Mat img = imread(s);
        photos.push_back(img);
    }

    // prepare a large nap for drawing

    nap = Mat(600, 1024, CV_8UC3);
    mask = Mat(600, 1024, CV_8UC1, Scalar_<uchar>::all(uchar(-1)));

    // assemble images one by one

    namedWindow("Image", 1);
    namedWindow("Control", 1);

    value_x = new int[photos.size()];
    value_y = new int[photos.size()];

    // show the empty image

    imshow("Image", nap);

    // initialize all images

    for (int i = 0; i < photos.size(); i++) {
        value_x[i] = 0;
        value_y[i] = 0;
        // add position control
        createTrackbar("Row_" + to_string(i + 1), "Control", value_x + i, max_height - photos[i].rows - 1, track);
        createTrackbar("Col_" + to_string(i + 1), "Control", value_y + i, max_width - photos[i].cols - 1, track);
    }

    waitKey(0);

    delete[] value_x;
    delete[] value_y;

    return EXIT_SUCCESS;
}