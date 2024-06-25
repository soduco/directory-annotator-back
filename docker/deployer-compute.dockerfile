FROM python:3.10-slim-bookworm as build-env

# Update package lists and install any necessary packages
RUN apt-get update && \
    apt-get install -y cmake unzip g++ perl-openssl-defaults libfindbin-libs-perl libssl-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*
    # apt-get install -y cmake unzip g++ perl-openssl-defaults libfindbin-libs-perl libtesseract-dev libssl-dev

# Install conan with pip
RUN pip install conan

# Set the working directory
WORKDIR /tmp/app-build

# Install the conan dependencies
COPY conanfile.py /tmp/app-build/
RUN touch CMakeLists.txt && \
    conan remote add lrde-public https://artifactory.lrde.epita.fr/artifactory/api/conan/lrde-public --index 0 && \
    conan profile detect && \
    conan install . -of build -s compiler.cppstd=20 --build=missing

# Copy the source code and install deps
COPY CMakeLists.txt /tmp/app-build/
COPY sources /tmp/app-build/sources
COPY thirdparty /tmp/app-build/thirdparty

# Build the application
RUN cmake -S . -B build --preset conan-release && \
    cmake --build ./build && \
    cmake --install ./build --prefix /app 


#######################################################################
### Second stage: runtime environment for CLI and improg/bakcend   ####
#######################################################################
# FROM debian:bookworm-slim as directory-annotator-imgproc

# RUN apt-get update && \
#     apt-get install --no-install-recommends -q -y ca-certificates
#     # apt-get install --no-install-recommends -q -y libtesseract5 tesseract-ocr-fra ca-certificates

# # Copy the built application from the build environment
# COPY --from=build-env /app /app

# # Expose any necessary ports
# # EXPOSE 8000

# # set environment variables
# ENV LD_LIBRARY_PATH=/app/bin/lib

# # Set the working directory
# WORKDIR /app/bin

# Define the command to run the application
# CMD /app/bin/server --prefix "${SCRIPT_NAME}" --port 8000 --storage-uri "${SODUCO_STORAGE_URI}"



############################################################
### Third stage: runtime environment for full-backend   ####
############################################################


FROM python:3.10-slim-bookworm AS directory-annotator-back-base


RUN apt-get update && \
    apt-get install -y libgl1 libtbb12 libtbbbind-2-5 libtbbmalloc2 && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Get the sources 
# COPY server /app/server/
COPY back /app/back/

# Copy bins/libs application from the build environment
COPY --from=build-env /app /app

RUN set -ex && \
    cd /app/bin/lib && \
    install -t /usr/local/lib \
        libfreeimage.so \
        && \
    ldconfig
        # libtbb.so.12 \
        # libtbbbind_2_5.so.3 \
        # libtbbmalloc.so.2 \
        # libtbbmalloc_proxy.so.2 \

# RUN apt-get update && \
#     apt-get install -y libtbb12 libtbbbind-2-5 libtbbmalloc2 && \
#     apt-get clean && \
#     rm -rf /var/lib/apt/lists/*

# set environment variables
ENV LC_ALL=C

WORKDIR /app/bin

CMD [ "/app/bin/cli", "--help" ]

############################################################
### Fourth stage: runtime environment for full-backend  ####
############################################################
FROM directory-annotator-back-base as directory-annotator-back-cli

WORKDIR /app/cli

COPY docker/requirements-back-runtime.txt .
# FIXME freeze the python dependencies
RUN pip install --no-cache-dir -r requirements-back-runtime.txt

COPY cli ./

# FIXME install the python dependencies

CMD [ "python", "soduco_cli.py", "--help"]

############################################################
###  Fifth stage: runtime environment for full-backend  ####
############################################################

# FROM directory-annotator-back-base as directory-annotator-back

# WORKDIR /app

# # Expose any necessary ports
# EXPOSE 8000

# CMD gunicorn --timeout 500 --access-logfile - --bind 0.0.0.0:8000 --proxy-allow-from='*' server:app


