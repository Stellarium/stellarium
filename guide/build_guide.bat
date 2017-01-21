pdflatex guide
makeindex guide.idx -s StyleInd.ist
biber guide
pdflatex guide
pdflatex guide
del *.aux
del *.bbl
del *.bcf
del *.blg
del *.idx
del *.ilg
del *.ind
del *.log
del *.ptc
del *.run.xml
del *.toc
