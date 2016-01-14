//
// Created by czx on 08/01/16.
//

#include "montage.h"
#include <map>
#include "maxflow/graph.h"


const int infinity = 1<<30;

Montage::Montage(int row, int col, int ex_row, int ex_col): extra_row(ex_row), extra_col(ex_col) {
    max_row = row + 2 * ex_row;
    max_col = col + 2 * ex_col;
    nap = Mat(max_row, max_col, CV_8UC3);
    mask = Mat(max_row, max_col, CV_16SC3);
    fixed = Mat(max_row, max_col, CV_8SC1);
}

inline bool Montage::is_overlapped(int row, int col) const {
    if (row < 0 || row >= max_row)
        return false;
    if (col < 0 || col >= max_col)
        return false;
    return mask.at<Vec3s>(row, col)[0] >= 0;
}

inline bool Montage::is_center_photo(int row, int col, int photo_index) const {
    int r = photos[photo_index].rows;
    int c = photos[photo_index].cols;
    return (abs(row - r / 2) < r / center_size && abs(col - c / 2) < c / center_size);
}

inline bool Montage::is_center_photo(pair<int,int> pixel, int photo_index) const {
    return is_center_photo(pixel.first, pixel.second, photo_index);
}

inline bool Montage::is_border_photo(int row, int col, int photo_index) const {
    if (row == 0 || row == photos[photo_index].rows - 1)
        return true;
    return (col == 0 || col == photos[photo_index].cols - 1);
}

inline bool Montage::is_border_photo(pair<int, int> pixel, int photo_index) const {
    return is_border_photo(pixel.first, pixel.second, photo_index);
}

