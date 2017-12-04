rem:  this batch file builds the cfitsio library 
rem:  using the Borland C++ v4.5 or new free v5.5 compiler
rem:
bcc32 -c buffers.c
bcc32 -c cfileio.c
bcc32 -c checksum.c
bcc32 -c drvrfile.c
bcc32 -c drvrmem.c
bcc32 -c editcol.c
bcc32 -c edithdu.c
bcc32 -c eval_l.c
bcc32 -c eval_y.c
bcc32 -c eval_f.c
bcc32 -c fitscore.c
bcc32 -c getcol.c
bcc32 -c getcolb.c
bcc32 -c getcolsb.c
bcc32 -c getcoli.c
bcc32 -c getcolj.c
bcc32 -c getcolui.c
bcc32 -c getcoluj.c
bcc32 -c getcoluk.c
bcc32 -c getcolk.c
bcc32 -c getcole.c
bcc32 -c getcold.c
bcc32 -c getcoll.c
bcc32 -c getcols.c
bcc32 -c getkey.c
bcc32 -c group.c
bcc32 -c grparser.c
bcc32 -c histo.c
bcc32 -c iraffits.c
bcc32 -c modkey.c
bcc32 -c putcol.c
bcc32 -c putcolb.c
bcc32 -c putcolsb.c
bcc32 -c putcoli.c
bcc32 -c putcolj.c
bcc32 -c putcolui.c
bcc32 -c putcoluj.c
bcc32 -c putcoluk.c
bcc32 -c putcolk.c
bcc32 -c putcole.c
bcc32 -c putcold.c
bcc32 -c putcols.c
bcc32 -c putcoll.c
bcc32 -c putcolu.c
bcc32 -c putkey.c
bcc32 -c region.c
bcc32 -c scalnull.c
bcc32 -c swapproc.c
bcc32 -c wcsutil.c
bcc32 -c wcssub.c
bcc32 -c imcompress.c
bcc32 -c quantize.c
bcc32 -c ricecomp.c
bcc32 -c pliocomp.c
bcc32 -c fits_hcompress.c
bcc32 -c fits_hdecompress.c
bcc32 -c zuncompress.c
bcc32 -c zcompress.c
bcc32 -c adler32.c
bcc32 -c crc32.c
bcc32 -c inffast.c
bcc32 -c inftrees.c
bcc32 -c trees.c
bcc32 -c zutil.c
bcc32 -c deflate.c
bcc32 -c infback.c
bcc32 -c inflate.c
bcc32 -c uncompr.c
del cfitsio.lib
tlib cfitsio +buffers +cfileio +checksum +drvrfile +drvrmem 
tlib cfitsio +editcol +edithdu +eval_l +eval_y +eval_f +fitscore
tlib cfitsio +getcol +getcolb +getcolsb +getcoli +getcolj +getcolk +getcoluk 
tlib cfitsio +getcolui +getcoluj +getcole +getcold +getcoll +getcols
tlib cfitsio +getkey +group +grparser +histo +iraffits +modkey +putkey 
tlib cfitsio +putcol  +putcolb +putcoli +putcolj +putcolk +putcole +putcold
tlib cfitsio +putcoll +putcols +putcolu +putcolui +putcoluj +putcoluk
tlib cfitsio +region +scalnull +swapproc +wcsutil +wcssub +putcolsb
tlib cfitsio +imcompress +quantize +ricecomp +pliocomp
tlib cfitsio +fits_hcompress +fits_hdecompress
tlib cfitsio +zuncompress +zcompress +adler32 +crc32 +inffast
tlib cfitsio +inftrees +trees +zutil +deflate +infback +inflate +uncompr
bcc32 -f testprog.c cfitsio.lib
bcc32 -f cookbook.c cfitsio.lib

