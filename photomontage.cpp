/*
 * Photomontage program
 *
 * This program assembles two photos manually. User needs to enter the desired position of two photos in order to
 * see the result.
 *
 */

#include <iostream>
#include <map>
#include <opencv2/imgproc/imgproc.hpp>

#include "maxflow/graph.h"
#include "montage.h"

//#include "graph.h"

using namespace std;
using namespace cv;

const int infinity = 1<<30;

vector<Mat> photos;

int* value_row;
int* value_col;
Mat nap;
Mat mask;

int max_row = 600; // number of rows in the output
int max_col = 1024; // number of columns in the output
int center_size = 6;

Montage montage(600,1024);
// show the mask matrix
void drawMask(){
    Mat tmp(mask.rows, mask.cols, CV_8U);
    for(int row = 0; row < mask.rows; row++)
        for(int col = 0; col < mask.cols; col++)
            tmp.at<uchar>(row, col) = uchar((mask.at<Vec3s>(row, col)[0] + 1) * 255 / photos.size());
    imshow("Mask", tmp);
}

// reset global variables
void init() {
    for (int row = 0; row < nap.rows; row++)
        for (int col = 0; col < nap.cols; col++) {
            nap.at<Vec3b>(row, col) = Vec3b(0, 0, 0);
            mask.at<Vec3s>(row, col) = Vec3s(-1, 0, 0);
        }
}

inline bool is_overlapped(int row, int col) {
    if (row < 0 || row >= max_row)
        return false;
    if (col < 0 || col >= max_col)
        return false;
    return mask.at<Vec3s>(row, col)[0] >= 0;
}

inline bool is_center_photo(int row, int col, int photo_index){
    int r = photos[photo_index].rows;
    int c = photos[photo_index].cols;
    return (abs(row - r / 2) < r / center_size && abs(col - c / 2) < c / center_size);
}

inline bool is_center_photo(pair<int,int> pixel, int photo_index){
    return is_center_photo(pixel.first, pixel.second, photo_index);
}

inline bool is_border_photo(int row, int col, int photo_index){
   if (row == 0 || row == photos[photo_index].rows - 1)
       return true;
   return (col == 0 || col == photos[photo_index].cols - 1);
}

inline bool is_border_photo(pair<int, int> pixel, int photo_index){
    return is_border_photo(pixel.first, pixel.second, photo_index);
}

inline bool is_border_mask(int row, int col) {
    if (row == 0 || row == mask.rows - 1)
        return true;
    if (col == 0 || col == mask.cols - 1)
        return true;
    if (mask.at<Vec3s>(row - 1, col)[0] == -1)
        return true;
    if (mask.at<Vec3s>(row + 1, col)[0] == -1)
        return true;
    if (mask.at<Vec3s>(row, col - 1)[0] == -1)
        return true;
    return (mask.at<Vec3s>(row, col + 1)[0] == -1);
}

// Return the norm of photos[a][row,col] - photos[b][row,col]
inline int norm(int index_a, int index_b, int row, int col) {
    int offset_row_a = value_row[index_a];
    int offset_col_a = value_col[index_a];
    int offset_row_b = value_row[index_b];
    int offset_col_b = value_col[index_b];
    int a = int(photos[index_a].at<Vec3b>(row - offset_row_a,col - offset_col_a)[0]) - int(photos[index_b].at<Vec3b>(row - offset_row_b, col - offset_col_b)[0]);
    int b = int(photos[index_a].at<Vec3b>(row - offset_row_a,col - offset_col_a)[1]) - int(photos[index_b].at<Vec3b>(row - offset_row_b, col - offset_col_b)[1]);
    int c = int(photos[index_a].at<Vec3b>(row - offset_row_a,col - offset_col_a)[2]) - int(photos[index_b].at<Vec3b>(row - offset_row_b, col - offset_col_b)[2]);
    return int(sqrt(a * a + b * b + c * c));
}

