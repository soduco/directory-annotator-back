#include <scribo.hpp>

#include <mln/core/image/ndimage.hpp>
#include <mln/morpho/opening.hpp>
#include <mln/morpho/closing.hpp>
#include <mln/morpho/reconstruction.hpp>
#include <mln/morpho/maxtree.hpp>
#include <mln/core/neighborhood/c4.hpp>
#include <mln/core/neighborhood/c8.hpp>
#include <mln/core/se/disc.hpp>
#include <mln/core/se/rect2d.hpp>
#include <mln/core/extension/padding.hpp>

#include <mln/core/algorithm/transform.hpp>
#include <mln/core/algorithm/for_each.hpp>
#include <mln/io/imsave.hpp>
#include <fmt/core.h>

#include <climits>

namespace
{


  class Diameter : public mln::Accumulator<Diameter>
  {
  public:
    using result_type = int;
    using argument_type = mln::point2d;

    void init() { *this = Diameter(); }

    void take(mln::point2d p)
    {
      x1 = std::min(x1, p.x());
      y1 = std::min(y1, p.y());
      x2 = std::max(x2, p.x());
      y2 = std::max(y2, p.y());
    }

    void take(const Diameter& o)
    {
      x1 = std::min(x1, o.x1);
      y1 = std::min(y1, o.y1);
      x2 = std::max(x2, o.x2);
      y2 = std::max(y2, o.y2);
    }

    int to_result() const
    {
      int a = x2 - x1;
      int b = y2 - y1;
      //return a * a + b * b;
      return 2 * std::max(a * a, b * b);
    }

  private:
    int x1 = INT_MAX;
    int x2 = INT_MIN;
    int y1 = INT_MAX;
    int y2 = INT_MIN;
  };

  void background_substraction(mln::image2d<uint8_t> input,          //
                               int                   kMinDiameter,   //
                               int                   kMinWidth,      //
                               int                   kMinHeight,     //
                               int                   kMinGrayLevel,  //
                               int                   kOpeningRadius, //
                               const char*           debug_prefix)
  {
    const int kMinDiameterSqr = kMinDiameter * kMinDiameter;

    auto negate = [](uint8_t& x) { x = UINT8_MAX - x; };
    mln::for_each(input, negate);

    mln::image2d<uint8_t> diff;
    {
      auto [mt, nodemap] = mln::morpho::maxtree(input, mln::c8);
      auto diameter      = mt.compute_attribute_on_points(nodemap, Diameter{});
      mt.filter(mln::morpho::CT_FILTER_DIRECT, nodemap,
                [&diameter, kMinDiameterSqr](auto nodeid) { return diameter[nodeid] > kMinDiameterSqr; });

      auto b = mt.reconstruct(nodemap);
      mln::transform(input, b, b, std::minus<uint8_t>());
      diff = std::move(b);
    }
    auto mask = mln::morpho::closing(diff, mln::se::disc(kOpeningRadius));
    {
      int borders[2][2] = {{1, 1}, {1, 1}};
      mln::pad(mask, mln::PAD_ZERO, borders);
    }
    auto seeds = mln::morpho::opening(mask, mln::se::rect2d{kMinWidth, kMinHeight});
    auto clo   = mln::morpho::opening_by_reconstruction(mask, seeds, mln::c8);

    int maxv = 0;
    mln::for_each(clo, [t = UINT8_MAX - kMinGrayLevel, &maxv](uint8_t& x) {
      int tmp = (x < t) ? 0 : x;
      maxv    = std::max(maxv, tmp);
      x       = tmp;
    });

    if (debug_prefix)
    {
      mln::io::imsave(input, fmt::format("{}-01-input.tiff", debug_prefix));
      mln::io::imsave(diff, fmt::format("{}-02-diff.tiff", debug_prefix));
      mln::io::imsave(mask, fmt::format("{}-03-mask.tiff", debug_prefix));
      mln::io::imsave(seeds, fmt::format("{}-04-seeds.tiff", debug_prefix));
      mln::io::imsave(clo, fmt::format("{}-05-clo.tiff", debug_prefix));
    }

    // Negate + scaling
    mln::transform(clo, diff, input, [s = UINT8_MAX / float(maxv)](uint8_t a, uint8_t b) {
      int x  = std::min(a, b);
      int sx = int(x * s + 0.5f);
      return UINT8_MAX - sx;
    });

    if (debug_prefix)
      mln::io::imsave(input, fmt::format("{}-06-bg-removal.tiff", debug_prefix));
  }

} // namespace

namespace scribo
{

  void background_substraction_inplace(mln::image2d<uint8_t>& input,
                                       float                  kMinDiameter,   //
                                       float                  kMinWidth,      //
                                       float                  kMinHeight,     //
                                       float                  kMinGrayLevel,  //
                                       float                  kOpeningRadius, //
                                       std::string_view       debug_prefix)
  {
    return ::background_substraction(input,
                                   (int)(0.5f + kMinDiameter),   //
                                   (int)(0.5f + kMinWidth),      //
                                   (int)(0.5f + kMinHeight),     //
                                   (int)(0.5f + kMinGrayLevel),  //
                                   (int)(0.5f + kOpeningRadius), //
                                   debug_prefix.empty() ? nullptr : debug_prefix.data());
  }
}
