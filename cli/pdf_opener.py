from io import BytesIO
from pikepdf import Pdf, PdfImage


class DocumentReadError(RuntimeError):
    pass

class InvalidViewIndexError(RuntimeError):
    pass


def get_pdf_page(path: str, view: int) -> BytesIO:
    """
    Open a pdf at `path` and return its `page`

    The `view` is a 0-based index
    """
    image_data = None
    try:
        pdf_file = Pdf.open(path)
        num_pages = len(pdf_file.pages)
        print(f"Reading PDF file {path} with {num_pages} pages.")
        if not 0 <= view < num_pages:
            raise InvalidViewIndexError()
        page = pdf_file.pages[view]

        for image in page.images:
            pdf_image = PdfImage(page.images[image])
            pil_image = pdf_image.as_pil_image()

            image_data = BytesIO()
            pil_image.save(image_data, format='PNG')
            break
    except InvalidViewIndexError:
        raise
    except RuntimeError:
        raise DocumentReadError()

    return image_data