// return the matching cost between nap[row1,col1] and nap[row2,col2]
inline int cost(int index_a, int index_b, int row1, int col1, int row2, int col2) {
    return norm(index_a, index_b, row1, col1) + norm(index_a, index_b, row2, col2);
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

    // Graph cut

    int num_node = int(overlap.size()); // the number of seam does not exceed the double of number of pixels

    Graph<int,int,int> graph(num_node * 3, num_node * 4 * 3); // including the source and the sink
    if (num_node != 0)
        graph.add_node(num_node);

    int inside_b = 0;
    int inside_a = 0;

    int seam_index = num_node;

    for (int i = 0; i < overlap.size(); i++) {

        // Consider the pixel under it and on its right

        int row = overlap[i].first;
        int col = overlap[i].second;

        int row_mask = row + offset_row;
        int col_mask = col + offset_col;

        // Add adjacent edges and seams

        if (is_overlapped(row_mask + 1, col_mask)) {
            if (mask.at<Vec3s>(row_mask + 1, col_mask)[0] != mask.at<Vec3s>(row_mask, col_mask)[0]){
                graph.add_node(1);
                graph.add_tweights(seam_index,0,
                                   cost(mask.at<Vec3s>(row_mask+1, col_mask)[0],mask.at<Vec3s>(row_mask, col_mask)[0],row_mask,col_mask,row_mask + 1,col_mask));
                graph.add_edge(i,seam_index,cost(mask.at<Vec3s>(row_mask, col_mask)[0],index_new, row_mask,col_mask,row_mask + 1,col_mask),
                               cost(mask.at<Vec3s>(row_mask, col_mask)[0],index_new, row_mask,col_mask,row_mask + 1,col_mask));
                graph.add_edge(seam_index,map_overlap[make_pair(row + 1, col)], cost(mask.at<Vec3s>(row_mask + 1, col_mask)[0],index_new, row_mask,col_mask,row_mask + 1,col_mask),
                               cost(mask.at<Vec3s>(row_mask + 1, col_mask)[0],index_new, row_mask,col_mask,row_mask + 1,col_mask));
                seam_index++;
            }else {
                graph.add_edge(i, map_overlap[make_pair(row + 1, col)], cost(index_new, mask.at<Vec3s>(row_mask + 1, col_mask)[0], row_mask,col_mask,row_mask + 1,col_mask),
                               cost(index_new, mask.at<Vec3s>(row_mask + 1, col_mask)[0], row_mask,col_mask,row_mask + 1,col_mask));
            }
        }

        if (is_overlapped(row_mask, col_mask + 1)) {
            if (mask.at<Vec3s>(row_mask, col_mask + 1)[0] != mask.at<Vec3s>(row_mask, col_mask)[0]){
                graph.add_node(1);
                graph.add_tweights(seam_index,0,
                                   cost(mask.at<Vec3s>(row_mask, col_mask + 1)[0],mask.at<Vec3s>(row_mask, col_mask)[0],row_mask,col_mask,row_mask,col_mask + 1));
                graph.add_edge(i,seam_index,cost(mask.at<Vec3s>(row_mask, col_mask)[0],index_new, row_mask,col_mask,row_mask,col_mask + 1),
                               cost(mask.at<Vec3s>(row_mask, col_mask)[0],index_new, row_mask,col_mask,row_mask,col_mask + 1));
                graph.add_edge(seam_index,map_overlap[make_pair(row, col + 1)], cost(mask.at<Vec3s>(row_mask, col_mask + 1)[0],index_new, row_mask,col_mask,row_mask,col_mask + 1),
                               cost(mask.at<Vec3s>(row_mask, col_mask + 1)[0],index_new, row_mask,col_mask,row_mask,col_mask + 1));
                seam_index++;
            }else {
                graph.add_edge(i, map_overlap[make_pair(row, col + 1)], cost(index_new, mask.at<Vec3s>(row_mask, col_mask + 1)[0], row_mask,col_mask,row_mask,col_mask + 1),
                               cost(index_new, mask.at<Vec3s>(row_mask, col_mask + 1)[0], row_mask,col_mask,row_mask,col_mask + 1));
            }
        }

        // Add constraints for source and sink

        if (is_center_photo(overlap[i], index_new)) { // the center of patch must remain
            inside_b++;
            graph.add_tweights(i, 0, infinity);
        } else {
            if (is_border_mask(overlap[i].first + offset_row, overlap[i].second + offset_col)) {
                inside_b++;
                graph.add_tweights(i, 0, infinity);
            } else if (is_border_photo(overlap[i],index_new)){
                inside_a++;
                graph.add_tweights(i, infinity, 0);
            } else
                graph.add_tweights(i, 0, 0);
        }
    }
    cout << "B: " << inside_b << " A: " << inside_a << endl;

    // Compute the min-cut

    int flow = graph.maxflow();

    // Get new color for all overlapped pixels

    for (int row = 0; row < patch.rows; row++)
        for (int col = 0; col < patch.cols; col++)
            if (mask.at<Vec3s>(row + offset_row, col + offset_col)[0] == -1) {
                mask.at<Vec3s>(row + offset_row, col + offset_col) = Vec3s(short(index_new), short(row), short(col));
                nap.at<Vec3b>(row + offset_row, col + offset_col) = patch.at<Vec3b>(row,col);
            }

    for(int i = 0; i < overlap.size(); i++){
        if (graph.what_segment(i) == Graph<int,int,int>::SINK) {
            mask.at<Vec3s>(overlap[i].first + offset_row, overlap[i].second + offset_col) = Vec3s(short(index_new), short(overlap[i].first), short(overlap[i].second));
            nap.at<Vec3b>(overlap[i].first + offset_row, overlap[i].second + offset_col) = Vec3b(patch.at<Vec3b>(overlap[i].first,overlap[i].second));
        }
    }

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

    imshow("Image", nap);
    drawMask();
}

