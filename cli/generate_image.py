#!/usr/bin/env python3

from typing import Tuple
from matplotlib import pyplot
import numpy as np

from back import scribo
from back.scribocxx import DOMCategory

NCOLORS = 8


def __to_rgba_dims(image: np.array) -> np.array: # TODO: refacto in a more vectorize way
    image = np.array([
        image,
        image,
        image,
        np.full(image.shape, 255, dtype="uint8")
    ])

    return np.moveaxis(image, 0, -1)

def __get_coords(elt: scribo.LayoutRegion, h: int, w: int) -> Tuple[Tuple[int, int], Tuple[int, int]]:
    x0, y0, width, height = (elt.bbox.x, elt.bbox.y, elt.bbox.width, elt.bbox.height)
    x0 = int(np.clip(x0, 0, w))
    x1 = int(np.clip(x0 + width, 0, w))
    y0 = int(np.clip(y0, 0, h))
    y1 = int(np.clip(y0 + height, 0, h))

    return (x0, y0), (x1, y1)

def __get_letter_mask(image: np.array, elements: list) -> np.array:
    h, w, _ = image.shape

    mask = np.zeros((h, w), dtype="uint8")
    for idx, elt in enumerate(elements):
        (x0, y0), (x1, y1) = __get_coords(elt, h, w)
        mask[y0:y1, x0:x1] = idx % NCOLORS + 1

    background = image[:, :, 0] >= 150
    mask[background] = 0

    return mask

def __get_entry_mask(doc: list, image: np.array) -> np.array:
    entry_boxes = list(filter(lambda elt : elt.type == DOMCategory.ENTRY, doc))
    return __get_letter_mask(image, entry_boxes)

def __merge_masks(mask1: np.array, mask2: np.array) -> np.array:
    return np.where(mask1 > 0, mask1, mask2)

def __get_box_mask(image: np.array, elements: list) -> np.array:
    LINE_WIDTH = 4
    h, w, _ = image.shape

    mask = np.zeros((h, w), dtype="uint8")
    for idx, elt in enumerate(elements):
        (x0, y0), (x1, y1) = __get_coords(elt, h, w)

        tmp_mask = np.zeros((h, w), dtype="uint8")
        tmp_mask[y0:y1, x0:x1] = idx % NCOLORS + 1
        tmp_mask[y0 + LINE_WIDTH:y1 - LINE_WIDTH, x0 + LINE_WIDTH:x1 - LINE_WIDTH] = 0

        mask = __merge_masks(tmp_mask, mask)

    return mask

def __get_non_entry_box_mask(doc: list, image: np.array) -> np.array:
    non_entry = [
        DOMCategory.PAGE,
        DOMCategory.TITLE_LEVEL_1,
        DOMCategory.TITLE_LEVEL_2,
        DOMCategory.COLUMN_LEVEL_1
    ]
    non_entry_boxes = list(filter(lambda elt : elt.type in non_entry, doc))

    return __get_box_mask(image, non_entry_boxes)

def __get_background_mask(image: np.array, elements: list) -> np.array:
    h, w, _ = image.shape

    mask = np.full((h, w), False, dtype="uint8")
    for elt in elements:
        (x0, y0), (x1, y1) = __get_coords(elt, h, w)
        mask[y0:y1, x0:x1] = True

    return mask

def __get_column_binary_mask(doc: list, image: np.array) -> np.array:
    column_boxes = list(filter(lambda elt : elt.type == DOMCategory.COLUMN_LEVEL_1, doc))
    return __get_background_mask(image, column_boxes)

def __color_image_with_mask_light_red(image: np.array, mask: np.array) -> np.array:
    red_image = np.copy(image)
    red_image[:, :, 0] = 255

    return np.where(mask[:, :, np.newaxis], red_image, image)

def __generate_random_LUT() -> np.array:
    cmap = pyplot.get_cmap(name="Dark2", lut=NCOLORS)
    colors = cmap.colors
    colors = np.insert(colors, 0, (0, 0, 0, 1), axis=0)
    LUT = np.array(colors * 255, dtype="uint8")
    return LUT

def __color_image_with_mask_random(image: np.array, mask: np.array) -> np.array:
    LUT = __generate_random_LUT()

    color = LUT[mask]
    out = np.where(mask[:, :, np.newaxis] > 0, color, image)

    return out

def generate_image(doc: list, image: np.array) -> np.array:
    """
    Generate from a document and an image the corresponding output image.

    Column's background are colored in pink.
    The main document's structuring element are surrounded with a random color.
    The detected entries are also colored with a random color.
    """
    image = __to_rgba_dims(image)

    entry_mask = __get_entry_mask(doc, image)
    non_entry_mask = __get_non_entry_box_mask(doc, image)
    column_binary_mask = __get_column_binary_mask(doc, image)

    random_color_mask = __merge_masks(entry_mask, non_entry_mask)
    image = __color_image_with_mask_light_red(image, column_binary_mask)

    return __color_image_with_mask_random(image, random_color_mask)
