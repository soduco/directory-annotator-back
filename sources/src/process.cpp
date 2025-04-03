#include "process.hpp"


#include <mln/core/image/ndimage.hpp>
#include <scribo.hpp>
#include <spdlog/spdlog.h>

#include <mln/io/imsave.hpp>
#include <mln/core/algorithm/transform.hpp> 

#include "config.hpp"
#include "display.hpp"
#include "export.hpp"




void process(mln::ndbuffer_image _input, const params& params)
{
    // Convert to grayscale
    mln::image2d<uint8_t> input;
    if (auto* tmp = _input.cast_to<uint8_t, 2>(); tmp != nullptr) {
      input = *tmp;
    } else if (auto* tmp = _input.cast_to<mln::rgb8, 2>(); tmp != nullptr) {
      input = mln::transform(*tmp, [](mln::rgb8 c) -> uint8_t { return (c[0] + c[1] + c[2]) / 3; });
    } else {
      auto err = fmt::format("Unsupported image format (ndim={}, sample_type={})", _input.pdim(), (int)_input.sample_type());
      spdlog::info(err);
      throw std::runtime_error(err);
    }



    // 1. Cleaning
    scribo::cleaning_parameters cparams;
    cparams.xheight = params.xheight;

    mln::image2d<uint8_t> deskewed;
    auto clean = scribo::clean_document(input, cparams, params.bg_suppression ? nullptr : &deskewed);

    spdlog::info("[Cleaning] x-height: {}", cparams.xheight);
    spdlog::info("[Cleaning] x-width: {}", cparams.xwidth);
    spdlog::info("[Cleaning] Deskew angle: {}", cparams.deskew_angle);
    spdlog::info("[Cleaning] Denoising: {}", cparams.denoise);

    if (!params.output_path.empty())
    {
      auto manifest_file = params.output_path.substr(0, params.output_path.find_last_of('.')).append("-manifest.json");
      export_manifest(manifest_file.c_str(), cparams);
      auto& exported = params.bg_suppression ? clean : deskewed;
      mln::io::imsave(exported, params.output_path);
    }

    if (params.json == nullptr)
      return;

    // 2.

    auto segments = scribo::extract_segments(input);
    scribo::deskew_segments(segments, cparams.deskew_angle);
    auto config  = KConfig(cparams.xheight, 1);
    auto regions = scribo::XYCutLayoutExtraction(clean, segments, config);

    std::vector<Box> text_boxes;
    std::vector<int> text_boxes_ids;
    for (std::size_t i = 0; i < regions.size(); ++i)
      if (regions[i].type == DOMCategory::COLUMN_LEVEL_2)
      {
        text_boxes.push_back(regions[i].bbox);
        text_boxes_ids.push_back(i);
      }

    mln::image2d<int16_t> ws;

    {
      int start = regions.size();
      {
        std::vector<scribo::LayoutRegion> line_regions;
        ws = scribo::WSLineExtraction(clean, text_boxes, (kDebugLevel >= 2) ? "debug-ws" : "", config, &line_regions);
        for (auto&& t : line_regions)
          t.parent_id = text_boxes_ids[t.parent_id];

        std::ranges::copy(line_regions, std::back_inserter(regions));
        // mln::io::imsave(ws, "ws.tiff");
      }

      // Entry extraction
      int end = regions.size();
      regions.reserve(regions.size() * 2);
      for (int i = start; i < end;)
      {
        auto q = regions[i].parent_id;
        int  k = i;
        while (i < end && regions[i].parent_id == q)
          ++i;
        if (k < i)
          scribo::EntryExtraction(regions[q].bbox, std::span(regions.begin() + k, regions.begin() + i), regions);
      }
    }


    auto disp = display(clean, regions, segments, &ws, params.display_opts);

    if (!params.output_layout_file.empty())
      mln::io::imsave(disp, params.output_layout_file);

    if (params.json != nullptr)
      scribo::to_json(regions, *params.json);
}