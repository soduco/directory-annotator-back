#include "scribo.hpp"

#include <mln/core/image/ndimage.hpp>
#include <mln/core/image/view/operators.hpp>
#include <mln/core/neighborhood/c4.hpp>
#include <mln/core/se/periodic_line2d.hpp>
#include <mln/core/se/rect2d.hpp>
#include <mln/core/algorithm/transform.hpp>
#include <mln/morpho/erosion.hpp>
#include <mln/io/imsave.hpp>
#include <mln/labeling/accumulate.hpp>
#include <mln/morpho/closing.hpp>
#include <mln/morpho/opening.hpp>
#include <mln/morpho/reconstruction.hpp>

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <mln/filters/nms.hpp>
#include <mln/core/canvas/private/traverse2d.hpp>

#include "config.hpp"
#include "gaussian_directional_2d.hpp"
#include "watershed.hpp"


#include <span>

namespace
{
  /*
  int labelize_local_min_of_first_col(const mln::image2d<uint8_t>& input, mln::image2d<int16_t>& output, mln::box2d roi,
                                      int nlabel)
  {
    int y0 = roi.y();
    int x  = roi.x();
    int h  = roi.height();

    std::vector<int>     tmp0(h + 2);
    std::vector<uint8_t> tmp(h + 2, true);
    int*                 vals         = tmp0.data() + 1;
    bool*                is_local_min = (bool*)(tmp.data()) + 1;

    vals[-1] = 255;
    vals[h]  = 255;

    // Copy column
    for (int i = 0; i < h; ++i)
      vals[i] = input({x, y0 + i});

    // Forward
    for (int i = 0; i < h; ++i)
    {
      if (vals[i - 1] < vals[i])
        is_local_min[i] = false;
      else if (vals[i - 1] <= vals[i] && !is_local_min[i - 1])
        is_local_min[i] = false;
    }

    // Backward
    for (int i = h - 1; i >= 0; --i)
    {
      if (vals[i + 1] < vals[i])
        is_local_min[i] = false;
      else if (vals[i + 1] <= vals[i] && !is_local_min[i + 1])
        is_local_min[i] = false;
    }

    // Copy column
    is_local_min[-1] = false;
    for (int y = 0; y < h; ++y)
      if (is_local_min[y] && !is_local_min[y - 1] && vals[y] < UINT8_MAX)
        output({x, y0 + y}) = ++nlabel;

    return nlabel;
  }
  */

  struct bbox : mln::Accumulator<bbox>
  {
    using P             = mln::point2d;
    using argument_type = mln::point2d;
    using result_type   = bbox;


    bbox() = default;
    void init() noexcept { *this = bbox(); }

    void take(mln::point2d p)
    {
      xmin = std::min(xmin, p.x());
      xmax = std::max(xmax, p.x());
      ymin = std::min(ymin, p.y());
      ymax = std::max(ymax, p.y());
      mc_x += p.x();
      mc_y += p.y();
      count += 1;
    }

    void take(bbox o)
    {
      xmin = std::min(xmin, o.xmin);
      xmax = std::max(xmax, o.xmax);
      ymin = std::min(ymin, o.ymin);
      ymax = std::max(ymax, o.ymax);
      mc_x += o.mc_x;
      mc_y += o.mc_y;
      count += o.count;
    }

    result_type to_result() const noexcept
    {
      return *this;
    }

    mln::box2d box() const
    {
      if (xmin < xmax && ymin < ymax)
        return mln::box2d(xmin, ymin, xmax - xmin + 1, ymax - ymin + 1);
      else
        return {};
    }

    float get_mc_y() const { return float(mc_y) / count; }


  private:
    int xmin = INT_MAX;
    int xmax = INT_MIN;
    int ymin = INT_MAX;
    int ymax = INT_MIN;
    int mc_y = 0;
    int mc_x = 0;
    int count = 0;
  };


  void compute_line_bboxes(mln::image2d<uint8_t> input, mln::image2d<int16_t> ws, mln::box2d region, int nlabel, std::vector<scribo::LayoutRegion>* out, int parent_id, KConfig config)
  {
    using mln::box2d;
    using namespace mln::view::ops;


    // const float kLineVerticalSigma = (kLineHeight / 2.f) / 10.f;
    // const float kLineHorizontalSigma = (kWordWidth / 2.f);
    input = input.clip(region);
    ws     = ws.clip(region);

    auto tmp   = mln::view::ifelse(input < config.kLayoutWhiteLevel, ws, int16_t(0));
    auto attr = mln::labeling::accumulate(tmp, nlabel, bbox{});

    // Remove regions on the same line
    std::vector<int> labels(nlabel);
    std::iota(labels.begin(), labels.end(), 1);
    auto subrange = std::ranges::stable_partition(labels, [&attr](int i) { return attr[i].box().empty();});


    if (subrange.empty())
      return;

    std::ranges::stable_sort(subrange, std::ranges::less{}, [&attr](int i) { return attr[i].get_mc_y(); });

    std::vector<int16_t> newlabels(nlabel+1);
    std::iota(newlabels.begin(), newlabels.end(), 0);

    int prev = subrange[0];
    float a = attr[prev].get_mc_y();
    for (std::size_t k = 1; k < subrange.size(); ++k)
    {
      int cur = subrange[k];
      float b = attr[cur].get_mc_y();
      if ((b - a) < config.kxheight)
      {
        attr[prev].take(attr[cur]);
        attr[cur].init();
        newlabels[cur] = prev;
      }
      else
      {
        a = b;
        prev = cur;
      }
    }

    mln::for_each(ws, [&newlabels] (int16_t& v){ v = newlabels[v]; });

    for (auto i : subrange)
    {
      auto b = attr[i].box();
      if (!b.empty())
      {
        scribo::LayoutRegion r;
        r.bbox        = {b.x(), b.y(), b.width(), b.height()};
        r.parent_id   = parent_id;
        r.type        = DOMCategory::LINE;
        r.mask_instance_id = i;
        out->push_back(r);
      }
    }

  }


