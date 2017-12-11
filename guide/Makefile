# Makefile for the Stellarium User Guide
# On Windows with git shell or cygwin, use rm, not del. 
# Note that make resets SHELL to sh.exe. Cygwin/MinGW/Git shell's version includes full path name with forward-slash.

ifeq ($(OS),Windows_NT)
	ifeq ($(findstring /sh.exe, $(SHELL)),/sh.exe)
		RM=rm -fr
	else
		RM=del /Q
	endif
else
	RM=rm -fr 
endif

.DEFAULT:
.PHONY: help clean distclean guide pdf manual 

.SUFFIXES:
.SUFFIXES: .tex .aux .log .lot .lof .toc .bbl .ind .dvi .pdf .eps .jpg .png 

help:
	@echo ""
	@echo "              Stellarium Manual Options"
	@echo "              =========================="
	@echo "targets: clean, distclean, pdf=manual"
	@echo "pdf = guide = manual = guide.pdf   Stellarium User Guide"

clean:
	${RM} *~ core *.aux *.log *.lof *.toc *.out *.blg *.bcf *.bbl *.idx *.ilg *.ind *.ptc *.run.xml 
	
distclean: clean
	${RM} guide.pdf

manual: guide.pdf

guide: guide.pdf

pdf: guide.pdf

# ================ GENERAL RULES ==================================

#%.tex : ;
#Makefile: ;
# we must call makeindex again just before final run to update page numbers. Not too intuitive, but seems necessary...
guide.pdf : guide.bbl guide.ind 
	pdflatex guide.tex
	makeindex guide.idx -s StyleInd.ist
	pdflatex guide.tex

guide.ind: guide.idx guide.toc
	makeindex guide.idx -s StyleInd.ist
	
guide.bbl: guide.bcf guide.bib
	biber guide
	
guide.idx: $(wildcard *.tex)
	pdflatex guide.tex

guide.toc: guide.log	
	
#guide.bcf: $(wildcard *.tex) 
#	pdflatex guide.tex
# this does not have to do anything else: BCF is created in the same process like LOG.
guide.bcf: guide.log
	
guide.log: $(wildcard *.tex) 
	pdflatex guide.tex
