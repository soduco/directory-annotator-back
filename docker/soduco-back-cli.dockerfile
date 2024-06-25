FROM python:3.9-slim-bullseye
RUN apt-get update && \
    apt-get install -q -y \
    libfreeimage3 \
    wget \
    && \
    rm -rf /var/lib/apt/lists/*
    # libtbb2 \
    # tesseract-ocr-fra \
    # libtesseract4 \
# libfreeimage-dev

# qqn n'a pas bien fait son travail…
RUN ln -s /usr/lib/x86_64-linux-gnu/libfreeimage.so.3 /usr/lib/x86_64-linux-gnu/libfreeimage.so

# we have a dependency on libtbb12 now, which is not available on bullseye
RUN set -ex \
    && wget https://github.com/oneapi-src/oneTBB/releases/download/v2021.9.0/oneapi-tbb-2021.9.0-lin.tgz \
    && tar -xzf oneapi-tbb-2021.9.0-lin.tgz \
    && mv oneapi-tbb-2021.9.0/lib/intel64/gcc4.8/libtbb* /usr/lib/x86_64-linux-gnu/ \
    && rm -rf oneapi-tbb-2021.9.0* \
    && test -f /usr/lib/x86_64-linux-gnu/libtbb.so.12 \
    && echo "TBB12: installation complete"

# Install curl for health check
RUN apt-get update && apt-get install -y curl && apt-get clean


# # NER deps
# RUN apt-get update && apt-get install -y wget && apt-get clean
# RUN set -ex \
#     && mkdir -p /app \
#     && cd /app \
#     && wget https://www.lrde.epita.fr/~jchazalo/SHARE/fr_ner_directories-20210720.tar.gz \
#     && pip install -q --no-cache-dir fr_ner_directories-20210720.tar.gz \
#     && rm fr_ner_directories-20210720.tar.gz

# # NER Bert deps (roughly 900 MB X_x)
# RUN pip install -q --no-cache-dir transformers[torch]

# # PERO OCR deps
# RUN apt-get update && apt-get install -y libglib2.0-0 libgl1 wget unzip && apt-get clean
# RUN set -ex \
#     && mkdir -p /app \
#     && cd /app \
#     && wget https://www.lrde.epita.fr/~jchazalo/SHARE/pero-printed_modern-public-2022-11-18.zip \
#     && unzip pero-printed_modern-public-2022-11-18.zip \
#     && rm pero-printed_modern-public-2022-11-18.zip \
#     && ls -lAR /app
# produces the following files:
# - config_cpu.ini
# - config.ini
# - ocr_engine.json
# - ParseNet_296000.pt
# - ParseNet_296000.pt.cpu
# - VGG_LSTM_B64_L17_S4_CB4.2022-09-22.700000.pt
# - VGG_LSTM_B64_L17_S4_CB4.2022-09-22.700000.pt.cpu


# COPY ./docker/requirements-peroocr.txt /app/requirements-peroocr.txt
# RUN pip install -q --no-cache-dir -r /app/requirements-peroocr.txt

# ADD ./pero_eu_cz_print_newspapers_2020-10-09.tar.gz /data/pero_ocr/
# Will all following files:
# /data/pero_ocr/pero_eu_cz_print_newspapers_2020-10-07:
# - ParseNet.pb
# - checkpoint_350000.pth
# - config.ini
# - ocr_engine.json
#
# We could also cache this:
# Downloading: "https://download.pytorch.org/models/vgg16-397923af.pth" to /root/.cache/torch/hub/checkpoints/vgg16-397923af.pth
# COPY vgg16-397923af.pth /root/.cache/torch/hub/checkpoints/vgg16-397923af.pth

# FIXME remove when back cli is properly deactivated / back is in separated project
# Python deps
RUN pip install -q --no-cache-dir \
    fastspellchecker \
    filelock \
    flask \
    flask-cors \
    gunicorn \
    numpy \
    pillow \
    pikepdf \
    pytest \
    python-dotenv \
    regex \
    unidecode

ADD build/soducocxx-py39-*.tar.gz /
RUN mkdir -p /app/back/ && mv /soducocxx-py39-0.1.1-Linux/back/*.so /app/back/

COPY ./resources /app/resources
COPY ./back /app/back
COPY ./server /app/server
COPY ./cli /app/cli

ENV LD_LIBRARY_PATH=/app/lib
ENV LC_ALL=C
WORKDIR /app




CMD python cli/soduco_cli.py

