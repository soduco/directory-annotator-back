#include "scribo.hpp"
#include <mln/core/image/ndimage.hpp>

#include <spdlog/spdlog.h>

#include "signal.hpp"
#include "Interval.hpp"
#include "config.hpp"

#include <mln/core/se/periodic_line2d.hpp>
#include <mln/io/imsave.hpp>
#include <mln/morpho/opening.hpp>
//#include <mln/morpho/rank_filter.hpp>

#include <climits>
#include <span>

namespace
{
  constexpr int        kWhiteThreshold = 200; // Gray value for which a pixel is considered white (background)
  static constexpr int kExtraMargin = 2; // Add this as a margin to block

  // Detect the left border
  std::pair<int, int> detect_border(const uint8_t* is_white, int n, KConfig config)
  {
    int l = static_cast<int>(config.kLayoutPageMargin * n);
    int r = static_cast<int>((1 - config.kLayoutPageMargin) * n);

    while (l > 0 && !is_white[l])
      l--;

    while (l < r && is_white[l])
      l++;

    while (r < (n - 1) && !is_white[r])
      r++;

    while (r > l && is_white[r])
      r--;

    return std::make_pair(l, r + 1);
  }


  ///
  /// Compute the real ROI of the image, ignoring black borders
  mln::box2d crop(mln::image2d<uint8_t> input, const char* debug_path, KConfig config)
  {
    using mln::box2d;
    using mln::point2d;

    int width  = input.width();
    int height = input.height();

    box2d roi;
    // 1. Opening by super-large lines to crop the document to the good region
    {
      mln::image2d<uint8_t> vblock, hblock;
      // Detect left/right border
      {
        mln::se::periodic_line2d l(point2d{0, 1}, config.kLayoutPageOpeningHeight / 2);
        hblock = mln::morpho::opening(input, l);

        auto is_column_white =
            is_number_of_black_pixels_less_than(hblock, kWhiteThreshold, 1.f - config.kLayoutPageFullLineWhite, Axis::Y);
        auto [a, b]  = detect_border(is_column_white.data(), width, config);
        roi.tl().x() = a;
        roi.br().x() = b;
      }
      // Detect bottom/top border
      {
        {
          mln::se::periodic_line2d l(point2d{1, 0}, config.kLayoutPageOpeningWidth / 2);
          vblock = mln::morpho::opening(input, l);
        }
        mln::image2d<uint8_t> vblock2;
        // Opening with a vertical SE to connect lines (makes block)
        {
          mln::se::periodic_line2d l(point2d{0, 1}, config.kLayoutBlockOpeningHeight);
          vblock2 = mln::morpho::opening(vblock, l);
        }

        auto is_row_white =
            is_number_of_black_pixels_less_than(vblock2, kWhiteThreshold, 1.f - config.kLayoutPageFullLineWhite, Axis::X);
        auto [a, b]  = detect_border(is_row_white.data(), height, config);
        roi.tl().y() = a;
        roi.br().y() = b;
      }

      if (kDebugLevel > 1)
      {
        mln::io::imsave(hblock, fmt::format("{}-01-crop-h.tiff", debug_path));
        mln::io::imsave(vblock, fmt::format("{}-02-crop-v.tiff", debug_path));
      }
      spdlog::debug("Document border (x1,y1,x2,y2): {} {} {} {}", roi.tl().x(), roi.tl().y(), roi.br().x(),
                    roi.br().y());
    }
    return roi;
  }

  // Split horizontally (vertical separation)
  void hsplit(const mln::image2d<uint8_t>& input, std::span<Segment> segments, std::vector<scribo::LayoutRegion>& out, KConfig config,
              int parent_id, int level = 0);


      // Split vertically - (horizontal separation)
  void vsplit(const mln::image2d<uint8_t>& input, std::span<Segment> segments, std::vector<scribo::LayoutRegion>& out, KConfig config,
              int parent_id, int level = 0);


