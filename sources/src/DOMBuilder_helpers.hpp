#pragma once

#include <mln/core/image/ndimage.hpp>
#include <vector>







/// Given an array A[i] of integer and a threshold t.
/// Returns argmin(A[i] < t)
int detect_leading_space(const int* values, std::size_t n, int threshold);

/// Given a boolean array A[i], return the posiition of the first 0
/// Returns argmin(A[i] == 0)
int detect_leading_space(const uint8_t* values, std::size_t n);




/// Sum-up the values of an image along the given axes
std::vector<int> sum_along_x_axis(const mln::image2d<uint8_t>& input);
std::vector<int> sum_along_y_axis(const mln::image2d<uint8_t>& input);



/// 
//void count_less_than(const mln::experimental::image2d<uint8_t>& input, uint8_t threshold, float* out, Axis axis);
//std::vector<float> count_less_than(const mln::experimental::image2d<uint8_t>& input, uint8_t threshold, Axis axis);



/*
/// Compute the rank value of an image along the given axes
/// \param rank Rank value in the range [0,1)
void rank_along_x_axis(const mln::experimental::image2d<uint8_t>& input, float rank, int* out);
void rank_along_y_axis(const mln::experimental::image2d<uint8_t>& input, float rank, int* out);
std::vector<int> rank_along_x_axis(const mln::experimental::image2d<uint8_t>& input, float rank);
std::vector<int> rank_along_y_axis(const mln::experimental::image2d<uint8_t>& input, float rank);
*/

int rank(const mln::image2d<uint8_t>& input, float r);

