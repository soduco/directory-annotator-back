#pragma once

#include "CoreTypes.hpp"
#include "DOMTypes.hpp"
#include <mln/core/colors.hpp>
#include <mln/core/image/ndimage_fwd.hpp>

#include <span>
#include <string>


namespace scribo
{
  using ::DOMCategory;

  /// \brief This function is not thread-safe
  void set_debug_level(int debug_level) noexcept;
  int  get_debug_level() noexcept;


  /// \brief Get the segments from an image
  auto extract_segments(const mln::image2d<uint8_t>& input) -> std::vector<Segment>;

  // Deskew facilities
  /// \{
  /// \brief Get the deskew angle from a list of segments and deskew if the document
  /// \param input
  /// \param segment
  /// \param[optional] out
  mln::image2d<uint8_t> deskew_image(const mln::image2d<uint8_t> &input, float angle, uint8_t fill_value = 255);
  std::vector<Segment>& deskew_segments(std::vector<Segment>& segments, float angle);
  float                 deskew_estimation(const std::vector<Segment>& segments, int image_width, float angle_tolerance);
  float                 skew_estimation(const mln::image2d<uint8_t>& input, int xwidth, int xheight);
  /// \}

  

  // Document cleaning
  // \{
  
  /// 


  /**
   * @brief Perform the document cleaning (denoising, deskew...) and estimates the global font-size of the document.
   * 
   * @param input The input image 
   * @param xw the x-width (estimated if <= 0)
   * @param xh the x-height (estimated if <= 0)
   * @param denoise True if denoising must be performed (tends to destroy the data, be careful)
   * @return mln::image2d<uint8_t> 
   */
  mln::image2d<uint8_t> background_substraction(const mln::image2d<uint8_t>& input, int& xw, int& xh, bool denoise = false);

  struct cleaning_parameters
  {
    enum Mode { AUTO = -1, NO = 0, YES = 1};

    float deskew_angle;
    int xwidth = -1;
    int xheight = -1;
    int denoise = AUTO;
    int resize = AUTO;  // Automatic resize to a 2048px wide image if necessary
  };

  mln::image2d<uint8_t> clean_document(const mln::image2d<uint8_t>& input, cleaning_parameters& params,
                                       mln::image2d<uint8_t>* deskewed = nullptr);

  // \}


  /// \brief Inplace backrgound suppression
  ///
  /// :param kMinDiameter: Diameter to remove the letters (default = ~5em)
  /// :param kMinWidth: Min width for word (default = ~4em)
  /// :param kMinHeight: Min height for word (default = 1\lineheight)
  /// :param kMinGrayLevel: Gray level threshold to consider black/blank
  /// :param kOpeningRadius: Radius for disc opening used to given word matter (default = ~1.\lineheight)
  void background_substraction_inplace(mln::image2d<uint8_t>& input,
                                       float                  kMinDiameter,   //
                                       float                  kMinWidth,     //
                                       float                  kMinHeight,     //
                                       float                  kMinGrayLevel,  //
                                       float                  kOpeningRadius, //
                                       std::string_view       debug_prefix);

  /// Layout extraction facilities
  /// \{
  struct LayoutRegion
  {
    Box         bbox;
    DOMCategory type;
    int         parent_id = -1;
    int         mask_instance_id = -1;

    int x() const { return bbox.x; }
    int y() const { return bbox.y; }
    int width() const { return bbox.width; }
    int height() const { return bbox.height; }
    void set_x(int v) { bbox.x = v; }
    void set_y(int v) { bbox.y = v; }
    void set_width(int v) { bbox.width = v; }
    void set_height(int v) { bbox.height = v; }
  };


  /// \brief XY Layout Cut
  auto XYCutLayoutExtraction(const mln::image2d<uint8_t>& input, std::span<Segment> segments, KConfig config) -> std::vector<LayoutRegion>;

  /// \brief Line extraction based on watershed algorithm
  auto WSLineExtraction(const mln::image2d<uint8_t>& input, std::span<Box> regions, std::string_view debug_path, KConfig config, std::vector<LayoutRegion>* bboxes = nullptr) -> mln::image2d<int16_t>;

  /// \brief
  /// \param region The text block
  /// \param lines The lines in the text block
  /// \return A vector that maps (line number) -> 1 if it starts an entry, 0 otherwise
  void EntryExtraction(Box region, std::span<LayoutRegion> lines, std::vector<LayoutRegion>& out);

  /// \brief
  auto TesseractTextExtraction(const mln::image2d<uint8_t>& input, std::span<Box> regions,
                               bool is_line = false) -> std::vector<std::string>;

} // namespace scribo
