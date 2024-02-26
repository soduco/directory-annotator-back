import dataclasses
import logging
import html
from collections import defaultdict
from functools import lru_cache
from dataclasses import dataclass, field
from typing import List
from transformers import AutoTokenizer, AutoModelForTokenClassification, pipeline
from typing import Iterator
from enum import Enum, auto
import spacy
import os
import pathlib

MODEL_SPACY_NAME = "fr_ner_directories"
MODEL_BERT_NAME = "HueyNemud/berties"

HUGGINGFACE_MODEL_DIR = os.environ.get("HUGGINGFACE_MODEL_DIR")
if HUGGINGFACE_MODEL_DIR is not None:
    MODEL_BERT_NAME =  pathlib.Path.joinpath(pathlib.Path(HUGGINGFACE_MODEL_DIR), MODEL_BERT_NAME)



@dataclass
class Address:
    """Represents an address in a directory entry"""

    street_name: str = ""
    street_numbers: List[str] = field(default_factory=list)


@dataclass
class Entity:
    """A structured representation of a directory entry"""

    addresses: List[Address]
    persons: List[str]
    ner_xml: str = None


class Labels(Enum):
    """Named Entity types to extract"""
    LOC = auto()
    ACT = auto()
    CARDINAL = auto()
    PER = auto()


@dataclass
class _NamedEntity:
    """A unified representation of a Named Entity"""

    label: str
    score: float
    start: int
    end: int
    text: str

    ##
    # Factory methods
    ##

    @staticmethod
    def from_spacy(ent: spacy.tokens.span.Span) -> "_NamedEntity":
        return _NamedEntity(
            label=ent.label_,
            start=ent.start_char,
            end=ent.end_char,
            text=ent.text,
            score=None,
        )

    @staticmethod
    def from_huggingface(ent: dict) -> "_NamedEntity":
        ne = _NamedEntity(
            label=ent["entity_group"],
            start=ent["start"],
            end=ent["end"],
            text=ent["word"],
            score=ent["score"],
        )

        # we shift the span start to take into account the leading whitespace removed during tokenization
        # if ne.start > 0:
        #     ne.start += 1
        return ne


def detect_named_entities(texts: List[str], model: str = "bert") -> List[dict]:
    """Detects the entities for a list of texts

    Args:
        texts (List[str]): A list of text string
        model (str, optional): A NER model name in {"cnn","bert"}. Defaults to "cnn".

    Returns:
        List[dict]: The list of detected entities as a list of dictionaries
    """

    ner_available = {"cnn": _detect_with_cnn, "bert": _detect_with_bert}
    ner = ner_available[model]
    result = []
    for entry in ner(texts):
        logging.debug("Detected entry: %s", entry)
        result.append(dataclasses.asdict(entry))

    return result


def _build_entry(source_text:str, named_entities: List[_NamedEntity]) -> Entity:
    """From a list of named entities detected in a textual directory entry, creates an Entity object which holds
    the information extracted from the entry.
    Adresses are constructed here.

    Args:
        source_text (str): The entry text.
        named_entities (List[_NamedEntity]): The named entities detected in a directory entry.

    Returns:
        Entity: The structured representation of the processed entry.
    """
    span = lambda txt, start, end: txt[start:end].strip()
    attributes = defaultdict(lambda: [])
    address = Address()
    for e in named_entities:
        text = span(source_text, e.start, e.end)
        attributes[e.label].append(text)

        if e.label != Labels.CARDINAL.name and address.street_name:
            attributes["addresses"].append(address)
            address = Address()

        # FIXME If a cardinal is separated from its street name by any unlabeled text,
        # it will be added as a street number for this address, even if the unlabeled text is long.
        # This is due to the unlabeled text being ignored during the entity restructuration.
        if e.label == Labels.CARDINAL.name and address.street_name:
            address.street_numbers.append(text)

        if e.label == Labels.LOC.name:
            address.street_name = text

    if address.street_name:
        attributes["addresses"].append(address)

    return Entity(
        addresses=attributes["addresses"], persons=attributes[Labels.PER.name]
    )