  // Split horizontally (vertical separation)
  void hsplit(const mln::image2d<uint8_t>& input, std::span<Segment> segments, std::vector<scribo::LayoutRegion>& out, KConfig config,
              int parent_id, int level)
  {
    const auto parent = out[parent_id];
    Box   region = parent.bbox;


    // Retrieve segments in the region
    std::vector<Segment> region_segments;
    std::vector<Segment> ver_segments;

    for (const Segment& seg : segments)
      if (region.has(Point2D{seg.start.x, seg.start.y}) && region.has(Point2D{seg.end.x, seg.end.y}))
      {
        region_segments.push_back(seg);
        if (seg.is_vertical(config.kAngleTolerance))
          ver_segments.push_back(seg);
      }


    auto rnks = is_number_of_black_pixels_less_than(input.clip(mln::box2d{region.x, region.y, region.width, region.height}), config.kLayoutWhiteLevel, 0.01f, Axis::Y);
    // std::vector<int> rnks    = rank_along_y_axis(blocks2.clip(region), 0.02f);


    // For every vertical segment, force the split
    {
      for (const auto& s : ver_segments)
      {
        spdlog::debug("Vertical split forced by segment (x={}, y1={} y2={} angle={})", s.start.x, s.start.y, s.end.y,
                      s.angle);
        float x  = (s.start.x + s.end.x) / 2.f - region.x;
        int   x0 = std::max(0.f, x - 3 * kExtraMargin);
        int   x1 = std::min((float)region.width, x + 3 * kExtraMargin);
        for (int x = x0; x < x1; ++x)
          rnks[x] = UINT8_MAX; // White out
      }
    }

    auto columns = partitions(rnks, config.kColumnMinSpacing);


    spdlog::debug("{:<{}}** Vertical split - number of regions={}", "", level * 2, columns.size());
    for (auto [x0, x1] : columns)
    {
      scribo::LayoutRegion sec;
      x0       = std::max(region.x0(), region.x + x0 - 3 * kExtraMargin);
      x1       = std::min(region.x1(), region.x + x1 + 3 * kExtraMargin);
      sec.bbox = {x0, region.y, x1 - x0, region.height};
      if (parent.type == DOMCategory::SECTION_LEVEL_1)
        sec.type = DOMCategory::COLUMN_LEVEL_1;
      else if (parent.type == DOMCategory::SECTION_LEVEL_2)
        sec.type = DOMCategory::COLUMN_LEVEL_2;
      else
      {
        spdlog::error("Invalid parent type {} (Expected: {} or {})", (int)parent.type, (int)DOMCategory::SECTION_LEVEL_1, (int)DOMCategory::SECTION_LEVEL_2);
        throw std::runtime_error("Invalid document layout.");
      }
      sec.parent_id = parent_id;

      spdlog::debug("{:>{}} Detected x-section [{}--{}]", "+", level * 2, x0, x1);

      out.push_back(sec);
      if (sec.type == DOMCategory::COLUMN_LEVEL_1)
        vsplit(input, std::span{region_segments.begin(), region_segments.end()}, out, config, out.size() - 1, level + 1);
    }
  }

