import pytest
import os
import os.path as osp
import re
import json
import io
from flask import url_for, request
from conftest import path_utils as pu

PDF_NAME = "Didot1843a700extr.pdf"
PDF_NAME2 = "Didot1843multextr.pdf"
PAGE = "1"
PDF_NAME_FAIL = "DidotFail"

# List of tuple (pdf, page)
params = [
    (PDF_NAME, PAGE),
    (osp.splitext(osp.basename(PDF_NAME))[0], PAGE),
]
params.extend([(PDF_NAME2, "{}".format(i)) for i in range(1, 6)])

# List with juste the pdfs
params_pdf = [pdf[0] for pdf in params]

# Header that contains the debug token
token_debug = {'Authorization': '12345678'}


def test_fail_root(client):
    '''
    Test error root route not found
    route: '/'
    '''
    response = client.get('/')
    assert response.status_code == 404

def test_fail_authrization(client):
    '''
    Test a bad token
    '''
    response = client.get('/directories/', headers={'Authorization': '00000001'})
    assert response.status_code == 403

def test_success_root(client):
    '''
    Test success route with directories prefix and get the list of the directories
    route: '/directories/'
    '''
    response = client.get('/directories/', headers=token_debug)
    assert response.status_code == 200

    response_json = response.get_json()
    assert 'directories' in response_json
    assert isinstance(response_json['directories'], dict)

@pytest.mark.skipif(reason='obsolete')
@pytest.mark.parametrize("pdf", params_pdf)
def test_success_meta_pdf(client, pdf):
    '''
    Test to get metadata from a pdf
    route: '/directories/{path_to_pdf}'
    '''
    response = client.get(osp.join('/directories/', pdf, ''), headers=token_debug)
    assert response.status_code == 200

    response_json = response.get_json()
    assert 'metadata' in response_json
    assert isinstance(response_json['metadata'], dict)
    for meta in response_json['metadata']:
        assert re.match('^st_.*$', meta) is not None

def test_fail_meta_pdf_not_existing(client):
    '''
    Test to get metadata from a non existing pdf
    route: '/directories/{path_to_pdf}'
    '''
    response = client.get(osp.join('/directories/', PDF_NAME_FAIL, ''), headers=token_debug)
    assert response.status_code == 404

@pytest.mark.parametrize("pdf, page", params)
def test_success_get_image(client, pdf, page, mock_app, mock_image):
    '''
    Test to get the image data
    route: '/directories/{path_to_pdf}/{page_view}/image'
    '''
    response = client.get(osp.join('/directories/', pdf, page, "image"), headers=token_debug)
    assert response.status_code == 200

    response_mimetype = response.mimetype
    assert response_mimetype == "image/jpeg"

    response_data = response.get_data()
    assert isinstance(response_data, bytes)

def test_fail_get_image_not_existing(client):
    '''
    Test to get the image data from a non existing directory
    '''
    response = client.get(osp.join('/directories/', PDF_NAME_FAIL, PAGE, "image"), headers=token_debug)
    assert response.status_code == 404

@pytest.mark.parametrize("pdf, page", params)
def test_success_get_annotation_cache(client, pdf, page, mock_loader, mock_saver):
    '''
    Test to get the content of the pdf
    route: '/directories/{path_to_pdf}/{page_view}/annotation'
    '''
    response = client.get(osp.join('/directories/', pdf, page, "annotation?force_compute=0"), headers=token_debug)
    assert response.status_code == 200

    response_json = response.get_json()
    assert 'content' in response_json
    assert isinstance(response_json['content'], dict)

    assert 'mode' in response_json
    assert response_json['mode'] == 'cached'

@pytest.mark.parametrize("pdf, page", params)
def test_success_get_annotation_force_compute(client, pdf, page, mock_app, mock_saver):
    '''
    Test to get the content of the pdf with force compute option
    route: '/directories/{path_to_pdf}/{page_view}/annotation?force_compute=1'
    '''
    response = client.get(osp.join('/directories/', pdf, page, "annotation?force_compute=1"), headers=token_debug)
    assert response.status_code == 200

    response_json = response.get_json()
    assert 'content' in response_json
    assert isinstance(response_json['content'], dict)

    assert 'mode' in response_json
    assert response_json['mode'] == 'computed'

@pytest.mark.parametrize("pdf, page", params)
def test_success_put_annotation(client, pdf, page):
    '''
    Test to save content in the json stored in the zip file
    route: '/directories/{path_to_pdf}/{page_view}/annotation'
    '''
    data_request = { 'content': { 'name': 'Thibault' } }
    response = client.put(osp.join('/directories/', pdf, page, "annotation"), data=json.dumps(data_request), headers=token_debug)
    assert response.status_code == 200
    assert response.get_data() == b'Content saved on the server'

@pytest.mark.parametrize("pdf, page", params)
def test_success_download_page(client, pdf, page):
    '''
    Test to download a single page as a json file from the server
    route: '/directories/{path_to_pdf}/{page_view}/annotation?download=1'
    '''
    response=client.get(osp.join('/directories', pdf, page, "annotation?download=1"), headers=token_debug)
    assert response.status_code == 200
    assert f"{int(page):04}.json" in response.headers[0][1]

def test_fail_download_page_not_existing(client):
    '''
    Test to download a non existing page
    route: '/directories/{path_to_pdf}/{page_view}/annotation?download=1'
    '''
    response=client.get(osp.join('/directories', PDF_NAME_FAIL, PAGE, "annotation?download=1"), headers=token_debug)
    assert response.status_code == 404

@pytest.mark.parametrize("pdf", params_pdf)
def test_success_download_directory(client, pdf):
    '''
    Test to download a whole directory as a zip file from the server
    route: '/directories/{path_to_pdf}/download_directory'
    '''
    response=client.get(osp.join('/directories', pdf, "download_directory"), headers=token_debug)
    assert response.status_code == 200
    assert response.content_type == 'application/zip'
    assert pu.get_stem_with_extension(pdf, "zip") in response.headers[0][1]

def test_fail_download_directory_not_existing(client):
    '''
    Test to download a non existing directory from the server
    route: '/directories/{path_to_pdf}/download_directory'
    '''
    response=client.get(osp.join('/directories', PDF_NAME_FAIL, 'download_directory'), headers=token_debug)
    assert response.status_code == 404



