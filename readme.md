#Photomontage and Texture generation

##About

This project is based on Mr. Kwatra's paper [Graphcut Textures: Image and Video Synthesis Using Graph Cuts](http://www.cc.gatech.edu/cpl/projects/graphcuttextures/gc-final-lowres.pdf) 
and Mr. Agarwala's [Interactive Digital Photomontage](http://kneecap.cs.berkeley.edu/papers/photomontage/photomontage.pdf). It is an implementation of graph cuts technique.
We use two applications, photomontage and texture generation to illustrate this algorithm.

Texture generation is automatic, while photomontage need human order. The how-to is inside the code and the user interface should be easy to understand.

##Usage

Compile the program with CMake, you will need OpenCV to run the program. 

```
texture -i [input_file] -o [output_file] -h [height] -w [weight] -s [scale] -d [direction] -m [patch_mode] -t [iteration] -r [rotation_range]
montage -i [number_of_photos] [photo_1] .. [photo_n] -o [output_file] -h [height] -w [weight]
```

Here are two examples:

```
texture -i samples/floor.jpg -o results/floor.jpg -h 256 -w 256 -t 1000 -r 180
montage -i 2 photos/left.jpg photos/right.jpg -o results/montage.jpg -h 384 -w 512
```