#include <nlohmann/json.hpp>
#include <fstream>

#include "scribo.hpp"

using json = nlohmann::json;

namespace scribo
{
  void to_json(std::span<const LayoutRegion> regions, std::ostream& os)
  {
    static const std::string enum2str[] = {
        "PAGE",            //
        "TITLE_LEVEL_1",   //
        "TITLE_LEVEL_2",   //
        "SECTION_LEVEL_1", //
        "SECTION_LEVEL_2", //
        "COLUMN_LEVEL_1",  //
        "COLUMN_LEVEL_2",  //
        "ENTRY",           //
        "LINE",            //
    };


    auto root = json::array();
    for (std::size_t i = 0; i < regions.size(); ++i)
    {
      auto&& e = regions[i];

      auto b       = e.bbox;
      auto element = json::object({
          {"id", i},                                          //
          {"parent", e.parent_id},                            //
          {"type", enum2str[int(e.type)]},                    //
          {"box", json::array({b.x, b.y, b.width, b.height})} //
      });
      root.push_back(std::move(element));
    }
    os << std::setw(4) << root << std::endl;
  }

  void export_manifest(const char* filename, const cleaning_parameters& cparams)
  {
    auto root = json::object({
      {"angle", cparams.deskew_angle},
      {"x-width", cparams.xwidth},
      {"x-height", cparams.xheight},
      {"denoising", cparams.denoise}
    });
    std::ofstream o(filename);
    o << std::setw(4) << root << std::endl;
  }

} // namespace scribo
