#include "signal.hpp"

#include <mln/core/image/ndimage.hpp>
#include <mln/core/range/foreach.hpp>
#include <algorithm>

std::vector<int> find_peaks(std::span<const float> count)
{
  int n = (int)count.size();

  std::vector<bool> is_peak(n, true);

  for (int i = 1; i < n; ++i)
  {
    if (count[i - 1] > count[i])
      is_peak[i] = false;
    else if (count[i - 1] == count[i] && !is_peak[i - 1])
      is_peak[i] = false;
  }

  for (int i = n - 2; i >= 0; --i)
  {
    if (count[i + 1] > count[i])
      is_peak[i] = false;
    else if (count[i + 1] == count[i] && !is_peak[i + 1])
      is_peak[i] = false;
  }

  std::vector<int> peaks;
  for (int i = 0; i < n; ++i)
    if (is_peak[i])
      peaks.push_back(i);

  std::ranges::sort(peaks, std::ranges::greater{}, [&count](size_t k) { return count[k]; });
  return peaks;
}


namespace
{
  template <class T, class P>
  std::vector<std::pair<int, int>> split_section_T(std::span<T> data, int min_seperator_size,
                                                   int min_section_size, P proj)
  {
    std::vector<std::pair<int, int>> sections;
    int n = static_cast<int>(data.size());

    int i = 0;
    // Ignore the leading 1-sections
    while (i < n && proj(data[i]))
      i++;

    int left = i;
    while (i < n)
    {
      // Advance to the end of the 0-section
      while (i < n && !proj(data[i]))
        i++;

      int  right  = i;
      bool forced = false;
      // Advance to the end of the 1-section
      while (i < n && proj(data[i]))
      {
        forced |= (data[i] == std::numeric_limits<T>::max());
        i++;
      }

      int sep_size = i - right;
      if (i == n || sep_size >= min_seperator_size || forced)
      {
        if ((right - left) >= min_section_size)
          sections.push_back(std::make_pair(left, right));
        left = i;
      }
    }
  return sections;
  }
} // namespace


std::vector<std::pair<int, int>> partitions(std::span<const uint8_t> data, int min_seperator_size,
                                                 int min_section_size)
{
    return split_section_T(data, min_seperator_size, min_section_size, std::identity{});
}


std::vector<std::pair<int, int>> partitions(std::span<const int> data, int threshold, int min_seperator_size,
                                                 int min_section_size)
{
    return split_section_T(data, min_seperator_size, min_section_size, [threshold](int x) { return x >= threshold; });
}

void is_number_of_black_pixels_less_than(mln::image2d<uint8_t> input, int white_level, int threshold,
                                         Axis axis, uint8_t* out)
{
  std::vector<int> count;

  if (axis == Axis::Y)
  {
    int x0 = input.domain().x();
    count.resize(input.width(), 0);
    mln_foreach (auto px, input.pixels())
      count[px.point().x() - x0] += (px.val() < white_level);
  }
  else
  {
    int y0 = input.domain().y();
    count.resize(input.height(), 0);
    mln_foreach (auto px, input.pixels())
      count[px.point().y() - y0] += (px.val() < white_level);
  }

  std::transform(std::begin(count), std::end(count), out, [threshold](int c) { return c < threshold; });
}

void is_number_of_black_pixels_less_than(mln::image2d<uint8_t> input, int white_level, float percentile,
                                         Axis axis, uint8_t* out)
{
  int n = (axis == Axis::Y) ? input.height() : input.width();
  int np = int(percentile * n);
  is_number_of_black_pixels_less_than(input, white_level, np, axis, out);
}

std::vector<uint8_t> is_number_of_black_pixels_less_than(mln::image2d<uint8_t> input, int white_level,
                                                         float percentile, Axis axis)
{
  std::vector<uint8_t> out;
  out.resize((axis == Axis::Y) ? input.width() : input.height());
  is_number_of_black_pixels_less_than(input, white_level, percentile, axis, out.data());
  return out;
}