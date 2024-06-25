# Usage:
# - checkout soduco/directory-annotator-back
# - build this image `docker build -t soduco/back_builder -f soduco-back-cppbuilder.dockerfile .`
# - run `docker run --rm -it -v $(pwd):/app/ soduco/back_builder sh build.sh`
# - your artefacts will be ready in the `build/` folder

FROM python:3.9-slim-bullseye

ENV DEBIAN_FRONTEND noninteractive

RUN echo 'deb http://deb.debian.org/debian bullseye-backports main' >> /etc/apt/sources.list \
 && apt-get update \
 && apt-get install -y --no-install-recommends \
    cmake-data/bullseye-backports \
    cmake/bullseye-backports \
    g++ \
    libboost-dev \
    libfindbin-libs-perl \
    libfreeimage-dev \
    libpoppler-cpp-dev \
    libtesseract-dev \
    make \
    ninja-build \
 && apt-get clean autoclean \
 && apt-get autoremove -y \
 && rm -rf /var/lib/apt/lists/* \
 && rm -f /var/cache/apt/archives/*.deb

# seriously... python package distribution is broken...
RUN pip3 install --no-cache -q setuptools wheel \
 && pip3 install --no-cache -q conan \
 && conan remote add lrde-public https://artifactory.lrde.epita.fr/artifactory/api/conan/lrde-public

WORKDIR /app

COPY .gitlabci/gcc-10 gcc-10
ENV CONAN_DEFAULT_PROFILE_PATH /app/gcc-10
RUN conan profile detect

CMD [ "/bin/bash" ]
