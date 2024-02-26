#include "scribo.hpp"

#include <mln/core/image/ndimage.hpp>
#include <mln/core/algorithm/transform.hpp>
#include <mln/core/algorithm/accumulate.hpp>
#include <mln/core/neighborhood/c4.hpp>
#include <mln/morpho/maxtree.hpp>
#include <mln/morpho/opening.hpp>
#include <mln/morpho/closing.hpp>
#include <mln/morpho/reconstruction.hpp>

#include <mln/core/se/periodic_line2d.hpp>
#include <mln/core/se/rect2d.hpp>
#include <algorithm>
#include <mln/core/image/view/maths.hpp>
#include <mln/accu/accumulators/max.hpp>
#include <mln/data/stretch.hpp>

#include <fmt/core.h>
#include <mln/core/trace.hpp>
#include "signal.hpp"
#include <mln/io/imsave.hpp>
#include <spdlog/spdlog.h>

namespace
{


    struct cc_info_t : mln::Accumulator<cc_info_t>
    {
        using result_type = cc_info_t;
        using argument_type = mln::point2d; 

        cc_info_t() = default;

        void init() noexcept
        {
            *this = cc_info_t{};
        }

        void take(mln::point2d p) noexcept
        {
            count += 1;
            x0 = std::min(x0, (int16_t) p.x());
            x1 = std::max(x1, (int16_t) p.x());
            y0 = std::min(y0, (int16_t) p.y());
            y1 = std::max(y1, (int16_t) p.y());
        }
        void take(cc_info_t& other) noexcept
        {
            count += other.count;
            x0 = std::min(x0, other.x0);
            x1 = std::max(x1, other.x1);
            y0 = std::min(y0, other.y0);
            y1 = std::max(y1, other.y1);
            peak_value = std::max(peak_value, other.peak_value);
        }

        auto to_result() const noexcept { return *this; }
        int width() const {return x1 - x0 + 1; }
        int height() const { return y1 - y0 + 1; }

        int count = 0;
        uint8_t peak_value = 0;
        int16_t x0 = INT16_MAX, x1 = INT16_MIN, y0 = INT16_MAX, y1 = INT16_MIN;
    };

}


namespace scribo
{

