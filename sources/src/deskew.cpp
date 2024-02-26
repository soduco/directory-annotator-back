#include <scribo.hpp>

#include <mln/core/image/ndimage.hpp>
#include <spdlog/spdlog.h>
#include <cmath>

#include "config.hpp"

namespace
{

  // Deskew offset detection
  // Sort *vertical* segments from left to right and display their angle (compute their average value)
  float detect_offset(const std::vector<Segment>& segments, int width, float angle_tolerance)
  {
    constexpr float kBorder = 0.1f; // 10% of the total width

    fmt::memory_buffer buf;
    double sum = 0;
    int    count = 0;
    for (const auto& s : segments)
    {
      double angle = s.angle;
      if (!s.is_vertical(angle_tolerance))
        continue;

      // Dismiss segments on the left/right edges
      if (std::min(s.start.x, s.end.x) < kBorder * width ||
          std::max(s.start.x, s.end.x) > (1-kBorder) * width)
        continue;

      sum += angle;
      count++;
      fmt::format_to(std::back_inserter(buf), "{},", angle);
    }
    if (count == 0)
    {
      spdlog::info("No segment detected !");
      return 90;
    }

    spdlog::debug("Segment angles: {}", std::string_view{buf.data(), buf.size()});
    return sum / count;
  }
} // namespace


namespace scribo
{
  std::vector<Segment>& deskew_segments(std::vector<Segment>& segments, float angle)
  {
    float c      = std::cos(angle * M_PI / 180);

    for (auto& s : segments)
    {
      s.start.x -= s.start.y * c;
      s.end.x -= s.end.y * c;
    }
    return segments;
  }

  float deskew_estimation(const std::vector<Segment>& segments, int image_width, float angle_tolerance)
  {
    float angle = detect_offset(segments, image_width, angle_tolerance);
    return angle;
  }


  mln::image2d<uint8_t> deskew_image(const mln::image2d<uint8_t> &input, float angle, uint8_t fill_value)
  {
    mln::image2d<uint8_t> out = mln::imconcretize(input).set_init_value(0);

    float c      = std::cos(angle * M_PI / 180);
    int   width  = input.width();
    int   height = input.height();

    const uint8_t* ilineptr = input.buffer();
    uint8_t*       olineptr = out.buffer();

    for (int y = 0; y < height; ++y)
    {
      float offset = y * c;
      for (int x = 0; x < width; ++x)
      {
        float xin = x + offset;
        int   x0  = std::floor(xin);
        int   x1  = x0 + 1;

        if (x1 <= 0 || x0 >= (width - 1))
          olineptr[x] = fill_value;
        else
        {
          float alpha = (xin - x0);
          olineptr[x] = (1.f - alpha) * ilineptr[x0] + alpha * ilineptr[x1];
        }
      }
      ilineptr += input.stride();
      olineptr += out.stride();
    }
    return out;
  }

}

