#include "detect_separators.hpp"

#include <mln/core/algorithm/transform.hpp>
#include <mln/core/image/ndimage.hpp>
#include <scribo/segdet.hpp>

extern "C"
{
#include <lsd.h>
}

#include <spdlog/spdlog.h>

#include <cmath>

namespace
{
  // Configuration
  constexpr int kValidRegionBorder = 100;
  constexpr int kMinLength = 100;
}


std::vector<Segment> detect_separators_LSD(const mln::image2d<uint8_t>& input)
{
  // Convert to double for LSD with no-border
  mln::image_build_params params;
  params.border = 0;

  mln::image2d<double> f(input.domain(), params);
  mln::transform(input, f, [](uint8_t x) { return static_cast<double>(x); });


  // Run LSD
  int n_segment = 0;
  double* results;
  {
    int*    tmp = nullptr;
    int     x, y;
    double* buf         = f.buffer();
    double  scale       = 0.5;
    double  sigma_scale = 0.6; /* Sigma for Gaussian filter is computed as
                               sigma = sigma_scale/scale.                    */
    double quant = 2.0;        /* Bound to the quantization error on the
                                   gradient norm.                                */
    double ang_th     = 22.5;  /* Gradient angle tolerance in degrees.           */
    double log_eps    = 2.0;   /* Detection threshold: -log10(NFA) > log_eps     */
    double density_th = 0.7;   /* Minimal density of region points in rectangle. */
    int    n_bins     = 1024;  /* Number of bins in pseudo-ordering of gradient
                                  modulus.                                       */

    results = LineSegmentDetection(&n_segment, buf, f.width(), f.height(), scale, sigma_scale, quant, ang_th, log_eps, density_th,
                                   n_bins, nullptr, nullptr, nullptr);
    //auto tmpi = mln::image2d<int>::from_buffer((void*)tmp, mln::box2d{{0, 0}, {y, x}});
    //mln::io::imsave(tmpi, "tmp.tiff");
  }

  // Convert back the segments
  std::vector<Segment> segments;
  double* res = results;
  for (int i = 0; i < n_segment; ++i)
  {
    Segment s;
    s.start = {(int)res[0], (int)res[1]};
    s.end   = {(int)res[2], (short)res[3]};
    if (s.end.y < s.start.y)
      std::swap(s.start, s.end);

    s.length = std::hypot(res[2] - res[0], res[3] - res[1]);
    s.angle  = std::atan2(res[3] - res[1], res[2] - res[0]) * 180 / M_PI;
    if (s.angle < 0)
      s.angle += 180.;
    s.width = res[4];
    s.nfa   = res[6];


    segments.push_back(s);
    res += 7;
  }
  free(results);


  // Filter segments
  auto end          = segments.end();
  auto valid_region = f.domain();
  valid_region.inflate(-kValidRegionBorder);
  end = std::remove_if(segments.begin(), end, [valid_region](const auto& s) {
    bool valid = true;
    valid &= valid_region.has(mln::point2d{s.start.x, s.start.y});
    valid &= valid_region.has(mln::point2d{s.end.x, s.end.y});
    valid &= s.length >= kMinLength;
    return !valid;
  });

  segments.resize(end - segments.begin());

  // Debug
  spdlog::debug("LSD - Detected segments");
  for (const auto& s : segments)
  {
    spdlog::debug("x1={} y1={} x2={} y2={} width={} nfa={} length={} angle={}", s.start.x, s.start.y, s.end.x,
                  s.end.y, s.width, s.nfa, s.length, s.angle);
  }

  return segments;
}


std::vector<Segment> detect_separators_Kalman(const mln::image2d<uint8_t>& input, mln::image2d<uint8_t>* binseg)
{

  auto segs = scribo::detect_line_vector(input, kMinLength);

}




namespace scribo
{

  auto extract_segments(const mln::image2d<uint8_t>& input) -> std::vector<Segment>
  {
    return detect_separators_LSD(input);
  }

}

