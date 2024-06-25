import argparse
import numpy as np
from PIL import Image

import sys
sys.path.insert(1, '../')

from process_options import process_options
from pdf_opener import get_pdf_page

levels = ('DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL')


def _get_parser():
    parser = argparse.ArgumentParser(
        prog="python soduco_cli.py",
        description="Soduco CLI"
        )

    parser.add_argument(
        "input",
        help="Path to the pdf",
        )
    parser.add_argument(
        "output_img",
        help="Path to the output image (JPG)",
        )

    parser.add_argument(
        "--page",
        "-p",
        type=int,
        required=True,
        help="Page to demat",
        )

    parser.add_argument(
        "--output-json",
        "-o",
        required=False,
        help="Path to the output json file",
        )

    parser.add_argument('--log-level',
        default='INFO',
        choices=levels,
        help="Logging level (if DEBUG, it also exports debug images)",
        )

    parser.add_argument(
        "--deskew-only",
        action="store_true",
        required=False,
        help="Only perform the deskew",
        )

    parser.add_argument(
        "--font-size",
        default=-1,
        required=False,
        type=int,
        help="Set document body font size in px (default=-1 : auto)",
    )

    parser.add_argument(
        "--layout-file",
        required=False,
        help="Provide entries from a JSON layout file instead of running the layout extraction"
        )

    '''
    parser.add_argument(
        "--show-ws",
        action="store_true",
        required=False,
        help="Show/Hide the watershed lines  (Obsolete for now)",
    )   
    parser.add_argument(
        "--show-grid",
        action="store_true",
        required=False,
        help="Show/Hide the grid  (Obsolete for now)",
        )
    parser.add_argument(
        "--show-layout",
        action="store_true",
        required=False,
        help="Show/Hide the layout lines  (Obsolete for now)",
        )
    parser.add_argument(
        "--show-segments",
        action="store_true",
        required=False,
        help="Show/Hide the LSD segments  (Obsolete for now)",
        )
    parser.add_argument(
        "--color-lines",
        default="entry",
        required=False,
        metavar="value",
        choices=["no", "entry", "indent", "eol", "number", "0", "1", "2", "3", "4"],
        help="Colorize lines (ENUM:value in {no->0,entry->1,indent->2,eol->3,number->4} "\
        "OR {0,1,2,3,4})  (Obsolete for now)",
    )

    group = parser.add_mutually_exclusive_group()
    group.add_argument(
        "--force-left", 
        action="store_true",
        required=False,
        help="Force left indent",
        )
    group.add_argument(
        "--force-right",
        action="store_true",
        required=False,
        help="Force right indent",
        )
    '''


    return parser

def _to_grey(img):
    return img.convert("L")

if __name__ == "__main__":
    parser = _get_parser()

    # Parse arguments
    args = parser.parse_args()
    vargs = vars(args)

    # Call C++ function and process
    page = vargs.pop("page")
    img_bytes = get_pdf_page(vargs.pop("input"), page - 1) # Page index are 0-base
    with Image.open(img_bytes, "r") as img:
        input_buf = np.array(_to_grey(img))


    process_options(input_buf, **vargs)


