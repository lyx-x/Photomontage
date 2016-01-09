//
// Created by czx on 08/01/16.
//

#ifndef MONTAGE_H
#define MONTAGE_H
#include <vector>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

class Montage {
    vector<pair<int,int> > offset;
    vector<Mat> photos;
    Mat mask, nap, fixed;

    int max_row = 600; // number of rows in the output
    int max_col = 1024; // number of columns in the output
    int center_size = 6;

private:
    inline bool is_overlapped(int row, int col);
    inline bool is_center_photo(int row, int col, int photo_index);
    inline bool is_center_photo(pair<int,int> pixel, int photo_index);
    inline bool is_border_photo(int row, int col, int photo_index);
    inline bool is_border_photo(pair<int, int> pixel, int photo_index);
    inline bool is_border_mask(int row, int col);
    inline int norm(int index_a, int index_b, int row, int col);
    inline int cost(int index_a, int index_b, int row1, int col1, int row2, int col2);

public:
    Montage(int width, int height);
    void add_photo(Mat photo);
    void assemble(int index, int row, int col);
    void reset();
    void show();
    void save(string img_name, string mask_name);
};


#endif //MONTAGE_H
