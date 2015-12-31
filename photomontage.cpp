/*
 * Photomontage program
 *
 * This program assembles two photos manually. User needs to enter the desired position of two photos in order to
 * see the result.
 *
 */

#include <iostream>
#include <opencv2/highgui/highgui.hpp>

#include "maxflow/graph.h"

using namespace std;
using namespace cv;

const int infinity = 1<<31;

vector<Mat> photos;

int*value_row;
int*value_col;
Mat nap;
Mat mask;

void init() {
    for (int row = 0; row < nap.rows; row++)
        for (int col = 0; col < nap.cols; col++) {
            nap.at<Vec3b>(row, col) = Vec3b(0, 0, 0);
            mask.at<Vec3s>(row, col) = Vec3s(0, 0, 0);
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

// return the norm of nap[x1,y1] - photos[i][x2,y2]
int cost(int i, int x1, int y1, int x2, int y2) {
    return 0;
}

void assemble(int index_new) {
    
    // the overlapped part of nap and photos[index_new]
    
    int height = 0;
    int width = 0;
    int x1 = 0, y1 = 0;
    int x2 = 0, y2 = 0;
    
    Mat patch = photos[index_new];
    int offset_row = value_row[index_new];
    int offset_col = value_col[index_new];
    
    vector<pair<int,int>> overlap;
    for (int row = 0; row < nap.rows; row++)
        for (int col = 0; col < nap.cols; col++)
            if (mask.at<Vec3s>(row + offset_row, col + offset_col)[0] >= 0)
                overlap.push_back(make_pair(row, col));


    // Graph cut

    int overlap_area = int(overlap.size());
    Graph<int,int,int> g(overlap_area + 2 , 1);

    g.add_node(overlap_area);
    
    /* 
     * 0 4 8
     * 1 5 9
     * 2 6 10
     * 3 7 11
     */

    // add arcs from border to source and sink
    
    for(int i = 0; i < height; i++){
        g.add_tweights(i, infinity, infinity);
        g.add_tweights(height * width - i , infinity, infinity);
    }

    // calculate the cost M of all pixels
    
    Mat costs(height, width, CV_8UC1);
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            costs.at<uchar>(i,j) = (int)cost(index_new, x1 + i, y1 + j, x2 + i, y2 + j);
        }
    }

    //add edge between adjacent nodes
    for(int i = 0; i < height - 1; i++){
        for(int j = 0; j < width - 1; j++){
            int M1 = costs.at<uchar>(i,j) + costs.at<uchar>(i+1,j);
            int M2 = costs.at<uchar>(i,j) + costs.at<uchar>(i,j + 1);
            g.add_edge(i * width + j, i * width + j + 1, M1, M1);
            g.add_edge(i * width + j, (i + 1) * width + j, M2, M2);
        }
    }
    int flow = g.maxflow();
    
}

void assemble() {

    // empty the old image
    init();

    // put the first photo
    for (int row = 0; row < photos[0].rows; row++)
        for (int col = 0; col < photos[0].cols; col++) {
            nap.at<Vec3b>(row + value_row[0], col + value_col[0]) = photos[0].at<Vec3b>(row, col);
            mask.at<uchar>(row + value_row[0], col + value_col[0]) = 0;
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
    mask = Mat(600, 1024, CV_8SC3, Scalar_<Vec3s>::all(Vec3s(0, 0, 0)));

    // assemble images one by one

    namedWindow("Image", 1);
    namedWindow("Control", 1);

    value_row = new int[photos.size()];
    value_col = new int[photos.size()];

    // show the empty image

    imshow("Image", nap);

    // initialize all images

    for (int i = 0; i < photos.size(); i++) {
        value_row[i] = 0;
        value_col[i] = 0;
        // add position control
        createTrackbar("Row_" + to_string(i + 1), "Control", value_row + i, max_height - photos[i].rows - 1, track);
        createTrackbar("Col_" + to_string(i + 1), "Control", value_col + i, max_width - photos[i].cols - 1, track);
    }

    waitKey(0);

    delete[] value_row;
    delete[] value_col;

    return EXIT_SUCCESS;
}