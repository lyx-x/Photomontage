/*
 * Photomontage program
 *
 * This program assembles two photos manually. User needs to enter the desired position of two photos in order to
 * see the result.
 *
 */

#include <iostream>
#include <map>
#include <opencv2/highgui/highgui.hpp>

#include "maxflow/graph.h"

//#include "graph.h"

using namespace std;
using namespace cv;

struct seam_t {

};

const int infinity = 1<<30;

vector<Mat> photos;

int*value_row;
int*value_col;
Mat nap;
Mat mask;

int max_row = 600; // number of rows in the output
int max_col = 1024; // number of columns in the output
int interior_gap = 32;

void init() {
    for (int row = 0; row < nap.rows; row++)
        for (int col = 0; col < nap.cols; col++) {
            nap.at<Vec3b>(row, col) = Vec3b(0, 0, 0);
            mask.at<Vec3s>(row, col) = Vec3s(-1, 0, 0);
        }
}

inline int to_index(int row, int col) {
    return row * max_col + col;
}

inline int to_index(pair<int,int> pixel) {
    return to_index(pixel.first, pixel.second);
}

inline pair<int,int> to_pixel(int index) {
    return make_pair(index / max_col, index % max_col);
};

inline bool is_overlapped(int row, int col) {
    if (row < 0 || row >= max_row)
        return false;
    if (col < 0 || col >= max_col)
        return false;
    return mask.at<Vec3s>(row, col)[0] >= 0;
}

inline bool is_overlapped(pair<int,int> pixel) {
    return is_overlapped(pixel.first, pixel.second);
}

inline bool is_interior_photo(int row, int col, int photo_index){
    return (row >= interior_gap && row + interior_gap < photos[photo_index].rows
            && col >= interior_gap && col + interior_gap < photos[photo_index].cols);
}

inline bool is_interior_photo(pair<int,int> pixel, int photo_index){
    return is_interior_photo(pixel.first, pixel.second, photo_index);
}

inline bool is_interior_mask(int row, int col){
    if (row - interior_gap < 0 || row + interior_gap >= max_row)
        return false;
    if (col - interior_gap < 0 || col + interior_gap >= max_col)
        return false;
    return (mask.at<Vec3s>(row, col)[0] >= 0 && mask.at<Vec3s>(row - interior_gap, col - interior_gap)[0] >= 0 && mask.at<Vec3s>(row + interior_gap, col + interior_gap)[0] >= 0);
}

inline bool is_interior_mask(pair<int,int> pixel){
    return is_interior_mask(pixel.first, pixel.second);
}

inline bool at_photo_border(int row, int col, int photo_index){
   if (row == 0 || row == photos[photo_index].rows - 1)
       return true;
   return (col == 0 || col == photos[photo_index].cols - 1);
}

inline bool at_photo_border(pair<int,int>pixel, int photo_index){
    return at_photo_border(pixel.first, pixel.second, photo_index);
}

// Return the norm of nap[row + offset_row,col + offset_col] - photos[index_new][row,col]
inline int norm(int index_new, int row, int col) {
    int offset_row = value_row[index_new];
    int offset_col = value_col[index_new];
    int a = int(photos[index_new].at<Vec3b>(row,col)[0]) - int(nap.at<Vec3b>(row + offset_row, col + offset_col)[0]);
    int b = int(photos[index_new].at<Vec3b>(row,col)[1]) - int(nap.at<Vec3b>(row + offset_row, col + offset_col)[1]);
    int c = int(photos[index_new].at<Vec3b>(row,col)[2]) - int(nap.at<Vec3b>(row + offset_row, col + offset_col)[2]);
    return (int)(sqrt(a * a + b * b + c * c));
}

