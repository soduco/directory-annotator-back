from __future__ import annotations
import itertools
from typing import Dict, List, Tuple
import numpy as np
import json

from . import scribocxx

class LayoutRegion(scribocxx.LayoutRegion):
    def to_json(self) -> dict:
        e = self.__dict__.copy()
        e["box"] = (self.x, self.y, self.width, self.height)
        e["type"] = self.type.name
        e["parent"] = self.parent_id
        return e

    @staticmethod
    def from_json(x) -> LayoutRegion:
        e = LayoutRegion()
        (bx, by, bw, bh) = x.pop("box")
        e.x, e.y, e.width, e.height = (int(bx), int(by), int(bw), int(bh))
        e.type = getattr(scribocxx.DOMCategory, x.pop("type"))
        e.parent_id = x.pop("parent", -1)
        e.__dict__.update(x)
        return e 

scribocxx.LayoutRegion.to_json = LayoutRegion.to_json
scribocxx.LayoutRegion.from_json = LayoutRegion.from_json

class ScriboJSONEncoder(json.JSONEncoder):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def default(self, o):
        if isinstance(o, scribocxx.LayoutRegion):
            return o.to_json()
        return super().default(o)


def extract_segments(input):
    segments = scribocxx._extract_segments(input)
    return segments

def deskew_segments(input, angle):
    segments = scribocxx._deskew_segments(input, angle)
    return segments

def deskew(input: np.ndarray, segments: List, angle_tolerance: float) -> Tuple:
    """Estimate an angle from a list of segments and deskew an image.

    Args:
        input (np.ndarray): The image to deskew
        segments (list): The list of segments from which the deskew angle is estamed
        angle_tolerance (float): The tolerance (in degree) to consider a segment as vertical/horizontal

    Returns:
        tuple: A triplet (angle, the new segments after deskew, the new image after deskew)
    """
    angle = scribocxx._deskew_estimation(segments, input.shape[1], float(angle_tolerance))
    new_segments = scribocxx._deskew_segments(segments, angle)
    new_image = scribocxx._deskew_image(input, angle)
    return angle, new_segments, new_image


def background_substraction(input: np.ndarray,
                kMinDiameter = None,      
                kMinWidth = None,         
                kMinHeight = None,
                kOpeningRadius = None,
                kMinGrayLevel = 220,           
                debug_prefix = "",
                inplace = False):
    '''
        kMinDiameter = 3.0 * _em,      
        kMinWidth = 4.0 * _em,         
        kMinHeight = 0.5 * kLineHeight,
        kMinGrayLevel = 220,           
        kOpeningRadius = 1.5 * _em,
    '''

    out = input if inplace else np.copy(input)
    scribocxx._background_substraction_inplace(out, kMinDiameter, kMinWidth, kMinHeight, kMinGrayLevel, kOpeningRadius, debug_prefix)
    return out



def clean_document(input: np.ndarray, xwidth = -1, xheight = -1, denoise = False) -> Tuple[np.ndarray, Dict]:
    """Clean the document

    Args:
        input (np.ndarray): _description_
        xwidth (int, optional): global x-width. Defaults to -1 (estimated).
        xheight (int, optional): global y-width. Defaults to -1 (estimated).
        denoise (bool, optional): Perform a denoising

    Returns:
        (output, params) with params that holds
        {
            "xwidth" : int,
            "xheigth" : int,
            "deskew_angle" : angle (in degree)
        }
    """
    return scribocxx._clean_document(input, xwidth, xheight, denoise)



def EntryExtraction(regions: List[scribocxx.LayoutRegion], lines: List[scribocxx.LayoutRegion]):
    """Estimate the entries from a list of lines and add them to the region list. The new regions
        have type "ENTRY" and the parent id of corresponding lines are updated.

    Args:
        regions (list[scribocxx.LayoutRegion]): The list of regions
        lines (list[scribocxx.LayoutRegion]): The sublist of regions that are lines

    Returns:
        list[scribocxx.LayoutRegion]: The new list of regions with new entries
    """
    newlines = []
    for id, group in itertools.groupby(lines, lambda x: x.parent_id):
        regions, newlines_ = scribocxx._EntryExtraction(regions[id].bbox, list(group), regions)
        newlines.extend(newlines_)
    regions.extend(newlines)
    return regions


def XYCutLayoutExtraction(input: np.ndarray, segments: List, config: scribocxx.KConfig) -> List[scribocxx.LayoutRegion]:
    """Perform a 2-levels XYCut from an image

    Args:
        input (np.ndarray): Input image
        segments (list): A list of segments in the image (used to prevent splits) 
        config (scribocxx.KConfig): Extra configuration options

    Returns:
        list[scribocxx.LayoutRegion]: A list of extracted regions
    """
    regions = scribocxx._XYCutLayoutExtraction(input, segments, config)
    return regions



def TesseractTextExtraction(input: np.ndarray, regions: List[scribocxx.LayoutRegion], is_line=False):
    texts = scribocxx._TesseractTextExtraction(input, regions, is_line=is_line)
    return texts

def WSLineExtraction(input: np.ndarray, regions: List[scribocxx.LayoutRegion], config: scribocxx.KConfig, debug_prefix="") -> Tuple[np.ndarray, scribocxx.LayoutRegion]:
    """Extract the lines from a list of regions. It returns a pair (ws, lines) where `ws` is an image of labeled lines.
    The `lines` are LayoutRegion of type *LINE* with parent id set to the id of the region.

    Args:
        input (np.ndarray): Input image
        regions (list): The list of regions where the lines are to be detected.
        config (scribocxx.KConfig): [description]
        debug_prefix (str, optional): [description]. Defaults to "".

    Returns:
        Tuple[np.ndarray, scribocxx.LayoutRegion]: A pair (ws, lines). 
    """
    ws, lines = scribocxx._WSLineExtraction(input, regions, debug_prefix, config)
    return ws, lines
