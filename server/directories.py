from io import BytesIO

import numpy as np
from flask import Blueprint, request, jsonify, send_file, abort
from PIL import Image

import back
from back.Application import App
from storage_proxy import get_input_image


bp_directories = Blueprint('directories', __name__, url_prefix='/directories')
bp_directories.config = {}

@bp_directories.before_request
def before_request_func():
    if request.method == "OPTIONS":
        return
    request_token = request.headers.get('Authorization')
    #if Authorization header not defined, request_token = None
    if request_token not in bp_directories.config['TOKENS']:
        abort(403, "Invalid request token.")

@bp_directories.record
def record_config(setup_state):
    bp_directories.config = setup_state.app.config

@bp_directories.route('/<directory>/<int:view>/image_deskew', methods=['GET'])
def get_image_deskew(directory, view):
    image_in = get_input_image(directory, view, bp_directories.config["SODUCO_STORAGE_URI"], bp_directories.config["SODUCO_STORAGE_AUTH_TOKEN"])
    image = None
    if image_in is not None:
        app = App(PERO_CONFIG_DIR=bp_directories.config['PERO_CONFIG_DIR'])
        image_deskewed = app.deskew(image_in)
        image = Image.fromarray(image_deskewed)
    else:
        # FIXME return a clear error code to the front instead of buggy image
        # Create a fake gray image here to avoid crash from the front
        image = Image.fromarray(np.full((100, 100), 128, np.uint8))
    data = BytesIO()
    image.save(data, 'JPEG')
    data.seek(0)
    return send_file(data, mimetype='image/jpeg')


@bp_directories.route('/<directory>/<int:view>/annotation', methods=['GET'])
def access_annotation(directory, view):
    '''
    Compute the page and cache it and return the content in json
    '''
    image_in = get_input_image(directory, view, bp_directories.config["SODUCO_STORAGE_URI"], bp_directories.config["SODUCO_STORAGE_AUTH_TOKEN"])
    content = None
    if image_in is not None:
        # Extract content from the image using the backend
        ocr_engine = request.args.get('ocr-engine', default = "pero", type=str)
        ocr_engine = getattr(back.Application.OCREngine, ocr_engine.upper(), None)
        if not ocr_engine:
            abort(400, description="Invalid OCR Engine")
        fontsize = request.args.get('font-size', default = -1, type=int)

        app = App(PERO_CONFIG_DIR=bp_directories.config['PERO_CONFIG_DIR'],
                  logging_level=int(bp_directories.config.get('SODUCO_DEBUG_LEVEL', 0)))

        opts = { "ocr_engine" : ocr_engine }
        if fontsize > 0:
            opts["fontsize"] = fontsize
        content, _ = app.process(image_in, **opts)
    else:
        # FIXME return a clear error message to the front instead of some buggy content
        # Create a return an empty annotation set if we cannot compute anything
        content = []

    content = [ x.to_json() for x in content ]
    return jsonify({"content" : content, "mode" : "computed"})