def _get_xml_string(source_text: str, named_entities: List[_NamedEntity]) -> str:
    """Builds a XML representation of a directory entry from the list of named entities detected inside.
    Entites are assumed to be in order of appearance in the source text.

    Args:
        source_text (str): The entry text.
        named_entities (List[_NamedEntity]): The named entities detected in the source text.

    Returns:
        str: A XML string representing this entry with its named entities.
    """
    htmlescape = lambda x: html.escape(x, quote=False)
    xmlize = lambda tag, text: f"<{tag}>{htmlescape(text)}</{tag}>"

    cursor = 0
    parts = []
    for ent in named_entities:
        if ent.start > cursor:
            skipped_txt = htmlescape(source_text[cursor : ent.start])
            parts.append(skipped_txt)
        span = source_text[ent.start : ent.end]

        # Patch for BERT : the span may start with a whitespace or a \n.
        # In this case the leading character is moved in front of the named entity
        if span.startswith(("\t", " ", "\n")):
            parts.append(span[0])
            span = span[1:]

        xml_ent = xmlize(ent.label, span)
        parts.append(xml_ent)
        cursor = ent.end

    if cursor < len(source_text):
        last_words = htmlescape(source_text[cursor:])
        parts.append(last_words)

    return "".join(parts)


def _detect_with_cnn(texts: List[str]) -> Iterator[Entity]:
    """Named entity extraction using a Spacy CNN fine-tuned on the directories.

    Args:
        texts (List[str]): Entries to process, as strings.

    Yields:
        Iterator[Entity]: A generator of Entity objects containing the data extracted from the input texts.
    """

    nlp = _load_cnn()
    docs = nlp.pipe(texts)
    for doc in docs:
        named_entities = [_NamedEntity.from_spacy(e) for e in doc.ents]
        entry = _build_entry(doc.text, named_entities)
        entry.ner_xml = _get_xml_string(doc.text, named_entities)

        yield entry


def _detect_with_bert(texts: List[str]) -> Iterator[Entity]:
    """Named entity extraction using a BERT model fine-tuned on the directories.

    Args:
        texts (List[str]): Entries to process, as strings.

    Yields:
        Iterator[Entity]: A generator of Entity objects containing the data extracted from the input texts.
    """
    nlp = _load_bert()
    docs = nlp(texts)
    for ix, entities in enumerate(docs):
        named_entities = [_NamedEntity.from_huggingface(e) for e in entities]
        entry = _build_entry(texts[ix], named_entities)
        entry.ner_xml = _get_xml_string(texts[ix], named_entities)

        yield entry


@lru_cache(maxsize=1)
def _load_cnn():
    return spacy.load(MODEL_SPACY_NAME)


@lru_cache(maxsize=1)
def _load_bert(device = -1):
    """Load the best model

    Args:
        device (int, optional): Use >=0 to use a GPU device. Defaults to -1 (CPU).

    Returns:
        _type_: _description_
    """
    tokenizer = AutoTokenizer.from_pretrained(MODEL_BERT_NAME)
    model = AutoModelForTokenClassification.from_pretrained(MODEL_BERT_NAME)
    return pipeline(
        "ner", model=model, tokenizer=tokenizer, aggregation_strategy="simple", device=device
    )


if __name__ == "__main__":
    print(
        detect_named_entities(
            [
                """ Beaumont, fab. de laffetas d'An-
gleterre de toutes couleurs , taffe-
tas-Leperdriel, papier-compresse,
procure des élèves à MM. les phar-
maciens et des places à MM. les
élèves sans rétribution, Ecriv.,10.""",
                "Mme Michou, Rue du Marché Saint-Honoré, facteur, champ non reconnu. Il était une fois une chèvre blanche. 59 et 64. L'activité n'est pas facile. 56 et 59. L'activité n'est pas facile.",
                "Beaumont R. Ⓑ, cigaretier, Saint-Honoré 270, 272 et Saint-Antoine 272.",
            ],
            "cnn",
        )
    )
