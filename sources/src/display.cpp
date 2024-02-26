#include "display.hpp"

#include "InternalTypes.hpp"

#include "region_lut.hpp"
#include <array>
#include <mln/core/algorithm/for_each.hpp>
#include <mln/core/algorithm/transform.hpp>
#include <mln/core/image/view/zip.hpp>
#include <mln/core/range/view/zip.hpp>
#include <random>
#include <algorithm>

#include <blend2d.h>
#include <spdlog/spdlog.h>

namespace
{

  struct bgra_t
  {
    uint8_t b,g,r,a;
  };

  void draw(const scribo::LayoutRegion& r, BLContext* ctx)
  {
      if (r.type == DOMCategory::SECTION_LEVEL_2)
      {
        ctx->setFillStyle(BLRgba32(255u, 0u, 0u, 25u));
        ctx->fillBox(r.bbox.x, r.bbox.y, r.bbox.x1(), r.bbox.y1());
      }
      else if (r.type == DOMCategory::TITLE_LEVEL_1 || r.type == DOMCategory::TITLE_LEVEL_2)
      {

        ctx->setFillStyle(BLRgba32(0u, 255u, 0u, 25u));
        ctx->fillBox(r.bbox.x, r.bbox.y, r.bbox.x1(), r.bbox.y1());
      }
      else if (r.type == DOMCategory::COLUMN_LEVEL_2)
      {
        ctx->setStrokeStyle(BLRgba32(50, 50, 50));
        ctx->setStrokeWidth(3);
        ctx->strokeBox(r.bbox.x, r.bbox.y, r.bbox.x1(), r.bbox.y1());
      }
      else if (r.type == DOMCategory::SECTION_LEVEL_1)
      {
        ctx->setStrokeStyle(BLRgba32(255, 0, 255));
        ctx->setStrokeWidth(7);
        ctx->strokeLine(r.bbox.x0() - 20, r.bbox.y0(), r.bbox.x1() + 20, r.bbox.y0());
        ctx->strokeLine(r.bbox.x0() - 20, r.bbox.y1(), r.bbox.x1() + 20, r.bbox.y1());
      }
      else if (r.type == DOMCategory::COLUMN_LEVEL_1)
      {
        ctx->setStrokeStyle(BLRgba32(0, 255, 255));
        ctx->setStrokeWidth(7);
        ctx->strokeLine(r.bbox.x0(), r.bbox.y0() - 20, r.bbox.x0(), r.bbox.y1() + 20);
        ctx->strokeLine(r.bbox.x1(), r.bbox.y0() - 20, r.bbox.x1(), r.bbox.y1() + 20);
      }
      else if (r.type == DOMCategory::LINE)
      {
        ctx->setStrokeStyle(BLRgba32(255, 0, 0));
        ctx->setStrokeWidth(2);
        ctx->strokeBox(r.bbox.x0(), r.bbox.y0(), r.bbox.x1(), r.bbox.y1());
      }
      else if (r.type == DOMCategory::PAGE)
      {
        ctx->setStrokeStyle(BLRgba32(255, 0, 255));
        ctx->setStrokeWidth(3);
        ctx->strokeBox(r.bbox.x, r.bbox.y, r.bbox.x1(), r.bbox.y1());
      }

  }

  
  void draw_ws_lines(const mln::image2d<int16_t>& lbls, mln::image2d<mln::rgb8>& out)
  {
    auto z = mln::view::zip(lbls, out);
    mln::for_each(z, [](const auto& v) {
      auto [lbl, vout] = v;
      if (lbl == 0)
        vout = mln::rgb8{0, 0, 0};
    });
  }

    
  void labelize_line(const mln::image2d<uint8_t>& input_,  //
                     const mln::image2d<int16_t>& ws_,     //
                     mln::image2d<bgra_t>&        output_,
                     std::span<scribo::LayoutRegion> regions,
                     int label)

  {
    int q = regions[label].parent_id;
    if (!(q >= 0 && regions[q].type == DOMCategory::ENTRY))
    {
      spdlog::error("Invalid layout detected.");
      return;
    }


    auto roi = regions[label].bbox;
    mln::box2d region(roi.x, roi.y, roi.width, roi.height);

    auto in = input_.clip(region);
    auto out  = output_.clip(region);
    auto lbls = ws_.clip(region);

    mln::rgb8 c = region_lut(q);
    bgra_t c2 = {c[2], c[1], c[0], 255};

    int instance_id = regions[label].mask_instance_id;

    auto vals = mln::ranges::view::zip(in.values(), out.values(), lbls.values());
    for (auto&& r : mln::ranges::rows(vals))
      for (auto && [vin, vout, vlbl] : r)
      {
        if (vlbl == instance_id && vin < 150)
          vout = c2;
      }
  }

} // namespace


mln::image2d<mln::rgb8> //
display(const mln::image2d<uint8_t>& input,
        std::span<scribo::LayoutRegion> regions,
        std::span<Segment> segments,
        mln::image2d<int16_t>* ws,
        int display_options)
{
  //auto lbls = data->ws;
  //auto segments = data->deskewed.segments;

  auto out     = mln::transform(input, [](uint8_t x) -> bgra_t { return {x, x, x, 0}; });

  BLImage  img;
  BLResult err;
  int      width  = out.width();
  int      height = out.height();
  int      stride = out.byte_stride();
  spdlog::debug("w={} s={}", width, stride);
  err = img.createFromData(width, height, BL_FORMAT_PRGB32, out.buffer(), stride);
  if (err != BL_SUCCESS)
    spdlog::error("Unable to create rendering image.");


  BLContext ctx(img);
  err = ctx.setCompOp(BL_COMP_OP_SRC_OVER);
  if (err != BL_SUCCESS)
    spdlog::error("Unable to set comp op.");

  std::ranges::for_each(regions, [&ctx](const scribo::LayoutRegion& r) { draw(r, &ctx); });

  // Draw the grid
  if (display_options & DISPLAY_GRID)
  {
    ctx.setStrokeStyle(BLRgba32(50, 50, 50));
    ctx.setStrokeWidth(1);
    for (int y = 0; y < height; y += 30)
      ctx.strokeLine(0, y, width - 1, y);
    for (int x = 0; x < width; x += 30)
      ctx.strokeLine(x, 0, x, height - 1);
  }

  if (display_options & DISPLAY_SEGMENTS)
  {
    int k = 0;
    for (const auto& s : segments)
    {
      mln::rgb8 c = region_lut(k++);
      ctx.setStrokeStyle(BLRgba32(c[0], c[1], c[2]));
      ctx.setStrokeWidth(s.width);
      ctx.strokeLine(s.start.x, s.start.y, s.end.x, s.end.y);
    }
  }
  ctx.end();

  for (std::size_t i = 0; i < regions.size(); ++i)
    if (regions[i].type == DOMCategory::LINE)
      labelize_line(input, *ws, out, regions, i);

  auto res = mln::transform(out, [](const bgra_t& x) -> mln::rgb8 { return {x.r, x.g, x.b}; });

  // Draw watershed lines
  if ((display_options & DISPLAY_WS) && ws != nullptr)
    draw_ws_lines(*ws, res);

  return res;
}