  // Split vertically - (horizontal separation)
  void vsplit(const mln::image2d<uint8_t>& input, std::span<Segment> segments, std::vector<scribo::LayoutRegion>& out, KConfig config,
              int parent_id, int level)
  {
    int   kInnerRegion = 10;
    const auto parent  = out[parent_id];
    Box   region       = parent.bbox;


    // Retrieve segments in the region
    std::vector<Segment> region_segments;
    std::vector<Segment> hor_segments;
    IntervalSet          ver_segments;

    {
      auto outer_region = region;
      auto inner_region = region;
      inner_region.inflate(-kInnerRegion);

      for (const Segment& seg : segments)
      {
        if (outer_region.has(seg))
          region_segments.push_back(seg);
        if (seg.is_horizontal(config.kAngleTolerance) && outer_region.has(seg))
          hor_segments.push_back(seg);
        else if (seg.is_vertical(config.kAngleTolerance) && inner_region.has(seg))
          ver_segments.insert(seg.start.y, seg.end.y);
      }
    }

    // At level 0, we consider a blank line as 90% of blank pixels (because of separators)
    // At level 1, we consider a blank line if:
    ///   1. There are 95% of blank pixels on the 1st 1/3rd (because a line can be composed of a single word as a large
    ///   as a centered separator )
    ///   2. There are 70% of blank pixels on the 2nd 1/3rd (because a title might be centered and we dont want
    ///   separator to considered as text).
    std::vector<uint8_t> ysum;
    // if (hsec->type() == DOMCategory::PAGE)
    //{
    //  //ysum = rank_along_x_axis(blocks1.clip(region), 0.05f);
    //  ysum = is_number_of_black_pixels_less_than(blocks1.clip(region), kWhiteThreshold, 0.10f, Axis::X);
    //}
    // else
    {
      int h = region.height;
      int w = region.width;
      ysum.resize(3 * h);

      mln::box2d roi = {region.x, region.y, region.width, region.height};
      roi.set_width(w / 4);
      is_number_of_black_pixels_less_than(input.clip(roi), kWhiteThreshold, config.kCountEmptyLine, Axis::X,
                                          ysum.data()); // 90% on 1st third
      // rank_along_x_axis(blocks1.clip(roi), 0.05f, ysum.data());

      roi.tl().x() = region.x + w / 3;
      roi.br().x() = region.x + 2 * (w / 3);
      is_number_of_black_pixels_less_than(input.clip(roi), kWhiteThreshold, config.kCountEmptyLine, Axis::X,
                                          ysum.data() + h); // 90% on the 2nd third


      roi.tl().x() = region.x + 2 * (w / 3);
      roi.br().x() = region.x + w;
      is_number_of_black_pixels_less_than(input.clip(roi), kWhiteThreshold, config.kCountEmptyLine, Axis::X,
                                          ysum.data() + 2 * h); // 90% on the 3rd third
      // rank_along_x_axis(blocks1.clip(roi), 0.3f, ysum.data() + h); // 50% on 2nd third

      for (int i = 0; i < h; ++i)
      {
        ysum[i] = std::min({ysum[i], ysum[i + h], ysum[i + 2 * h]}); // Min <=> Logical And
        //fmt::print("{},",ysum[i]);
      }
      //fmt::print("\n");
      ysum.resize(h);
    }

    // For every horizontal segment > 25% of the block width, force the split
    {
      for (const auto& s : hor_segments)
      {
        if (s.length < 0.25f * region.width)
          continue;

        spdlog::debug("Horizontal split forced by segment (y={}, x1={} x2={} angle={})", s.start.y, s.start.x, s.end.x,
                      s.angle);
        int y0 = std::max(0, s.start.y - region.y - 3);
        int y1 = std::min(s.end.y + 3, region.y1()) - region.y;
        for (int y = y0; y < y1; ++y)
          ysum[y] = 255; // White out
      }
    }
    // For every vertical segment in the region => no split
    {
      for (auto s : ver_segments.intervals())
      {
        spdlog::debug("Horizontal split prevented by segment (y0={} y1={})", s.a, s.b);
        int y0 = std::max(0, s.a - region.y);
        int y1 = std::min(region.y1(), s.b) - region.y;
        std::fill(ysum.data() + y0, ysum.data() + y1, 0);
      }

    }
    //for (auto v: ysum)
    //  fmt::print("{},",v);
    //fmt::print("\n");
    auto DOM = partitions(ysum, config.kSectionMinSpacing, config.kSectionMinSize);
    
    spdlog::debug("{:<{}}** Horizontal split - number of regions={}", "", level * 2, DOM.size());

    scribo::LayoutRegion* last = nullptr;
    int           children_start = out.size();
    for (auto [y0, y1] : DOM)
    {
      last            = &out.emplace_back();
      last->parent_id = parent_id;
      y0              = y0 + region.y - kExtraMargin;
      y1              = y1 + region.y + kExtraMargin;

      // Determine the section type
      if ((y1 - y0) < (3 * (config.kLineHeight / 2.f))) // We have a title box (with height < 3*lineHeight)
      {
        if (parent.type == DOMCategory::PAGE)
          last->type = DOMCategory::TITLE_LEVEL_1;
        else if (parent.type == DOMCategory::COLUMN_LEVEL_1)
          last->type = DOMCategory::TITLE_LEVEL_2;
        else
        {
          spdlog::error("Invalid parent type {} (Expected: {} or {})", (int)parent.type, (int)DOMCategory::PAGE, (int)DOMCategory::COLUMN_LEVEL_1);
          throw std::runtime_error("Invalid document layout.");
        }
      }
      else
      {
        if (parent.type == DOMCategory::PAGE)
          last->type = DOMCategory::SECTION_LEVEL_1;
        else if (parent.type == DOMCategory::COLUMN_LEVEL_1)
          last->type = DOMCategory::SECTION_LEVEL_2;
        else
        {
          spdlog::error("Invalid parent type {} (Expected: {} or {})", (int)parent.type, (int)DOMCategory::PAGE, (int)DOMCategory::COLUMN_LEVEL_1);
          throw std::runtime_error("Invalid document layout.");
        }
      }
      y0         = std::max(region.y0(), y0);
      y1         = std::min(region.y1(), y1);
      last->bbox = {region.x, y0, region.width, (y1 - y0)};
      spdlog::debug("{:<{}} Detected y-section [{}--{}]", "", level * 2, y0, y1);
    }


    int children_end = out.size();
    for (int i = children_start; i < children_end; i++)
    {
      auto sec = out[i];
      if (sec.type == DOMCategory::SECTION_LEVEL_1 || sec.type == DOMCategory::SECTION_LEVEL_2)
        hsplit(input, std::span{region_segments.begin(), region_segments.end()}, out, config, i, level + 1);
    }
  }
} // namespace


