from enum import Enum, auto
from typing import List
import numpy as np
import pathlib
import time
import logging
import math

from . import scribocxx
from . import scribo

import cv2


def detect_scale(width):
    s = math.log2(2048 / width)
    rs = round(s)
    if abs(s - rs) > 0.2:
        return -1
    return int(rs)


class App:

    def __init__(self, logger = None, logging_level = logging.INFO):
        if logger is None:
            self._logger = logging.Logger("scribo", level=logging_level)
            ch = logging.StreamHandler()
            ch.setLevel(logging_level)
            formatter = logging.Formatter('{asctime} - {levelname} - {message}', style="{")
            ch.setFormatter(formatter)
            self._logger.addHandler(ch)
        else:
            self._logger = logger
            self._logger.setLevel(logging_level)

        self._logging_level = self._logger.getEffectiveLevel()
        if self._logging_level == logging.DEBUG:
            scribocxx._set_debug_level(2)

        self._pero = None



    def deskew_with_segments(self, input: np.ndarray):
        start = time.process_time_ns()
        segments = scribo.extract_segments(input)
        end = time.process_time_ns()
        self._logger.info("Segment extraction in %.1f ms", 1e-6 * (end - start))

        start = time.process_time_ns()
        angle, segments, deskewed = scribo.deskew(input, segments, scribocxx.KConfig.kAngleTolerance)
        end = time.process_time_ns()
        self._logger.info("Document deskew in %.1f ms", 1e-6 * (end - start))
        return (segments, deskewed)

    @staticmethod
    def _rescale(input0):
        h, w = input0.shape
        h = int(2048 * h / w)
        return cv2.resize(input0, (2048, h))


    def deskew(self, input: np.ndarray, font_size = -1):
        input = self._rescale(input)

        start = time.process_time_ns()
        clean, params = scribo.clean_document(input, -1, font_size)
        end = time.process_time_ns()
        self._logger.info("Cleaning in %.1f ms", 1e-6 * (end - start))

        return clean

    @staticmethod
    def objectlist_to_dictlist(segments: list):
        import pandas as pd
        data = [ { k : getattr(s, k) for k in dir(s) if not k.startswith("__")} for s in segments ]
        for d, s in zip(data, segments):
            d.update(getattr(s, "__dict__", dict()))
        return data




    def process(self, input: np.ndarray, font_size = -1):
        input = self._rescale(input)

        start = time.process_time_ns()
        clean, params = scribo.clean_document(input, -1, font_size)
        end = time.process_time_ns()
        self._logger.info("Cleaning in %.1f ms", 1e-6 * (end - start))

        start = time.process_time_ns()
        segments = scribo.extract_segments(input)
        segments = scribo.deskew_segments(segments, params["deskew_angle"])
        end = time.process_time_ns()
        self._logger.info("Segmeent extraction in %.1f ms", 1e-6 * (end - start))


        scale = 1
        config = scribocxx.KConfig(params["xheight"], scale)

        # Hack to make a C-contiguous array
        clean = clean.copy()

        start = time.process_time_ns()
        regions = scribo.XYCutLayoutExtraction(clean, segments, config)
        end = time.process_time_ns()
        self._logger.info("Block extraction in %.1f ms", 1e-6 * (end - start))

        ## Line segmentation
        start = time.process_time_ns()
        text_block_indexes, text_blocks = zip(*[ (i, r.bbox) for i,r in enumerate(regions) if r.type  == scribocxx.DOMCategory.COLUMN_LEVEL_2])
        text_block_indexes = np.array(text_block_indexes)
        debug_prefix = "debug-05" if self._logging_level == logging.DEBUG else ""
        ws, lines = scribo.WSLineExtraction(clean, text_blocks, config, debug_prefix=debug_prefix)
        for i, l in enumerate(lines):
            l.parent_id = text_block_indexes[l.parent_id]
        end = time.process_time_ns()
        self._logger.info("Line extraction in %.1f ms", 1e-6 * (end - start))

        ## Extract entries (and push them in regions)
        start = time.process_time_ns()
        regions = scribo.EntryExtraction(regions, lines)
        end = time.process_time_ns()
        self._logger.info("Entry extraction in %.1f ms", 1e-6 * (end - start))

        # Add extra tags, offset ids and sanitize
        for i, x in enumerate(regions):
            x.id = 256 + i
            x.origin = "computer"
            x.parent_id += 256

        return regions, clean

