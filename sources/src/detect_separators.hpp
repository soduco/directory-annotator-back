#pragma once


#include <CoreTypes.hpp>
#include <mln/core/image/ndimage_fwd.hpp>
#include <vector>
#include <cstdint>


std::vector<Segment> detect_separators_LSD(const mln::image2d<uint8_t>& input);
std::vector<Segment> detect_seperators_Kalman(const mln::image2d<uint8_t>& input, mln::image2d<bool>* binseg);
