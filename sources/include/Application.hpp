#pragma once

#include <mln/core/image/ndimage_fwd.hpp>
#include <mln/core/colors.hpp>

#include "DOMTypes.hpp"
#include "display_options.hpp"


struct ApplicationData;


class Application
{
public:
  struct ExecutionOptions
  {
    enum DebugLevel {
      INFO = 0,
      DEBUG,
    };

    bool           deskew_only  = false;
    bool           skip_ocr     = false;
    int            font_size    = -1;
    e_force_indent force_indent = e_force_indent::FORCE_NONE;
    DebugLevel     debug_level  = DebugLevel::INFO;
  };


  Application(mln::image2d<uint8_t> input_image,
              const display_options_t &display_opts,
              const ExecutionOptions &eopts);
  ~Application();


  // Get the input page as a 8-bits graylevel image
  mln::image2d<uint8_t> GetInputImage();

  // Get the output page as a 8-bits graylevel image
  mln::image2d<mln::rgb8> GetOutputImage();

  // Get the deskewed input image as 8-bits graylevel image
  mln::image2d<uint8_t> GetDeskewedImage() const;

  // Return the root document object as a json
  DOMElement*           GetDocument();


private:
  std::unique_ptr<ApplicationData> m_app_data;
  std::unique_ptr<DOMElement>      m_document;
  const display_options_t         &m_display_opts;
};