void track(int, void*) {
    assemble();
}

string file = "samples/floor.jpg";
int number = 3;

void track_montage(int, void*){
    montage.reset();
    for(int i = 0; i < number; i++){
        montage.assemble(i,value_row[i],value_col[i]);
    }
    montage.show();
    montage.save("image" + to_string(value_row[0])+".jpg","mask" + to_string(value_row[0])+".jpg");
}

int main() {

    Mat img = imread(file);
    if (img.rows > 500)
        resize(img,img,Size(img.cols/5, img.rows/5));
    imshow("img",img);
    waitKey(0);

    for (int i = 0; i < number; i++) {
        montage.add_photo(img);
    }

    value_row = new int[number];
    value_col = new int[number];

    namedWindow("Control");

    for (int i = 0; i < number; i++) {
        value_row[i] = rand() % (max_row - img.rows);
        value_col[i] = rand() % (max_col - img.cols);
        // add position control
        createTrackbar("Row_" + to_string(i + 1), "Control", value_row + i, max_row - img.rows - 1, track_montage);
        createTrackbar("Col_" + to_string(i + 1), "Control", value_col + i, max_col - img.cols - 1, track_montage);
    }

    waitKey(0);

    //files.push_back("samples/bean.jpg");
    //files.push_back("samples/bean.jpg");
    //files.push_back("samples/bean.jpg");

    // read images

    /*for (auto s: files) {
        Mat img = imread(s.c_str());
        photos.push_back(img);
    }

    namedWindow("Image");
    namedWindow("Control");
    namedWindow("Mask");

    // prepare a large nap for drawing

    nap = Mat(600, 1024, CV_8UC3);
    mask = Mat(600, 1024, CV_16SC3);

    // assemble images one by one

    init();

    value_row = new int[photos.size()];
    value_col = new int[photos.size()];

    // show the empty image

    imshow("Image", nap);

    // initialize all images

    for (int i = 0; i < photos.size(); i++) {
        value_row[i] = rand() % (nap.rows - photos[i].rows);
        value_col[i] = rand() % (nap.cols - photos[i].cols);
        // add position control
        createTrackbar("Row_" + to_string(i + 1), "Control", value_row + i, max_row - photos[i].rows - 1, track);
        createTrackbar("Col_" + to_string(i + 1), "Control", value_col + i, max_col - photos[i].cols - 1, track);
    }

    assemble();

    waitKey(0);

    delete[] value_row;
    delete[] value_col;
    */

    delete[] value_row;
    delete[] value_col;
    return EXIT_SUCCESS;
}