    mln::image2d<uint8_t> background_substraction(const mln::image2d<uint8_t>& input, int& xw, int& xh, bool denoise)
    {
        mln_entering("background-substraction");
        constexpr int border = 10;

        if (input.width() != 2048 && input.width() != 2047)
            throw std::runtime_error("Expected an image of width=2048.");



        auto [tree, nodemap] = mln::morpho::maxtree(input, mln::c4);

        auto attr = tree.compute_attribute_on_points(nodemap, cc_info_t{}, false);

        // 1. Compute attribute
        int node_count = (int)tree.parent.size();
        for (int i = node_count - 1; i > 0; --i)
        {
            auto q = tree.parent[i];
            attr[i].peak_value = std::max(attr[i].peak_value, tree.values[i]);
            attr[q].take(attr[i]);
        }

        // 2. Remove all branches to close from the border
        const int w = input.width();
        const int h = input.height();


        // 3. Remove big vertical and horizontal objects
        constexpr int wordsize[2] = {50, 30};
        
        auto se_vline = mln::se::periodic_line2d({0,1}, wordsize[1] * 2);
        auto se_hline = mln::se::periodic_line2d({1,0}, wordsize[0] * 1);
        auto vlines = mln::morpho::opening(input, se_vline, mln::extension::bm::fill(uint8_t(0)));
        auto hlines = mln::morpho::opening(input, se_hline, mln::extension::bm::fill(uint8_t(0)));
        auto all_lines = mln::view::transform(vlines, hlines, [](uint8_t a, uint8_t b) -> uint8_t { return std::max(a, b); });
        
        auto markers = tree.compute_attribute_on_values(nodemap, all_lines, mln::accu::features::max<>{});

        // Create the predicate
        {
            auto pred = [&attr, &values = tree.values, &markers, l = border, t = border, r = w - border, b = h - border ] (int x) {
                bool is_inside =  attr[x].x0 >= l && attr[x].x1 < r && attr[x].y0 >= t && attr[x].y1 < b;
                bool is_not_a_big_line = values[x] > markers[x];
                return is_inside && is_not_a_big_line;
            };
            tree.values[0] = 0;
            tree.filter(mln::morpho::ct_filtering::CT_FILTER_SUBTRACTIVE, nodemap, pred);
        }
        {
            auto pred = [&tree, &attr, t = 20, denoise] (int x) {
                bool r = tree.values[x] >= t;
                if (denoise)
                    r &= attr[x].count >= 100;
                return r;
            };
            tree.filter(mln::morpho::ct_filtering::CT_FILTER_DIRECT, nodemap, pred);
        }

        auto g3 = tree.reconstruct(nodemap);

        //{
        //    std::vector<uint32_t> areas(node_count, 0);
        //    std::ranges::transform(attr, areas.begin(), [](cc_info_t x) { return x.count; });
        //    areas[0] = 0;
        //    auto area = tree.reconstruct_from(nodemap, ::ranges::span{areas});
        //    mln::io::imsave(area, "/tmp/area.tiff");
        //}
        //mln::io::imsave(g3, "tmp-01.tiff");

        // Compute the width/height of letters
        float xheight = xh;
        float xwidth = xw;
        if (xheight <= 0)
        {
            std::vector<float> histo(51, 0);

            auto C = [](int v) {
                return 7 <= v && v <= 50;
            };
            int rootv = tree.values[0];
            for (int i = 1; i < node_count; ++i)
            {
                int q = tree.parent[i];
                int hi = attr[i].height();
                //if (q == 0 && tree.values[q] < tree.values[i])
                //    fmt::print("H:{} VQ:{} V:{}\n", h, tree.values[q], tree.values[i]);
                if (C(hi) && q == 0 && tree.values[i] > rootv)
                    histo[hi] += attr[i].peak_value / 255.f;
            }

            int kMinFontSize = 9;
            std::vector<float> smooth(51);
            smooth[kMinFontSize] = histo[kMinFontSize];
            smooth[50] = histo[50];
            for (int i = kMinFontSize+1; i <= 49; ++i)
                smooth[i] = histo[i-1] * 0.15f + histo[i] * 0.60f + histo[i+1] * 0.15f;

            //for (int i = 7; i <= 50;)
            //{
            //    for (int j = 0; j < 10 && i <= 50; j++, i++)
            //        fmt::print("{:4} ", int(smooth[i]));
            //    fmt::print("\n");
            //}

            auto peaks = find_peaks(smooth);
            if (peaks.empty())
                throw std::runtime_error("Unable to determine the size of the letters.");

            xheight = peaks[0];
            if (peaks.size() >= 2) // Minuscule - Majuscule
            {
                auto [m, M] = std::minmax(peaks[0], peaks[1]);
                spdlog::debug("peak-0={} peak-1={}", m, M);
                float r = M / float(m);
                if (std::abs(r - 1.6f) < 0.3f)
                {
                    xheight = m;
                    spdlog::debug("xheight={} X-height={}", m, M);
                }
            }
        }
        if (xwidth <= 0)
            xwidth = 0.75f * xheight;

        auto word_se = mln::se::rect2d(xwidth*3, xheight);
        auto d1 = mln::morpho::closing(g3, word_se, mln::extension::bm::fill(uint8_t(0)));
    
        auto word_se_vline = mln::se::periodic_line2d({0,1}, std::round(2 * xheight / 2.f));
        auto word_se_hline = mln::se::periodic_line2d({1,0}, std::round(3 * xwidth / 2.f));
        auto d3_1 = mln::morpho::opening(d1, word_se_hline);
        auto d3_2 = mln::morpho::opening(d1, word_se_vline);
        auto d3 = mln::transform(d3_1, d3_2, [](auto a, auto b) { return std::max(a,b); });



        auto mask = mln::morpho::opening_by_reconstruction(d1, d3, mln::c4);

        float gthreshold = 0.2f * mln::accumulate(mask, mln::accu::features::max<>{});
        auto r = mln::se::rect2d(127,127);
        auto local_max = mln::morpho::dilation(mask, r, mln::extension::bm::fill(uint8_t(0)));
        //auto lthreshold = 0.5f * lmax; // Local threshold for show-thru removal (0.5 * (max - min))


        auto M = tree.compute_attribute_on_values(nodemap, mask, mln::accu::features::max<>{});
        auto tNode = tree.compute_attribute_on_values(nodemap, local_max, mln::accu::features::max<>{});

        {
            auto pred = [&M, &tNode, gthreshold](int x) {
                return M[x] > gthreshold && M[x] > (tNode[x] / 2); // Local threshold for show-thru removal (0.5 * (max - min))
            };
            tree.values[0] = 0;
            tree.filter(mln::morpho::CT_FILTER_SUBTRACTIVE, nodemap, pred);
        }
     

        auto out = tree.reconstruct(nodemap);

        xh = xheight;
        xw = xwidth;
        return out;
    }


}
