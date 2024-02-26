from flask import abort, Blueprint, jsonify, request

from back.ner import detect_named_entities


bp_ner = Blueprint('ner', __name__, url_prefix='/ner')
bp_ner.config = {}

@bp_ner.record
def record_config(setup_state):
    bp_ner.config = setup_state.app.config

@bp_ner.before_request
def before_request_func():
    if request.method == "OPTIONS":
        return
    request_token = request.headers.get('Authorization')
    #if Authorization header not defined, request_token = None
    if request_token not in bp_ner.config['TOKENS']:
        abort(403, "Invalid request token.")

@bp_ner.route('/<string:texts>', methods=['GET'])
def get_named_entity(texts: str):
    model = request.args.get("model","")
   
    # basic sanitizing
    model = model.lower().strip()
    model = "bert" if model not in ["cnn","bert"] else model

    return jsonify(detect_named_entities([texts], model))
  


@bp_ner.route('/', methods=['POST'])
def get_named_entity_post():
    """Run the ner on an array of texts

    Payload (POST):
        {
            texts : Array<string>   // An array of strings
            model : string?         // Model to run {"bert" (default), "cnn"}
        }


    Args:
        texts (str): _description_

    Returns: A list of objects with a "ner_xml" field
        {
            [ 
                { ner_xml : string }  // One for each input text
            ]
        }
    """
    content = request.json
    model = content.get("model", "")

    # basic sanitizing
    model = model.lower().strip()
    model = "bert" if model not in ["cnn","bert"] else model
    texts = content["texts"]
    print(texts)
    return jsonify(detect_named_entities(texts, model))