# Install Dev-Tools and Build dependancies

## Conan C++ package manager

As *root* or as a user, install *Conan* and add the *lrde* artifactory:
```
pip install conan  # or 'pip3 install conan --user'
conan remote add lrde-public https://artifactory.lrde.epita.fr/artifactory/api/conan/lrde-public
```

(see https://docs.conan.io/en/latest/installation.html)

## Install build dependencies

### Ubuntu
```
sudo apt install \
    enchant \
    hunspell-fr \
    libboost1.71-dev \
    libfreeimage-dev \
    libpoppler-cpp-dev \
    libtesseract-dev \
    tesseract-ocr-fra
```

# Build (need cmake 3.11+)

There is a [build script](./build.sh) that runs the process described here.

```bash
mkdir build && cd build
conan install .. --build missing -s compiler.libcxx=libstdc++11 -s compiler.cppstd=20
cmake .. -DCMAKE_BUILD_TYPE=Release  # -DPYTHON_EXECUTABLE=/usr/bin/python3.8 # (opt.) force Python executable
cmake --build . --config Release
```
