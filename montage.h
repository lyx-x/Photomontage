//
// Created by czx on 08/01/16.
//

#ifndef MONTAGE_H
#define MONTAGE_H

#include <vector>
#include <set>
#include <iostream>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

class Montage {
    vector<pair<int,int> > offset;
    vector<Mat> photos;
    Mat mask, nap, fixed;

    int max_row = 600; // number of rows in the output
    int max_col = 1024; // number of columns in the output
    int center_size = 8;

private:
    inline bool is_overlapped(int row, int col) const;
    inline bool is_center_photo(int row, int col, int photo_index) const;
    inline bool is_center_photo(pair<int,int> pixel, int photo_index) const;
    inline bool is_border_photo(int row, int col, int photo_index) const;
    inline bool is_border_photo(pair<int, int> pixel, int photo_index) const;
    inline bool is_border_mask(int row, int col) const;
    inline int norm(int index_a, int index_b, int row, int col) const;
    inline int cost(int index_a, int index_b, int row1, int col1, int row2, int col2) const;

public:
    Montage(int width, int height);
    void add_photo(Mat photo); // add a photo to queue
    void assemble(int index, int row, int col, set<pair<int,int>> *constraint = NULL); // add a new image at a specific position
    void reset();
    void show() const; // show result
    void save_mask(string mask_name) const; // save the mask after cropping
    void save_output(Mat &output) const; // export the nap to output without cropping
};


#endif //MONTAGE_H