namespace scribo
{

  // Extract the top-level blocks (Sections + Columns + Sub-section + Sub-column)
  auto XYCutLayoutExtraction(const mln::image2d<uint8_t>& input, std::span<Segment> segments, KConfig config)
      -> std::vector<scribo::LayoutRegion>
  {
    const char* debug_path = "debug";

    using mln::point2d;

    if (kDebugLevel > 1)
      mln::io::imsave(input, fmt::format("{}-00-input.tiff", debug_path));

    // 1. Page region detection
    auto roi    = crop(input, debug_path, config);
    auto input0 = input.clip(roi);

    // 2. Make blocks (connect letters/word and lines)
    mln::image2d<uint8_t> blocks;
    {
      // Opening with a vertical SE to connect lines (makes block)
      {
        // mln::se::periodic_line2d l1(point2d{0,1}, int(0.1 * config.kOneEm + 0.5f));
        mln::se::periodic_line2d l2(point2d{0, 1}, config.kLayoutBlockOpeningHeight / 2);
        blocks = mln::morpho::opening(input0, l2);
        // blocks = mln::morpho::erosion(blocks, l2);
      }
      // Opening with a horizontal SE to connect lines (makes block)
      {
        mln::se::periodic_line2d l(point2d{1, 0}, config.kLayoutBlockOpeningWidth / 2);
        blocks = mln::morpho::opening(blocks, l);
      }
      if (kDebugLevel > 1)
        mln::io::imsave(blocks, fmt::format("{}-03-blocks.tiff", debug_path));
    }


    // 3. Remove vertical line separator
    // mln::image2d<uint8_t> blocks2;
    // {
    //   mln::se::periodic_line2d l(point2d{1, 0}, 3); // was 3
    //   using R = std::ratio<5, 7>;
    //   blocks2 = mln::morpho::rank_filter<R>(blocks, l, mln::extension::bm::fill(uint8_t(255)));
    //   if (kDebugLevel > 1)
    //     mln::io::imsave(blocks2, fmt::format("{}-04-blocks.tiff", debug_path));
    // }


    std::sort(segments.begin(), segments.end(), [](auto& s1, auto& s2) { return s1.start.y < s2.start.y; });

    std::vector<scribo::LayoutRegion> out;
    scribo::LayoutRegion              page;
    page.bbox = {roi.x(), roi.y(), roi.width(), roi.height()};
    page.type = DOMCategory::PAGE;
    out.push_back(page);
    vsplit(blocks, segments, out, config, 0);
    return out;
  }
}
