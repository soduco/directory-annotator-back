#include "scribo.hpp"

#include <mln/core/image/ndimage.hpp>
#include <mln/core/algorithm/for_each.hpp>
#include <mln/data/stretch.hpp>
#include "subsample.hpp"

namespace
{

    static bool isbw(const mln::image2d<uint8_t>& input)
    {
      int b = 0;
      int w = 0;
      mln::for_each(input, [&b,&w](uint8_t x) {
        if (x == 0) b++;
        else if (x == UINT8_MAX) w++;
      });

      int n = input.width() * input.height();
      float r = ((b + w) / float(n));
      return  r > 0.75f;
    }

}

namespace scribo
{
     mln::image2d<uint8_t> clean_document(const mln::image2d<uint8_t>& input_, cleaning_parameters& params,
                                        mln::image2d<uint8_t>* deskewed)
    {
        auto input = input_;
        auto inverse = [](uint8_t& x) { x = 255 - x;};
        mln::for_each(input, inverse);

        if (params.denoise == cleaning_parameters::AUTO)
            params.denoise = isbw(input);

        if (params.resize == cleaning_parameters::AUTO)
            params.resize = std::abs(input.width() - 2048) > 10;

        if (params.resize == cleaning_parameters::YES) {
            input = ::resize(input, 2048.0f / input.width());
        }

        auto clean = scribo::background_substraction(input, params.xwidth, params.xheight, params.denoise);
        mln::data::stretch_to(clean, clean);


        params.deskew_angle = scribo::skew_estimation(clean, params.xwidth, params.xheight);

        if (deskewed) {
            auto tmp = scribo::deskew_image(input, params.deskew_angle, 0);
            mln::for_each(tmp, inverse);
            *deskewed = std::move(tmp);
        }

    
        clean = scribo::deskew_image(clean, params.deskew_angle, 0);
        mln::for_each(clean, inverse);

        return clean;
    }

}