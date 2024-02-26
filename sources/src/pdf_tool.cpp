#include <mln/core/image/ndimage.hpp>
#include "poppler-document.h"
#include "poppler-page.h"
#include "poppler-page-renderer.h"
#include <spdlog/spdlog.h>

#include "pdf_tool.hpp"


pdf_file::pdf_file(const char* filename)
{
    m_pdf.reset(poppler::document::load_from_file(filename));
}

pdf_file::~pdf_file()
{
}

poppler::document* pdf_file::get() const
{
    return m_pdf.get();
}



mln::image2d<uint8_t> load_from_pdf(const pdf_file& pdf, int page)
{
    const int target_width = 2048;

    poppler::document* doc = pdf.get();

    if (page >= doc->pages())
        throw std::runtime_error(fmt::format("Invalid page number {}.", page));
    

    auto p = std::unique_ptr<poppler::page>(doc->create_page(page - 1)); // 1-offset
    auto dims = p->page_rect();
    float res = 72.0 * target_width / dims.width();


    poppler::page_renderer r;
    r.set_image_format(poppler::image::format_gray8);
    r.set_render_hint(poppler::page_renderer::antialiasing);
    
    auto image = r.render_page(p.get(), res, res);
    if (!image.is_valid())
        throw std::runtime_error("Unable to render the input page.");


    int sizes[] = {image.width(), image.height()};
    spdlog::info("Renderered page {} at resolution {}x{}.\n", page, image.width(), image.height());
    std::ptrdiff_t strides[] = {sizeof(uint8_t), image.bytes_per_row()};  
    auto input =  mln::image2d<uint8_t>::from_buffer((uint8_t*)image.const_data(), sizes, strides, true);

    return input;
}
    

