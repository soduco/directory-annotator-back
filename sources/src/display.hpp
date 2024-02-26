#pragma once

#include <scribo.hpp>
#include <mln/core/image/ndimage_fwd.hpp>

enum {
    DISPLAY_GRID = 0x01,
    DISPLAY_SEGMENTS = 0x02,
    DISPLAY_WS = 0x04,
};

mln::image2d<mln::rgb8> display(const mln::image2d<uint8_t>& input, std::span<scribo::LayoutRegion> regions,
                                std::span<Segment> segments, mln::image2d<int16_t>* wslines, int display_options);