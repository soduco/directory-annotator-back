#include <mln/io/imread.hpp>
#include <mln/io/imsave.hpp>
#include <mln/core/image/ndimage.hpp>
#include <mln/core/algorithm/for_each.hpp>
#include <mln/data/stretch.hpp>
#include <scribo.hpp>
#include "config.hpp"
#include "display.hpp"
#include <cmath>
#include <spdlog/spdlog.h>
#include <CLI/CLI.hpp>
#include "pdf_tool.hpp"

#include <fmt/format.h>
#include <ranges>
#include <charconv>



namespace scribo
{
    mln::image2d<uint8_t> background_substraction(const mln::image2d<uint8_t>& input, int& xwidth, int& xheight, bool denoise);

    std::pair<mln::image2d<uint8_t>, mln::image2d<uint8_t>>
    compute_gradients(mln::image2d<uint8_t> input, int xwidth, int xheight);

     float skew_estimation(mln::image2d<uint8_t> input, int xwidth, int xheight);

      mln::image2d<uint8_t> deskew_image(const mln::image2d<uint8_t> &input, float angle);

    void to_json(const char* filename, std::span<const LayoutRegion> regions);
    void export_manifest(const char* filename, const cleaning_parameters& cparams);
}



struct params
{
    int display_opts = 0;
    int denoising = -1;
    bool deskew = true;
    bool bg_suppression = true;
    int debug = 0;
    int xheight = -1;



    std::string output_path;
    std::string output_layout_file;
    std::string json_path;
};

void process(mln::image2d<uint8_t> input, const params& params)
{
    // 1. Cleaning
    scribo::cleaning_parameters cparams;
    cparams.xheight = params.xheight;

    mln::image2d<uint8_t> deskewed;
    auto clean = scribo::clean_document(input, cparams, params.bg_suppression ? nullptr : &deskewed);

    spdlog::info("[Cleaning] x-height: {}", cparams.xheight);
    spdlog::info("[Cleaning] x-width: {}", cparams.xwidth);
    spdlog::info("[Cleaning] Deskew angle: {}", cparams.deskew_angle);
    spdlog::info("[Cleaning] Denoising: {}", cparams.denoise);

    {
      auto manifest_file = params.output_path.substr(0, params.output_path.find_last_of('.')).append("-manifest.json");
      export_manifest(manifest_file.c_str(), cparams);
      auto& exported = params.bg_suppression ? clean : deskewed;
      mln::io::imsave(exported, params.output_path);
    }

    if (params.json_path.empty())
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

    if (!params.json_path.empty())
      scribo::to_json(params.json_path.c_str(), regions);
}

auto parse_range(std::string pages)
{
  int a = 0, b = 0;
  auto sep = pages.find("--");
  if (sep == pages.npos)
  {
    a = std::atoi(pages.c_str());
    b = a;
  }
  else
  {
    a = std::stoi(pages.substr(0, sep));
    b = std::stoi(pages.substr(sep+2));
  }
  spdlog::info("Processing pages {}--{}.", a, b);
  return std::views::iota(a, b+1);
}


int main(int argc, char** argv)
{
    using namespace std::literals::string_view_literals;

    CLI::App app{"Image denoiser and layout parser"};


    bool debug = false;
    std::string input_path;
    std::string pages;

    params args;

    app.add_flag("-d,--debug", debug, "Enable debug log/outputs");
    app.add_flag("!--no-deskew", args.deskew, "Disable deskewing");
    app.add_flag("!--no-bg-removal", args.bg_suppression, "Disable background suppression");
    app.add_flag("--denoise", args.denoising, "Force denoising (small components suppression). Enabled by default on B&W images");
    app.add_flag("!--no-denoise", args.denoising, "Disable denoising (small components suppression)");
    app.add_option("--ex", args.xheight, "Force the x-height (in pixels).");
    app.add_option("--page", pages, "Set the pdf view number (accept ranges as in '151--1400').");


    bool show_grid = false;
    bool show_segments = false;
    bool show_ws = false;
    app.add_flag("--show-grid", show_grid, "Draw the alignment grid");
    app.add_flag("--show-segments", show_segments, "Draw the detected segments");
    app.add_flag("--show-ws", show_ws, "Draw the watershed lines");

    //std::initializer_list<const char*> actions = {"clean", "layout"};
    //std::string action;
    //app.add_option("action", action)->required()->check(CLI::IsMember(actions));
    app.add_option("input", input_path, "Input image")->required()->check(CLI::ExistingFile);
    app.add_option("output", args.output_path, "Clean/deskewed input image  (ex: Didot-1851a/{page:04}.jpg)")->required();
    app.add_option("json", args.json_path, "Output layout file as a json file (ex: Didot-1851a/{page:04}.json)");
    app.add_option("--output-layout-image", args.output_layout_file, "Output layout image (debug) (ex: debug-{page:04}.json)");


    CLI11_PARSE(app, argc, argv);

    args.display_opts |= (show_grid * DISPLAY_GRID);
    args.display_opts |= (show_ws * DISPLAY_WS);
    args.display_opts |= (show_segments * DISPLAY_SEGMENTS);


    if (debug)
    {
      kDebugLevel = debug;
      spdlog::set_level(spdlog::level::debug);
    }
    else
    {
      spdlog::set_level(spdlog::level::warn);
    }


   
  
  if (!input_path.ends_with(".pdf"))
  {
    mln::image2d<uint8_t> input;
    mln::io::imread(input_path, input);
    process(input, args);
  }
  else
  {
    pdf_file pdf(input_path.c_str());
    for (int p : parse_range(pages))
    {
      auto input = load_from_pdf(input_path.c_str(), p);
      auto aa = args;
      aa.output_path = fmt::format(fmt::runtime(aa.output_path), fmt::arg("page", p));
      aa.output_layout_file = fmt::format(fmt::runtime(aa.output_layout_file), fmt::arg("page", p));
      aa.json_path = fmt::format(fmt::runtime(aa.json_path), fmt::arg("page", p));
      process(input, aa);
    }
  }
}
