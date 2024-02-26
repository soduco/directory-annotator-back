#include <scribo.hpp>

#include <spdlog/spdlog.h>
#include <tesseract/baseapi.h>
#include <mln/core/image/ndimage.hpp>

namespace
{
  void tesseract_init(tesseract::TessBaseAPI& tess_api, const mln::image2d<uint8_t>& input, bool is_line)
  {
    if (tess_api.Init(NULL, "fra"))
    {
      spdlog::error("Could not initialize tesseract.");
      throw std::runtime_error("Could not initialize tesseract");
    }
    else
      spdlog::info("Tesseract has been initialized.");

    auto seg_mode = tesseract::PSM_SINGLE_BLOCK;
    if (is_line)
      seg_mode = tesseract::PSM_SINGLE_LINE;

    tess_api.SetPageSegMode(seg_mode);
    tess_api.SetImage(input.buffer(), input.width(), input.height(), input.byte_stride(0), input.byte_stride(1));
    tess_api.SetSourceResolution(150);
  }

  void tesseract_end(tesseract::TessBaseAPI &tess_api)
  {
    tess_api.End();
  }
}

auto scribo::TesseractTextExtraction(const mln::image2d<uint8_t>& input, std::span<Box> regions,
                                     bool is_line) -> std::vector<std::string>
{
  tesseract::TessBaseAPI tess_api;
  tesseract_init(tess_api, input, is_line);

  std::vector<std::string> out;
  for (const auto& region : regions)
  {
    if (region.empty())
      continue;

    spdlog::debug("Start extraction of rect (x={},y={},w={},h={})", region.x, region.y, region.width, region.height);
    tess_api.SetRectangle(region.x, region.y, region.width, region.height);

    char* txt = tess_api.GetUTF8Text();
    out.push_back(txt);
    delete[] txt;
  }

  tesseract_end(tess_api);
  return out;
}
