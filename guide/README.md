# README for Stellarium User Guide

Georg Zotti, December 28, 2017

Stellarium's user guide (SUG) for versions 0.15 and later has been re-assembled from the Stellarium wiki by Alexander Wolf and Georg Zotti and greatly extended into what they thought was a complete and optimal format: hyperlinked PDF, created by PDFLaTeX. LaTeX is intimidating to the uninitiated, but the best system in the world for those who are. By V0.17 it had almost 350 pages and came as 28MB PDF file. 

During moving the project to Github some demand has been expressed for again providing an online version of the manual, i.e., HTML. Other developers discussed even leaving LaTeX, but the guide editors resist moving. We decided to widen our typesetting capabilities further by trying to make the PDF smaller, and to make also an HTML version using `htlatex` or something similar. 

The interesting part is now getting all tools right to create the documents. Of course `make` is applied by using a Makefile. But regarding variants in TeX Systems, our multi-platform universe provides many options.

## Installation of tools
### Windows

The classic TeX environment on Windows is MikTeX, a very complete TeX environment with its own package management system. Install it with the option to download missing packages as required. 

Alternatively, to be inline with the other Systems, consider TeXlive also for Windows (https://www.tug.org/texlive/windows.html). 

Windows is still the most common desktop platform in 2019, like it or not. However, its command shell is an embarassing relic of the DOS era and a far cry from tools available on the tiniest Linux system. 
The shortcomings of the command line have brought several Options for Linux-affine working:

* GNUWin32 tools
* Cygwin
* MinGW
* Windows Subsystem for Linux (WSL): Ubuntu for Windows 10. Consider installing Version 18.04 LTS.

In addition, the GIT Version Control System brings its own MinGW-based git shell. 

The GNUWin32 project provides many important tools to fill in the gaps. From GnuWin32, we need to install make and uname. We also need to install GhostScript. Make sure the relevant programs are found in PATH. Also install Clink from http://mridgers.github.io/clink.

Cygwin and Ubuntu can bring their own optional TeXLive installations, which may however be a bit outdated. If you have problems with the following setup, consider installing current TeXlive (see below). But if you have already MikTeX installed, you can use it from the MinGW/git, cygwin or Ubuntu bash shell and save the diskspace (up to a few gigabytes) for a TeX Installation. However, while cygwin and MinGW based shells will find executables in the Windows PATH called without the .exe extension, WSL/Ubuntu's shell needs the .exe in the called filenames. 

The Makefile is able to detect all these setups on Windows. You can check with 

```
make diag
```

whether your System is properly identified. Note that it does not check whether the programs are installed! If Ubuntu's TeXlive is not installed, we only *assume* that MiKTeX has been installed and the programs available in PATH.

### Linux

These instructions are based on Ubuntu. Find out and add the changes required for your System.

Ubuntu 14.04's TeXlive is a bit outdated. Install TeXlive directly. See https://www.tug.org/texlive/quickinstall.html

On Ubuntu 18.04 and later the following should work: 

```
sudo apt-get install texlive-base texlive-bibtex-extra texlive-latex-recommended \
texlive-latex-extra texlive-pictures texlive-fonts-recommended biber make
```


### Mac OS X

It is assumed that some system similar to TeXlive is available for Mac OS X. See https://www.tug.org/texlive/quickinstall.html




## Building the PDF Guide

if you run `make` or `make help`, you will see some instructions. 

With a completely configured system, all that should be required is

```
make SUG
```

* `make SUG` creates guide.pdf (full resolution) and a compressed version, SUG.pdf, which needs only about 33% of the size. 

You will find guide.pdf (full resolution) and the compressed version, SUG.pdf.


## Building the HTML Guide

Because of differences in the toolchains we must clean the stuff created with the PDF version.

```
make clean
make html
make clean
```

* `make html` creates the online version in a subdirectory. 
* `make clean` removes intermediate files. PDF and HTML directory will remain.
* `make distclean` deletes all created files, also the PDF and HTML files.

## htlatex and TeX4ht
The workhorse for HTML creation from TeX-based sources is very flexible, but needs some configuration. I am not familiar yet with it, and it seems there is no good online tutorial for NOOBs. I will add my observations here as the project will continue.


### Aim for the HTML Version

The Stellarium Online Guide currently is a set of HTML pages cut at the chapter level. It could become a classical frameset: a narrow box on the left side with table of contents, and the content area on the right, maybe with a separate area for footnotes. Each chapter should become a page to facilitate fast loading. Colored boxes should be available, largely as they appear in the PDF Version. Hyperlinks to external sites should open a new tab or window, internal links should of course just load the other page.


### State
From what we know already: 

`htlatex` does not create HTML from the TeX files, but it creates a classical DVI file which is then further processed by the HTML creation process `ht4lt`. Therefore it seems useful to make sure a classical dvi file can be created. I added a dvi target for this. 

### Detect the processor

We must discern pdflatex from Latex and htlatex. 

```
\ifpdf
    stuff that is processed by pdflatex only
\else
    stuff for htlatex or latex
\fi
```

To encapsulate htlatex-only and pdflatex-only material, we defined code at the beginning of guide.tex and can now use (together with pdftex's built-in pdfoutput)

```
\ifhtlatex
\documentclass[12pt,fleqn,dvipdfmx]{book} % Default font size and left-justified equations
\else
\ifx\pdfoutput\undefined
  \documentclass[12pt,fleqn,dvipdfmx]{book} % Standard LaTeX
\else
  \documentclass[12pt,fleqn]{book} % runs with PDFLaTeX
\fi
\fi
```



### Updating TeXlive

Ubuntu in Windows 10 Comes with TeXlive2013. Some online fora indicate we should use TeXlive's own package Manager.

```
sudo apt-get install xzdec
tlmgr init-usertree
tlmgr update --self

```



However, now Biber complains with a Version clash about outdated biblatex. Biber and biblatex need to match! See https://sourceforge.net/projects/biblatex-biber/files/biblatex-biber/current/documentation/biber.pdf/download

--> Time to install Texlive 2017 on Ubuntu, so  `apt-get remove texlive-full biber`.

In case you are on Cygwin: Install fontconfig, ghostscript, libXaw7 and ncurses from the Cygwin package Manager, then you can join Ubuntuists in the next step.

I.e., download from http://tug.org/texlive/acquire-netinstall.html and run 

```
tar zxvf install-tl-DATE.tar.gz
cd install-tl-DATE
sudo ./install-tl
```

This brings you into a simple text-based menu System. The full Installation takes about 5GB. Press [I] to start. If you are on a slow line, go to sleep. (And hope your Windows does not decide to reboot for an update as soon as you look away! I really despise this "feature"!)

Then also fix PATHs in `~.profile`:

```
export PATH=/usr/local/texlive/2017/bin/x86_64-linux:$PATH
export MANPATH=/usr/local/texlive/2017/texmf-dist/doc/man:$MANPATH
export INFOPATH=/usr/local/texlive/2017/texmf-dist/doc/info:$INFOPATH
```

To install some new package, e.g. titling, from TUG:

```
tlmgr install titling
```

To update, 

```
(sudo apt-get install xzdec)
tlmgr update --self
tlmgr update --all
```


## Why biblatex&biber, and not the classic bibtex?

biblatex&biber is much more flexible. See 
https://tex.stackexchange.com/questions/25701/bibtex-vs-biber-and-biblatex-vs-natbib/25702
However, both packages need to be updated in sync.

## Finally a guide for beginners!

I hope we can follow https://github.com/michal-h21/helpers4ht/wiki/tex4ht-tutorial

Some more info also at https://www.12000.org/my_notes/faq/LATEX/html_and_latex.htm

OK, we need more tools: tidy can cleanup HTML.

```
sudo apt-get install tidy
```


## Increase Memory of your LaTeX system
Edit /usr/local/texlive/2017/texmf.cnf and add these lines:

```
buf_size=90000000
pool_size=9000000
main_memory=8000000
stack_size = 15000       % simultaneous input sources
```

Then call

```
sudo fmtutil-sys --all
```

## Forbidden things

Very odd: It seems section labels must not contain the name ":config". Or we have far too many labels. This seems to be a problem in the chapters with many tables. I had to reduce the number of valid labels.  Else: stack size exceeded. 

# Help Wanted!

If you have some experience with tex4ht to create a pleasing online version of Stellarium's User Guide, please feel free to contribute your TeXnical wisdom. Discuss with the team what kind of format is most useful. Apparently a frameset is possible but requires some extra work in the configuration. (And it is pretty 90ies-ish. But what's wrong with that? Do something better then.) "The LaTeX Web Companion" will be your best friend, it is surprising how little in-depth information is available online.

Some info is in 

* https://tex.stackexchange.com/questions/317686/trouble-building-a-site-with-frames-using-tex4ht and also 
* http://cvr.cc/?p=504.