  void blur_region_and_labelize(const mln::image2d<uint8_t>& in, mln::box2d region, mln::image2d<uint8_t>& output, KConfig config)
  {
    const float kLineVerticalSigma   = config.kLineHeight * 0.1f;
    const float kLineHorizontalSigma = config.kWordWidth * 0.3f;
    const int kWhiteLevelForLineSplit = 230;

    auto out = output.clip(region);
    mln::copy(in.clip(region), out);
    gaussian2d(out, kLineHorizontalSigma, kLineVerticalSigma, 255);

    auto r   = mln::se::rect2d(static_cast<int>(2.0f * config.kOneEm + 0.5f),
                               static_cast<int>(0.5f * config.kxheight + 0.5f));
    auto clo = mln::morpho::closing(out, r);

    // Suppression of non-local minima
    {
      mln::image2d<uint8_t> markers;
      markers.resize(region, mln::image_build_params{.init_value = uint8_t(255)});

      for (int y = 0; y < region.height(); ++y)
      {
        uint8_t* lineptr = clo.buffer() + y * clo.stride();
        for (int x = 0; x < region.width();)
        {
          while (x < region.width() && lineptr[x] > kWhiteLevelForLineSplit)
            x++;
          int xmin = x;
          while (x < region.width() && lineptr[x] <= kWhiteLevelForLineSplit)
            {
              if (lineptr[x] < lineptr[xmin])
                xmin = x;
              x++;
            }
          if (xmin < region.width())
            markers({region.x() + xmin, region.y() + y}) = lineptr[xmin];
        }
      }
      markers = mln::morpho::closing(markers, mln::se::periodic_line2d({0,1}, int(config.kxheight / 4.f)));
      //mln::io::imsave(markers, "markers.tiff");

      clo = mln::morpho::closing_by_reconstruction(clo, markers, mln::c4);
      mln::copy(clo, out);
    }
  }
}



namespace scribo
{
  mln::image2d<int16_t> WSLineExtraction(const mln::image2d<uint8_t>& input, std::span<Box> regions, std::string_view debug_path, KConfig config, std::vector<scribo::LayoutRegion>* bboxes)
  {
    using mln::point2d;

    // Opening with a horizontal SE to give matters to letters (merge letter/words but not lines)
    mln::image2d<uint8_t> f;
    {
      mln::se::periodic_line2d l(point2d{1, 0}, config.kLayoutBlockOpeningWidth / 2);
      f = mln::morpho::opening(input, l);

      if (!debug_path.empty())
        mln::io::imsave(f, fmt::format("{}-00-input.tiff", debug_path));
    }

    mln::image2d<uint8_t> blurred;
    mln::resize(blurred, f).set_init_value(uint8_t(255));



    // 1. Blur the image in columns only and prepare WS markers
    for (auto r : regions)
      blur_region_and_labelize(input, mln::box2d{r.x, r.y, r.width, r.height}, blurred, config);

    if (!debug_path.empty())
      mln::io::imsave(blurred, fmt::format("{}-01-blurred.tiff", debug_path));

    //impl::watershed(blurred, mln::c4, markers, nlabel);
    int nlabel;
    auto ws = mln::morpho::watershed<int16_t>(blurred, mln::c4, nlabel);

    //auto ws = markers;

    // 4. Compute the bounding box and insert lines
    {
      if (bboxes)
      {
        bboxes->clear();
        for (int i = 0; i < (int)regions.size(); ++i)
        {
          const auto& r = regions[i];
          compute_line_bboxes(input, ws, mln::box2d{r.x, r.y, r.width, r.height}, nlabel, bboxes, i, config);
        }
        if ((int)bboxes->size() != nlabel)
          spdlog::warn("Invalid number of between WS (={}) and output (={}). A layout error is likely.", nlabel, bboxes->size());
      }
    }

    if (!debug_path.empty())
      mln::io::imsave(ws, fmt::format("{}-05-ws.tiff", debug_path));

    return ws;
  }


} // namespace scribo
