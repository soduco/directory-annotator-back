#include <algorithm>
#include <fmt/core.h>
#include <mln/core/algorithm/transform.hpp>
#include <mln/core/image/ndimage.hpp>
#include <mln/core/se/periodic_line2d.hpp>
#include <mln/io/imsave.hpp>
#include <mln/morpho/closing.hpp>
#include <mln/morpho/opening.hpp>
#include <mln/transforms/hough_lines.hpp>

namespace
{

  mln::image2d<uint8_t> grad(mln::image2d<uint8_t> in, mln::point2d offset)
  {
    int h = in.height();
    int w = in.width();

    mln::image2d<uint8_t> out(w, h, mln::image_build_params{.init_value = uint8_t(0)});

    std::ptrdiff_t boffset = in.delta_index(offset);

    for (int y = 3; y < (h - 3); ++y)
    {
      const uint8_t* ptr = &in.at({0, y});
      uint8_t*       c   = &out.at({0, y});
      const uint8_t* a   = ptr - boffset;
      const uint8_t* b   = ptr + boffset;

      for (int x = 3; x < (w - 3); ++x)
        c[x] = std::clamp(b[x] - a[x], 0, 255);
    }
    return out;
  }

  std::pair<mln::image2d<uint8_t>, mln::image2d<uint8_t>> compute_gradients(mln::image2d<uint8_t> input, int xwidth,
                                                                            int xheight)
  {
    auto word_se_vline = mln::se::periodic_line2d({0, 1}, 2 * xheight);
    auto word_se_hline = mln::se::periodic_line2d({1, 0}, 3 * xwidth);

    auto m1 = mln::morpho::closing(input, word_se_hline, mln::extension::bm::fill(uint8_t{0}));
    m1      = mln::morpho::opening(m1, word_se_hline, mln::extension::bm::fill(uint8_t{0}));

    auto m2 = mln::morpho::closing(m1, word_se_vline, mln::extension::bm::fill(uint8_t{0}));
    m2      = mln::morpho::opening(m2, word_se_vline, mln::extension::bm::fill(uint8_t{0}));


    auto g1 = grad(m1, {3, 0});
    auto g2 = grad(m2, {0, 3});

    return {g1, g2};
  }

  float skew_estimate(const mln::image2d<uint8_t>& ima, std::span<float> angles)
  {
    // rough estimation
    auto g1f = mln::transform(ima, [](int x) { return x / 255.f; });

    float angle;

    {
      auto vote  = mln::transforms::hough_lines(g1f, angles);
      auto peaks = mln::transforms::hough_lines_detect_peak_angles(vote, angles, 0.3f, 50, 3);
      // for (auto [angle, count] : peaks)
      //     fmt::print("a={} c={}\n", rad2deg(angle), count);

      angle = peaks[0].angle;
    }
    std::vector<float> refine_angle = {-0.01745329, -0.01396263, -0.01047198, -0.00698132, -0.00349066, 0.,
                                       0.00349066,  0.00698132,  0.01047198,  0.01396263,  0.01745329};
    std::ranges::for_each(refine_angle, [y = angle](float& x) { x += y; });

    {
      auto vote  = mln::transforms::hough_lines(g1f, refine_angle);
      auto peaks = mln::transforms::hough_lines_detect_peak_angles(vote, refine_angle, 0.3f, 50, 3);
      // for (auto [angle, count] : peaks)
      //     fmt::print("a2={} c2={}\n", rad2deg(angle), count);
      angle = peaks[0].angle;
    }
    // fmt::print("detect angle={}\n", rad2deg(angle));
    return angle;
  }
} // namespace

namespace scribo
{
  float skew_estimation(const mln::image2d<uint8_t>& input, int xwidth, int xheight)
  {
    auto [g1, g2] = compute_gradients(input, xwidth, xheight);


    // mln::io::imsave(g2, "g2.tiff");

    float v_angle;

    {
      // Angles between 85 and 95 degree
      float angles[] = {1.48352986, 1.49225651, 1.50098316, 1.5097098,  1.51843645, //
                        1.5271631,  1.53588974, 1.54461639, 1.55334303, 1.56206968, //
                        1.57079633, 1.57952297, 1.58824962, 1.59697627, 1.60570291, //
                        1.61442956, 1.6231562,  1.63188285, 1.6406095,  1.64933614, //
                        1.65806279};

      v_angle = skew_estimate(g1, angles);
    }

    /* (Horizontal estimation - not used)
    float h_angle;
    {
      // Angles between -5 and 5 degree
      float angles[] = {0.08726646,  -0.07853982, -0.06981317, -0.06108652, -0.05235988, //
                        -0.04363323, -0.03490659, -0.02617994, -0.01745329, -0.00872665, //
                        0.,          0.00872665,  0.01745329,  0.02617994,  0.03490659,  //
                        0.04363323,  0.05235988,  0.06108652,  0.06981317,  0.07853982,  //
                        0.08726646};

      h_angle = skew_estimate(g2, angles);
    }
    */

    return (M_PI - v_angle) * 180 / M_PI;
  }
} // namespace scribo