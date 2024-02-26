import os
import sys
from flask import Flask
from flask_cors import CORS

# FIXME: bad use
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import directories
import ner_routes
import ocr_routes
import imgproc_routes
import health_check


app = Flask(__name__, instance_relative_config=True)
app.config.from_envvar("SODUCO_SETTINGS", silent=True)
app.config.from_prefixed_env()

tokens = list()
path_secret_key = app.config.get("SODUCO_PATH_SECRET_KEY", None)
if path_secret_key and os.path.exists(path_secret_key):
    with open(path_secret_key, 'r') as tokens_file:
        for line in tokens_file.readlines():
            # Remove trailing whitespace
            line = line.rstrip()
            if not line.startswith('#') and len(line) > 0:
                tokens.append(line)
if app.debug:
    tokens.append('12345678')

if "SODUCO_STORAGE_URI" not in app.config:
    raise ValueError("SODUCO_STORAGE_URI is not set")


CORS(app,
     headers=['Content-Type', 'Authorization'], 
     expose_headers='Authorization')


app.config['TOKENS'] = tokens
app.register_blueprint(directories.bp_directories)
app.register_blueprint(ner_routes.bp_ner)
app.register_blueprint(ocr_routes.bp)
app.register_blueprint(imgproc_routes.bp)
app.register_blueprint(health_check.bp)