// return the matching cost between nap[row1,col1] and nap[row2,col2]
inline int cost(int index_new, int row1, int col1, int row2, int col2) {
    return norm(index_new, row1, col1) + norm(index_new, row2, col2);
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
    
    // the overlapped part of nap and photos[index_new]
    
    Mat patch = photos[index_new];
    int offset_row = value_row[index_new];
    int offset_col = value_col[index_new];
    
    vector<pair<int,int>> overlap;
    map<pair<int,int>,int> map_overlap;
    for (int row = 0; row < patch.rows; row++)
        for (int col = 0; col < patch.cols; col++)
            if (is_overlapped(row + offset_row, col + offset_col)) {
                map_overlap[make_pair(row, col)] = int(overlap.size()); // store the index
                overlap.push_back(make_pair(row, col));
            }

    vector<seam_t> old_seams;

    // TODO add old seams in the graph

    // Graph cut

    int num_node =int(overlap.size() + old_seams.size());
    cout << num_node << endl ;
    if (num_node == 0)
        return;
    Graph<int,int,int> graph(num_node,num_node * 4); // including the source and the sink
    graph.add_node(num_node);

    int interior = 0;
    int exterior = 0;

    for (int i = 0; i < overlap.size(); i++) {
        // Consider the pixel under it and on its right
        int row = overlap[i].first;
        int col = overlap[i].second;
        // Add adjacent edges
        if (is_overlapped(row + 1 + offset_row, col + offset_col)) {
            graph.add_edge(i, map_overlap[make_pair(row + 1, col)], cost(index_new, row, col, row + 1, col),
                           cost(index_new, row, col, row + 1, col));
        }
        if (is_overlapped(row + offset_row, col + 1 + offset_col)) {
            graph.add_edge(i, map_overlap[make_pair(row, col + 1)], cost(index_new, row, col, row, col + 1),
                           cost(index_new, row, col, row, col + 1));
        }
        // Add constraints for source and sink
        if (is_interior_photo(overlap[i],index_new)){
            graph.add_tweights(i,0,infinity);
            interior++;
        }else if(is_interior_mask(row + offset_row, col + offset_col) && at_photo_border(overlap[i],index_new)){
            exterior++;
            graph.add_tweights(i,infinity,0);
        }else{
            graph.add_tweights(i,0,0);
        }
    }
    cout << "interior: " << interior << " exterior: " <<exterior << endl;
    // Add seam edges

    // TODO

    // Compute the min-cut

    int flow = graph.maxflow();

    // Get new color for all overlapped pixels

    for (int row = 0; row < patch.rows; row++)
        for (int col = 0; col < patch.cols; col++)
            if (mask.at<Vec3s>(row + offset_row, col + offset_col)[0] == -1) {
                mask.at<Vec3s>(row + offset_row, col + offset_col) = Vec3s(index_new, row, col);
                nap.at<Vec3b>(row + offset_row, col + offset_col) = patch.at<Vec3b>(row,col);
            }

    int belong = 0;
    for(int i = 0; i < overlap.size(); i++){
        if (graph.what_segment(i) == Graph<int,int,int>::SINK) {
            belong++;
            mask.at<Vec3s>(overlap[i].first + offset_row, overlap[i].second + offset_col) = Vec3s(index_new,
                                                                                                  overlap[i].first,
                                                                                                  overlap[i].second);
            nap.at<Vec3b>(overlap[i].first + offset_row, overlap[i].second + offset_col) = Vec3b(patch.at<Vec3b>(overlap[i].first,overlap[i].second));
        }
    }
    cout << belong << ";" << endl;

    /*

    int height = 0;
    int width = 0;
    int x1 = 0, y1 = 0;
    int x2 = 0, y2 = 0;

    int overlap_area = int(overlap.size());
    Graph<int,int,int> g(overlap_area + 2 , 1);

    g.add_node(overlap_area);
    
    /* 
     * 0 4 8
     * 1 5 9
     * 2 6 10
     * 3 7 11
     *

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
    */
}

void assemble() {

    // empty the old image
    init();

    // put the first photo
    for (int row = 0; row < photos[0].rows; row++)
        for (int col = 0; col < photos[0].cols; col++) {
            nap.at<Vec3b>(row + value_row[0], col + value_col[0]) = Vec3b(photos[0].at<Vec3b>(row, col));
            mask.at<Vec3s>(row + value_row[0], col + value_col[0]) = Vec3s(0,row,col);
        }

    // add the other patch
    for (int i = 1; i < photos.size(); i++)
        assemble(i);

}

void drawMask(){
    Mat m(mask.rows, mask.cols, CV_8U);
    for(int row = 0; row < mask.rows; row++){
        for(int col = 0; col < mask.cols; col++){
            m.at<uchar>(row,col) = int((mask.at<Vec3s>(row,col)[0] + 1) * 255 / photos.size());
        }
    }
    imshow("Mask",m);
}

void track(int, void*) {
    assemble();
    imshow("Image", nap);
    drawMask();
}

int main() {


    // read arguments, to be replaced
    cout << infinity << endl;
    vector<string> files;

    files.push_back("samples/bean.jpg");
    files.push_back("samples/bean.jpg");

    // read images

    for (auto s: files) {
        Mat img = imread(s.c_str());
        photos.push_back(img);
    }

    // prepare a large nap for drawing

    nap = Mat(600, 1024, CV_8UC3);
    mask = Mat(600, 1024, CV_8SC3);

    // assemble images one by one

    namedWindow("Image");
    namedWindow("Control");
    namedWindow("Mask");

    value_row = new int[photos.size()];
    value_col = new int[photos.size()];

    // show the empty image

    imshow("Image", nap);

    // initialize all images

    for (int i = 0; i < photos.size(); i++) {
        value_row[i] = i * 256;
        value_col[i] = i * 256;
        // add position control
        createTrackbar("Row_" + to_string(i + 1), "Control", value_row + i, max_row - photos[i].rows - 1, track);
        createTrackbar("Col_" + to_string(i + 1), "Control", value_col + i, max_col - photos[i].cols - 1, track);
    }

    waitKey(0);

    delete[] value_row;
    delete[] value_col;

    return EXIT_SUCCESS;
}