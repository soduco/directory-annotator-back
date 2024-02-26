#pragma once
#include <mln/core/image/ndimage_fwd.hpp>
#include <cstdint>

namespace poppler
{
    class document;
}

class pdf_file
{
    std::unique_ptr<poppler::document> m_pdf;
    public:
        pdf_file(const char* filename);
        ~pdf_file();
        poppler::document* get() const;
};

mln::image2d<uint8_t> load_from_pdf(const pdf_file& pdf, int page);