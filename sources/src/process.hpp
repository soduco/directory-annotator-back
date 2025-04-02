#pragma once

#include <string>
#include <mln/core/image/ndimage_fwd.hpp>
#include <cstdint>

struct params
{
    int display_opts = 0;
    int denoising = -1;
    bool deskew = true;
    bool bg_suppression = true;
    int debug = 0;
    int xheight = -1;



    std::string output_path;        // The path to save the cleaned/deskewed image
    std::string output_layout_file; // The path to save the layout **image** (debug purpose)
    std::ostream* json = nullptr;   // Stream to output the detected regions as a JSON stream
};




/// @brief Helper function to process an image with given execution parameters
/// @param input 
/// @param params
/// @return A string with the detected layout
void process(mln::image2d<uint8_t> input, const params& params);