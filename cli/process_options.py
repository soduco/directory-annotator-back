import json
import logging
import numpy as np
from PIL import Image

from generate_image import generate_image
from back.Application import App, OCREngine
from back import scribo, scribocxx


def process_options(input_buf: np.array, **kwargs) -> np.array:
    '''
    Handle options of the CLI and process the input image buffer accordingly

    arguments:
        input_buf (np.array): The input image to process
        dict_args (dict): The options arguments passed to the CLI
    '''
    output_img_path = kwargs.pop("output_img", None)
    output_json_path = kwargs.pop("output_json", None)
    logging_level = kwargs.pop("log_level", logging.INFO)
    deskew_only = kwargs.pop("deskew_only", False)
    layout_file = kwargs.pop("layout_file", None)

    force_indent_switcher = {
        (True , False): scribocxx.FORCE_LEFT,
        (False, True ): scribocxx.FORCE_RIGHT,
        (False, False): scribocxx.FORCE_NONE,
    }

    app = App(PERO_CONFIG_DIR="../pero_eu_cz_print_newspapers_2020-10-07/",
              logging_level=logging_level)


    if deskew_only:
        deskewed = app.deskew(input_buf)
        data = []
    elif layout_file:
        with open(layout_file, 'r') as f:
            data = json.load(f)
            data = [ scribo.LayoutRegion.from_json(x) for x in data ]
        deskewed = app.deskew(input_buf)
        if not kwargs.get("disable_OCR", False):
            app.process_ocr(deskewed, data, kwargs.get("ocr_engine", OCREngine.PERO))
        if not kwargs.get("disable_NER", False):
            app.process_ner(data)
    else:
        data, deskewed = app.process(input_buf, **kwargs)

    if output_json_path:
        with open(output_json_path, "w", encoding="UTF-8") as f:
            json.dump(data, f, cls=scribo.ScriboJSONEncoder, ensure_ascii=False)

    if output_img_path:
        img = generate_image(data, deskewed)
        output_img = Image.fromarray(img).convert("RGB")
        output_img.save(output_img_path, "JPEG")

    
