#pragma once

#include <scribo.hpp>
#include <span>
#include <ostream>


namespace scribo
{
    void to_json(std::span<const LayoutRegion> regions, std::ostream& os);


    void export_manifest(const char* filename, const cleaning_parameters& cparams);
}