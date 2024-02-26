# Command line interfaces


## Processing a batch of pages

```
usage: batch_process.py [-h] -p PAGES -o OUTFILE file

Command line interface for document extraction. The tool extracts the content of some
pages and export the information into a json file. Ex: python batch_process.py
Didot1851a.pdf -p 424-600 -o out.json Will create out-0424.json, out-0425.json...

positional arguments:
  file        Path to the pdf

optional arguments:
  -h, --help  show this help message and exit
  -p PAGES    Comma separated list of pages with page range support (ex 1,3,8-100)
  -o OUTFILE  Path to output jsons files
  -y          Option to assume you want to replace all the existing files
```


### Usefull one-liners


Given a file with list of arguments as in this [csv file](./liste_alphabetique.csv)
```
Henrichs_1838.pdf       ;222-427; COMMENTAIRE
Lamy_1839.pdf           ;402-565; COMMENTAIRE
Lamy_1840.pdf           ;128-417; COMMENTAIRE
Didot_1841a.pdf         ;153-425; COMMENTAIRE
```

```sh
$ parallel -u --bar --csv --colsep ';' -a input.csv  python ./batch_process.py  {1} -p {2} -o {1.}-'{:04}.json'
python ./batch_process.py Henrichs_1838.pdf -p 222-427 -o "Henrichs_1838-{:04}.json"
python ./batch_process.py Lamy_1839.pdf -p 402-565 -o "Lamy_1839-{:04}.json"
python ./batch_process.py Lamy_1840.pdf -p 128-417 -o "Lamy_1840-{:04}.json"
python ./batch_process.py Didot_1841a.pdf -p 153-425 -o "Didot_1841a-{:04}.json"
python ./batch_process.py Didot_1842a.pdf -p 191-491 -o "Didot_1842a-{:04}.json"
```

Note, to use the GPUs with parallel and disable Multithreading with Tesseract

```sh
$ export OMP_THREAD_LIMIT=1
$ export CUDA_DEVICE_ORDER=PCI_BUS_ID
$ parallel -u --bar --csv --colsep ';' -a input.csv CUDA_VISIBLE_DEVICES="{%}" python ./batch_process.py {1} -p {2} -o new/{1.}-'{:04}.json' --layout-file-pattern in/{1.}-'{:04}.json' --ocr-engine pero
```




# C++ Image processing backend interface

## Synopsis
```
Usage: python soduco_cli.py [OPTIONS] input output


Positional arguments:
  input  TEXT      REQUIRED   Path to the pdf.
  output TEXT      REQUIRED   Path to the output image (JPG).

Optional arguments:
  -h,--help                   Print this help message and exit.
  -o TEXT                     Path to the output json file.
  --deskew-only               Only perform the deskew.
  --font-size INT             Set document body font size (default=20px)
  -p,--page INT REQUIRED      Page to demat.
  -d,--debug                  Export tmp images and print debug logs  (Obsolete for now).
  --show-grid                 Show/Hide the grid  (Obsolete for now).
  --show-ws                   Show/Hide the watershed lines  (Obsolete for now).
  --show-layout               Show/Hide the layout lines  (Obsolete for now).
  --show-segments             Show/Hide LSD segments  (Obsolete for now).
  --skip-ocr                  Skip tesseract text extraction.
  --force-left                Force left indent.
  --force-right               Force right indent.
  --color-lines ENUM:value in {no->0,entry->1,indent->2,eol->3,number->4} OR {0,1,2,3,4}
                              Colorize lines  (Obsolete for now).
  --ocr-engine ENUM:value in {pero->0,tesseract->1} OR {0,1}
                              Specify the ocr engine used 
```

## Run on the whole pdf

```
mkdir -p /tmp/results
parallel -j8 ./soduco-cli ../data/Didot1851a.pdf -p {} /tmp/results/out-{}.tiff -o  /tmp/results/out-{}.json ::: $(seq 427 887)
```

## Generate (deskewed) images

```
mkdir -p /tmp/results
parallel -j8 ./soduco-cli --deskew-only ../data/Didot1851a.pdf -p {} /tmp/results/input-{}.tiff ::: $(seq 427 887)
```
