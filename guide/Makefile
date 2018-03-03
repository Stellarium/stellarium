# Makefile for the Stellarium User Guide (SUG)
# Required tools:
# WINDOWS 
# ========
# It is possible to run make from the Windows command shell or from Linux-like environments: CygWin, Ubuntu shell or at least the Git shell. 
# If cygwin or Ubuntu are your favorite environments and you have already installed TeXlive there, 
# you won't need MiKTeX, but can use the existing installations. Else install these tools:
# * make and touch from GnuWin32
# * MiKTeX from http://miktex.org. Configure it to auto-download packages as required.
# * pdftk.exe from https://www.pdflabs.com/tools/pdftk-server/ or the pdftk.exe in the program folder of the "private" GUI version.
# * GhostScript from https://www.ghostscript.com. Make sure to have the distribution's bin directory in PATH.
# GIT shell: Install these tools and just make sure they are found in PATH!
# CygWin: To build SUG with native tools, install the relevant packages from the Cygwin repository; alternatively, the Windows tools will be called
# Ubuntu: apt-get install make texlive-full gs pdftk. On Windows' Ubuntu shell, the native Windows tools can be used if available in PATH.
#
# LINUX
# =======
# Debian, Ubuntu: apt-get install make texlive-full gs pdftk. 
# Others: make sure to install the abovementioned packages for your distribution.
#
# MAC OS X
# ==========
# * TODO: please add instructions here.



# We try to detect Windows or other system to configure the tool names properly. 
# Problem: Ubuntu shell on Windows does not know environment variable OS. 
# https://stackoverflow.com/questions/38086185/how-to-check-if-a-program-is-run-in-bash-on-ubuntu-on-windows-and-not-just-plain
#
# On Windows with git shell, Ubuntu bash or cygwin, use rm and mv, not del and move. 
# Note that make resets SHELL to sh.exe. 
# Cygwin/Ubuntu/MinGW/Git shell's version includes full path name with forward-slash, signalling bash use.
# So far we can also assume that we are on pure (poor) Windows commandline if SHELL=sh.exe

PDFLATEX=pdflatex
HTLATEX=htlatex
BIBER=biber
MAKEINDEX=makeindex
MAKE4HT=make4ht

ifeq ($(OS),Windows_NT) # variable OS not set under Win10/Ubuntu!
	ifeq ($(findstring /sh, $(SHELL)),/sh)
		RM=rm -fr
		MV=mv
		ifeq ($(findstring gs,$(shell which gs 2>/dev/null)),gs)
			GS=gs
		else
			ifeq ($(findstring gswin64c,$(shell which gswin64c 2>/dev/null)),gswin64c)
				GS=gswin64c.exe
			else
				GS=gswin32c.exe
			endif
			MAKE4HT=make4ht.exe
		endif
		PDFTK=pdftk
	else # cmd shell. TODO: hide error message of "where".
		RM=del /Q
		MV=rename
		ifeq ($(findstring gswin64c,$(shell where gswin64c)),gswin64c)
				GS=gswin64c.exe
			else
				GS=gswin32c.exe
			endif
		PDFTK=pdftk.exe
		MAKE4HT=make4ht.exe
	endif
else 
	# OS not Windows on most Non-Windows environments, but may still be Ubuntu bash on Windows! 
	# In essence, if tool not found, fall back to that tools' names from the Windows environment.
	# It is assumed that at least the Windows tools are configured to lie in PATH.
	ifneq ($(findstring pdflatex, $(shell which pdflatex)),pdflatex)
		PDFLATEX=pdflatex.exe
	endif
	ifneq ($(findstring htlatex, $(shell which htlatex)),htlatex)
		HTLATEX=htlatex.exe
	endif
	ifneq ($(findstring biber, $(shell which biber)),biber)
		BIBER=biber.exe
	endif
	ifneq ($(findstring makeindex, $(shell which makeindex)),makeindex)
		MAKEINDEX=makeindex.exe
	endif
	ifneq ($(findstring pdftk, $(shell which pdftk)),pdftk)
		PDFTK=pdftk.exe
	else	
		PDFTK=pdftk
	endif
	ifneq ($(findstring gs,$(shell which gs)),gs)
		GS=gswin64c.exe
	else
		GS=gs
	endif
	RM=rm -fr 
	MV=mv
	SHELL=/bin/bash
