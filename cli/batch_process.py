
import argparse
import numpy as np
import pathlib
import logging
import re

import progressbar
from PIL import Image


import sys
sys.path.insert(1, '../')

from process_options import process_options
from pdf_opener import get_pdf_page




desc = '''
Command line interface for document extraction.

The tool extracts the content of some pages and export the information into a json file.

Ex:
python batch_process.py Didot1851a.pdf -p 424-600 -o out.json

Will create out-0424.json, out-0425.json...
'''


class PageParserAction(argparse.Action):

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.re = re.compile(r"(\d+-\d+|\d+)(,(\d+-\d+|\d+))*") 

    def __call__(self, parser, namespace, values, option_string):
        if not self.re.fullmatch(values):
            msg = "Invalid pages: {}".format(values)
            raise argparse.ArgumentTypeError(msg)

        rngs = values.split(",")
        pages = set()
        for r in rngs:
            r = r.split("-")
            a = int(r[0])
            b = 1 + (a if len(r) == 1 else int(r[1]))
            pages.update(range(a,b))
        setattr(namespace, self.dest, pages)

def __checkSuffix(string):
    path = pathlib.Path(string)
    suffix = path.suffix
    if suffix != '.json' and suffix != '.zip':
        msg = 'invalid output file suffix: expected ".json" or ".zip"' 
        raise argparse.ArgumentTypeError(msg)
    
    return string


parser = argparse.ArgumentParser(description=desc)
parser.add_argument('file', help='Path to the pdf')
parser.add_argument('-p', required=True, action=PageParserAction, dest="pages",
                    help='Comma separated list of pages with page range support (ex 1,3,8-100)')
parser.add_argument('-o',
    required=True,
    help='Path to output jsons files pattern ({} will be replaced by the page number)',
    dest="outfile_pattern",
    type=__checkSuffix)

# FIXME: factorize as a subparser
parser.add_argument(
        "--layout-file-pattern",
        required=False,
        help="Provide entries from a JSON layout file instead of running the layout extraction. '{}' will be replaced by the page number."
        )

args = parser.parse_args()
vargs = vars(args)
input_file = vargs.pop("file")
output_pattern = vargs.pop("outfile_pattern")
layout_file_pattern = vargs.pop("layout_file_pattern", None)


def run(page):
    try:
        # Call C++ function and process
        img_bytes = get_pdf_page(input_file, page - 1) # 0 - based index
        with Image.open(img_bytes, "r") as img:
            img = img.convert("L")
            img = np.array(img)

        vargs["output_json"] = output_pattern.format(page)
        vargs["log_level"] = logging.WARNING
        if layout_file_pattern:
            vargs["layout_file"] = layout_file_pattern.format(page)
        process_options(img,**vargs)
    except:
        logging.exception("An error occured when processing the page %d from %s.", page, input_file)
        return

def doExecutor():
    #with ThreadPoolExecutor(max_workers=1) as executor:
    #    executor.map(do, args.pages)
    pages = vargs.pop("pages")
    for p in progressbar.progressbar(pages, redirect_stdout=True):
        run(p)

doExecutor()
