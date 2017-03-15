#!/bin/bash 

pdflatex guide
makeindex guide.idx -s StyleInd.ist
biber guide
pdflatex guide
pdflatex guide
rm *.aux
rm *.bbl
rm *.bcf
rm *.blg
rm *.idx
rm *.ilg
rm *.ind
rm *.log
rm *.ptc
rm *.run.xml
rm *.toc