endif

# htlatex needs bounding box files.
PICSpdf=$(wildcard pictures/*.pdf)
PICSpng=$(wildcard pictures/*.png) $(subst .pdf,.png, ${PICSpdf})
PICSjpg=$(wildcard pictures/*.jpg)
PICSbbx=$(subst .png,.xbb, ${PICSpng}) $(subst .jpg,.xbb, ${PICSjpg})


.DEFAULT:
.PHONY: help diag clean distclean guide pdf manual SUG html

.SUFFIXES:
.SUFFIXES: .tex .aux .log .lot .lof .toc .bbl .ind .dvi .pdf .eps .jpg .png .html .ps

# This is to keep intermediate files until explicitly removed with make clean.
.SECONDARY:


help:
	@echo               Stellarium Manual Options
	@echo               ==========================
	@echo PDFLaTeX creates the prettiest PDF, but it is quite large.
	@echo targets: 
	@echo clean: remove intermediate files.
	@echo distclean: also remove the PDF targets SUG.pdf, guide.pdf
	@echo pdf = guide = manual = guide.pdf :  Stellarium User Guide \(hi-res\)
	@echo SUG = SUG.pdf : as final target applies compression via GhostScript \(slow!\)
	@echo html: create online version. \(EXPERIMENTAL, WIP\)
	@echo diag: print some diagnostics about tool names.

clean:
	${RM} *~ core *.aux *.log *.lof *.toc *.out *.blg *.bcf *.bbl *.idx *.ilg *.ind *.ptc
	${RM} *.run.xml *.4ct *.4tc *.tmp *.dvi guide_meta.txt *.html *.lg *.idv *.css *.svg *.xref guide*.png 

distclean: clean
	${RM} guide.pdf SUG.pdf ${PICSbbx} html

manual: guide.pdf

guide: guide.pdf

pdf: guide.pdf 

dvi: guide.dvi

html: guide.html

# Installation notes for some tools:
# Use any recent LaTeX distribution like TeXlive (Linux) or MiKTeX 2.9 or later. 

# The size reduction will need GhostScript and some add-on tools.
# 2 options were explored on Windows 10's Ubuntu 14.04 LTS: 
# * install https://github.com/pts/pdfsizeopt 
#   sudo apt-get install default-jre 
#   download Multivalent.jar from https://sourceforge.net/projects/multivalent/?source=typ_redirect and put it into PATH
#   The official project seems defunct. Found old version at  http://www.accountingenhancements.com/multivalent.sourceforge.net/files/Multivalent20060102.jar 
#   run target SUG.pdf
# * optpdf. This removes the opening view settings from the PDF which have to be restored with pdfmark hacks.
#   We also need pdftk for this.
# --> make SUG.pdf

# ================ GENERAL RULES ==================================

# TODO: A rule that securely detects when guide.pdf is complete and no more action is required. Probably we need to make out own logfile grep for "Rerun Latex" and a stop file target.
# Issue: bbl depends on bcf which is created on every run. Likewise, ind depends in idx. We should detect when pdflatex does not emit a warning about changed page numbers.
# Currently, we assume page numbers have settled by the 3rd run, and artificially change the file ages with touch to convince make to stop.

#%.tex : ;
#Makefile: ;
# we must call makeindex again just before final run to update page numbers. Not too intuitive, but seems necessary...
guide.pdf : guide.ind guide.bbl 
	${PDFLATEX} guide.tex
	${MAKEINDEX} guide.idx -s StyleInd.ist
	${PDFLATEX} guide.tex
	${PDFLATEX} guide.tex
	@touch -c guide.ind
	@touch -c guide.bbl
	@touch -c $@

guide.dvi : guide.tex
	+make ${PICSpng}
	+make ${PICSbbx}
	latex $<

#guide.ind: guide.idx guide.toc
guide.ind: guide.idx guide.aux
	${MAKEINDEX} guide.idx -s StyleInd.ist

guide.bbl: guide.bcf guide.bib
	${BIBER} guide

guide.idx: guide.log

guide.toc: guide.log	

guide.bcf: guide.log

guide.aux: guide.log

guide.log: $(wildcard *.tex) 
	${PDFLATEX} guide.tex

SUG SUG.pdf: guide_opt.pdf
	${MV} $< SUG.pdf

%_meta.txt : %.pdf
	${PDFTK} $< dump_data_utf8 > $@


# Reduced file size by 65%. But we need to restore some meta info.
%_opt.pdf : %.pdf %_meta.txt %_pdfmark.ps
	${GS} -sDEVICE=pdfwrite -dCompatibilityLevel=1.6 -dNOPAUSE -dQUIET -dBATCH  -sOutputFile=$*_gswrite.pdf  $*.pdf $*_pdfmark.ps 
	${PDFTK} $*_gswrite.pdf update_info_utf8 $*_meta.txt output $@
	${RM} $*_gswrite.pdf $*_meta.txt 


pdf2png: $(subst .pdf,.png, ${PICSpdf})

pictures/%.png : pictures/%.pdf
	convert $< $@

%.xbb: %.png
	ebb -x $<

%.xbb: %.jpg
	ebb -x $<

## pdfsizeopt unfortunately hardly makes the SUG smaller :-(	
## But it is recommended to run Multivalent after that!
## NOPE! It destroys images.
#SUG.pdf : guide.pdf	
#	pdfsizeopt $< $@
#	java -classpath Multivalent20060102.jar tool.pdf.Compress -jpeg $@
#	mv SUG-o.pdf SUG.pdf

htdia:
	@echo Want to create ${PICSbbx}


# HTML: Work in progress. I run into many issues that have been discussed in various TeX fora.
# TODO: Whoever knows about htlatex/tex4ht is welcome to help in fixing formatting macros or properly
# configuring the build of a friendly frame set.
# it seems tex4ht needs more than just running htlatex, but we need some configuration file.
#guide.html: guide.bbl guide.ind
#
# See https://github.com/michal-h21/helpers4ht/wiki/tex4ht-tutorial
# htlatex filename "options for tex4ht.sty" "options for tex4ht" "options for t4ht" "LaTeX options"

html-draft: $(wildcard *.tex) ${PICSbbx}
	@echo Shell is $(SHELL)
	make4ht --output-dir html --utf8 guide -m draft guide "htFixes" 
#	make4ht --output-dir html --utf8 guide -m draft guide "htFixes"


# GZ we don't use the htlatex command script because it does not call biber or makeindex. 
# guide.mk4 is a config file for Stellarium HTML Guide to be processed by make4ht. See https://github.com/michal-h21/make4ht for details.

guide.html: $(wildcard *.tex) guide.mk4 htFixes.cfg ${PICSbbx}
	@echo Shell is $(SHELL)
	${MAKE4HT} --output-dir html --utf8 guide "htFixes" 
#	make4ht --output-dir html --utf8 guide "htFixes" " -cunihtf -utf8"
#	${HTLATEX} guide "htFixes" " -cunihtf -utf8"
# Currently, everything after running biber fails. :-(
#	${BIBER} guide
#	tex '\def\filename{{guide}{idx}{4dx}{ind}} \input  idxmake.4ht'
#	${MAKEINDEX} -o guide.ind guide.4dx
#	${HTLATEX} guide "htFixes,html,1,next,index=3,fn-in,frames,mouseover,info"
#	${HTLATEX} guide "htFixes,html,1,next,index=3,fn-in,frames,mouseover,info"



# This throws an error by lua when processing the tex line. The currently used mk4 lacks the index related stuff.
# Also, it seems ${filename} in the biber argument does not work properly. Should be target without extension.
guide.mk4: 		
	@echo if mode == "draft" then   > $@
	@echo Make:htlatex()           >> $@
	@echo else                     >> $@
	@echo   Make:add("biber", "biber ${input}")                                                        >> $@
	@echo   Make:add("texindex", "tex '\def\filename{{guide}{idx}{4dx}{ind}} \input  idxmake.4ht'") >> $@
	@echo   Make:add("makeindex", "$makeindex ${filename}.idx -s StyleInd.ist")                     >> $@
	@echo   local filter = require "make4ht-filter"                                                 >> $@
	@echo   local process = filter{"cleanspan", "fixligatures", "hruletohr", "fix-links"}           >> $@
	@echo   Make:htlatex()     >> $@
	@echo   Make:biber()       >> $@
	@echo   Make:texindex()    >> $@
	@echo   Make:makeindex()   >> $@
	@echo   Make:htlatex()     >> $@
	@echo   Make:htlatex()     >> $@
	@echo   Make:match("html$",process)                                 >> $@
 	@echo  Make:match("html$", "tidy -m -xml -utf8 -q -i ${filename}")  >> $@
	@echo end                  >> $@

	
	
# Totally non-tex4ht: Try pandoc? (start to collect arguments here, try "make pandoc")
# Ubuntu on Windows only has pandoc 1.12. Maybe V2 would be more complete? In any case, tabu or some other package seems to be a problem.
pandoc:
		pandoc -f latex -t plain guide.tex

		
# Hint on restoring initial view settings
# from https://stackoverflow.com/questions/25544002/command-line-tool-to-mark-pdf-to-open-in-single-page-view	
#guide_pdfmark.ps: Makefile
#		echo "[ /PageMode /UseOutlines  % Display bookmarks upon opening the doc\n\
#             %/PageMode /UseThumbs    % Display thumbnails upon opening the doc\n\
#              %/PageMode /FullScreen   % Open the document in fullscreen mode\n\
#              %/PageMode /None         % Display neither bookmarks nor thumbnails upon opening\n\
#              /Page 1                 % Open document with page 1\n\
#              %/View [ /XYZ null null null ]\n\
#                          % Go to specified page and retain same ... \n\
#                          % ... horizontal/vertical offset+zoom as current page\n\
#              /View [ /Fit ]           % Fit page to window\n\
#              %/View /FitB             % Fit visible part of page to window\n\
#              %/View [/FitH 220]       % Fit page width to window; 220 is distance ...\n\
#                                       % ... of page origin from top of window\n\
#                                         /DOCVIEW    pdfmark\n\
#              \n\
#              [ {Catalog} <<\n\
#               /ViewerPreferences<</DisplayDocTitle true>>\n\
#               %/PageLayout /SinglePage\n\
#               %/PageLayout /OneColumn\n\
#               /PageLayout /TwoPageRight\n\
#               %/PageLayout /TwoColumnLeft\n\
#               >>                        /PUT        pdfmark"\
#			 > $@


# Very hackish diagnostics. There are many ways to get a decent Linux-ish system even under Windows. 
# CygWin, MinGW and Git bash understand calling programs with .exe from the Windows environment. 
# Ubuntu needs .exe extension to be given explicitly. This makes the Makefile a bit ugly.
diag:
ifeq ($(SHELL),sh.exe)
	@echo Looks like plain WINDOWS command shell. 
else
 ifeq ($(findstring Git, $(SHELL)),Git)
	@echo INFO: We are in MinGW/git shell, right?
 else
  ifeq ($(findstring Microsoft, $(shell cat /proc/version)), Microsoft)
	@echo INFO: This looks like Ubuntu on Windows
  else
   ifeq ($(findstring CYGWIN, $(shell uname)),CYGWIN)
	@echo INFO: We are on Cygwin, right?
   else
	@echo I assume $(shell uname) is a valid environment. Please enhance this line.
   endif
  endif 
 endif 
endif
	@echo SHELL=${SHELL}
	@echo PDFLATEX=${PDFLATEX}
	@echo MAKEINDEX=${MAKEINDEX}
	@echo BIBER=${BIBER}
	@echo PDFTK=${PDFTK}
	@echo GS=${GS}
	@echo RM=${RM}
	@echo MV=${MV}
	@echo MAKE4HT=$(MAKE4HT)
