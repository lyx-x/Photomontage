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

vector<string> files;
vector<Mat> photos;

/*
 * Assemble two photos, the existing image is described with a mask matrix indicating to witch image belongs each pixel,
 * it will be partly rewritten by the new image
 *
 * Params:
 *      photos: list of images
 *      existing: old image
 *      index_new: index of the new patch
 *      offset_x: offset position in x of the new patch relatively to the left-top of the old image
 *      offset_y: offset position in y
 *      mask: matrix of indices, mask[i,j] = k only if photos[k][i,j] = existing[i,j], the default value is -1
 *
 */
void assemble(vector<Mat>& photos, Mat& existing, int index_new, int offset_x, int offset_y, Mat& mask) {

}

template <Mat& M>
void track_x(int x) {
    cout << M.rows << endl;
    cout << x << endl;
}

int main() {

    int max_height = 600;
    int max_width = 1024;

    // read arguments, to be replaced

    files.push_back("samples/bean.jpg");
    files.push_back("samples/floor.jpg");

    // read images

    for (auto s: files) {
        Mat img = imread(s);
        photos.push_back(img);
    }

    // prepare a large nap for drawing

    Mat nap(600, 1024, CV_8UC3, Scalar_<Vec3b>::all(0));
    Mat mask(600, 1024, CV_8UC1, Scalar_<uchar>::all(-1));

    // assemble images one by one

    namedWindow("Image", 1);

    int* value_x = new int[photos.size()];
    int* value_y = new int[photos.size()];

    for (int i = 0; i < photos.size(); i++) {
        value_x[i] = 0;
        value_y[i] = 0;
        assemble(photos, nap, i, 0, 0, mask);
        createTrackbar("Row_" + to_string(i + 1), "Image", value_x, 600 - photos[i].rows - 1, )
    }

    imshow("Image", nap);
    waitKey(0);

    delete[] value_x;
    delete[] value_y;

    return EXIT_SUCCESS;
}