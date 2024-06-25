# SODUCO CLI tools for document image preprocessing and layout detection

## Usage
Quick Docker usage:
```shell
# Build
docker build -t soducocli -f docker/deployer-compute.dockerfile .
# Run the container (FIXME add a bind mount)
docker run --rm -it --name sdctest  soducocli bash
# Try the CLI tools:
# - C++
/app/bin/cli --help
# - Python
python /app/cli/soduco_cli.py --help
```

Sample usage with Docker:
```shell
# C++ tool
docker run --rm -it --name sdctest -v$(pwd)/test:/data:ro -v/tmp/output:/output  soducocli /app/bin/cli /data/data/Didot_1851a_300-page.jpg /output/imgcpp.jpg /output/jsoncpp.json

# FIXME bug with PDF input (no result)
docker run --rm -it --name sdctest -v$(pwd)/test:/data:ro -v/tmp/output:/output  soducocli /app/bin/cli /data/directories/Didot1843a700extr.pdf /output/imgcpp.jpg /output/jsoncpp.json --page 0--1

# Python tool
docker run --rm -it --name sdctest -v$(pwd)/test:/data:ro -v/tmp/output:/output  soducocli python /app/cli/soduco_cli.py /data/directories/Didot1843a700extr.pdf  -p 1 --output-json /output/img.json
```

You can also build the project from sources to generate a standalone application.

## Building from sources

To build the project from C++ sources, see the related [documentation](./build.md)


## Python Commandline interface

The command line interface is described [here](doc/cli.md)


## DOM & JSON Format description (outdated)

### Document Model

```
Page      := ( Section_1 | Title_1 )*
Section_1 := ( Column_1 )*
Column_1  := ( Section_2 )*
Section_2 := ( Column_2 )*
Column_2  := ( Entry )*
Entry     := ( Line )*

Terminaux:
Title_1   := 
Title_2   := 
Line      := 
```

### JSON Schema

* Non-terminal nodes map to JSON Element objects

  ```js
  {
    "type" : STRING // One of ( "PAGE" | "SECTION" | "COLUMN" | "ENTRY" | "LINE" | "TITLE" )   
    "bbox" : ARRAY OF INT // Element Bounding box coordinates [x, y, width, height]
    "children" : ARRAY OF OBJECTS // List of Children Elements   
  }

note that bounding box coordinates are for the **deskewed** image

* Text nodes (TITLE and LINE) have also the ``"text" : STRING`` attribute.


### Example


Input                    |          Output           | JSON
------------------------ | ------------------------- | ---
![](./doc/input-439.png) | ![](./doc/output-439.jpg) | [JSON file](./doc/output-439.json)

