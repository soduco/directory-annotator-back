from io import BytesIO

from flask import abort, Blueprint, current_app, request, send_file
from PIL import Image
import numpy as np

from back.Application import App

from storage_proxy import get_input_image


bp = Blueprint('imgproc', __name__, url_prefix='/imgproc')
bp.config = {}

@bp.before_request
def before_request_func():
    if request.method == "OPTIONS":
        return
    request_token = request.headers.get('Authorization')
    #if Authorization header not defined, request_token = None
    if request_token not in bp.config['TOKENS']:
        abort(403, "Invalid request token.")

@bp.record
def record_config(setup_state):
    bp.config = setup_state.app.config

@bp.route('/deskew', methods=['POST'])
def image_deskew():
    """Compute image deskew.

        Parameters must be passed as JSON post content:
        - "document" (string): document name (eg: `"Didot_1851a.pdf"`)
        - "view" (int): view to process (eg: `700`)

        Response content is a binary buffer containing a JPEG image.

        Example (Python):
        ```
        headers = { 'Authorization': DEBUG_TOKEN }
        query_dict = {"document": "Didot_1842a.pdf", "view": 700}
        response = requests.put(
            url=f"{server_uri}/deskew,
            headers=h,
            json=query_dict)
        img_bytes = response.content
        img_IO = io.BytesIO(img_bytes)
        img_IO.seek(0)
        img = PIL.Image.open(img_IO)
        ```

        Example (curl):
        ```
        curl -X POST \
        --url https://apps.lrde.epita.fr:8000/soduco/imgproc/deskew \
        --header "Authorization: $DEBUG_TOKEN" \
        --header "Content-type: application/json" \
        --data '{"document": "Didot_1842a.pdf", "view": 700}' \
        > image.jpg && identify image.jpg
        ```
    """
    # Parse request and check
    json_data: dict = request.get_json(force=True)
    # we use `force=True` to tolerate PUT requests with 'Content-Type' header different from 'application/json'
    if json_data is None or not isinstance(json_data, dict):
        current_app.logger.info("Could not parse JSON payload")
        return "Could not parse JSON payload.", 500
    document = json_data.get("document")
    view = json_data.get("view")
    error_str = ""
    if not isinstance(document, str):
        error_str += "Invalid required 'document' parameter (not a str or absent). "
    if not isinstance(view, int):
        error_str += "Invalid required 'view' parameter (not an int or absent). "
    
    if len(error_str) > 0:
        current_app.logger.info(error_str)
        return error_str, 500

    # Process request
    image_in = get_input_image(document, view, bp.config["SODUCO_STORAGE_URI"], bp.config["SODUCO_STORAGE_AUTH_TOKEN"])
    image = None
    if image_in is not None:
        app = App(PERO_CONFIG_DIR=bp.config['PERO_CONFIG_DIR'])
        image_deskewed = app.deskew(image_in)
        image = Image.fromarray(image_deskewed)
    else:
        # FIXME return a clear error code to the front instead of buggy image
        # Create a fake gray image here to avoid crash from the front
        image = Image.fromarray(np.full((100, 100), 128, np.uint8))
    # Save in buffer and send back
    data = BytesIO()
    image.save(data, 'JPEG')
    data.seek(0)
    return send_file(data, mimetype='image/jpeg')  # FIXME send software info back in header

