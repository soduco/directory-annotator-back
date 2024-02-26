import pytest
import os
import sys
from flask import Flask

def import_module(path):
    p = os.path.dirname(os.path.abspath(__file__))
    path_to_import = os.path.abspath(os.path.join(p, path))
    sys.path.append(path_to_import)

# FIXME: bad use
#sys.path.append(os.path.abspath("../server/"))
#sys.path.append(os.path.abspath("../"))

import_module("../server")
import_module("../")

import directories
import path_utils

@pytest.fixture(scope="module")
def app():
    '''
    fixture to setup the app with the test environment
    '''
    app = Flask(__name__, instance_relative_config=True)
    app.config['SODUCO_ANNOTATIONS_PATH'] = 'test/annotations'
    app.config['SODUCO_DIRECTORIES_PATH'] = 'test/directories'
    app.config['TOKENS'] = '12345678'
    app.register_blueprint(directories.bp_directories)
    return app

@pytest.fixture(scope="function")
def mock_app(mocker):
    '''
    mocking the app class
    '''
    mocker.patch('directories.Application')
    return mocker

@pytest.fixture(scope="function")
def mock_saver(mocker):
    '''
    mocking the saver class
    '''
    mocker.patch('directories.Saver')
    return mocker

@pytest.fixture(scope="function")
def mock_loader(mocker):
    '''
    mocking the loader class
    '''
    mocker.patch('directories.Loader')
    return mocker

@pytest.fixture(scope="function")
def mock_image(mocker):
    '''
    mocking the image module
    '''
    mocker.patch('directories.img')
    return mocker
