#include "scribo.hpp"
#include "config.hpp"
#include "subsample.hpp"

#include <spdlog/spdlog.h>

#include "CoreTypes.hpp"
#include "DOMTypes.hpp"
#include <mln/core/image/ndimage.hpp>
#include <pln/image_cast.hpp>

#include <pybind11/stl.h>
#include <pybind11/iostream.h>


namespace py = pybind11;

namespace scribo
{
  /// \brief This function is not thread-safe
  void set_debug_level(int debug_level) noexcept
  {
    kDebugLevel = debug_level;
    if (debug_level == 0)
      spdlog::set_level(spdlog::level::level_enum::info);
    else
      spdlog::set_level(spdlog::level::level_enum::debug);
  }

  int  get_debug_level() noexcept { return kDebugLevel; }


  std::vector<Segment> _extract_segments(mln::ndbuffer_image input)
  {
    auto img = input.cast_to<std::uint8_t, 2>();
    if (!img)
      throw std::runtime_error("Unable to cast the input image");


    [[maybe_unused]] py::call_guard<py::scoped_ostream_redirect, py::scoped_estream_redirect> _g;
    return extract_segments(*img);
  }


  mln::ndbuffer_image _deskew_image(mln::ndbuffer_image input, float angle)
  {
    mln::image2d<uint8_t>* img_in = input.cast_to<std::uint8_t, 2>();
    if (!img_in)
      throw std::runtime_error("Unable to cast the input/output image");

    [[maybe_unused]] py::call_guard<py::scoped_ostream_redirect, py::scoped_estream_redirect> _g;
    return deskew_image(*img_in, angle);
  }

  void _background_substraction_inplace(mln::ndbuffer_image input,
                                        float               kMinDiameter,   //
                                        float               kMinWidth,      //
                                        float               kMinHeight,     //
                                        float               kMinGrayLevel,  //
                                        float               kOpeningRadius, //
                                        std::string_view    debug_prefix = {})
  {
    mln::image2d<uint8_t>* img = input.cast_to<std::uint8_t, 2>();

    if (!img)
      throw std::runtime_error("Unable to cast the input/output image");

    [[maybe_unused]] py::call_guard<py::scoped_ostream_redirect, py::scoped_estream_redirect> _g;
    background_substraction_inplace(*img, kMinDiameter, kMinWidth, kMinHeight, kMinGrayLevel, kOpeningRadius, debug_prefix);
  }

  std::pair<mln::ndbuffer_image, py::dict> _clean_document(mln::ndbuffer_image input, int xheight, int xwidth, bool denoise) 
  {
    mln::image2d<uint8_t>* img = input.cast_to<std::uint8_t, 2>();

    if (!img)
      throw std::runtime_error("Unable to cast the input/output image");

    [[maybe_unused]] py::call_guard<py::scoped_ostream_redirect, py::scoped_estream_redirect> _g;
    scribo::cleaning_parameters params {
      .xwidth = xwidth,
      .xheight = xheight,
      .denoise = denoise
    };
    auto result = scribo::clean_document(*img, params);

    py::dict res;
    res["xwidth"] = params.xwidth;
    res["xheight"] = params.xheight;
    res["deskew_angle"] = params.deskew_angle;

    return {std::move(result), res};
  }

  mln::ndbuffer_image _subsample(mln::ndbuffer_image input)
  {
    mln::image2d<uint8_t>* img = input.cast_to<std::uint8_t, 2>();

    if (!img)
      throw std::runtime_error("Unable to cast the input/output image");

    return subsample(*img);
  }


  std::vector<scribo::LayoutRegion> _XYCutLayoutExtraction(mln::ndbuffer_image input, std::vector<Segment> segments, KConfig config)
  {
    mln::image2d<uint8_t>* img = input.cast_to<std::uint8_t, 2>();
    if (!img)
      throw std::runtime_error("Unable to cast the input/output image");

    [[maybe_unused]] py::call_guard<py::scoped_ostream_redirect, py::scoped_estream_redirect> _g;
    return XYCutLayoutExtraction(*img, std::span{segments.begin(), segments.end()}, config);
  }


  /// \brief Line extraction based on watershed algorithm
  auto _WSLineExtraction(mln::ndbuffer_image input, std::vector<Box> regions, std::string_view debug_path, KConfig config)
      -> std::pair<mln::ndbuffer_image, std::vector<LayoutRegion>>
  {
    mln::image2d<uint8_t>* img = input.cast_to<std::uint8_t, 2>();
    if (!img)
      throw std::runtime_error("Unable to cast the input/output image");

    [[maybe_unused]] py::call_guard<py::scoped_ostream_redirect, py::scoped_estream_redirect> _g;
    std::vector<LayoutRegion> bboxes;
    mln::ndbuffer_image ws = WSLineExtraction(*img, std::span{regions.begin(), regions.end()}, debug_path, config, &bboxes);
    return std::make_pair(ws, std::move(bboxes));
  }

