#include <fstream>

#include <mln/io/imread.hpp>
#include <mln/core/image/ndimage.hpp>

#include "config.hpp"
#include "display.hpp"

#include <spdlog/spdlog.h>
#include <CLI/CLI.hpp>
#include "pdf_tool.hpp"

#include "process.hpp"

#include <fmt/format.h>
#include <ranges>


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
    std::string json_format_path;
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
    app.add_option("json", json_format_path, "Output layout file as a json file (ex: Didot-1851a/{page:04}.json)");
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
      auto json_file = std::fstream(fmt::format(fmt::runtime(json_format_path), fmt::arg("page", p)));

      aa.output_path = fmt::format(fmt::runtime(aa.output_path), fmt::arg("page", p));
      aa.output_layout_file = fmt::format(fmt::runtime(aa.output_layout_file), fmt::arg("page", p));
      aa.json = &json_file;
      process(input, aa);
    }
  }
}
