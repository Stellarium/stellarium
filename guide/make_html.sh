#!/bin/bash

pdf2htmlEX --embed cfijo --external-hint-tool=ttfautohint --dest-dir guide --zoom 1.6 guide.pdf
mv ./guide/guide.html ./guide/index.html