  std::pair<std::vector<LayoutRegion>, std::vector<LayoutRegion>> _EntryExtraction(Box region, std::vector<LayoutRegion> lines, std::vector<LayoutRegion> regionset)
  {
    [[maybe_unused]] py::call_guard<py::scoped_ostream_redirect, py::scoped_estream_redirect> _g;
    EntryExtraction(region, lines, regionset);
    return std::make_pair(regionset, lines);
  }


  /// \brief
  auto _TesseractTextExtraction(mln::ndbuffer_image input, std::vector<Box> regions,
                                bool is_line = false) -> std::vector<std::string>
  {
    mln::image2d<uint8_t>* img = input.cast_to<std::uint8_t, 2>();
    if (!img)
      throw std::runtime_error("Unable to cast the input/output image");

    [[maybe_unused]] py::call_guard<py::scoped_ostream_redirect, py::scoped_estream_redirect> _g;
    return TesseractTextExtraction(*img, std::span{regions.begin(), regions.end()}, is_line);
  }
} // namespace scribo




PYBIND11_MODULE(scribocxx, m)
{
  pln::init_pylena_numpy(m);
  m.doc() = "A simple cpp image manipulator package";

  py::class_<KConfig>(m, "KConfig")
    .def(py::init<int, int>())
    .def_readwrite("kLineHeight", &KConfig::kLineHeight)
    .def_readwrite("kWordSpacing", &KConfig::kWordSpacing)
    .def_readwrite("kWordWidth", &KConfig::kWordWidth)
    .def_readwrite("kOneEm", &KConfig::kOneEm)
    .def_readwrite_static("kLineHorizontalSigma", &KConfig::kLineHorizontalSigma)
    .def_readwrite_static("kLineVerticalSigma", &KConfig::kLineVerticalSigma)
    .def_readwrite_static("kAngleTolerance", &KConfig::kAngleTolerance)
    .def_readwrite_static("kLayoutPageOpeningWidth", &KConfig::kLayoutPageOpeningWidth)
    .def_readwrite_static("kLayoutPageOpeningHeight", &KConfig::kLayoutPageOpeningHeight)
    .def_readwrite_static("kLayoutPageFullLineWhite", &KConfig::kLayoutPageFullLineWhite)
    .def_readwrite_static("kLayoutPageMargin", &KConfig::kLayoutPageMargin)
    .def_readwrite_static("kLayoutWhiteLevel", &KConfig::kLayoutWhiteLevel)
    .def_readwrite_static("kLayoutBlockFillingRatio", &KConfig::kLayoutBlockFillingRatio)
    .def_readwrite("kLayoutBlockOpeningWidth", &KConfig::kLayoutBlockOpeningWidth)
    .def_readwrite("kLayoutBlockOpeningHeight", &KConfig::kLayoutBlockOpeningHeight)
    .def_readwrite("kLayoutBlockMinHeight", &KConfig::kLayoutBlockMinHeight)
    .def_readwrite("kLayoutBlockMinWidth", &KConfig::kLayoutBlockMinWidth)
    ;

  py::class_<Point2D>(m, "Point2D")
    .def(py::init<>())
    .def_readwrite("x", &Point2D::x)
    .def_readwrite("y", &Point2D::y)
    ;

  py::class_<Segment>(m, "Segment")
    .def(py::init<>())
    .def_property("x0", &Segment::x0, &Segment::set_x0)
    .def_property("y0", &Segment::y0, &Segment::set_y0)
    .def_property("x1", &Segment::x1, &Segment::set_x1)
    .def_property("y1", &Segment::y1, &Segment::set_y1)
    .def_readwrite("width", &Segment::width)
    .def_readwrite("length", &Segment::length)
    .def_readwrite("angle", &Segment::angle)
    .def("scale", &Segment::scale)
    .def("__dir__", [](Segment&) -> std::vector<std::string> { return { "x0", "x1", "y0", "y1", "width", "length", "angle"}; })
    .def("__repr__", [](const Segment& s) -> py::str {
      py::dict d;
      d["x0"] = s.x0();
      d["y0"] = s.y0();
      d["x1"] = s.x1();
      d["y1"] = s.y1();
      d["width"] = s.width;
      d["length"] = s.length;
      d["angle"] = s.angle;
      return py::repr(d);
    })
    ;

  py::class_<Box>(m, "box", py::dynamic_attr())
    .def(py::init<int,int,int,int>())
    .def_readwrite("x", &Box::x)
    .def_readwrite("y", &Box::y)
    .def_readwrite("width", &Box::width)
    .def_readwrite("height", &Box::height)
    .def("x0", &Box::x0)
    .def("y0", &Box::y0)
    .def("x1", &Box::x1)
    .def("y1", &Box::y1)
    .def("scale", &Box::scale)
    .def("is_valid", &Box::is_valid)
    .def("__repr__", [](const Box& b) -> py::str {
      py::tuple d = py::make_tuple(b.x, b.y, b.width, b.height);
      return py::repr(d);
    })
    ;


  py::class_<scribo::LayoutRegion>(m, "LayoutRegion", py::dynamic_attr())
    .def(py::init<>())
    .def_property("x",      &scribo::LayoutRegion::x, &scribo::LayoutRegion::set_x)
    .def_property("y",      &scribo::LayoutRegion::y, &scribo::LayoutRegion::set_y)
    .def_property("width",  &scribo::LayoutRegion::width, &scribo::LayoutRegion::set_width)
    .def_property("height", &scribo::LayoutRegion::height, &scribo::LayoutRegion::set_height)
    .def_readwrite("mask_instance_id", &scribo::LayoutRegion::mask_instance_id)
    .def_readwrite("bbox", &scribo::LayoutRegion::bbox)
    .def_readwrite("type", &scribo::LayoutRegion::type)
    .def_readwrite("parent_id", &scribo::LayoutRegion::parent_id)
    .def("__dir__", [](scribo::LayoutRegion&) -> std::vector<std::string> { return { "x", "y", "width", "height", "type", "parent_id", "mask_instance_id"}; })
    .def("__repr__", [](py::object handle) -> py::str {
      py::dict d;
      d = py::getattr(handle, "__dict__", d);
      d = d.attr("copy")();
      scribo::LayoutRegion* r = handle.cast<scribo::LayoutRegion*>();
      d["x"] = r->x();
      d["y"] = r->y();
      d["width"] = r->width();
      d["heigth"] = r->height();
      d["type"] = r->type;
      d["parent_id"] = r->parent_id;
      d["mask_instance_id"] = r->mask_instance_id;
      return py::repr(d);
    })
    ;


  m.def("_extract_segments", &scribo::_extract_segments)
      .def("_deskew_segments", &scribo::deskew_segments)
      .def("_deskew_image", &scribo::_deskew_image)
      .def("_clean_document", &scribo::_clean_document)
      .def("_deskew_estimation", &scribo::deskew_estimation)
      .def("_deskew_segments", &scribo::deskew_segments)
      .def("_background_substraction_inplace", &scribo::_background_substraction_inplace)
      .def("_XYCutLayoutExtraction", &scribo::_XYCutLayoutExtraction)
      .def("_WSLineExtraction", &scribo::_WSLineExtraction)
      .def("_EntryExtraction", &scribo::_EntryExtraction)
      .def("_TesseractTextExtraction", &scribo::_TesseractTextExtraction,
           py::arg("input"), py::arg("regions"), py::arg("is_line") = false)
      .def("_set_debug_level", &scribo::set_debug_level)
      .def("_subsample", &scribo::_subsample);

  py::enum_<scribo::DOMCategory>(m, "DOMCategory")
      .value("PAGE", DOMCategory::PAGE)
      .value("TITLE_LEVEL_1", DOMCategory::TITLE_LEVEL_1)
      .value("TITLE_LEVEL_2", DOMCategory::TITLE_LEVEL_2)
      .value("SECTION_LEVEL_1", DOMCategory::SECTION_LEVEL_1)
      .value("SECTION_LEVEL_2", DOMCategory::SECTION_LEVEL_2)
      .value("COLUMN_LEVEL_1", DOMCategory::COLUMN_LEVEL_1)
      .value("COLUMN_LEVEL_2", DOMCategory::COLUMN_LEVEL_2)
      .value("ENTRY", DOMCategory::ENTRY)
      .value("LINE", DOMCategory::LINE)
      .export_values();


  py::enum_<e_force_indent>(m, "ForceIndent")
      .value("FORCE_NONE", e_force_indent::FORCE_NONE)
      .value("FORCE_LEFT", e_force_indent::FORCE_LEFT)
      .value("FORCE_RIGHT", e_force_indent::FORCE_RIGHT)
      .export_values();

  // py::enum_<Application::ExecutionOptions::DebugLevel>(eopts, "DebugLevel")
  //   .value("INFO", Application::ExecutionOptions::DebugLevel::INFO)
  //   .value("DEBUG", Application::ExecutionOptions::DebugLevel::DEBUG)
  //   .export_values();
}
