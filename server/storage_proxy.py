import numpy as np
import logging
import requests

from flask import abort
from PIL import Image
from io import BytesIO

def get_input_image(directory: str, view: int, storage_url: str, auth_token: str) -> np.array:
    url = "{base_url}/directories/{}/{}/image".format(
        directory, view,
        base_url = storage_url
    )

    headers = {"Authorization": auth_token}

    response = requests.get(url=url, headers=headers)
    try:
        response.raise_for_status()
    except requests.HTTPError:
        abort(404, f"pdf file of {directory} not found in the storage server")

    img_bytes = response.content
    img_IO = BytesIO(img_bytes)
    img_IO.seek(0)

    img = Image.open(img_IO).convert("L")
    res = np.asarray(img)

    logging.info(f"Got the image of page {view} of {directory} annual")
    return res
