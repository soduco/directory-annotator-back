'''
Very simple unrestricted heath-check endpoint.
'''

from flask import Blueprint

bp = Blueprint('health_check', __name__, url_prefix='/health_check')

@bp.route('/', methods=['GET'])
def health_check():
    return "BACKEND server alive."