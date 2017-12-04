#include <string.h>
#include <stdlib.h>
#include "fitsio.h"
int main(void);

int main()
{
/*  
    This is a big and complicated program that tests most of
    the cfitsio routines.  This code does not represent
    the most efficient method of reading or writing FITS files 
    because this code is primarily designed to stress the cfitsio
    library routines.
*/
    char asciisum[17];
    unsigned long checksum, datsum;
    int datastatus, hdustatus, filemode;
    int status, simple, bitpix, naxis, extend, hdutype, hdunum, tfields;
    long ii, jj, extvers;
    int nkeys, nfound, colnum, typecode, signval,nmsg;
    char cval, cvalstr[2];
    long repeat, offset, width, jnulval;
    int anynull;
/*    float vers;   */
    unsigned char xinarray[21], binarray[21], boutarray[21], bnul;
    short         iinarray[21], ioutarray[21], inul;
    int           kinarray[21], koutarray[21], knul;
    long          jinarray[21], joutarray[21], jnul;
    float         einarray[21], eoutarray[21], enul, cinarray[42];
    double        dinarray[21], doutarray[21], dnul, minarray[42];
    double scale, zero;
    long naxes[3], pcount, gcount, npixels, nrows, rowlen, firstpix[3];
    int existkeys, morekeys, keynum;

    char larray[42], larray2[42], colname[70], tdisp[40], nulstr[40];
    char oskey[] = "value_string";
    char iskey[21];
    int olkey = 1;
    int ilkey;
    short oshtkey, ishtkey;
    long ojkey = 11, ijkey;
    long otint = 12345678;
    float ofkey = 12.121212f;
    float oekey = 13.131313f, iekey;
    double ogkey = 14.1414141414141414;
    double odkey = 15.1515151515151515, idkey;
    double otfrac = .1234567890123456;

    double xrval,yrval,xrpix,yrpix,xinc,yinc,rot,xpos,ypos,xpix,ypix;
    char xcoordtype[] = "RA---TAN";
    char ycoordtype[] = "DEC--TAN";
    char ctype[5];

    char *lsptr;    /* pointer to long string value */
    char  comm[73];
    char *comms[3];
    char *inskey[21];
    char *onskey[3] = {"first string", "second string", "        "};
    char *inclist[2] = {"key*", "newikys"};
    char *exclist[2] = {"key_pr*", "key_pkls"};

    int   onlkey[3] = {1, 0, 1}, inlkey[3];
    long  onjkey[3] = {11, 12, 13}, injkey[3];
    float onfkey[3] = {12.121212f, 13.131313f, 14.141414f};
    float onekey[3] = {13.131313f, 14.141414f, 15.151515f}, inekey[3];
    double ongkey[3] = {14.1414141414141414, 15.1515151515151515,
           16.1616161616161616};
    double ondkey[3] = {15.1515151515151515, 16.1616161616161616,
           17.1717171717171717}, indkey[3];

    long tbcol[5] = {1, 17, 28, 43, 56};

    char filename[40], card[FLEN_CARD], card2[FLEN_CARD];
    char keyword[FLEN_KEYWORD];
    char value[FLEN_VALUE], comment[FLEN_COMMENT];
    unsigned char uchars[80];

    fitsfile *fptr, *tmpfptr;
    char *ttype[10], *tform[10], *tunit[10];
    char tblname[40];
    char binname[] = "Test-BINTABLE";
    char templt[] = "testprog.tpt";
    char errmsg[FLEN_ERRMSG];
    short imgarray[30][19], imgarray2[20][10];
    long fpixels[2], lpixels[2], inc[2];

    status = 0;
    strcpy(tblname, "Test-ASCII");

/*    ffvers(&vers); 
    printf("CFITSIO TESTPROG, v%.3f\n\n",vers);
*/
    printf("CFITSIO TESTPROG\n\n");
    printf("Try opening then closing a nonexistent file:\n");
    fits_open_file(&fptr, "tq123x.kjl", READWRITE, &status);
    printf("  ffopen fptr, status  = %lu %d (expect an error)\n", 
           (unsigned long) fptr, status);
    ffclos(fptr, &status);
    printf("  ffclos status = %d\n\n", status);
    ffcmsg();
    status = 0;

    for (ii = 0; ii < 21; ii++)  /* allocate space for string column value */
        inskey[ii] = (char *) malloc(21);   

    for (ii = 0; ii < 10; ii++)
    {
      ttype[ii] = (char *) malloc(20);
      tform[ii] = (char *) malloc(20);
      tunit[ii] = (char *) malloc(20);
    }

    comms[0] = comm;

    /* delete previous version of the file, if it exists (with ! prefix) */
    strcpy(filename, "!testprog.fit");

    status = 0;

    /*
      #####################
      #  create FITS file #
      #####################
    */

    ffinit(&fptr, filename, &status);
    printf("ffinit create new file status = %d\n", status);
    if (status)
        goto errstatus;

    filename[0] = '\0';
    ffflnm(fptr, filename, &status);

    ffflmd(fptr, &filemode, &status);
    printf("Name of file = %s, I/O mode = %d\n", filename, filemode);
    simple = 1;
    bitpix = 32;
    naxis = 2;
    naxes[0] = 10;
    naxes[1] = 2;
    npixels = 20;
    pcount = 0;
    gcount = 1;
    extend = 1;
    /*
      ############################
      #  write single keywords   #
      ############################
    */

    if (ffphps(fptr, bitpix, naxis, naxes, &status) > 0)
        printf("ffphps status = %d\n", status);

    if (ffprec(fptr, 
    "key_prec= 'This keyword was written by fxprec' / comment goes here", 
     &status) > 0 )
        printf("ffprec status = %d\n", status);

    printf("\ntest writing of long string keywords:\n");
    strcpy(card, "1234567890123456789012345678901234567890");
    strcat(card, "12345678901234567890123456789012345");
    ffpkys(fptr, "card1", card, "", &status);
    ffgkey(fptr, "card1", card2, comment, &status);
    printf(" %s\n%s\n", card, card2);
    
    strcpy(card, "1234567890123456789012345678901234567890");
    strcat(card, "123456789012345678901234'6789012345");
    ffpkys(fptr, "card2", card, "", &status);
    ffgkey(fptr, "card2", card2, comment, &status);
    printf(" %s\n%s\n", card, card2);
    
    strcpy(card, "1234567890123456789012345678901234567890");
    strcat(card, "123456789012345678901234''789012345");
    ffpkys(fptr, "card3", card, "", &status);
    ffgkey(fptr, "card3", card2, comment, &status);
    printf(" %s\n%s\n", card, card2);
    
    strcpy(card, "1234567890123456789012345678901234567890");
    strcat(card, "123456789012345678901234567'9012345");
    ffpkys(fptr, "card4", card, "", &status);
    ffgkey(fptr, "card4", card2, comment, &status);
    printf(" %s\n%s\n", card, card2);
    
    if (ffpkys(fptr, "key_pkys", oskey, "fxpkys comment", &status) > 0)
        printf("ffpkys status = %d\n", status);

    if (ffpkyl(fptr, "key_pkyl", olkey, "fxpkyl comment", &status) > 0)
        printf("ffpkyl status = %d\n", status);

    if (ffpkyj(fptr, "key_pkyj", ojkey, "fxpkyj comment", &status) > 0)
        printf("ffpkyj status = %d\n", status);

    if (ffpkyf(fptr, "key_pkyf", ofkey, 5, "fxpkyf comment", &status) > 0)
        printf("ffpkyf status = %d\n", status);

    if (ffpkye(fptr, "key_pkye", oekey, 6, "fxpkye comment", &status) > 0)
        printf("ffpkye status = %d\n", status);

    if (ffpkyg(fptr, "key_pkyg", ogkey, 14, "fxpkyg comment", &status) > 0)
        printf("ffpkyg status = %d\n", status);

    if (ffpkyd(fptr, "key_pkyd", odkey, 14, "fxpkyd comment", &status) > 0)
        printf("ffpkyd status = %d\n", status);

    if (ffpkyc(fptr, "key_pkyc", onekey, 6, "fxpkyc comment", &status) > 0)
        printf("ffpkyc status = %d\n", status);

    if (ffpkym(fptr, "key_pkym", ondkey, 14, "fxpkym comment", &status) > 0)
        printf("ffpkym status = %d\n", status);

    if (ffpkfc(fptr, "key_pkfc", onekey, 6, "fxpkfc comment", &status) > 0)
        printf("ffpkfc status = %d\n", status);

    if (ffpkfm(fptr, "key_pkfm", ondkey, 14, "fxpkfm comment", &status) > 0)
        printf("ffpkfm status = %d\n", status);

    if (ffpkls(fptr, "key_pkls", 
"This is a very long string value that is continued over more than one keyword.",
     "fxpkls comment", &status) > 0)
        printf("ffpkls status = %d\n", status);

    if (ffplsw(fptr, &status) > 0 )
        printf("ffplsw status = %d\n", status);

    if (ffpkyt(fptr, "key_pkyt", otint, otfrac, "fxpkyt comment", &status) > 0)
        printf("ffpkyt status = %d\n", status);

    if (ffpcom(fptr, "  This keyword was written by fxpcom.", &status) > 0)
        printf("ffpcom status = %d\n", status);

    if (ffphis(fptr, "    This keyword written by fxphis (w/ 2 leading spaces).",
        &status) > 0)
        printf("ffphis status = %d\n", status);

    if (ffpdat(fptr, &status) > 0)
    {
        printf("ffpdat status = %d\n", status);
        goto errstatus;
    }

    /*
      ###############################
      #  write arrays of keywords   #
      ###############################
    */
    nkeys = 3;

    comms[0] = comm;  /* use the inskey array of pointers for the comments */

    strcpy(comm, "fxpkns comment&");
    if (ffpkns(fptr, "ky_pkns", 1, nkeys, onskey, comms, &status) > 0)
        printf("ffpkns status = %d\n", status);

    strcpy(comm, "fxpknl comment&");
    if (ffpknl(fptr, "ky_pknl", 1, nkeys, onlkey, comms, &status) > 0)
        printf("ffpknl status = %d\n", status);

    strcpy(comm, "fxpknj comment&");
    if (ffpknj(fptr, "ky_pknj", 1, nkeys, onjkey, comms, &status) > 0)
        printf("ffpknj status = %d\n", status);

    strcpy(comm, "fxpknf comment&");
    if (ffpknf(fptr, "ky_pknf", 1, nkeys, onfkey, 5, comms, &status) > 0)
        printf("ffpknf status = %d\n", status);

    strcpy(comm, "fxpkne comment&");
    if (ffpkne(fptr, "ky_pkne", 1, nkeys, onekey, 6, comms, &status) > 0)
        printf("ffpkne status = %d\n", status);

    strcpy(comm, "fxpkng comment&");
    if (ffpkng(fptr, "ky_pkng", 1, nkeys, ongkey, 13, comms, &status) > 0)
        printf("ffpkng status = %d\n", status);

    strcpy(comm, "fxpknd comment&");
    if (ffpknd(fptr, "ky_pknd", 1, nkeys, ondkey, 14, comms, &status) > 0)
    {
        printf("ffpknd status = %d\n", status);
        goto errstatus;
    }
    /*
      ############################
      #  write generic keywords  #
      ############################
    */

    strcpy(oskey, "1");
    if (ffpky(fptr, TSTRING, "tstring", oskey, "tstring comment", &status) > 0)
        printf("ffpky status = %d\n", status);

    olkey = TLOGICAL;
    if (ffpky(fptr, TLOGICAL, "tlogical", &olkey, "tlogical comment",
        &status) > 0)
        printf("ffpky status = %d\n", status);

    cval = TBYTE;
    if (ffpky(fptr, TBYTE, "tbyte", &cval, "tbyte comment", &status) > 0)
        printf("ffpky status = %d\n", status);

    oshtkey = TSHORT;
    if (ffpky(fptr, TSHORT, "tshort", &oshtkey, "tshort comment", &status) > 0)
        printf("ffpky status = %d\n", status);

    olkey = TINT;
    if (ffpky(fptr, TINT, "tint", &olkey, "tint comment", &status) > 0)
        printf("ffpky status = %d\n", status);

    ojkey = TLONG;
    if (ffpky(fptr, TLONG, "tlong", &ojkey, "tlong comment", &status) > 0)
        printf("ffpky status = %d\n", status);

    oekey = TFLOAT;
    if (ffpky(fptr, TFLOAT, "tfloat", &oekey, "tfloat comment", &status) > 0)
        printf("ffpky status = %d\n", status);

    odkey = TDOUBLE;
    if (ffpky(fptr, TDOUBLE, "tdouble", &odkey, "tdouble comment",
              &status) > 0)
        printf("ffpky status = %d\n", status);

    /*
      ############################
      #  write data              #
      ############################
    */
    /* define the null value (must do this before writing any data) */
    if (ffpkyj(fptr, "BLANK", -99, "value to use for undefined pixels",
       &status) > 0)
        printf("BLANK keyword status = %d\n", status);

    /* initialize arrays of values to write to primary array */
    for (ii = 0; ii < npixels; ii++)
    {
        boutarray[ii] = (unsigned char) (ii + 1);
        ioutarray[ii] = (short) (ii + 1);
        joutarray[ii] = ii + 1;
        eoutarray[ii] = (float) (ii + 1);
        doutarray[ii] = ii + 1;
    }

    /* write a few pixels with each datatype */
    /* set the last value in each group of 4 as undefined */

/*
    ffpprb(fptr, 1,  1, 2, &boutarray[0],  &status);
    ffppri(fptr, 1,  5, 2, &ioutarray[4],  &status);
    ffpprj(fptr, 1,  9, 2, &joutarray[8],  &status);
    ffppre(fptr, 1, 13, 2, &eoutarray[12], &status);
    ffpprd(fptr, 1, 17, 2, &doutarray[16], &status);
*/

/*  test the newer ffpx routine, instead of the older ffppr_ routines */
    firstpix[0]=1;
    firstpix[1]=1;
    ffppx(fptr, TBYTE, firstpix, 2, &boutarray[0],  &status);
    firstpix[0]=5;
    ffppx(fptr, TSHORT, firstpix, 2, &ioutarray[4],  &status);
    firstpix[0]=9;
    ffppx(fptr, TLONG, firstpix, 2, &joutarray[8],  &status);
    firstpix[0]=3;
    firstpix[1]=2;
    ffppx(fptr, TFLOAT, firstpix, 2, &eoutarray[12],  &status);
    firstpix[0]=7;
    ffppx(fptr, TDOUBLE, firstpix, 2, &doutarray[16],  &status);

/*
    ffppnb(fptr, 1,  3, 2, &boutarray[2],   4, &status);
    ffppni(fptr, 1,  7, 2, &ioutarray[6],   8, &status);
    ffppnj(fptr, 1, 11, 2, &joutarray[10],  12, &status);
    ffppne(fptr, 1, 15, 2, &eoutarray[14], 16., &status);
    ffppnd(fptr, 1, 19, 2, &doutarray[18], 20., &status);
*/
    firstpix[0]=3;
    firstpix[1]=1;
    bnul = 4;
    ffppxn(fptr, TBYTE, firstpix, 2, &boutarray[2], &bnul,  &status);
    firstpix[0]=7;
    inul = 8;
    ffppxn(fptr, TSHORT, firstpix, 2, &ioutarray[6], &inul, &status);
    firstpix[0]=1;
    firstpix[1]=2;
    jnul = 12;
    ffppxn(fptr, TLONG, firstpix, 2, &joutarray[10], &jnul, &status);
    firstpix[0]=5;
    enul = 16.;
    ffppxn(fptr, TFLOAT, firstpix, 2, &eoutarray[14], &enul,  &status);
    firstpix[0]=9;
    dnul = 20.;
    ffppxn(fptr, TDOUBLE, firstpix, 2, &doutarray[18], &dnul, &status);

    ffppru(fptr, 1, 1, 1, &status);


    if (status > 0)
    {
        printf("ffppnx status = %d\n", status);
        goto errstatus;
    }

    ffflus(fptr, &status);   /* flush all data to the disk file */ 
    printf("ffflus status = %d\n", status);
    printf("HDU number = %d\n", ffghdn(fptr, &hdunum));

    /*
      ############################
      #  read data               #
      ############################
    */
    /* read back the data, setting null values = 99 */
    printf("\nValues read back from primary array (99 = null pixel)\n");
    printf("The 1st, and every 4th pixel should be undefined:\n");

    anynull = 0;
    ffgpvb(fptr, 1,  1, 10, 99, binarray, &anynull, &status); 

    ffgpvb(fptr, 1, 11, 10, 99, &binarray[10], &anynull, &status);

    for (ii = 0; ii < npixels; ii++)
        printf(" %2d", binarray[ii]);
    printf("  %d (ffgpvb)\n", anynull);  

    ffgpvi(fptr, 1, 1, npixels, 99,   iinarray, &anynull, &status);

    for (ii = 0; ii < npixels; ii++)
        printf(" %2d", iinarray[ii]);
    printf("  %d (ffgpvi)\n", anynull);  

    ffgpvj(fptr, 1, 1, npixels, 99,  jinarray, &anynull, &status);

    for (ii = 0; ii < npixels; ii++)
        printf(" %2ld", jinarray[ii]);
    printf("  %d (ffgpvj)\n", anynull);  

    ffgpve(fptr, 1, 1, npixels, 99., einarray, &anynull, &status);

    for (ii = 0; ii < npixels; ii++)
        printf(" %2.0f", einarray[ii]);
    printf("  %d (ffgpve)\n", anynull);  

    ffgpvd(fptr, 1,  1, 10, 99.,  dinarray, &anynull, &status);
    ffgpvd(fptr, 1, 11, 10, 99.,  &dinarray[10], &anynull, &status);

    for (ii = 0; ii < npixels; ii++)
        printf(" %2.0f", dinarray[ii]);
    printf("  %d (ffgpvd)\n", anynull);  

    if (status > 0)
    {
        printf("ERROR: ffgpv_ status = %d\n", status);
        goto errstatus;
    }
    if (anynull == 0)
       printf("ERROR: ffgpv_ did not detect null values\n");

    /* reset the output null value to the expected input value */
    for (ii = 3; ii < npixels; ii += 4)
    {
        boutarray[ii] = 99;
        ioutarray[ii] = 99;
        joutarray[ii] = 99;
        eoutarray[ii] = 99.;
        doutarray[ii] = 99.;
    }
        ii = 0;
        boutarray[ii] = 99;
        ioutarray[ii] = 99;
        joutarray[ii] = 99;
        eoutarray[ii] = 99.;
        doutarray[ii] = 99.;

    /* compare the output with the input; flag any differences */
    for (ii = 0; ii < npixels; ii++)
    {
       if (boutarray[ii] != binarray[ii])
           printf("bout != bin = %u %u \n", boutarray[ii], binarray[ii]);

       if (ioutarray[ii] != iinarray[ii])
           printf("iout != iin = %d %d \n", ioutarray[ii], iinarray[ii]);

       if (joutarray[ii] != jinarray[ii])
           printf("jout != jin = %ld %ld \n", joutarray[ii], jinarray[ii]);

       if (eoutarray[ii] != einarray[ii])
           printf("eout != ein = %f %f \n", eoutarray[ii], einarray[ii]);

       if (doutarray[ii] != dinarray[ii])
           printf("dout != din = %f %f \n", doutarray[ii], dinarray[ii]);
    }

    for (ii = 0; ii < npixels; ii++)
    {
      binarray[ii] = 0;
      iinarray[ii] = 0;
      jinarray[ii] = 0;
      einarray[ii] = 0.;
      dinarray[ii] = 0.;
    }

    anynull = 0;
    ffgpfb(fptr, 1,  1, 10, binarray, larray, &anynull, &status);
    ffgpfb(fptr, 1, 11, 10, &binarray[10], &larray[10], &anynull, &status);

    for (ii = 0; ii < npixels; ii++)
      if (larray[ii])
        printf("  *");
      else
        printf(" %2d", binarray[ii]);
    printf("  %d (ffgpfb)\n", anynull);  

    ffgpfi(fptr, 1, 1, npixels, iinarray, larray, &anynull, &status);

    for (ii = 0; ii < npixels; ii++)
      if (larray[ii])
        printf("  *");
      else
        printf(" %2d", iinarray[ii]);
    printf("  %d (ffgpfi)\n", anynull);  

    ffgpfj(fptr, 1, 1, npixels, jinarray, larray, &anynull, &status);

    for (ii = 0; ii < npixels; ii++)
      if (larray[ii])
        printf("  *");
      else
        printf(" %2ld", jinarray[ii]);
    printf("  %d (ffgpfj)\n", anynull);  

    ffgpfe(fptr, 1, 1, npixels, einarray, larray, &anynull, &status);

    for (ii = 0; ii < npixels; ii++)
      if (larray[ii])
        printf("  *");
      else
        printf(" %2.0f", einarray[ii]);
    printf("  %d (ffgpfe)\n", anynull);  

    ffgpfd(fptr, 1,  1, 10, dinarray, larray, &anynull, &status);
    ffgpfd(fptr, 1, 11, 10, &dinarray[10], &larray[10], &anynull, &status);

    for (ii = 0; ii < npixels; ii++)
      if (larray[ii])
        printf("  *");
      else
        printf(" %2.0f", dinarray[ii]);
    printf("  %d (ffgpfd)\n", anynull);  

    if (status > 0)
    {
        printf("ERROR: ffgpf_ status = %d\n", status);
        goto errstatus;
    }
    if (anynull == 0)
       printf("ERROR: ffgpf_ did not detect null values\n");


    /*
      ##########################################
      #  close and reopen file multiple times  #
      ##########################################
    */

    for (ii = 0; ii < 10; ii++)
    {
      if (ffclos(fptr, &status) > 0)
      {
        printf("ERROR in ftclos (1) = %d", status);
        goto errstatus;
      }

      if (fits_open_file(&fptr, filename, READWRITE, &status) > 0)
      {
        printf("ERROR: ffopen open file status = %d\n", status);
        goto errstatus;
      }
    }
    printf("\nClosed then reopened the FITS file 10 times.\n");
    printf("HDU number = %d\n", ffghdn(fptr, &hdunum));

    filename[0] = '\0';
    ffflnm(fptr, filename, &status);

    ffflmd(fptr, &filemode, &status);
    printf("Name of file = %s, I/O mode = %d\n", filename, filemode);

    /*
      ############################
      #  read single keywords    #
      ############################
    */

    simple = 0;
    bitpix = 0;
    naxis = 0;
    naxes[0] = 0;
    naxes[1] = 0;
    pcount = -99;
    gcount =  -99;
    extend = -99;
    printf("\nRead back keywords:\n");
    ffghpr(fptr, 99, &simple, &bitpix, &naxis, naxes, &pcount,
           &gcount, &extend, &status);
    printf("simple = %d, bitpix = %d, naxis = %d, naxes = (%ld, %ld)\n",
           simple, bitpix, naxis, naxes[0], naxes[1]);
    printf("  pcount = %ld, gcount = %ld, extend = %d\n",
               pcount, gcount, extend);

    ffgrec(fptr, 9, card, &status);
    printf("%s\n", card);
    if (strncmp(card, "KEY_PREC= 'This", 15) )
       printf("ERROR in ffgrec\n");

    ffgkyn(fptr, 9, keyword, value, comment, &status);
    printf("%s : %s : %s :\n",keyword, value, comment);
    if (strncmp(keyword, "KEY_PREC", 8) )
       printf("ERROR in ffgkyn: %s\n", keyword);

    ffgcrd(fptr, keyword, card, &status);
    printf("%s\n", card);

    if (strncmp(keyword, card, 8) )
       printf("ERROR in ffgcrd: %s\n", keyword);

    ffgkey(fptr, "KY_PKNS1", value, comment, &status);
    printf("KY_PKNS1 : %s : %s :\n", value, comment);

    if (strncmp(value, "'first string'", 14) )
       printf("ERROR in ffgkey: %s\n", value);

    ffgkys(fptr, "key_pkys", iskey, comment, &status);
    printf("KEY_PKYS %s %s %d\n", iskey, comment, status);

    ffgkyl(fptr, "key_pkyl", &ilkey, comment, &status);
    printf("KEY_PKYL %d %s %d\n", ilkey, comment, status);

    ffgkyj(fptr, "KEY_PKYJ", &ijkey, comment, &status);
    printf("KEY_PKYJ %ld %s %d\n",ijkey, comment, status);

    ffgkye(fptr, "KEY_PKYJ", &iekey, comment, &status);
    printf("KEY_PKYJ %f %s %d\n",iekey, comment, status);

    ffgkyd(fptr, "KEY_PKYJ", &idkey, comment, &status);
    printf("KEY_PKYJ %f %s %d\n",idkey, comment, status);

    if (ijkey != 11 || iekey != 11. || idkey != 11.)
       printf("ERROR in ffgky[jed]: %ld, %f, %f\n",ijkey, iekey, idkey);

    iskey[0] = '\0';
    ffgky(fptr, TSTRING, "key_pkys", iskey, comment, &status);
    printf("KEY_PKY S %s %s %d\n", iskey, comment, status);

    ilkey = 0;
    ffgky(fptr, TLOGICAL, "key_pkyl", &ilkey, comment, &status);
    printf("KEY_PKY L %d %s %d\n", ilkey, comment, status);

    ffgky(fptr, TBYTE, "KEY_PKYJ", &cval, comment, &status);
    printf("KEY_PKY BYTE %d %s %d\n",cval, comment, status);

    ffgky(fptr, TSHORT, "KEY_PKYJ", &ishtkey, comment, &status);
    printf("KEY_PKY SHORT %d %s %d\n",ishtkey, comment, status);

    ffgky(fptr, TINT, "KEY_PKYJ", &ilkey, comment, &status);
    printf("KEY_PKY INT %d %s %d\n",ilkey, comment, status);

    ijkey = 0;
    ffgky(fptr, TLONG, "KEY_PKYJ", &ijkey, comment, &status);
    printf("KEY_PKY J %ld %s %d\n",ijkey, comment, status);

    iekey = 0;
    ffgky(fptr, TFLOAT, "KEY_PKYE", &iekey, comment, &status);
    printf("KEY_PKY E %f %s %d\n",iekey, comment, status);

    idkey = 0;
    ffgky(fptr, TDOUBLE, "KEY_PKYD", &idkey, comment, &status);
    printf("KEY_PKY D %f %s %d\n",idkey, comment, status);

    ffgkyd(fptr, "KEY_PKYF", &idkey, comment, &status);
    printf("KEY_PKYF %f %s %d\n",idkey, comment, status);

    ffgkyd(fptr, "KEY_PKYE", &idkey, comment, &status);
    printf("KEY_PKYE %f %s %d\n",idkey, comment, status);

    ffgkyd(fptr, "KEY_PKYG", &idkey, comment, &status);
    printf("KEY_PKYG %.14f %s %d\n",idkey, comment, status);

    ffgkyd(fptr, "KEY_PKYD", &idkey, comment, &status);
    printf("KEY_PKYD %.14f %s %d\n",idkey, comment, status);

    ffgkyc(fptr, "KEY_PKYC", inekey, comment, &status);
    printf("KEY_PKYC %f %f %s %d\n",inekey[0], inekey[1], comment, status);

    ffgkyc(fptr, "KEY_PKFC", inekey, comment, &status);
    printf("KEY_PKFC %f %f %s %d\n",inekey[0], inekey[1], comment, status);

    ffgkym(fptr, "KEY_PKYM", indkey, comment, &status);
    printf("KEY_PKYM %f %f %s %d\n",indkey[0], indkey[1], comment, status);

    ffgkym(fptr, "KEY_PKFM", indkey, comment, &status);
    printf("KEY_PKFM %f %f %s %d\n",indkey[0], indkey[1], comment, status);

    ffgkyt(fptr, "KEY_PKYT", &ijkey, &idkey, comment, &status);
    printf("KEY_PKYT %ld %.14f %s %d\n",ijkey, idkey, comment, status);

    ffpunt(fptr, "KEY_PKYJ", "km/s/Mpc", &status);
    ijkey = 0;
    ffgky(fptr, TLONG, "KEY_PKYJ", &ijkey, comment, &status);
    printf("KEY_PKY J %ld %s %d\n",ijkey, comment, status);
    ffgunt(fptr,"KEY_PKYJ", comment, &status);
    printf("KEY_PKY units = %s\n",comment);

    ffpunt(fptr, "KEY_PKYJ", "", &status);
    ijkey = 0;
    ffgky(fptr, TLONG, "KEY_PKYJ", &ijkey, comment, &status);
    printf("KEY_PKY J %ld %s %d\n",ijkey, comment, status);
    ffgunt(fptr,"KEY_PKYJ", comment, &status);
    printf("KEY_PKY units = %s\n",comment);

    ffpunt(fptr, "KEY_PKYJ", "feet/second/second", &status);
    ijkey = 0;
    ffgky(fptr, TLONG, "KEY_PKYJ", &ijkey, comment, &status);
    printf("KEY_PKY J %ld %s %d\n",ijkey, comment, status);
    ffgunt(fptr,"KEY_PKYJ", comment, &status);
    printf("KEY_PKY units = %s\n",comment);

    ffgkls(fptr, "key_pkls", &lsptr, comment, &status);
    printf("KEY_PKLS long string value = \n%s\n", lsptr);

    /* free the memory for the long string value */
    fits_free_memory(lsptr, &status);

    /* get size and position in header */
    ffghps(fptr, &existkeys, &keynum, &status);
    printf("header contains %d keywords; located at keyword %d \n",existkeys,
            keynum);

    /*
      ############################
      #  read array keywords     #
      ############################
    */
    ffgkns(fptr, "ky_pkns", 1, 3, inskey, &nfound, &status);
    printf("ffgkns:  %s, %s, %s\n", inskey[0], inskey[1], inskey[2]);
    if (nfound != 3 || status > 0)
       printf("\nERROR in ffgkns %d, %d\n", nfound, status);

    ffgknl(fptr, "ky_pknl", 1, 3, inlkey, &nfound, &status);
    printf("ffgknl:  %d, %d, %d\n", inlkey[0], inlkey[1], inlkey[2]);
    if (nfound != 3 || status > 0)
       printf("\nERROR in ffgknl %d, %d\n", nfound, status);

    ffgknj(fptr, "ky_pknj", 1, 3, injkey, &nfound, &status);
    printf("ffgknj:  %ld, %ld, %ld\n", injkey[0], injkey[1], injkey[2]);
    if (nfound != 3 || status > 0)
       printf("\nERROR in ffgknj %d, %d\n", nfound, status);

    ffgkne(fptr, "ky_pkne", 1, 3, inekey, &nfound, &status);
    printf("ffgkne:  %f, %f, %f\n", inekey[0], inekey[1], inekey[2]);
    if (nfound != 3 || status > 0)
       printf("\nERROR in ffgkne %d, %d\n", nfound, status);

    ffgknd(fptr, "ky_pknd", 1, 3, indkey, &nfound, &status);
    printf("ffgknd:  %f, %f, %f\n", indkey[0], indkey[1], indkey[2]);
    if (nfound != 3 || status > 0)
       printf("\nERROR in ffgknd %d, %d\n", nfound, status);

    /* get position of HISTORY keyword for subsequent deletes and inserts */
    ffgcrd(fptr, "HISTORY", card, &status);
    ffghps(fptr, &existkeys, &keynum, &status);
    keynum -= 2;

    printf("\nBefore deleting the HISTORY and DATE keywords...\n");
    for (ii = keynum; ii <= keynum + 3; ii++)
    {
        ffgrec(fptr, ii, card, &status);
        printf("%.8s\n", card);  /* don't print date value, so that */
    }                            /* the output will always be the same */
    /*
      ############################
      #  delete keywords         #
      ############################
    */

    ffdrec(fptr, keynum + 1, &status);
    ffdkey(fptr, "DATE", &status);

    printf("\nAfter deleting the keywords...\n");
    for (ii = keynum; ii <= keynum + 1; ii++)
    {
        ffgrec(fptr, ii, card, &status);
        printf("%s\n", card);
    }

    if (status > 0)
       printf("\nERROR deleting keywords\n");
    /*
      ############################
      #  insert keywords         #
      ############################
    */
    keynum += 4;
    ffirec(fptr, keynum - 3, "KY_IREC = 'This keyword inserted by fxirec'",
           &status);
    ffikys(fptr, "KY_IKYS", "insert_value_string", "ikys comment", &status);
    ffikyj(fptr, "KY_IKYJ", 49, "ikyj comment", &status);
    ffikyl(fptr, "KY_IKYL", 1, "ikyl comment", &status);
    ffikye(fptr, "KY_IKYE", 12.3456f, 4, "ikye comment", &status);
    ffikyd(fptr, "KY_IKYD", 12.345678901234567, 14, "ikyd comment", &status);
    ffikyf(fptr, "KY_IKYF", 12.3456f, 4, "ikyf comment", &status);
    ffikyg(fptr, "KY_IKYG", 12.345678901234567, 13, "ikyg comment", &status);

    printf("\nAfter inserting the keywords...\n");
    for (ii = keynum - 4; ii <= keynum + 5; ii++)
    {
        ffgrec(fptr, ii, card, &status);
        printf("%s\n", card);
    }

    if (status > 0)
       printf("\nERROR inserting keywords\n");
    /*
      ############################
      #  modify keywords         #
      ############################
    */
    ffmrec(fptr, keynum - 4, "COMMENT   This keyword was modified by fxmrec", &status);
    ffmcrd(fptr, "KY_IREC", "KY_MREC = 'This keyword was modified by fxmcrd'",
            &status);
    ffmnam(fptr, "KY_IKYS", "NEWIKYS", &status);

    ffmcom(fptr, "KY_IKYJ","This is a modified comment", &status);
    ffmkyj(fptr, "KY_IKYJ", 50, "&", &status);
    ffmkyl(fptr, "KY_IKYL", 0, "&", &status);
    ffmkys(fptr, "NEWIKYS", "modified_string", "&", &status);
    ffmkye(fptr, "KY_IKYE", -12.3456f, 4, "&", &status);
    ffmkyd(fptr, "KY_IKYD", -12.345678901234567, 14, "modified comment",
            &status);
    ffmkyf(fptr, "KY_IKYF", -12.3456f, 4, "&", &status);
    ffmkyg(fptr, "KY_IKYG", -12.345678901234567, 13, "&", &status);

    printf("\nAfter modifying the keywords...\n");
    for (ii = keynum - 4; ii <= keynum + 5; ii++)
    {
        ffgrec(fptr, ii, card, &status);
        printf("%s\n", card);
    }
    if (status > 0)
       printf("\nERROR modifying keywords\n");

    /*
      ############################
      #  update keywords         #
      ############################
    */
    ffucrd(fptr, "KY_MREC", "KY_UCRD = 'This keyword was updated by fxucrd'",
            &status);

    ffukyj(fptr, "KY_IKYJ", 51, "&", &status);
    ffukyl(fptr, "KY_IKYL", 1, "&", &status);
    ffukys(fptr, "NEWIKYS", "updated_string", "&", &status);
    ffukye(fptr, "KY_IKYE", -13.3456f, 4, "&", &status);
    ffukyd(fptr, "KY_IKYD", -13.345678901234567, 14, "modified comment",
            &status);
    ffukyf(fptr, "KY_IKYF", -13.3456f, 4, "&", &status);
    ffukyg(fptr, "KY_IKYG", -13.345678901234567, 13, "&", &status);

    printf("\nAfter updating the keywords...\n");
    for (ii = keynum - 4; ii <= keynum + 5; ii++)
    {
        ffgrec(fptr, ii, card, &status);
        printf("%s\n", card);
    }
    if (status > 0)
       printf("\nERROR modifying keywords\n");

    /* move to top of header and find keywords using wild cards */
    ffgrec(fptr, 0, card, &status);

    printf("\nKeywords found using wildcard search (should be 13)...\n");
    nfound = 0;
    while (!ffgnxk(fptr,inclist, 2, exclist, 2, card, &status))
    {
        nfound++;
        printf("%s\n", card);
    }
    if (nfound != 13)
    {
       printf("\nERROR reading keywords using wildcards (ffgnxk)\n");
       goto errstatus;
    }
    status = 0;

    /*
      ############################
      #  copy index keyword      #
      ############################
    */
    ffcpky(fptr, fptr, 1, 4, "KY_PKNE", &status);
    ffgkne(fptr, "ky_pkne", 2, 4, inekey, &nfound, &status);
    printf("\nCopied keyword: ffgkne:  %f, %f, %f\n", inekey[0], inekey[1],
           inekey[2]);

    if (status > 0)
    {
       printf("\nERROR in ffgkne %d, %d\n", nfound, status);
       goto errstatus;
    }

    /*
      ######################################
      #  modify header using template file #
      ######################################
    */
    if (ffpktp(fptr, templt, &status))
    {
       printf("\nERROR returned by ffpktp:\n");
       printf("Could not open or process the file 'testprog.tpt'.\n");
       printf("  This file is included with the CFITSIO distribution\n");
       printf("  and should be copied into the current directory\n");
       printf("  before running the testprog program.\n");
       status = 0;
    }
    printf("Updated header using template file (ffpktp)\n");
    /*
      ############################
      #  create binary table     #
      ############################
    */

    strcpy(tform[0], "15A");
    strcpy(tform[1], "1L");
    strcpy(tform[2], "16X");
    strcpy(tform[3], "1B");
    strcpy(tform[4], "1I");
    strcpy(tform[5], "1J");
    strcpy(tform[6], "1E");
    strcpy(tform[7], "1D");
    strcpy(tform[8], "1C");
    strcpy(tform[9], "1M");

    strcpy(ttype[0], "Avalue");
    strcpy(ttype[1], "Lvalue");
    strcpy(ttype[2], "Xvalue");
    strcpy(ttype[3], "Bvalue");
    strcpy(ttype[4], "Ivalue");
    strcpy(ttype[5], "Jvalue");
    strcpy(ttype[6], "Evalue");
    strcpy(ttype[7], "Dvalue");
    strcpy(ttype[8], "Cvalue");
    strcpy(ttype[9], "Mvalue");

    strcpy(tunit[0], "");
    strcpy(tunit[1], "m**2");
    strcpy(tunit[2], "cm");
    strcpy(tunit[3], "erg/s");
    strcpy(tunit[4], "km/s");
    strcpy(tunit[5], "");
    strcpy(tunit[6], "");
    strcpy(tunit[7], "");
    strcpy(tunit[8], "");
    strcpy(tunit[9], "");

    nrows = 21;
    tfields = 10;
    pcount = 0;

/*
    ffcrtb(fptr, BINARY_TBL, nrows, tfields, ttype, tform, tunit, binname,
            &status);
*/
    ffibin(fptr, nrows, tfields, ttype, tform, tunit, binname, 0L,
            &status);

    printf("\nffibin status = %d\n", status);
    printf("HDU number = %d\n", ffghdn(fptr, &hdunum));

    /* get size and position in header, and reserve space for more keywords */
    ffghps(fptr, &existkeys, &keynum, &status);
    printf("header contains %d keywords; located at keyword %d \n",existkeys,
            keynum);

    morekeys = 40;
    ffhdef(fptr, morekeys, &status);
    ffghsp(fptr, &existkeys, &morekeys, &status);
    printf("header contains %d keywords with room for %d more\n",existkeys,
            morekeys);

    fftnul(fptr, 4, 99, &status);   /* define null value for int cols */
    fftnul(fptr, 5, 99, &status);
    fftnul(fptr, 6, 99, &status);

    extvers = 1;
    ffpkyj(fptr, "EXTVER", extvers, "extension version number", &status);
    ffpkyj(fptr, "TNULL4", 99, "value for undefined pixels", &status);
    ffpkyj(fptr, "TNULL5", 99, "value for undefined pixels", &status);
    ffpkyj(fptr, "TNULL6", 99, "value for undefined pixels", &status);

    naxis = 3;
    naxes[0] = 1;
    naxes[1] = 2;
    naxes[2] = 8;
    ffptdm(fptr, 3, naxis, naxes, &status);

    naxis = 0;
    naxes[0] = 0;
    naxes[1] = 0;
    naxes[2] = 0;
    ffgtdm(fptr, 3, 3, &naxis, naxes, &status);
    ffgkys(fptr, "TDIM3", iskey, comment, &status);
    printf("TDIM3 = %s, %d, %ld, %ld, %ld\n", iskey, naxis, naxes[0],
         naxes[1], naxes[2]);

    ffrdef(fptr, &status);  /* force header to be scanned (not required) */

    /*
      ############################
      #  write data to columns   #
      ############################
    */

    /* initialize arrays of values to write to table */
    signval = -1;
    for (ii = 0; ii < 21; ii++)
    {
        signval *= -1;
        boutarray[ii] = (unsigned char) (ii + 1);
        ioutarray[ii] = (short) ((ii + 1) * signval);
        joutarray[ii] = (ii + 1) * signval;
        koutarray[ii] = (ii + 1) * signval;
        eoutarray[ii] = (float) ((ii + 1) * signval);
        doutarray[ii] = (ii + 1) * signval;
    }

    ffpcls(fptr, 1, 1, 1, 3, onskey, &status);  /* write string values */
    ffpclu(fptr, 1, 4, 1, 1, &status);  /* write null value */

    larray[0] = 0;
    larray[1] = 1;
    larray[2] = 0;
    larray[3] = 0;
    larray[4] = 1;
    larray[5] = 1;
    larray[6] = 0;
    larray[7] = 0;
    larray[8] = 0;
    larray[9] = 1;
    larray[10] = 1;
    larray[11] = 1;
    larray[12] = 0;
    larray[13] = 0;
    larray[14] = 0;
    larray[15] = 0;
    larray[16] = 1;
    larray[17] = 1;
    larray[18] = 1;
    larray[19] = 1;
    larray[20] = 0;
    larray[21] = 0;
    larray[22] = 0;
    larray[23] = 0;
    larray[24] = 0;
    larray[25] = 1;
    larray[26] = 1;
    larray[27] = 1;
    larray[28] = 1;
    larray[29] = 1;
    larray[30] = 0;
    larray[31] = 0;
    larray[32] = 0;
    larray[33] = 0;
    larray[34] = 0;
    larray[35] = 0;


    ffpclx(fptr, 3, 1, 1, 36, larray, &status); /*write bits*/

    for (ii = 4; ii < 9; ii++)   /* loop over cols 4 - 8 */
    {
        ffpclb(fptr, ii, 1, 1, 2, boutarray, &status);
        if (status == NUM_OVERFLOW)
            status = 0;
        ffpcli(fptr, ii, 3, 1, 2, &ioutarray[2], &status); 
        if (status == NUM_OVERFLOW)
            status = 0;
        ffpclk(fptr, ii, 5, 1, 2, &koutarray[4], &status); 
        if (status == NUM_OVERFLOW)
            status = 0;
        ffpcle(fptr, ii, 7, 1, 2, &eoutarray[6], &status);
        if (status == NUM_OVERFLOW)
            status = 0;
        ffpcld(fptr, ii, 9, 1, 2, &doutarray[8], &status);
        if (status == NUM_OVERFLOW)
            status = 0;

        ffpclu(fptr, ii, 11, 1, 1, &status);  /* write null value */
    }

    ffpclc(fptr, 9, 1, 1, 10, eoutarray, &status);
    ffpclm(fptr, 10, 1, 1, 10, doutarray, &status);

    for (ii = 4; ii < 9; ii++)   /* loop over cols 4 - 8 */
    {
        ffpcnb(fptr, ii, 12, 1, 2, &boutarray[11], 13, &status);
        if (status == NUM_OVERFLOW)
            status = 0;
        ffpcni(fptr, ii, 14, 1, 2, &ioutarray[13], 15, &status); 
        if (status == NUM_OVERFLOW)
            status = 0;
        ffpcnk(fptr, ii, 16, 1, 2, &koutarray[15], 17, &status); 
        if (status == NUM_OVERFLOW)
            status = 0;
        ffpcne(fptr, ii, 18, 1, 2, &eoutarray[17], 19., &status);
        if (status == NUM_OVERFLOW)
            status = 0;
        ffpcnd(fptr, ii, 20, 1, 2, &doutarray[19], 21., &status);
        if (status == NUM_OVERFLOW)
            status = 0;

    }
    ffpcll(fptr, 2, 1, 1, 21, larray, &status); /*write logicals*/
    ffpclu(fptr, 2, 11, 1, 1, &status);  /* write null value */
    printf("ffpcl_ status = %d\n", status);

    /*
      #########################################
      #  get information about the columns    #
      #########################################
    */

    printf("\nFind the column numbers; a returned status value of 237 is");
    printf("\nexpected and indicates that more than one column name matches");
    printf("\nthe input column name template.  Status = 219 indicates that");
    printf("\nthere was no matching column name.");

    ffgcno(fptr, 0, "Xvalue", &colnum, &status);
    printf("\nColumn Xvalue is number %d; status = %d.\n", colnum, status);

    while (status != COL_NOT_FOUND)
    {
      ffgcnn(fptr, 1, "*ue", colname, &colnum, &status);
      printf("Column %s is number %d; status = %d.\n", 
           colname, colnum, status);
    }
    status = 0;

    printf("\nInformation about each column:\n");

    for (ii = 0; ii < tfields; ii++)
    {
      ffgtcl(fptr, ii + 1, &typecode, &repeat, &width, &status);
      printf("%4s %3d %2ld %2ld", tform[ii], typecode, repeat, width);
      ffgbcl(fptr, ii + 1, ttype[0], tunit[0], cvalstr, &repeat, &scale,
           &zero, &jnulval, tdisp, &status);
      printf(" %s, %s, %c, %ld, %f, %f, %ld, %s.\n",
         ttype[0], tunit[0], cvalstr[0], repeat, scale, zero, jnulval, tdisp);
    }

    printf("\n");

    /*
      ###############################################
      #  insert ASCII table before the binary table #
      ###############################################
    */

    if (ffmrhd(fptr, -1, &hdutype, &status) > 0)
        goto errstatus;

    strcpy(tform[0], "A15");
    strcpy(tform[1], "I10");
    strcpy(tform[2], "F14.6");
    strcpy(tform[3], "E12.5");
    strcpy(tform[4], "D21.14");

    strcpy(ttype[0], "Name");
    strcpy(ttype[1], "Ivalue");
    strcpy(ttype[2], "Fvalue");
    strcpy(ttype[3], "Evalue");
    strcpy(ttype[4], "Dvalue");

    strcpy(tunit[0], "");
    strcpy(tunit[1], "m**2");
    strcpy(tunit[2], "cm");
    strcpy(tunit[3], "erg/s");
    strcpy(tunit[4], "km/s");

    rowlen = 76;
    nrows = 11;
    tfields = 5;

    ffitab(fptr, rowlen, nrows, tfields, ttype, tbcol, tform, tunit, tblname,
            &status);
    printf("ffitab status = %d\n", status);
    printf("HDU number = %d\n", ffghdn(fptr, &hdunum));

    ffsnul(fptr, 1, "null1", &status);   /* define null value for int cols */
    ffsnul(fptr, 2, "null2", &status);
    ffsnul(fptr, 3, "null3", &status);
    ffsnul(fptr, 4, "null4", &status);
    ffsnul(fptr, 5, "null5", &status);
 
    extvers = 2;
    ffpkyj(fptr, "EXTVER", extvers, "extension version number", &status);

    ffpkys(fptr, "TNULL1", "null1", "value for undefined pixels", &status);
    ffpkys(fptr, "TNULL2", "null2", "value for undefined pixels", &status);
    ffpkys(fptr, "TNULL3", "null3", "value for undefined pixels", &status);
    ffpkys(fptr, "TNULL4", "null4", "value for undefined pixels", &status);
    ffpkys(fptr, "TNULL5", "null5", "value for undefined pixels", &status);

    if (status > 0)
        goto errstatus;

    /*
      ############################
      #  write data to columns   #
      ############################
    */

    /* initialize arrays of values to write to table */
    for (ii = 0; ii < 21; ii++)
    {
        boutarray[ii] = (unsigned char) (ii + 1);
        ioutarray[ii] = (short) (ii + 1);
        joutarray[ii] = ii + 1;
        eoutarray[ii] = (float) (ii + 1);
        doutarray[ii] = ii + 1;
    }

    ffpcls(fptr, 1, 1, 1, 3, onskey, &status);  /* write string values */
    ffpclu(fptr, 1, 4, 1, 1, &status);  /* write null value */

    for (ii = 2; ii < 6; ii++)   /* loop over cols 2 - 5 */
    {
        ffpclb(fptr, ii, 1, 1, 2, boutarray, &status);  /* char array */
        if (status == NUM_OVERFLOW)
            status = 0;
        ffpcli(fptr, ii, 3, 1, 2, &ioutarray[2], &status);  /* short array */
        if (status == NUM_OVERFLOW)
            status = 0;
        ffpclj(fptr, ii, 5, 1, 2, &joutarray[4], &status);  /* long array */
        if (status == NUM_OVERFLOW)
            status = 0;
        ffpcle(fptr, ii, 7, 1, 2, &eoutarray[6], &status);  /* float array */
        if (status == NUM_OVERFLOW)
            status = 0;
        ffpcld(fptr, ii, 9, 1, 2, &doutarray[8], &status);  /* double array */
        if (status == NUM_OVERFLOW)
            status = 0;

        ffpclu(fptr, ii, 11, 1, 1, &status);  /* write null value */
    }
    printf("ffpcl_ status = %d\n", status);

    /*
      ################################
      #  read data from ASCII table  #
      ################################
    */
    ffghtb(fptr, 99, &rowlen, &nrows, &tfields, ttype, tbcol, 
           tform, tunit, tblname, &status);

    printf("\nASCII table: rowlen, nrows, tfields, extname: %ld %ld %d %s\n",
           rowlen, nrows, tfields, tblname);

    for (ii = 0; ii < tfields; ii++)
      printf("%8s %3ld %8s %8s \n", ttype[ii], tbcol[ii], 
                                   tform[ii], tunit[ii]);

    nrows = 11;
    ffgcvs(fptr, 1, 1, 1, nrows, "UNDEFINED", inskey,   &anynull, &status);
    ffgcvb(fptr, 2, 1, 1, nrows, 99, binarray, &anynull, &status);
    ffgcvi(fptr, 2, 1, 1, nrows, 99, iinarray, &anynull, &status);
    ffgcvj(fptr, 3, 1, 1, nrows, 99, jinarray, &anynull, &status);
    ffgcve(fptr, 4, 1, 1, nrows, 99., einarray, &anynull, &status);
    ffgcvd(fptr, 5, 1, 1, nrows, 99., dinarray, &anynull, &status);

    printf("\nData values read from ASCII table:\n");
    for (ii = 0; ii < nrows; ii++)
    {
      printf("%15s %2d %2d %2ld %4.1f %4.1f\n", inskey[ii], binarray[ii],
           iinarray[ii], jinarray[ii], einarray[ii], dinarray[ii]); 
    }

    ffgtbb(fptr, 1, 20, 78, uchars, &status);
    uchars[78] = '\0';
    printf("\n%s\n", uchars);
    ffptbb(fptr, 1, 20, 78, uchars, &status);

    /*
      #########################################
      #  get information about the columns    #
      #########################################
    */

    ffgcno(fptr, 0, "name", &colnum, &status);
    printf("\nColumn name is number %d; status = %d.\n", colnum, status);

    while (status != COL_NOT_FOUND)
    {
      ffgcnn(fptr, 1, "*ue", colname, &colnum, &status);
      printf("Column %s is number %d; status = %d.\n", 
           colname, colnum, status);
    }
    status = 0;

    for (ii = 0; ii < tfields; ii++)
    {
      ffgtcl(fptr, ii + 1, &typecode, &repeat, &width, &status);
      printf("%4s %3d %2ld %2ld", tform[ii], typecode, repeat, width);
      ffgacl(fptr, ii + 1, ttype[0], tbcol, tunit[0], tform[0], &scale,
           &zero, nulstr, tdisp, &status);
      printf(" %s, %ld, %s, %s, %f, %f, %s, %s.\n",
         ttype[0], tbcol[0], tunit[0], tform[0], scale, zero,
         nulstr, tdisp);
    }

    printf("\n");

    /*
      ###############################################
      #  test the insert/delete row/column routines #
      ###############################################
    */

    if (ffirow(fptr, 2, 3, &status) > 0)
        goto errstatus;

    nrows = 14;
    ffgcvs(fptr, 1, 1, 1, nrows, "UNDEFINED", inskey,   &anynull, &status);
    ffgcvb(fptr, 2, 1, 1, nrows, 99, binarray, &anynull, &status);
    ffgcvi(fptr, 2, 1, 1, nrows, 99, iinarray, &anynull, &status);
    ffgcvj(fptr, 3, 1, 1, nrows, 99, jinarray, &anynull, &status);
    ffgcve(fptr, 4, 1, 1, nrows, 99., einarray, &anynull, &status);
    ffgcvd(fptr, 5, 1, 1, nrows, 99., dinarray, &anynull, &status);


    printf("\nData values after inserting 3 rows after row 2:\n");
    for (ii = 0; ii < nrows; ii++)
    {
      printf("%15s %2d %2d %2ld %4.1f %4.1f\n",  inskey[ii], binarray[ii],
          iinarray[ii], jinarray[ii], einarray[ii], dinarray[ii]);
    }

    if (ffdrow(fptr, 10, 2, &status) > 0)
        goto errstatus;

    nrows = 12;
    ffgcvs(fptr, 1, 1, 1, nrows, "UNDEFINED", inskey,   &anynull, &status);
    ffgcvb(fptr, 2, 1, 1, nrows, 99, binarray, &anynull, &status);
    ffgcvi(fptr, 2, 1, 1, nrows, 99, iinarray, &anynull, &status);
    ffgcvj(fptr, 3, 1, 1, nrows, 99, jinarray, &anynull, &status);
    ffgcve(fptr, 4, 1, 1, nrows, 99., einarray, &anynull, &status);
    ffgcvd(fptr, 5, 1, 1, nrows, 99., dinarray, &anynull, &status);

    printf("\nData values after deleting 2 rows at row 10:\n");
    for (ii = 0; ii < nrows; ii++)
    {
      printf("%15s %2d %2d %2ld %4.1f %4.1f\n",  inskey[ii], binarray[ii],
          iinarray[ii], jinarray[ii], einarray[ii], dinarray[ii]);
    }
    if (ffdcol(fptr, 3, &status) > 0)
        goto errstatus;

    ffgcvs(fptr, 1, 1, 1, nrows, "UNDEFINED", inskey,   &anynull, &status);
    ffgcvb(fptr, 2, 1, 1, nrows, 99, binarray, &anynull, &status);
    ffgcvi(fptr, 2, 1, 1, nrows, 99, iinarray, &anynull, &status);
    ffgcve(fptr, 3, 1, 1, nrows, 99., einarray, &anynull, &status);
    ffgcvd(fptr, 4, 1, 1, nrows, 99., dinarray, &anynull, &status);

    printf("\nData values after deleting column 3:\n");
    for (ii = 0; ii < nrows; ii++)
    {
      printf("%15s %2d %2d %4.1f %4.1f\n", inskey[ii], binarray[ii],
          iinarray[ii], einarray[ii], dinarray[ii]);
    }

    if (fficol(fptr, 5, "INSERT_COL", "F14.6", &status) > 0)
        goto errstatus;

    ffgcvs(fptr, 1, 1, 1, nrows, "UNDEFINED", inskey,   &anynull, &status);
    ffgcvb(fptr, 2, 1, 1, nrows, 99, binarray, &anynull, &status);
    ffgcvi(fptr, 2, 1, 1, nrows, 99, iinarray, &anynull, &status);
    ffgcve(fptr, 3, 1, 1, nrows, 99., einarray, &anynull, &status);
    ffgcvd(fptr, 4, 1, 1, nrows, 99., dinarray, &anynull, &status);
    ffgcvj(fptr, 5, 1, 1, nrows, 99, jinarray, &anynull, &status);

    printf("\nData values after inserting column 5:\n");
    for (ii = 0; ii < nrows; ii++)
    {
      printf("%15s %2d %2d %4.1f %4.1f %ld\n", inskey[ii], binarray[ii],
          iinarray[ii], einarray[ii], dinarray[ii] , jinarray[ii]);
    }

    /*
      ############################################################
      #  create a temporary file and copy the ASCII table to it, #
      #  column by column.                                       #
      ############################################################
    */
    bitpix = 16;
    naxis = 0;

    strcpy(filename, "!t1q2s3v6.tmp");
    ffinit(&tmpfptr, filename, &status);
    printf("Create temporary file: ffinit status = %d\n", status);

    ffiimg(tmpfptr, bitpix, naxis, naxes, &status);
    printf("\nCreate null primary array: ffiimg status = %d\n", status);

    /* create an empty table with 12 rows and 0 columns */
    nrows = 12;
    tfields = 0;
    rowlen = 0;
    ffitab(tmpfptr, rowlen, nrows, tfields, ttype, tbcol, tform, tunit,
           tblname, &status);
    printf("\nCreate ASCII table with 0 columns: ffitab status = %d\n",
           status);

    /* copy columns from one table to the other */
    ffcpcl(fptr, tmpfptr, 4, 1, TRUE, &status);
    printf("copy column, ffcpcl status = %d\n", status);
    ffcpcl(fptr, tmpfptr, 3, 1, TRUE, &status);
    printf("copy column, ffcpcl status = %d\n", status);
    ffcpcl(fptr, tmpfptr, 2, 1, TRUE, &status);
    printf("copy column, ffcpcl status = %d\n", status);
    ffcpcl(fptr, tmpfptr, 1, 1, TRUE, &status);
    printf("copy column, ffcpcl status = %d\n", status);

    /* now repeat by copying ASCII input to Binary output table */
    ffibin(tmpfptr, nrows, tfields, ttype, tform, tunit,
           tblname,  0L, &status);
    printf("\nCreate Binary table with 0 columns: ffibin status = %d\n",
           status);

    /* copy columns from one table to the other */
    ffcpcl(fptr, tmpfptr, 4, 1, TRUE, &status);
    printf("copy column, ffcpcl status = %d\n", status);
    ffcpcl(fptr, tmpfptr, 3, 1, TRUE, &status);
    printf("copy column, ffcpcl status = %d\n", status);
    ffcpcl(fptr, tmpfptr, 2, 1, TRUE, &status);
    printf("copy column, ffcpcl status = %d\n", status);
    ffcpcl(fptr, tmpfptr, 1, 1, TRUE, &status);
    printf("copy column, ffcpcl status = %d\n", status);


/*
    ffclos(tmpfptr, &status);
    printf("Close the tmp file: ffclos status = %d\n", status);
*/

    ffdelt(tmpfptr, &status);  
    printf("Delete the tmp file: ffdelt status = %d\n", status);

    if (status > 0)
    {
        goto errstatus;
    }

    /*
      ################################
      #  read data from binary table #
      ################################
    */

    if (ffmrhd(fptr, 1, &hdutype, &status) > 0)
        goto errstatus;

    printf("HDU number = %d\n", ffghdn(fptr, &hdunum));

    ffghsp(fptr, &existkeys, &morekeys, &status);
    printf("header contains %d keywords with room for %d more\n",existkeys,
            morekeys);

    ffghbn(fptr, 99, &nrows, &tfields, ttype, 
           tform, tunit, binname, &pcount, &status);

    printf("\nBinary table: nrows, tfields, extname, pcount: %ld %d %s %ld\n",
           nrows, tfields, binname, pcount);

    for (ii = 0; ii < tfields; ii++)
      printf("%8s %8s %8s \n", ttype[ii], tform[ii], tunit[ii]);

    for (ii = 0; ii < 40; ii++)
        larray[ii] = 0;

    printf("\nData values read from binary table:\n");
    printf("  Bit column (X) data values: \n\n");

    ffgcx(fptr, 3, 1, 1, 36, larray, &status);
    for (jj = 0; jj < 5; jj++)
    {
      for (ii = 0; ii < 8; ii++)
        printf("%1d",larray[jj * 8 + ii]);
      printf(" ");
    }

    for (ii = 0; ii < nrows; ii++)
    {
      larray[ii] = 0;
      xinarray[ii] = 0;
      binarray[ii] = 0;
      iinarray[ii] = 0; 
      kinarray[ii] = 0;
      einarray[ii] = 0.; 
      dinarray[ii] = 0.;
      cinarray[ii * 2] = 0.; 
      minarray[ii * 2] = 0.;
      cinarray[ii * 2 + 1] = 0.; 
      minarray[ii * 2 + 1] = 0.;
    }

    printf("\n\n");
    ffgcvs(fptr, 1, 4, 1, 1, "",  inskey,   &anynull, &status);
    printf("null string column value = -%s- (should be --)\n",inskey[0]);

    nrows = 21;
    ffgcvs(fptr, 1, 1, 1, nrows, "NOT DEFINED",  inskey,   &anynull, &status);
    ffgcl( fptr, 2, 1, 1, nrows, larray, &status);
    ffgcvb(fptr, 3, 1, 1, nrows, 98, xinarray, &anynull, &status);
    ffgcvb(fptr, 4, 1, 1, nrows, 98, binarray, &anynull, &status);
    ffgcvi(fptr, 5, 1, 1, nrows, 98, iinarray, &anynull, &status);
    ffgcvk(fptr, 6, 1, 1, nrows, 98, kinarray, &anynull, &status);
    ffgcve(fptr, 7, 1, 1, nrows, 98., einarray, &anynull, &status);
    ffgcvd(fptr, 8, 1, 1, nrows, 98., dinarray, &anynull, &status);
    ffgcvc(fptr, 9, 1, 1, nrows, 98., cinarray, &anynull, &status);
    ffgcvm(fptr, 10, 1, 1, nrows, 98., minarray, &anynull, &status);

    printf("\nRead columns with ffgcv_:\n");
    for (ii = 0; ii < nrows; ii++)
    {
  printf("%15s %d %3d %2d %3d %3d %5.1f %5.1f (%5.1f,%5.1f) (%5.1f,%5.1f) \n",
        inskey[ii], larray[ii], xinarray[ii], binarray[ii], iinarray[ii], 
        kinarray[ii], einarray[ii], dinarray[ii], cinarray[ii * 2], 
        cinarray[ii * 2 + 1], minarray[ii * 2], minarray[ii * 2 + 1]);
    }

    for (ii = 0; ii < nrows; ii++)
    {
      larray[ii] = 0;
      xinarray[ii] = 0;
      binarray[ii] = 0;
      iinarray[ii] = 0; 
      kinarray[ii] = 0;
      einarray[ii] = 0.; 
      dinarray[ii] = 0.;
      cinarray[ii * 2] = 0.; 
      minarray[ii * 2] = 0.;
      cinarray[ii * 2 + 1] = 0.; 
      minarray[ii * 2 + 1] = 0.;
    }

    ffgcfs(fptr, 1, 1, 1, nrows, inskey,   larray2, &anynull, &status);
    ffgcfl(fptr, 2, 1, 1, nrows, larray,   larray2, &anynull, &status);
    ffgcfb(fptr, 3, 1, 1, nrows, xinarray, larray2, &anynull, &status);
    ffgcfb(fptr, 4, 1, 1, nrows, binarray, larray2, &anynull, &status);
    ffgcfi(fptr, 5, 1, 1, nrows, iinarray, larray2, &anynull, &status);
    ffgcfk(fptr, 6, 1, 1, nrows, kinarray, larray2, &anynull, &status);
    ffgcfe(fptr, 7, 1, 1, nrows, einarray, larray2, &anynull, &status);
    ffgcfd(fptr, 8, 1, 1, nrows, dinarray, larray2, &anynull, &status);
    ffgcfc(fptr, 9, 1, 1, nrows, cinarray, larray2, &anynull, &status);
    ffgcfm(fptr, 10, 1, 1, nrows, minarray, larray2, &anynull, &status);

    printf("\nRead columns with ffgcf_:\n");
    for (ii = 0; ii < 10; ii++)
    {
    
    printf("%15s %d %3d %2d %3d %3d %5.1f %5.1f (%5.1f,%5.1f) (%5.1f,%5.1f)\n",
        inskey[ii], larray[ii], xinarray[ii], binarray[ii], iinarray[ii], 
        kinarray[ii], einarray[ii], dinarray[ii], cinarray[ii * 2], 
        cinarray[ii * 2 + 1], minarray[ii * 2], minarray[ii * 2 + 1]);
    }
    for (ii = 10; ii < nrows; ii++)
    {
      /* don't try to print the NaN values */
      printf("%15s %d %3d %2d %3d \n",
        inskey[ii], larray[ii], xinarray[ii], binarray[ii], iinarray[ii]);
    }
    ffprec(fptr, 
    "key_prec= 'This keyword was written by f_prec' / comment here", &status);

    /*
      ###############################################
      #  test the insert/delete row/column routines #
      ###############################################
    */
    if (ffirow(fptr, 2, 3, &status) > 0)
        goto errstatus;

    nrows = 14;
    ffgcvs(fptr, 1, 1, 1, nrows, "NOT DEFINED",  inskey,   &anynull, &status);
    ffgcvb(fptr, 4, 1, 1, nrows, 98, binarray, &anynull, &status);
    ffgcvi(fptr, 5, 1, 1, nrows, 98, iinarray, &anynull, &status);
    ffgcvj(fptr, 6, 1, 1, nrows, 98, jinarray, &anynull, &status);
    ffgcve(fptr, 7, 1, 1, nrows, 98., einarray, &anynull, &status);
    ffgcvd(fptr, 8, 1, 1, nrows, 98., dinarray, &anynull, &status);

    printf("\nData values after inserting 3 rows after row 2:\n");
    for (ii = 0; ii < nrows; ii++)
    {
      printf("%15s %2d %3d %3ld %5.1f %5.1f\n",  inskey[ii], binarray[ii],
          iinarray[ii], jinarray[ii], einarray[ii], dinarray[ii]);
    }

    if (ffdrow(fptr, 10, 2, &status) > 0)
        goto errstatus;

    nrows = 12;
    ffgcvs(fptr, 1, 1, 1, nrows, "NOT DEFINED",  inskey,   &anynull, &status);
    ffgcvb(fptr, 4, 1, 1, nrows, 98, binarray, &anynull, &status);
    ffgcvi(fptr, 5, 1, 1, nrows, 98, iinarray, &anynull, &status);
    ffgcvj(fptr, 6, 1, 1, nrows, 98, jinarray, &anynull, &status);
    ffgcve(fptr, 7, 1, 1, nrows, 98., einarray, &anynull, &status);
    ffgcvd(fptr, 8, 1, 1, nrows, 98., dinarray, &anynull, &status);

    printf("\nData values after deleting 2 rows at row 10:\n");
    for (ii = 0; ii < nrows; ii++)
    {
      printf("%15s %2d %3d %3ld %5.1f %5.1f\n",  inskey[ii], binarray[ii],
          iinarray[ii], jinarray[ii], einarray[ii], dinarray[ii]);
    }

    if (ffdcol(fptr, 6, &status) > 0)
        goto errstatus;

    ffgcvs(fptr, 1, 1, 1, nrows, "NOT DEFINED",  inskey,   &anynull, &status);
    ffgcvb(fptr, 4, 1, 1, nrows, 98, binarray, &anynull, &status);
    ffgcvi(fptr, 5, 1, 1, nrows, 98, iinarray, &anynull, &status);
    ffgcve(fptr, 6, 1, 1, nrows, 98., einarray, &anynull, &status);
    ffgcvd(fptr, 7, 1, 1, nrows, 98., dinarray, &anynull, &status);

    printf("\nData values after deleting column 6:\n");
    for (ii = 0; ii < nrows; ii++)
    {
      printf("%15s %2d %3d %5.1f %5.1f\n", inskey[ii], binarray[ii],
          iinarray[ii], einarray[ii], dinarray[ii]);
    }

    if (fficol(fptr, 8, "INSERT_COL", "1E", &status) > 0)
        goto errstatus;

    ffgcvs(fptr, 1, 1, 1, nrows, "NOT DEFINED",  inskey,   &anynull, &status);
    ffgcvb(fptr, 4, 1, 1, nrows, 98, binarray, &anynull, &status);
    ffgcvi(fptr, 5, 1, 1, nrows, 98, iinarray, &anynull, &status);
    ffgcve(fptr, 6, 1, 1, nrows, 98., einarray, &anynull, &status);
    ffgcvd(fptr, 7, 1, 1, nrows, 98., dinarray, &anynull, &status);
    ffgcvj(fptr, 8, 1, 1, nrows, 98, jinarray, &anynull, &status);

    printf("\nData values after inserting column 8:\n");
    for (ii = 0; ii < nrows; ii++)
    {
      printf("%15s %2d %3d %5.1f %5.1f %ld\n", inskey[ii], binarray[ii],
          iinarray[ii], einarray[ii], dinarray[ii] , jinarray[ii]);
    }

    ffpclu(fptr, 8, 1, 1, 10, &status);

    ffgcvs(fptr, 1, 1, 1, nrows, "NOT DEFINED",  inskey,   &anynull, &status);
    ffgcvb(fptr, 4, 1, 1, nrows, 98, binarray, &anynull, &status);
    ffgcvi(fptr, 5, 1, 1, nrows, 98, iinarray, &anynull, &status);
    ffgcve(fptr, 6, 1, 1, nrows, 98., einarray, &anynull, &status);
    ffgcvd(fptr, 7, 1, 1, nrows, 98., dinarray, &anynull, &status);
    ffgcvj(fptr, 8, 1, 1, nrows, 98, jinarray, &anynull, &status);

    printf("\nValues after setting 1st 10 elements in column 8 = null:\n");
    for (ii = 0; ii < nrows; ii++)
    {
      printf("%15s %2d %3d %5.1f %5.1f %ld\n", inskey[ii], binarray[ii],
          iinarray[ii], einarray[ii], dinarray[ii] , jinarray[ii]);
    }

    /*
      ############################################################
      #  create a temporary file and copy the binary table to it,#
      #  column by column.                                       #
      ############################################################
    */
    bitpix = 16;
    naxis = 0;

    strcpy(filename, "!t1q2s3v5.tmp");
    ffinit(&tmpfptr, filename, &status);
    printf("Create temporary file: ffinit status = %d\n", status);

    ffiimg(tmpfptr, bitpix, naxis, naxes, &status);
    printf("\nCreate null primary array: ffiimg status = %d\n", status);

    /* create an empty table with 22 rows and 0 columns */
    nrows = 22;
    tfields = 0;
    ffibin(tmpfptr, nrows, tfields, ttype, tform, tunit, binname, 0L,
            &status);
    printf("\nCreate binary table with 0 columns: ffibin status = %d\n",
           status);

    /* copy columns from one table to the other */
    ffcpcl(fptr, tmpfptr, 7, 1, TRUE, &status);
    printf("copy column, ffcpcl status = %d\n", status);
    ffcpcl(fptr, tmpfptr, 6, 1, TRUE, &status);
    printf("copy column, ffcpcl status = %d\n", status);
    ffcpcl(fptr, tmpfptr, 5, 1, TRUE, &status);
    printf("copy column, ffcpcl status = %d\n", status);
    ffcpcl(fptr, tmpfptr, 4, 1, TRUE, &status);
    printf("copy column, ffcpcl status = %d\n", status);
    ffcpcl(fptr, tmpfptr, 3, 1, TRUE, &status);
    printf("copy column, ffcpcl status = %d\n", status);
    ffcpcl(fptr, tmpfptr, 2, 1, TRUE, &status);
    printf("copy column, ffcpcl status = %d\n", status);
    ffcpcl(fptr, tmpfptr, 1, 1, TRUE, &status);
    printf("copy column, ffcpcl status = %d\n", status);

/*
    ffclos(tmpfptr, &status);
    printf("Close the tmp file: ffclos status = %d\n", status);
*/

    ffdelt(tmpfptr, &status);
    printf("Delete the tmp file: ffdelt status = %d\n", status);
    if (status > 0)
    {
        goto errstatus;
    }
    /*
      ####################################################
      #  insert binary table following the primary array #
      ####################################################
    */

    ffmahd(fptr,  1, &hdutype, &status);

    strcpy(tform[0], "15A");
    strcpy(tform[1], "1L");
    strcpy(tform[2], "16X");
    strcpy(tform[3], "1B");
    strcpy(tform[4], "1I");
    strcpy(tform[5], "1J");
    strcpy(tform[6], "1E");
    strcpy(tform[7], "1D");
    strcpy(tform[8], "1C");
    strcpy(tform[9], "1M");

    strcpy(ttype[0], "Avalue");
    strcpy(ttype[1], "Lvalue");
    strcpy(ttype[2], "Xvalue");
    strcpy(ttype[3], "Bvalue");
    strcpy(ttype[4], "Ivalue");
    strcpy(ttype[5], "Jvalue");
    strcpy(ttype[6], "Evalue");
    strcpy(ttype[7], "Dvalue");
    strcpy(ttype[8], "Cvalue");
    strcpy(ttype[9], "Mvalue");

    strcpy(tunit[0], "");
    strcpy(tunit[1], "m**2");
    strcpy(tunit[2], "cm");
    strcpy(tunit[3], "erg/s");
    strcpy(tunit[4], "km/s");
    strcpy(tunit[5], "");
    strcpy(tunit[6], "");
    strcpy(tunit[7], "");
    strcpy(tunit[8], "");
    strcpy(tunit[9], "");

    nrows = 20;
    tfields = 10;
    pcount = 0;

    ffibin(fptr, nrows, tfields, ttype, tform, tunit, binname, pcount,
            &status);
    printf("ffibin status = %d\n", status);
    printf("HDU number = %d\n", ffghdn(fptr, &hdunum));

    extvers = 3;
    ffpkyj(fptr, "EXTVER", extvers, "extension version number", &status);


    ffpkyj(fptr, "TNULL4", 77, "value for undefined pixels", &status);
    ffpkyj(fptr, "TNULL5", 77, "value for undefined pixels", &status);
    ffpkyj(fptr, "TNULL6", 77, "value for undefined pixels", &status);

    ffpkyj(fptr, "TSCAL4", 1000, "scaling factor", &status);
    ffpkyj(fptr, "TSCAL5", 1, "scaling factor", &status);
    ffpkyj(fptr, "TSCAL6", 100, "scaling factor", &status);

    ffpkyj(fptr, "TZERO4", 0, "scaling offset", &status);
    ffpkyj(fptr, "TZERO5", 32768, "scaling offset", &status);
    ffpkyj(fptr, "TZERO6", 100, "scaling offset", &status);

    fftnul(fptr, 4, 77, &status);   /* define null value for int cols */
    fftnul(fptr, 5, 77, &status);
    fftnul(fptr, 6, 77, &status);
    /* set scaling */
    fftscl(fptr, 4, 1000., 0., &status);   
    fftscl(fptr, 5, 1., 32768., &status);
    fftscl(fptr, 6, 100., 100., &status);

    /*
      ############################
      #  write data to columns   #
      ############################
    */

    /* initialize arrays of values to write to table */
 
    joutarray[0] = 0;
    joutarray[1] = 1000;
    joutarray[2] = 10000;
    joutarray[3] = 32768;
    joutarray[4] = 65535;


    for (ii = 4; ii < 7; ii++)
    {
        ffpclj(fptr, ii, 1, 1, 5, joutarray, &status); 
        if (status == NUM_OVERFLOW)
        {
            printf("Overflow writing to column %ld\n", ii);
            status = 0;
        }

        ffpclu(fptr, ii, 6, 1, 1, &status);  /* write null value */
    }

    for (jj = 4; jj < 7; jj++)
    {
      ffgcvj(fptr, jj, 1, 1, 6, -999, jinarray, &anynull, &status);
      for (ii = 0; ii < 6; ii++)
      {
        printf(" %6ld", jinarray[ii]);
      }
      printf("\n");
    }

    printf("\n");
    /* turn off scaling, and read the unscaled values */
    fftscl(fptr, 4, 1., 0., &status);   
    fftscl(fptr, 5, 1., 0., &status);
    fftscl(fptr, 6, 1., 0., &status);

    for (jj = 4; jj < 7; jj++)
    {
      ffgcvj(fptr, jj, 1, 1, 6, -999, jinarray, &anynull, &status);
      for (ii = 0; ii < 6; ii++)
      {
        printf(" %6ld", jinarray[ii]);
      }
      printf("\n");
    }
    /*
      ######################################################
      #  insert image extension following the binary table #
      ######################################################
    */

    bitpix = -32;
    naxis = 2;
    naxes[0] = 15;
    naxes[1] = 25;
    ffiimg(fptr, bitpix, naxis, naxes, &status);
    printf("\nCreate image extension: ffiimg status = %d\n", status);
    printf("HDU number = %d\n", ffghdn(fptr, &hdunum));

    for (jj = 0; jj < 30; jj++)
    {
      for (ii = 0; ii < 19; ii++)
      {
        imgarray[jj][ii] = (short) ((jj * 10) + ii);
      }
    }

    ffp2di(fptr, 1, 19, naxes[0], naxes[1], imgarray[0], &status);
    printf("\nWrote whole 2D array: ffp2di status = %d\n", status);

    for (jj = 0; jj < 30; jj++)
    {
      for (ii = 0; ii < 19; ii++)
      {
        imgarray[jj][ii] = 0;
      }
    }
    
    ffg2di(fptr, 1, 0, 19, naxes[0], naxes[1], imgarray[0], &anynull,
           &status);
    printf("\nRead whole 2D array: ffg2di status = %d\n", status);

    for (jj = 0; jj < 30; jj++)
    {
      for (ii = 0; ii < 19; ii++)
      {
        printf(" %3d", imgarray[jj][ii]);
      }
      printf("\n");
    }

    for (jj = 0; jj < 30; jj++)
    {
      for (ii = 0; ii < 19; ii++)
      {
        imgarray[jj][ii] = 0;
      }
    }
    
    for (jj = 0; jj < 20; jj++)
    {
      for (ii = 0; ii < 10; ii++)
      {
        imgarray2[jj][ii] = (short) ((jj * -10) - ii);
      }
    }

    fpixels[0] = 5;
    fpixels[1] = 5;
    lpixels[0] = 14;
    lpixels[1] = 14;
    ffpssi(fptr, 1, naxis, naxes, fpixels, lpixels, 
         imgarray2[0], &status);
    printf("\nWrote subset 2D array: ffpssi status = %d\n", status);

    ffg2di(fptr, 1, 0, 19, naxes[0], naxes[1], imgarray[0], &anynull,
           &status);
    printf("\nRead whole 2D array: ffg2di status = %d\n", status);

    for (jj = 0; jj < 30; jj++)
    {
      for (ii = 0; ii < 19; ii++)
      {
        printf(" %3d", imgarray[jj][ii]);
      }
      printf("\n");
    }

    fpixels[0] = 2;
    fpixels[1] = 5;
    lpixels[0] = 10;
    lpixels[1] = 8;
    inc[0] = 2;
    inc[1] = 3;

    for (jj = 0; jj < 30; jj++)
    {
      for (ii = 0; ii < 19; ii++)
      {
        imgarray[jj][ii] = 0;
      }
    }
    
    ffgsvi(fptr, 1, naxis, naxes, fpixels, lpixels, inc, 0,
          imgarray[0], &anynull, &status);
    printf("\nRead subset of 2D array: ffgsvi status = %d\n", status);

    for (ii = 0; ii < 10; ii++)
    {
        printf(" %3d", imgarray[0][ii]);
    }
    printf("\n");

    /*
      ###########################################################
      #  insert another image extension                         #
      #  copy the image extension to primary array of tmp file. #
      #  then delete the tmp file, and the image extension      #
      ###########################################################
    */
    bitpix = 16;
    naxis = 2;
    naxes[0] = 15;
    naxes[1] = 25;
    ffiimg(fptr, bitpix, naxis, naxes, &status);
    printf("\nCreate image extension: ffiimg status = %d\n", status);
    printf("HDU number = %d\n", ffghdn(fptr, &hdunum));

    strcpy(filename, "t1q2s3v4.tmp");
    ffinit(&tmpfptr, filename, &status);
    printf("Create temporary file: ffinit status = %d\n", status);

    ffcopy(fptr, tmpfptr, 0, &status);
    printf("Copy image extension to primary array of tmp file.\n");
    printf("ffcopy status = %d\n", status);

    ffgrec(tmpfptr, 1, card, &status);
    printf("%s\n", card);
    ffgrec(tmpfptr, 2, card, &status);
    printf("%s\n", card);
    ffgrec(tmpfptr, 3, card, &status);
    printf("%s\n", card);
    ffgrec(tmpfptr, 4, card, &status);
    printf("%s\n", card);
    ffgrec(tmpfptr, 5, card, &status);
    printf("%s\n", card);
    ffgrec(tmpfptr, 6, card, &status);
    printf("%s\n", card);

    ffdelt(tmpfptr, &status);
    printf("Delete the tmp file: ffdelt status = %d\n", status);

    ffdhdu(fptr, &hdutype, &status);
    printf("Delete the image extension; hdutype, status = %d %d\n",
             hdutype, status);
    printf("HDU number = %d\n", ffghdn(fptr, &hdunum));

    /*
      ###########################################################
      #  append bintable extension with variable length columns #
      ###########################################################
    */

    ffcrhd(fptr, &status);
    printf("ffcrhd status = %d\n", status);

    strcpy(tform[0], "1PA");
    strcpy(tform[1], "1PL");
    strcpy(tform[2], "1PB"); /* Fortran FITSIO doesn't support  1PX */
    strcpy(tform[3], "1PB");
    strcpy(tform[4], "1PI");
    strcpy(tform[5], "1PJ");
    strcpy(tform[6], "1PE");
    strcpy(tform[7], "1PD");
    strcpy(tform[8], "1PC");
    strcpy(tform[9], "1PM");

    strcpy(ttype[0], "Avalue");
    strcpy(ttype[1], "Lvalue");
    strcpy(ttype[2], "Xvalue");
    strcpy(ttype[3], "Bvalue");
    strcpy(ttype[4], "Ivalue");
    strcpy(ttype[5], "Jvalue");
    strcpy(ttype[6], "Evalue");
    strcpy(ttype[7], "Dvalue");
    strcpy(ttype[8], "Cvalue");
    strcpy(ttype[9], "Mvalue");

    strcpy(tunit[0], "");
    strcpy(tunit[1], "m**2");
    strcpy(tunit[2], "cm");
    strcpy(tunit[3], "erg/s");
    strcpy(tunit[4], "km/s");
    strcpy(tunit[5], "");
    strcpy(tunit[6], "");
    strcpy(tunit[7], "");
    strcpy(tunit[8], "");
    strcpy(tunit[9], "");

    nrows = 20;
    tfields = 10;
    pcount = 0;

    ffphbn(fptr, nrows, tfields, ttype, tform, tunit, binname, pcount,
            &status);
    printf("Variable length arrays: ffphbn status = %d\n", status);


    extvers = 4;
    ffpkyj(fptr, "EXTVER", extvers, "extension version number", &status);

    ffpkyj(fptr, "TNULL4", 88, "value for undefined pixels", &status);
    ffpkyj(fptr, "TNULL5", 88, "value for undefined pixels", &status);
    ffpkyj(fptr, "TNULL6", 88, "value for undefined pixels", &status);

    /*
      ############################
      #  write data to columns   #
      ############################
    */

    /* initialize arrays of values to write to table */
    strcpy(iskey,"abcdefghijklmnopqrst");

    for (ii = 0; ii < 20; ii++)
    {
        boutarray[ii] = (unsigned char) (ii + 1);
        ioutarray[ii] = (short) (ii + 1);
        joutarray[ii] = ii + 1;
        eoutarray[ii] = (float) (ii + 1);
        doutarray[ii] = ii + 1;
    }

    larray[0] = 0;
    larray[1] = 1;
    larray[2] = 0;
    larray[3] = 0;
    larray[4] = 1;
    larray[5] = 1;
    larray[6] = 0;
    larray[7] = 0;
    larray[8] = 0;
    larray[9] = 1;
    larray[10] = 1;
    larray[11] = 1;
    larray[12] = 0;
    larray[13] = 0;
    larray[14] = 0;
    larray[15] = 0;
    larray[16] = 1;
    larray[17] = 1;
    larray[18] = 1;
    larray[19] = 1;

    /* write values in 1st row */
    /*  strncpy(inskey[0], iskey, 1); */
      inskey[0][0] = '\0';  /* write a null string (i.e., a blank) */
      ffpcls(fptr, 1, 1, 1, 1, inskey, &status);  /* write string values */
      ffpcll(fptr, 2, 1, 1, 1, larray, &status);  /* write logicals */
      ffpclx(fptr, 3, 1, 1, 1, larray, &status);  /* write bits */
      ffpclb(fptr, 4, 1, 1, 1, boutarray, &status);
      ffpcli(fptr, 5, 1, 1, 1, ioutarray, &status); 
      ffpclj(fptr, 6, 1, 1, 1, joutarray, &status); 
      ffpcle(fptr, 7, 1, 1, 1, eoutarray, &status);
      ffpcld(fptr, 8, 1, 1, 1, doutarray, &status);

    for (ii = 2; ii <= 20; ii++)   /* loop over rows 1 - 20 */
    {
      strncpy(inskey[0], iskey, ii);
      inskey[0][ii] = '\0';
      ffpcls(fptr, 1, ii, 1, 1, inskey, &status);  /* write string values */

      ffpcll(fptr, 2, ii, 1, ii, larray, &status);  /* write logicals */
      ffpclu(fptr, 2, ii, ii-1, 1, &status);

      ffpclx(fptr, 3, ii, 1, ii, larray, &status);  /* write bits */

      ffpclb(fptr, 4, ii, 1, ii, boutarray, &status);
      ffpclu(fptr, 4, ii, ii-1, 1, &status);

      ffpcli(fptr, 5, ii, 1, ii, ioutarray, &status); 
      ffpclu(fptr, 5, ii, ii-1, 1, &status);

      ffpclj(fptr, 6, ii, 1, ii, joutarray, &status); 
      ffpclu(fptr, 6, ii, ii-1, 1, &status);

      ffpcle(fptr, 7, ii, 1, ii, eoutarray, &status);
      ffpclu(fptr, 7, ii, ii-1, 1, &status);

      ffpcld(fptr, 8, ii, 1, ii, doutarray, &status);
      ffpclu(fptr, 8, ii, ii-1, 1, &status);
    }
    printf("ffpcl_ status = %d\n", status);

    /*
      #################################
      #  close then reopen this HDU   #
      #################################
    */


     ffmrhd(fptr, -1, &hdutype, &status);
     ffmrhd(fptr,  1, &hdutype, &status);

    /*
      #############################
      #  read data from columns   #
      #############################
    */

    ffgkyj(fptr, "PCOUNT", &pcount, comm, &status);
    printf("PCOUNT = %ld\n", pcount);

    /* initialize the variables to be read */
    strcpy(inskey[0]," ");
    strcpy(iskey," ");


    printf("HDU number = %d\n", ffghdn(fptr, &hdunum));
    for (ii = 1; ii <= 20; ii++)   /* loop over rows 1 - 20 */
    {
      for (jj = 0; jj < ii; jj++)
      {
        larray[jj] = 0;
        boutarray[jj] = 0;
        ioutarray[jj] = 0;
        joutarray[jj] = 0;
        eoutarray[jj] = 0;
        doutarray[jj] = 0;
      }

      ffgcvs(fptr, 1, ii, 1, 1, iskey, inskey, &anynull, &status);  
      printf("A %s %d\nL", inskey[0], status);

      ffgcl( fptr, 2, ii, 1, ii, larray, &status); 
      for (jj = 0; jj < ii; jj++)
        printf(" %2d", larray[jj]);
      printf(" %d\nX", status);

      ffgcx(fptr, 3, ii, 1, ii, larray, &status);
      for (jj = 0; jj < ii; jj++)
        printf(" %2d", larray[jj]);
      printf(" %d\nB", status);

      ffgcvb(fptr, 4, ii, 1, ii, 99, boutarray, &anynull, &status);
      for (jj = 0; jj < ii; jj++)
        printf(" %2d", boutarray[jj]);
      printf(" %d\nI", status);

      ffgcvi(fptr, 5, ii, 1, ii, 99, ioutarray, &anynull, &status); 
      for (jj = 0; jj < ii; jj++)
        printf(" %2d", ioutarray[jj]);
      printf(" %d\nJ", status);

      ffgcvj(fptr, 6, ii, 1, ii, 99, joutarray, &anynull, &status); 
      for (jj = 0; jj < ii; jj++)
        printf(" %2ld", joutarray[jj]);
      printf(" %d\nE", status);

      ffgcve(fptr, 7, ii, 1, ii, 99., eoutarray, &anynull, &status);
      for (jj = 0; jj < ii; jj++)
        printf(" %2.0f", eoutarray[jj]);
      printf(" %d\nD", status);

      ffgcvd(fptr, 8, ii, 1, ii, 99., doutarray, &anynull, &status);
      for (jj = 0; jj < ii; jj++)
        printf(" %2.0f", doutarray[jj]);
      printf(" %d\n", status);

      ffgdes(fptr, 8, ii, &repeat, &offset, &status);
      printf("Column 8 repeat and offset = %ld %ld\n", repeat, offset);
    }

    /*
      #####################################
      #  create another image extension   #
      #####################################
    */

    bitpix = 32;
    naxis = 2;
    naxes[0] = 10;
    naxes[1] = 2;
    npixels = 20;
 
/*    ffcrim(fptr, bitpix, naxis, naxes, &status); */
    ffiimg(fptr, bitpix, naxis, naxes, &status);
    printf("\nffcrim status = %d\n", status);

    /* initialize arrays of values to write to primary array */
    for (ii = 0; ii < npixels; ii++)
    {
        boutarray[ii] = (unsigned char) (ii * 2);
        ioutarray[ii] = (short) (ii * 2);
        joutarray[ii] = ii * 2;
        koutarray[ii] = ii * 2;
        eoutarray[ii] = (float) (ii * 2);
        doutarray[ii] = ii * 2;
    }

    /* write a few pixels with each datatype */
    ffppr(fptr, TBYTE,   1,  2, &boutarray[0],  &status);
    ffppr(fptr, TSHORT,  3,  2, &ioutarray[2],  &status);
    ffppr(fptr, TINT,    5,  2, &koutarray[4],  &status);
    ffppr(fptr, TSHORT,  7,  2, &ioutarray[6],  &status);
    ffppr(fptr, TLONG,   9,  2, &joutarray[8],  &status);
    ffppr(fptr, TFLOAT,  11, 2, &eoutarray[10], &status);
    ffppr(fptr, TDOUBLE, 13, 2, &doutarray[12], &status);
    printf("ffppr status = %d\n", status);

    /* read back the pixels with each datatype */
    bnul = 0;
    inul = 0;
    knul = 0;
    jnul = 0;
    enul = 0.;
    dnul = 0.;

    ffgpv(fptr, TBYTE,   1,  14, &bnul, binarray, &anynull, &status);
    ffgpv(fptr, TSHORT,  1,  14, &inul, iinarray, &anynull, &status);
    ffgpv(fptr, TINT,    1,  14, &knul, kinarray, &anynull, &status);
    ffgpv(fptr, TLONG,   1,  14, &jnul, jinarray, &anynull, &status);
    ffgpv(fptr, TFLOAT,  1,  14, &enul, einarray, &anynull, &status);
    ffgpv(fptr, TDOUBLE, 1,  14, &dnul, dinarray, &anynull, &status);

    printf("\nImage values written with ffppr and read with ffgpv:\n");
    npixels = 14;
    for (ii = 0; ii < npixels; ii++)
        printf(" %2d", binarray[ii]);
    printf("  %d (byte)\n", anynull);  
    for (ii = 0; ii < npixels; ii++)
        printf(" %2d", iinarray[ii]);
    printf("  %d (short)\n", anynull);  
    for (ii = 0; ii < npixels; ii++)
        printf(" %2d", kinarray[ii]);
    printf("  %d (int)\n", anynull); 
    for (ii = 0; ii < npixels; ii++)
        printf(" %2ld", jinarray[ii]);
    printf("  %d (long)\n", anynull); 
    for (ii = 0; ii < npixels; ii++)
        printf(" %2.0f", einarray[ii]);
    printf("  %d (float)\n", anynull);
    for (ii = 0; ii < npixels; ii++)
        printf(" %2.0f", dinarray[ii]);
    printf("  %d (double)\n", anynull);

    /*
      ##########################################
      #  test world coordinate system routines #
      ##########################################
    */

    xrval = 45.83;
    yrval =  63.57;
    xrpix =  256.;
    yrpix =  257.;
    xinc =   -.00277777;
    yinc =   .00277777;

    /* write the WCS keywords */
    /* use example values from the latest WCS document */
    ffpkyd(fptr, "CRVAL1", xrval, 10, "comment", &status);
    ffpkyd(fptr, "CRVAL2", yrval, 10, "comment", &status);
    ffpkyd(fptr, "CRPIX1", xrpix, 10, "comment", &status);
    ffpkyd(fptr, "CRPIX2", yrpix, 10, "comment", &status);
    ffpkyd(fptr, "CDELT1", xinc, 10, "comment", &status);
    ffpkyd(fptr, "CDELT2", yinc, 10, "comment", &status);
 /*   ffpkyd(fptr, "CROTA2", rot, 10, "comment", &status); */
    ffpkys(fptr, "CTYPE1", xcoordtype, "comment", &status);
    ffpkys(fptr, "CTYPE2", ycoordtype, "comment", &status);
    printf("\nWrote WCS keywords status = %d\n",status);

    xrval =  0.;
    yrval =  0.;
    xrpix =  0.;
    yrpix =  0.;
    xinc =   0.;
    yinc =   0.;
    rot =    0.;

    ffgics(fptr, &xrval, &yrval, &xrpix,
           &yrpix, &xinc, &yinc, &rot, ctype, &status);
    printf("Read WCS keywords with ffgics status = %d\n",status);

    xpix = 0.5;
    ypix = 0.5;

    ffwldp(xpix,ypix,xrval,yrval,xrpix,yrpix,xinc,yinc,rot,ctype,
           &xpos, &ypos,&status);

    printf("  CRVAL1, CRVAL2 = %16.12f, %16.12f\n", xrval,yrval);
    printf("  CRPIX1, CRPIX2 = %16.12f, %16.12f\n", xrpix,yrpix);
    printf("  CDELT1, CDELT2 = %16.12f, %16.12f\n", xinc,yinc);
    printf("  Rotation = %10.3f, CTYPE = %s\n", rot, ctype);
    printf("Calculated sky coordinate with ffwldp status = %d\n",status);
    printf("  Pixels (%8.4f,%8.4f) --> (%11.6f, %11.6f) Sky\n",
            xpix,ypix,xpos,ypos);
    ffxypx(xpos,ypos,xrval,yrval,xrpix,yrpix,xinc,yinc,rot,ctype,
           &xpix, &ypix,&status);
    printf("Calculated pixel coordinate with ffxypx status = %d\n",status);
    printf("  Sky (%11.6f, %11.6f) --> (%8.4f,%8.4f) Pixels\n",
            xpos,ypos,xpix,ypix);
    /*
      ######################################
      #  append another ASCII table        #
      ######################################
    */

    strcpy(tform[0], "A15");
    strcpy(tform[1], "I11");
    strcpy(tform[2], "F15.6");
    strcpy(tform[3], "E13.5");
    strcpy(tform[4], "D22.14");

    strcpy(ttype[0], "Name");
    strcpy(ttype[1], "Ivalue");
    strcpy(ttype[2], "Fvalue");
    strcpy(ttype[3], "Evalue");
    strcpy(ttype[4], "Dvalue");

    strcpy(tunit[0], "");
    strcpy(tunit[1], "m**2");
    strcpy(tunit[2], "cm");
    strcpy(tunit[3], "erg/s");
    strcpy(tunit[4], "km/s");

    nrows = 11;
    tfields = 5;
    strcpy(tblname, "new_table");

    ffcrtb(fptr, ASCII_TBL, nrows, tfields, ttype, tform, tunit, tblname,
            &status);
    printf("\nffcrtb status = %d\n", status);

    extvers = 5;
    ffpkyj(fptr, "EXTVER", extvers, "extension version number", &status);

    ffpcl(fptr, TSTRING, 1, 1, 1, 3, onskey, &status);  /* write string values */

    /* initialize arrays of values to write */
    
    for (ii = 0; ii < npixels; ii++)
    {
        boutarray[ii] = (unsigned char) (ii * 3);
        ioutarray[ii] = (short) (ii * 3);
        joutarray[ii] = ii * 3;
        koutarray[ii] = ii * 3;
        eoutarray[ii] = (float) (ii * 3);
        doutarray[ii] = ii * 3;
    }

    for (ii = 2; ii < 6; ii++)   /* loop over cols 2 - 5 */
    {
        ffpcl(fptr, TBYTE,   ii, 1, 1, 2, boutarray,     &status); 
        ffpcl(fptr, TSHORT,  ii, 3, 1, 2, &ioutarray[2], &status);  
        ffpcl(fptr, TLONG,   ii, 5, 1, 2, &joutarray[4], &status);  
        ffpcl(fptr, TFLOAT,  ii, 7, 1, 2, &eoutarray[6], &status);
        ffpcl(fptr, TDOUBLE, ii, 9, 1, 2, &doutarray[8], &status); 
    }
    printf("ffpcl status = %d\n", status);

    /* read back the pixels with each datatype */
    ffgcv(fptr, TBYTE,   2, 1, 1, 10, &bnul, binarray, &anynull, &status);
    ffgcv(fptr, TSHORT,  2, 1, 1, 10, &inul, iinarray, &anynull, &status);
    ffgcv(fptr, TINT,    3, 1, 1, 10, &knul, kinarray, &anynull, &status);
    ffgcv(fptr, TLONG,   3, 1, 1, 10, &jnul, jinarray, &anynull, &status);
    ffgcv(fptr, TFLOAT,  4, 1, 1, 10, &enul, einarray, &anynull, &status);
    ffgcv(fptr, TDOUBLE, 5, 1, 1, 10, &dnul, dinarray, &anynull, &status);

    printf("\nColumn values written with ffpcl and read with ffgcl:\n");
    npixels = 10;
    for (ii = 0; ii < npixels; ii++)
        printf(" %2d", binarray[ii]);
    printf("  %d (byte)\n", anynull);  
    for (ii = 0; ii < npixels; ii++)
        printf(" %2d", iinarray[ii]);
    printf("  %d (short)\n", anynull);

    for (ii = 0; ii < npixels; ii++)
        printf(" %2d", kinarray[ii]);
    printf("  %d (int)\n", anynull); 

    for (ii = 0; ii < npixels; ii++)
        printf(" %2ld", jinarray[ii]);
    printf("  %d (long)\n", anynull); 
    for (ii = 0; ii < npixels; ii++)
        printf(" %2.0f", einarray[ii]);
    printf("  %d (float)\n", anynull);
    for (ii = 0; ii < npixels; ii++)
        printf(" %2.0f", dinarray[ii]);
    printf("  %d (double)\n", anynull);

    /*
      ###########################################################
      #  perform stress test by cycling thru all the extensions #
      ###########################################################
    */
    printf("\nRepeatedly move to the 1st 4 HDUs of the file:\n");
    for (ii = 0; ii < 10; ii++)
    {
      ffmahd(fptr,  1, &hdutype, &status);
      printf("%d", ffghdn(fptr, &hdunum));
      ffmrhd(fptr,  1, &hdutype, &status);
      printf("%d", ffghdn(fptr, &hdunum));
      ffmrhd(fptr,  1, &hdutype, &status);
      printf("%d", ffghdn(fptr, &hdunum));
      ffmrhd(fptr,  1, &hdutype, &status);
      printf("%d", ffghdn(fptr, &hdunum));
      ffmrhd(fptr, -1, &hdutype, &status);
      printf("%d", ffghdn(fptr, &hdunum));
      if (status > 0)
         break;
    }
    printf("\n");

    printf("Move to extensions by name and version number: (ffmnhd)\n");
    extvers = 1;
    ffmnhd(fptr, ANY_HDU, binname, (int) extvers, &status);
    ffghdn(fptr, &hdunum);
    printf(" %s, %ld = hdu %d, %d\n", binname, extvers, hdunum, status);
    extvers = 3;
    ffmnhd(fptr, ANY_HDU, binname, (int) extvers, &status);
    ffghdn(fptr, &hdunum);
    printf(" %s, %ld = hdu %d, %d\n", binname, extvers, hdunum, status);
    extvers = 4;
    ffmnhd(fptr, ANY_HDU, binname, (int) extvers, &status);
    ffghdn(fptr, &hdunum);
    printf(" %s, %ld = hdu %d, %d\n", binname, extvers, hdunum, status);


    strcpy(tblname, "Test-ASCII");
    extvers = 2;
    ffmnhd(fptr, ANY_HDU, tblname, (int) extvers, &status);
    ffghdn(fptr, &hdunum);
    printf(" %s, %ld = hdu %d, %d\n", tblname, extvers, hdunum, status);

    strcpy(tblname, "new_table");
    extvers = 5;
    ffmnhd(fptr, ANY_HDU, tblname, (int) extvers, &status);
    ffghdn(fptr, &hdunum);
    printf(" %s, %ld = hdu %d, %d\n", tblname, extvers, hdunum, status);
    extvers = 0;
    ffmnhd(fptr, ANY_HDU, binname, (int) extvers, &status);
    ffghdn(fptr, &hdunum);
    printf(" %s, %ld = hdu %d, %d\n", binname, extvers, hdunum, status);
    extvers = 17;
    ffmnhd(fptr, ANY_HDU, binname, (int) extvers, &status);
    ffghdn(fptr, &hdunum);
    printf(" %s, %ld = hdu %d, %d", binname, extvers, hdunum, status);
    printf (" (expect a 301 error status here)\n");
    status = 0;

    ffthdu(fptr, &hdunum, &status);
    printf("Total number of HDUs in the file = %d\n", hdunum);
    /*
      ########################
      #  checksum tests      #
      ########################
    */
    checksum = 1234567890;
    ffesum(checksum, 0, asciisum);
    printf("\nEncode checksum: %lu -> %s\n", checksum, asciisum);
    checksum = 0;
    ffdsum(asciisum, 0, &checksum);
    printf("Decode checksum: %s -> %lu\n", asciisum, checksum);

    ffpcks(fptr, &status);

    /*
       don't print the CHECKSUM value because it is different every day
       because the current date is in the comment field.

       ffgcrd(fptr, "CHECKSUM", card, &status);
       printf("%s\n", card);
    */

    ffgcrd(fptr, "DATASUM", card, &status);
    printf("%.30s\n", card);

    ffgcks(fptr, &datsum, &checksum, &status);
    printf("ffgcks data checksum, status = %lu, %d\n",
            datsum, status);

    ffvcks(fptr, &datastatus, &hdustatus, &status); 
    printf("ffvcks datastatus, hdustatus, status = %d %d %d\n",
              datastatus, hdustatus, status);
 
    ffprec(fptr,
    "new_key = 'written by fxprec' / to change checksum", &status);
    ffupck(fptr, &status);
    printf("ffupck status = %d\n", status);

    ffgcrd(fptr, "DATASUM", card, &status);
    printf("%.30s\n", card);
    ffvcks(fptr, &datastatus, &hdustatus, &status); 
    printf("ffvcks datastatus, hdustatus, status = %d %d %d\n",
              datastatus, hdustatus, status);
 
    /*
      delete the checksum keywords, so that the FITS file is always
      the same, regardless of the date of when testprog is run.
    */

    ffdkey(fptr, "CHECKSUM", &status);
    ffdkey(fptr, "DATASUM",  &status);

    /*
      ############################
      #  close file and quit     #
      ############################
    */

 errstatus:  /* jump here on error */

    ffclos(fptr, &status); 
    printf("ffclos status = %d\n", status);

    printf("\nNormally, there should be 8 error messages on the stack\n");
    printf("all regarding 'numerical overflows':\n");

    ffgmsg(errmsg);
    nmsg = 0;

    while (errmsg[0])
    {
        printf(" %s\n", errmsg);
        nmsg++;
        ffgmsg(errmsg);
    }

    if (nmsg != 8)
        printf("\nWARNING: Did not find the expected 8 error messages!\n");

    ffgerr(status, errmsg);
    printf("\nStatus = %d: %s\n", status, errmsg);

    /* free the allocated memory */
    for (ii = 0; ii < 21; ii++) 
        free(inskey[ii]);   
    for (ii = 0; ii < 10; ii++)
    {
      free(ttype[ii]);
      free(tform[ii]);
      free(tunit[ii]);
    }

    return(status);
}

