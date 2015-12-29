/*
 * Texture program
 *
 * This program generates a texture image from a smaller sample image. It uses the Graph Cut technique described in
 * a 2003 paper "Graphcut Textures: Image and Video Synthesis Using Graph Cut".
 *
 * Parameters: (all values are integers)
 *      i: path to the sample png image (with a smaller dimension)
 *      o: path to the output png image
 *      h: height of output image
 *      w: width of output image
 *      r: rotation range (from -r to r) in degree
 *      s: scaling factor (the (0, 0) pixel is set to 1)
 *      a: scaling direction in cartesian form (y = ax + b)
 *      b: scaling direction in cartesian form (y = ax + b)
 *
 * Usage:
 *      texture -i [sample_image] -o [output_image] -h [height] -w [weight] -r [rotation_range]
 *              -s [scaling_factor] -a [a] -b [b]
 */

#include <iostream>

using namespace std;

int main() {
    cout << "Hello, World!" << endl;
    return 0;
}