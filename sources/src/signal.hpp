#pragma once

#include <span>
#include <vector>
#include <mln/core/image/ndimage.hpp>

enum class Axis
{
  Y = 0,
  X = 1
};

/**
 * @brief Find the peaks in a 1D signal
 * 
 * @param buffer
 * @return std::vector<int> The peak positions 
 */
std::vector<int> find_peaks(std::span<const float> buffer);



/// Give the partitions of 0 separated by 1 over a range (signal is padded with 1)
/// returns the 0-section that are separated by the the 1-section (i.e. returns the minima of B)
/// 1. The \p min_separator_size is used to has the size opening.
/// 2. The \p min_section_size is used to has the size closing
/// * The special value INT_MAX (or UINT8_MAX in the second version) is used to force the split
/// * The leading and the trailing 1-sections are ignored in the process
///
/// split_section([1 0 0 0 1 1 0 0 1 1 1 0 0 1]) with threshold = 1; min_sep_size = 3
///                  <----------->       <->
/// returns [ [1,8), [11, 13)]
std::vector<std::pair<int, int>>
partitions(std::span<const uint8_t> data, int min_seperator_size = 0, int min_section_size = 0);


/// A boolean array B[i] >= threshold is created  and the function
std::vector<std::pair<int, int>>
partitions(std::span<const int> data, int threshold, int min_seperator_size = 0, int min_section_size = 0);


// Return true column-wise or row-wise if the pourcent of black pixel (below white_level) is less than (strict) a given
// a threshold in percentage.
void is_number_of_black_pixels_less_than(mln::image2d<uint8_t> input, int white_level, int threshold, Axis axis,
                                         uint8_t* out);

// Return true column-wise or row-wise if the pourcent of black pixel (below white_level) is less than (strict) a given
// a threshold in percentage.
void is_number_of_black_pixels_less_than(mln::image2d<uint8_t> input, int white_level, float percentile, Axis axis,
                                         uint8_t* out);


std::vector<uint8_t> is_number_of_black_pixels_less_than(mln::image2d<uint8_t> input, int white_level, float percentile,
                                                         Axis axis);