inline bool Montage::is_border_mask(int row, int col) const {
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
inline int Montage::norm(int index_a, int index_b, int row, int col) const {
    int offset_row_a = offset[index_a].first;
    int offset_col_a = offset[index_a].second;
    int offset_row_b = offset[index_b].first;
    int offset_col_b = offset[index_b].second;
    int a = int(photos[index_a].at<Vec3b>(row - offset_row_a,col - offset_col_a)[0]) - int(photos[index_b].at<Vec3b>(row - offset_row_b, col - offset_col_b)[0]);
    int b = int(photos[index_a].at<Vec3b>(row - offset_row_a,col - offset_col_a)[1]) - int(photos[index_b].at<Vec3b>(row - offset_row_b, col - offset_col_b)[1]);
    int c = int(photos[index_a].at<Vec3b>(row - offset_row_a,col - offset_col_a)[2]) - int(photos[index_b].at<Vec3b>(row - offset_row_b, col - offset_col_b)[2]);
    return int(sqrt(a * a + b * b + c * c));
}

// return the matching cost between nap[row1,col1] and nap[row2,col2]
inline int Montage::cost(int index_a, int index_b, int row1, int col1, int row2, int col2) const {
    return norm(index_a, index_b, row1, col1) + norm(index_a, index_b, row2, col2);
}

void Montage::add_photo(Mat photo) {
    photos.push_back(photo);
}

/*
 * Assemble two photos, the existing image is described with a mask matrix indicating to witch image belongs each pixel,
 * it will be partly rewritten by the new image
 *
 * Params:
 *      (global) photos: list of images
 *      (global) nap: old image
 *      (global) mask: matrix of indices, mask[i,j] = k only if photos[k][i,j] = existing[i,j], the default value is -1
 *      offset_row: offset position in x
 *      offset_col: offset position in y
 *      index: index of the new patch
 *
 */
void Montage::assemble(int index, int offset_row, int offset_col, set<pair<int,int>> *constraint) {
    // the overlapped part of nap and photos[index_new]
    if (photos[index].rows + offset_row >= max_row || photos[index].cols + offset_col >= max_col){
        Rect myROI(0, 0, min(max_col - offset_col, photos[index].cols), min(max_row - offset_row, photos[index].rows));
        photos[index] = photos[index](myROI);
    }

    if (constraint != NULL)
        for (auto p : *constraint)
            fixed.at<schar>(p.first + offset_row, p.second + offset_col) = (schar)index;

    Mat patch = photos[index];
    while(offset.size() <= index)
        offset.push_back(make_pair(offset_row,offset_col));
    offset[index] = make_pair(offset_row,offset_col);

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

    int seam_index = num_node;

    for (int i = 0; i < overlap.size(); i++) {

        // Consider the pixel under it and on its right

        int row = overlap[i].first;
        int col = overlap[i].second;

        int row_mask = row + offset_row;
        int col_mask = col + offset_col;

        // Add adjacent edges and seams

        if (row + 1 < photos[index].rows && is_overlapped(row_mask + 1, col_mask)) {
            if (mask.at<Vec3s>(row_mask + 1, col_mask)[0] != mask.at<Vec3s>(row_mask, col_mask)[0]){
                graph.add_node(1);
                graph.add_tweights(seam_index, 0,
                                   cost(mask.at<Vec3s>(row_mask + 1, col_mask)[0],
                                        mask.at<Vec3s>(row_mask, col_mask)[0], row_mask, col_mask, row_mask + 1, col_mask));
                graph.add_edge(i, seam_index, cost(mask.at<Vec3s>(row_mask, col_mask)[0],
                                                 index, row_mask, col_mask, row_mask + 1, col_mask),
                               cost(mask.at<Vec3s>(row_mask, col_mask)[0], index, row_mask, col_mask, row_mask + 1, col_mask));
                graph.add_edge(seam_index, map_overlap[make_pair(row + 1, col)], cost(mask.at<Vec3s>(row_mask + 1, col_mask)[0], index, row_mask, col_mask, row_mask + 1, col_mask),
                               cost(mask.at<Vec3s>(row_mask + 1, col_mask)[0], index, row_mask, col_mask, row_mask + 1,col_mask));
                seam_index++;
            }else {
                graph.add_edge(i, map_overlap[make_pair(row + 1, col)], cost(index, mask.at<Vec3s>(row_mask + 1, col_mask)[0], row_mask, col_mask, row_mask + 1, col_mask),
                               cost(index, mask.at<Vec3s>(row_mask + 1, col_mask)[0], row_mask, col_mask, row_mask + 1, col_mask));
            }
        }

        if (col + 1 < photos[index].cols && is_overlapped(row_mask, col_mask + 1)) {
            if (mask.at<Vec3s>(row_mask, col_mask + 1)[0] != mask.at<Vec3s>(row_mask, col_mask)[0]){
                graph.add_node(1);
                graph.add_tweights(seam_index, 0,
                                   cost(mask.at<Vec3s>(row_mask, col_mask + 1)[0],
                                        mask.at<Vec3s>(row_mask, col_mask)[0], row_mask, col_mask, row_mask, col_mask + 1));
                graph.add_edge(i, seam_index,cost(mask.at<Vec3s>(row_mask, col_mask)[0],
                                                  index, row_mask,col_mask, row_mask,col_mask + 1),
                               cost(mask.at<Vec3s>(row_mask, col_mask)[0], index, row_mask, col_mask, row_mask, col_mask + 1));
                graph.add_edge(seam_index,map_overlap[make_pair(row, col + 1)], cost(mask.at<Vec3s>(row_mask, col_mask + 1)[0],
                                                                                     index, row_mask, col_mask, row_mask, col_mask + 1),
                               cost(mask.at<Vec3s>(row_mask, col_mask + 1)[0], index, row_mask, col_mask, row_mask, col_mask + 1));
                seam_index++;
            }else {
                graph.add_edge(i, map_overlap[make_pair(row, col + 1)], cost(index, mask.at<Vec3s>(row_mask, col_mask + 1)[0], row_mask, col_mask, row_mask, col_mask + 1),
                               cost(index, mask.at<Vec3s>(row_mask, col_mask + 1)[0], row_mask, col_mask, row_mask, col_mask + 1));
            }
        }

        // Add constraints for source and sink
        if (int(fixed.at<schar>(row_mask, col_mask)) == index)
           graph.add_tweights(i,0,infinity);
        else if (int(fixed.at<schar>(row_mask, col_mask)) != -1)
            graph.add_tweights(i,infinity,0);
        else if (is_center_photo(overlap[i], index) && constraint == NULL) { // the center of patch must remain
            graph.add_tweights(i, 0, infinity);
        } else {
            if (is_border_mask(overlap[i].first + offset_row, overlap[i].second + offset_col)) {
                graph.add_tweights(i, 0, infinity);
            } else if (is_border_photo(overlap[i],index)){
                graph.add_tweights(i, infinity, 0);
            } else
                graph.add_tweights(i, 0, 0);
        }
    }
    // Compute the min-cut

    graph.maxflow();

    // Get new color for all overlapped pixels

    for (int row = 0; row < patch.rows; row++)
        for (int col = 0; col < patch.cols; col++)
            if (mask.at<Vec3s>(row + offset_row, col + offset_col)[0] == -1) {
                mask.at<Vec3s>(row + offset_row, col + offset_col) = Vec3s(short(index), short(row), short(col));
                nap.at<Vec3b>(row + offset_row, col + offset_col) = patch.at<Vec3b>(row,col);
            }

    for(int i = 0; i < overlap.size(); i++){
        if (graph.what_segment(i) == Graph<int,int,int>::SINK) {
            mask.at<Vec3s>(overlap[i].first + offset_row, overlap[i].second + offset_col) = Vec3s(short(index), short(overlap[i].first), short(overlap[i].second));
            nap.at<Vec3b>(overlap[i].first + offset_row, overlap[i].second + offset_col) = Vec3b(patch.at<Vec3b>(overlap[i].first,overlap[i].second));
        }
    }

}

void Montage::reset() {
    for (int row = 0; row < nap.rows; row++)
        for (int col = 0; col < nap.cols; col++) {
            nap.at<Vec3b>(row, col) = Vec3b(0, 0, 0);
            mask.at<Vec3s>(row, col) = Vec3s(-1, 0, 0);
            fixed.at<schar>(row,col) = schar(-1);
        }
}

void Montage::show() {
    map<int,int> colors;
    int index = 0;
    short c;
    Mat tmp(mask.rows, mask.cols, CV_8U);
    for(int row = 0; row < mask.rows; row++)
        for(int col = 0; col < mask.cols; col++) {
            c = mask.at<Vec3s>(row, col)[0];
            if (colors.count(c) == 0)
                colors[c] = index++;
        }
    int count = int(colors.size()) - 1;
    for (auto a: colors)
        colors[a.first] = count--;
    for(int row = 0; row < mask.rows; row++)
        for(int col = 0; col < mask.cols; col++) {
            c = mask.at<Vec3s>(row, col)[0];
            tmp.at<uchar>(row, col) = uchar(colors[c] * 255.0 / colors.size());
        }

    // add border
    for (int row = 0; row < mask.rows; row++) {
        nap.at<Vec3b>(row, extra_col - 1) = Vec3b(0, 255, 0);
        nap.at<Vec3b>(row, nap.cols - extra_col) = Vec3b(0, 255, 0);
        tmp.at<uchar>(row, extra_col - 1) = 255;
        tmp.at<uchar>(row, tmp.cols - extra_col) = 255;
    }
    for (int col = 0; col < mask.cols; col++) {
        nap.at<Vec3b>(extra_row - 1, col) = Vec3b(0, 255, 0);
        nap.at<Vec3b>(nap.rows - extra_row, col) = Vec3b(0, 255, 0);
        tmp.at<uchar>(extra_row - 1, col) = 255;
        tmp.at<uchar>(nap.rows - extra_row, col) = 255;
    }

    imshow("Mask", tmp);
    imshow("Image", nap);
}

void Montage::save_mask(string mask_name) const {
    Rect rect(extra_col, extra_row, max_col - 2 * extra_col, max_row - 2 * extra_row);

    // find all existing patches

    Mat tmp(mask.rows, mask.cols, CV_8U);
    map<int,int> colors;
    int index = 0;
    short c;
    for(int row = 0; row < mask.rows; row++)
        for(int col = 0; col < mask.cols; col++) {
            c = mask.at<Vec3s>(row, col)[0];
            if (colors.count(c) == 0)
                colors[c] = index++;
        }

    // reorder colors

    int count = int(colors.size()) - 1;
    for (auto a: colors)
        colors[a.first] = count--;

    // compute new color

    for(int row = 0; row < mask.rows; row++)
        for(int col = 0; col < mask.cols; col++) {
            c = mask.at<Vec3s>(row, col)[0];
            tmp.at<uchar>(row, col) = uchar(colors[c] * 255.0 / colors.size());
        }

    // crop the result

    Mat output_mask = tmp(rect);
    imwrite(mask_name,output_mask);
}

void Montage::save_output(Mat &output) const {
    for (int row = 0; row < output.rows; row++)
        for (int col = 0; col < output.cols; col++)
            output.at<Vec3b>(row, col) = nap.at<Vec3b>(row + extra_row, col + extra_col);
}
