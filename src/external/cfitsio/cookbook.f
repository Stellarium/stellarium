      program main

C  This is the FITSIO cookbook program that contains an annotated listing of
C  various computer programs that read and write files in FITS format
C  using the FITSIO subroutine interface.  These examples are
C  working programs which users may adapt and modify for their own
C  purposes.  This Cookbook serves as a companion to the FITSIO User's
C  Guide that provides more complete documentation on all the
C  available FITSIO subroutines.

C  Call each subroutine in turn:

      call writeimage
      call writeascii
      call writebintable
      call copyhdu
      call selectrows
      call readheader
      call readimage
      call readtable
      print *
      print *,"All the fitsio cookbook routines ran successfully."

      end
C *************************************************************************
      subroutine writeimage

C  Create a FITS primary array containing a 2-D image

      integer status,unit,blocksize,bitpix,naxis,naxes(2)
      integer i,j,group,fpixel,nelements,array(300,200)
      character filename*80
      logical simple,extend

C  The STATUS parameter must be initialized before using FITSIO.  A
C  positive value of STATUS is returned whenever a serious error occurs.
C  FITSIO uses an `inherited status' convention, which means that if a
C  subroutine is called with a positive input value of STATUS, then the
C  subroutine will exit immediately, preserving the status value. For 
C  simplicity, this program only checks the status value at the end of 
C  the program, but it is usually better practice to check the status 
C  value more frequently.

      status=0

C  Name of the FITS file to be created:
      filename='ATESTFILEZ.FITS'

C  Delete the file if it already exists, so we can then recreate it.
C  The deletefile subroutine is listed at the end of this file.
      call deletefile(filename,status)

C  Get an unused Logical Unit Number to use to open the FITS file.
C  This routine is not required;  programmers can choose any unused
C  unit number to open the file.
      call ftgiou(unit,status)

C  Create the new empty FITS file.  The blocksize parameter is a
C  historical artifact and the value is ignored by FITSIO.
      blocksize=1
      call ftinit(unit,filename,blocksize,status)

C  Initialize parameters about the FITS image.
C  BITPIX = 16 means that the image pixels will consist of 16-bit
C  integers.  The size of the image is given by the NAXES values. 
C  The EXTEND = TRUE parameter indicates that the FITS file
C  may contain extensions following the primary array.
      simple=.true.
      bitpix=16
      naxis=2
      naxes(1)=300
      naxes(2)=200
      extend=.true.

C  Write the required header keywords to the file
      call ftphpr(unit,simple,bitpix,naxis,naxes,0,1,extend,status)

C  Initialize the values in the image with a linear ramp function
      do j=1,naxes(2)
          do i=1,naxes(1)
              array(i,j)=i - 1 +j - 1
          end do
      end do

C  Write the array to the FITS file.
C  The last letter of the subroutine name defines the datatype of the
C  array argument; in this case the 'J' indicates that the array has an
C  integer*4 datatype. ('I' = I*2, 'E' = Real*4, 'D' = Real*8).
C  The 2D array is treated as a single 1-D array with NAXIS1 * NAXIS2
C  total number of pixels.  GROUP is seldom used parameter that should
C  almost always be set = 1.
      group=1
      fpixel=1
      nelements=naxes(1)*naxes(2)
      call ftpprj(unit,group,fpixel,nelements,array,status)

C  Write another optional keyword to the header
C  The keyword record will look like this in the FITS file:
C
C  EXPOSURE=                 1500 / Total Exposure Time
C
      call ftpkyj(unit,'EXPOSURE',1500,'Total Exposure Time',status)

C  The FITS file must always be closed before exiting the program. 
C  Any unit numbers allocated with FTGIOU must be freed with FTFIOU.
      call ftclos(unit, status)
      call ftfiou(unit, status)

C  Check for any errors, and if so print out error messages.
C  The PRINTERROR subroutine is listed near the end of this file.
      if (status .gt. 0)call printerror(status)
      end
C *************************************************************************
      subroutine writeascii

C  Create an ASCII table containing 3 columns and 6 rows.  For convenience,
C  the ASCII table extension is appended to the FITS image file created 
C  previously by the WRITEIMAGE subroutine.

      integer status,unit,readwrite,blocksize,tfields,nrows,rowlen
      integer nspace,tbcol(3),diameter(6), colnum,frow,felem
      real density(6)
      character filename*40,extname*16
      character*16 ttype(3),tform(3),tunit(3),name(6)
      data ttype/'Planet','Diameter','Density'/
      data tform/'A8','I6','F4.2'/
      data tunit/' ','km','g/cm'/
      data name/'Mercury','Venus','Earth','Mars','Jupiter','Saturn'/
      data diameter/4880,12112,12742,6800,143000,121000/
      data density/5.1,5.3,5.52,3.94,1.33,0.69/

C  The STATUS parameter must always be initialized.
      status=0

C  Name of the FITS file to append the ASCII table to:
      filename='ATESTFILEZ.FITS'

C  Get an unused Logical Unit Number to use to open the FITS file.
      call ftgiou(unit,status)

C  Open the FITS file with write access.
C  (readwrite = 0 would open the file with readonly access).
      readwrite=1
      call ftopen(unit,filename,readwrite,blocksize,status)

C  FTCRHD creates a new empty FITS extension following the current
C  extension and moves to it.  In this case, FITSIO was initially
C  positioned on the primary array when the FITS file was first opened, so
C  FTCRHD appends an empty extension and moves to it.  All future FITSIO
C  calls then operate on the new extension (which will be an ASCII
C  table).
      call ftcrhd(unit,status)

C  define parameters for the ASCII table (see the above data statements)
      tfields=3
      nrows=6
      extname='PLANETS_ASCII'
      
C  FTGABC is a convenient subroutine for calculating the total width of
C  the table and the starting position of each column in an ASCII table.
C  Any number of blank spaces (including zero)  may be inserted between
C  each column of the table, as specified by the NSPACE parameter.
      nspace=1
      call ftgabc(tfields,tform,nspace,rowlen,tbcol,status)

C  FTPHTB writes all the required header keywords which define the
C  structure of the ASCII table. NROWS and TFIELDS give the number of
C  rows and columns in the table, and the TTYPE, TBCOL, TFORM, and TUNIT
C  arrays give the column name, starting position, format, and units,
C  respectively of each column. The values of the ROWLEN and TBCOL parameters
C  were previously calculated by the FTGABC routine.
      call ftphtb(unit,rowlen,nrows,tfields,ttype,tbcol,tform,tunit,
     &            extname,status)

C  Write names to the first column, diameters to 2nd col., and density to 3rd
C  FTPCLS writes the string values to the NAME column (column 1) of the
C  table.  The FTPCLJ and FTPCLE routines write the diameter (integer) and
C  density (real) value to the 2nd and 3rd columns.  The FITSIO routines
C  are column oriented, so it is usually easier to read or write data in a
C  table in a column by column order rather than row by row.
      frow=1
      felem=1
      colnum=1
      call ftpcls(unit,colnum,frow,felem,nrows,name,status)
      colnum=2
      call ftpclj(unit,colnum,frow,felem,nrows,diameter,status)  
      colnum=3
      call ftpcle(unit,colnum,frow,felem,nrows,density,status)  

C  The FITS file must always be closed before exiting the program. 
C  Any unit numbers allocated with FTGIOU must be freed with FTFIOU.
      call ftclos(unit, status)
      call ftfiou(unit, status)

C  Check for any error, and if so print out error messages.
C  The PRINTERROR subroutine is listed near the end of this file.
      if (status .gt. 0)call printerror(status)
      end
C *************************************************************************
      subroutine writebintable

C  This routine creates a FITS binary table, or BINTABLE, containing
C  3 columns and 6 rows.  This routine is nearly identical to the
C  previous WRITEASCII routine, except that the call to FTGABC is not
C  needed, and FTPHBN is called rather than FTPHTB to write the
C  required header keywords.

      integer status,unit,readwrite,blocksize,hdutype,tfields,nrows
      integer varidat,diameter(6), colnum,frow,felem
      real density(6)
      character filename*40,extname*16
      character*16 ttype(3),tform(3),tunit(3),name(6)
      data ttype/'Planet','Diameter','Density'/
      data tform/'8A','1J','1E'/
      data tunit/' ','km','g/cm'/
      data name/'Mercury','Venus','Earth','Mars','Jupiter','Saturn'/
      data diameter/4880,12112,12742,6800,143000,121000/
      data density/5.1,5.3,5.52,3.94,1.33,0.69/

C  The STATUS parameter must always be initialized.
      status=0

C  Name of the FITS file to append the ASCII table to:
      filename='ATESTFILEZ.FITS'

C  Get an unused Logical Unit Number to use to open the FITS file.
      call ftgiou(unit,status)

C  Open the FITS file, with write access.
      readwrite=1
      call ftopen(unit,filename,readwrite,blocksize,status)

C  Move to the last (2nd) HDU in the file (the ASCII table).
      call ftmahd(unit,2,hdutype,status)

C  Append/create a new empty HDU onto the end of the file and move to it.
      call ftcrhd(unit,status)

C  Define parameters for the binary table (see the above data statements)
      tfields=3
      nrows=6
      extname='PLANETS_BINARY'
      varidat=0
      
C  FTPHBN writes all the required header keywords which define the
C  structure of the binary table. NROWS and TFIELDS gives the number of
C  rows and columns in the table, and the TTYPE, TFORM, and TUNIT arrays
C  give the column name, format, and units, respectively of each column.
      call ftphbn(unit,nrows,tfields,ttype,tform,tunit,
     &            extname,varidat,status)

C  Write names to the first column, diameters to 2nd col., and density to 3rd
C  FTPCLS writes the string values to the NAME column (column 1) of the
C  table.  The FTPCLJ and FTPCLE routines write the diameter (integer) and
C  density (real) value to the 2nd and 3rd columns.  The FITSIO routines
C  are column oriented, so it is usually easier to read or write data in a
C  table in a column by column order rather than row by row.  Note that
C  the identical subroutine calls are used to write to either ASCII or
C  binary FITS tables.
      frow=1
      felem=1
      colnum=1
      call ftpcls(unit,colnum,frow,felem,nrows,name,status)
      colnum=2
      call ftpclj(unit,colnum,frow,felem,nrows,diameter,status)  
      colnum=3
      call ftpcle(unit,colnum,frow,felem,nrows,density,status)  

C  The FITS file must always be closed before exiting the program. 
C  Any unit numbers allocated with FTGIOU must be freed with FTFIOU.
      call ftclos(unit, status)
      call ftfiou(unit, status)

C  Check for any error, and if so print out error messages.
C  The PRINTERROR subroutine is listed near the end of this file.
      if (status .gt. 0)call printerror(status)
      end
C *************************************************************************
      subroutine copyhdu

C  Copy the 1st and 3rd HDUs from the input file to a new FITS file

      integer status,inunit,outunit,readwrite,blocksize,morekeys,hdutype
      character infilename*40,outfilename*40

C  The STATUS parameter must always be initialized.
      status=0

C     Name of the FITS files:
      infilename='ATESTFILEZ.FITS'
      outfilename='BTESTFILEZ.FITS'

C  Delete the file if it already exists, so we can then recreate it
C  The deletefile subroutine is listed at the end of this file.
      call deletefile(outfilename,status)

C  Get  unused Logical Unit Numbers to use to open the FITS files.
      call ftgiou(inunit,status)
      call ftgiou(outunit,status)

C  Open the input FITS file, with readonly access
      readwrite=0
      call ftopen(inunit,infilename,readwrite,blocksize,status)

C  Create the new empty FITS file (value of blocksize is ignored)
      blocksize=1
      call ftinit(outunit,outfilename,blocksize,status)

C  FTCOPY copies the current HDU from the input FITS file to the output
C  file.  The MOREKEY parameter allows one to reserve space for additional
C  header keywords when the HDU is created.   FITSIO will automatically
C  insert more header space if required, so programmers do not have to
C  reserve space ahead of time, although it is more efficient to do so if
C  it is known that more keywords will be appended to the header.
      morekeys=0
      call ftcopy(inunit,outunit,morekeys,status)

C  Append/create a new empty extension on the end of the output file
      call ftcrhd(outunit,status)

C  Skip to the 3rd extension in the input file which in this case
C  is the binary table created by the previous WRITEBINARY routine.
      call ftmahd(inunit,3,hdutype,status)

C  FTCOPY now copies the binary table from the input FITS file
C  to the output file.
      call ftcopy(inunit,outunit,morekeys,status)  

C  The FITS files must always be closed before exiting the program. 
C  Any unit numbers allocated with FTGIOU must be freed with FTFIOU.
C  Giving -1 for the value of the first argument causes all previously
C  allocated unit numbers to be released.

      call ftclos(inunit, status)
      call ftclos(outunit, status)
      call ftfiou(-1, status)

C  Check for any error, and if so print out error messages.
C  The PRINTERROR subroutine is listed near the end of this file.
      if (status .gt. 0)call printerror(status)
      end
C *************************************************************************
      subroutine selectrows

C  This routine copies selected rows from an input table into a new output
C  FITS table.  In this example all the rows in the input table that have
C  a value of the DENSITY column less that 3.0 are copied to the output
C  table.  This program illustrates several generally useful techniques,
C  including:
C      how to locate the end of a FITS file
C      how to create a table when the total number of rows in the table
C      is not known until the table is completed
C      how to efficiently copy entire rows from one table to another.

      integer status,inunit,outunit,readwrite,blocksize,hdutype
      integer nkeys,nspace,naxes(2),nfound,colnum,frow,felem
      integer noutrows,irow,temp(100),i
      real nullval,density(6)
      character infilename*40,outfilename*40,record*80
      logical exact,anynulls

C  The STATUS parameter must always be initialized.
      status=0

C     Names of the FITS files:
      infilename='ATESTFILEZ.FITS'
      outfilename='BTESTFILEZ.FITS'

C  Get  unused Logical Unit Numbers to use to open the FITS files.
      call ftgiou(inunit,status)
      call ftgiou(outunit,status)

C  The input FITS file is opened with READONLY access, and the output
C  FITS file is opened with WRITE access.
      readwrite=0
      call ftopen(inunit,infilename,readwrite,blocksize,status)
      readwrite=1
      call ftopen(outunit,outfilename,readwrite,blocksize,status)

C  move to the 3rd HDU in the input file (a binary table in this case)
      call ftmahd(inunit,3,hdutype,status)

C  This do-loop illustrates how to move to the last extension in any FITS
C  file.  The call to FTMRHD moves one extension at a time through the
C  FITS file until an `End-of-file' status value (= 107) is returned.
      do while (status .eq. 0)
          call ftmrhd(outunit,1,hdutype,status)
      end do

C  After locating the end of the FITS file, it is necessary to reset the
C  status value to zero and also clear the internal error message stack
C  in FITSIO.  The previous `End-of-file' error will have produced
C  an unimportant message on the error stack which can be cleared with
C  the call to the FTCMSG routine (which has no arguments).

      if (status .eq. 107)then
          status=0
          call ftcmsg
      end if

C  Create a new empty extension in the output file.
      call ftcrhd(outunit,status)

C  Find the number of keywords in the input table header.
      call ftghsp(inunit,nkeys,nspace,status)

C  This do-loop of calls to FTGREC and FTPREC copies all the keywords from
C  the input to the output FITS file.  Notice that the specified number
C  of rows in the output table, as given by the NAXIS2 keyword, will be
C  incorrect.  This value will be modified later after it is known how many
C  rows will be in the table, so it does not matter how many rows are specified
C  initially.
      do i=1,nkeys
          call ftgrec(inunit,i,record,status)
          call ftprec(outunit,record,status)
      end do

C  FTGKNJ is used to get the value of the NAXIS1 and NAXIS2 keywords,
C  which define the width of the table in bytes, and the number of
C  rows in the table.
      call ftgknj(inunit,'NAXIS',1,2,naxes,nfound,status)

C  FTGCNO gets the column number of the `DENSITY' column; the column
C  number is needed when reading the data in the column.  The EXACT
C  parameter determines whether or not the match to the column names
C  will be case sensitive.
      exact=.false.
      call ftgcno(inunit,exact,'DENSITY',colnum,status)

C  FTGCVE reads all 6 rows of data in the `DENSITY' column.  The number
C  of rows in the table is given by NAXES(2). Any null values in the
C  table will be returned with the corresponding value set to -99
C  (= the value of NULLVAL).  The ANYNULLS parameter will be set to TRUE
C  if any null values were found while reading the data values in the table.
      frow=1
      felem=1
      nullval=-99.
      call ftgcve(inunit,colnum,frow,felem,naxes(2),nullval,
     &            density,anynulls,status)

C  If the density is less than 3.0, copy the row to the output table.
C  FTGTBB and FTPTBB are low-level routines to read and write, respectively,
C  a specified number of bytes in the table, starting at the specified
C  row number and beginning byte within the row.  These routines do
C  not do any interpretation of the bytes, and simply pass them to or
C  from the FITS file without any modification.  This is a faster
C  way of transferring large chunks of data from one FITS file to another,
C  than reading and then writing each column of data individually.
C  In this case an entire row of bytes (the row length is specified
C  by the naxes(1) parameter) is transferred.  The datatype of the 
C  buffer array (TEMP in this case) is immaterial so long as it is
C  declared large enough to hold the required number of bytes.
      noutrows=0
      do irow=1,naxes(2)
          if (density(irow) .lt. 3.0)then
              noutrows=noutrows+1
              call ftgtbb(inunit,irow,1,naxes(1),temp,status)
              call ftptbb(outunit,noutrows,1,naxes(1),temp,status)
          end if
      end do

C  Update the NAXIS2 keyword with the correct no. of rows in the output file.
C  After all the rows have been written to the output table, the
C  FTMKYJ routine is used to overwrite the NAXIS2 keyword value with
C  the correct number of rows.  Specifying `\&' for the comment string
C  tells FITSIO to keep the current comment string in the keyword and
C  only modify the value.  Because the total number of rows in the table
C  was unknown when the table was first created, any value (including 0)
C  could have been used for the initial NAXIS2 keyword value.
      call ftmkyj(outunit,'NAXIS2',noutrows,'&',status)

C  The FITS files must always be closed before exiting the program. 
C  Any unit numbers allocated with FTGIOU must be freed with FTFIOU.
      call ftclos(inunit, status)
      call ftclos(outunit, status)
      call ftfiou(-1, status)

C  Check for any error, and if so print out error messages.
C  The PRINTERROR subroutine is listed near the end of this file.
      if (status .gt. 0)call printerror(status)
      end
C *************************************************************************
      subroutine readheader

C  Print out all the header keywords in all extensions of a FITS file

      integer status,unit,readwrite,blocksize,nkeys,nspace,hdutype,i,j
      character filename*80,record*80

C  The STATUS parameter must always be initialized.
      status=0

C  Get an unused Logical Unit Number to use to open the FITS file.
      call ftgiou(unit,status)

C     name of FITS file 
      filename='ATESTFILEZ.FITS'

C     open the FITS file, with read-only access.  The returned BLOCKSIZE
C     parameter is obsolete and should be ignored. 
      readwrite=0
      call ftopen(unit,filename,readwrite,blocksize,status)

      j = 0
100   continue
      j = j + 1

      print *,'Header listing for HDU', j

C  The FTGHSP subroutine returns the number of existing keywords in the
C  current header data unit (CHDU), not counting the required END keyword,
      call ftghsp(unit,nkeys,nspace,status)

C  Read each 80-character keyword record, and print it out.
      do i = 1, nkeys
          call ftgrec(unit,i,record,status)
          print *,record
      end do

C  Print out an END record, and a blank line to mark the end of the header.
      if (status .eq. 0)then
          print *,'END'
          print *,' '
      end if

C  Try moving to the next extension in the FITS file, if it exists.
C  The FTMRHD subroutine attempts to move to the next HDU, as specified by
C  the second parameter.   This subroutine moves by a relative number of
C  HDUs from the current HDU.  The related FTMAHD routine may be used to
C  move to an absolute HDU number in the FITS file.  If the end-of-file is
C  encountered when trying to move to the specified extension, then a
C  status = 107 is returned.
      call ftmrhd(unit,1,hdutype,status)

      if (status .eq. 0)then
C         success, so jump back and print out keywords in this extension
          go to 100

      else if (status .eq. 107)then
C         hit end of file, so quit
          status=0
      end if

C  The FITS file must always be closed before exiting the program. 
C  Any unit numbers allocated with FTGIOU must be freed with FTFIOU.
      call ftclos(unit, status)
      call ftfiou(unit, status)

C  Check for any error, and if so print out error messages.
C  The PRINTERROR subroutine is listed near the end of this file.
      if (status .gt. 0)call printerror(status)
      end
C *************************************************************************
      subroutine readimage

C  Read a FITS image and determine the minimum and maximum pixel value.
C  Rather than reading the entire image in
C  at once (which could require a very large array), the image is read
C  in pieces, 100 pixels at a time.  

      integer status,unit,readwrite,blocksize,naxes(2),nfound
      integer group,firstpix,nbuffer,npixels,i
      real datamin,datamax,nullval,buffer(100)
      logical anynull
      character filename*80

C  The STATUS parameter must always be initialized.
      status=0

C  Get an unused Logical Unit Number to use to open the FITS file.
      call ftgiou(unit,status)

C  Open the FITS file previously created by WRITEIMAGE
      filename='ATESTFILEZ.FITS'
      readwrite=0
      call ftopen(unit,filename,readwrite,blocksize,status)

C  Determine the size of the image.
      call ftgknj(unit,'NAXIS',1,2,naxes,nfound,status)

C  Check that it found both NAXIS1 and NAXIS2 keywords.
      if (nfound .ne. 2)then
          print *,'READIMAGE failed to read the NAXISn keywords.'
          return
       end if

C  Initialize variables
      npixels=naxes(1)*naxes(2)
      group=1
      firstpix=1
      nullval=-999
      datamin=1.0E30
      datamax=-1.0E30

      do while (npixels .gt. 0)
C         read up to 100 pixels at a time 
          nbuffer=min(100,npixels)
      
          call ftgpve(unit,group,firstpix,nbuffer,nullval,
     &            buffer,anynull,status)

C         find the min and max values
          do i=1,nbuffer
              datamin=min(datamin,buffer(i))
              datamax=max(datamax,buffer(i))
          end do

C         increment pointers and loop back to read the next group of pixels
          npixels=npixels-nbuffer
          firstpix=firstpix+nbuffer
      end do

      print *
      print *,'Min and max image pixels = ',datamin,datamax

C  The FITS file must always be closed before exiting the program. 
C  Any unit numbers allocated with FTGIOU must be freed with FTFIOU.
      call ftclos(unit, status)
      call ftfiou(unit, status)

C  Check for any error, and if so print out error messages.
C  The PRINTERROR subroutine is listed near the end of this file.
      if (status .gt. 0)call printerror(status)
      end
C *************************************************************************
      subroutine readtable

C  Read and print data values from an ASCII or binary table
C  This example reads and prints out all the data in the ASCII and
C  the binary tables that were previously created by WRITEASCII and
C  WRITEBINTABLE.  Note that the exact same FITSIO routines are
C  used to read both types of tables.

      integer status,unit,readwrite,blocksize,hdutype,ntable
      integer felem,nelems,nullj,diameter,nfound,irow,colnum
      real nulle,density
      character filename*40,nullstr*1,name*8,ttype(3)*10
      logical anynull

C  The STATUS parameter must always be initialized.
      status=0

C  Get an unused Logical Unit Number to use to open the FITS file.
      call ftgiou(unit,status)

C  Open the FITS file previously created by WRITEIMAGE
      filename='ATESTFILEZ.FITS'
      readwrite=0
      call ftopen(unit,filename,readwrite,blocksize,status)

C  Loop twice, first reading the ASCII table, then the binary table
      do ntable=2,3

C  Move to the next extension
          call ftmahd(unit,ntable,hdutype,status)

          print *,' '
          if (hdutype .eq. 1)then
              print *,'Reading ASCII table in HDU ',ntable
          else if (hdutype .eq. 2)then
              print *,'Reading binary table in HDU ',ntable
          end if

C  Read the TTYPEn keywords, which give the names of the columns
          call ftgkns(unit,'TTYPE',1,3,ttype,nfound,status)
          write(*,2000)ttype
2000      format(2x,"Row   ",3a10)

C  Read the data, one row at a time, and print them out
          felem=1
          nelems=1
          nullstr=' '
          nullj=0
          nulle=0.
          do irow=1,6
C             FTGCVS reads the NAMES from the first column of the table.
              colnum=1
              call ftgcvs(unit,colnum,irow,felem,nelems,nullstr,name,
     &                    anynull,status)

C             FTGCVJ reads the DIAMETER values from the second column.
              colnum=2
              call ftgcvj(unit,colnum,irow,felem,nelems,nullj,diameter,
     &                    anynull,status)

C             FTGCVE reads the DENSITY values from the third column.
              colnum=3
              call ftgcve(unit,colnum,irow,felem,nelems,nulle,density,
     &                    anynull,status)
              write(*,2001)irow,name,diameter,density
2001          format(i5,a10,i10,f10.2)
          end do
      end do

C  The FITS file must always be closed before exiting the program. 
C  Any unit numbers allocated with FTGIOU must be freed with FTFIOU.
      call ftclos(unit, status)
      call ftfiou(unit, status)

C  Check for any error, and if so print out error messages.
C  The PRINTERROR subroutine is listed near the end of this file.
      if (status .gt. 0)call printerror(status)
      end
C *************************************************************************
      subroutine printerror(status)

C  This subroutine prints out the descriptive text corresponding to the
C  error status value and prints out the contents of the internal
C  error message stack generated by FITSIO whenever an error occurs.

      integer status
      character errtext*30,errmessage*80

C  Check if status is OK (no error); if so, simply return
      if (status .le. 0)return

C  The FTGERR subroutine returns a descriptive 30-character text string that
C  corresponds to the integer error status number.  A complete list of all
C  the error numbers can be found in the back of the FITSIO User's Guide.
      call ftgerr(status,errtext)
      print *,'FITSIO Error Status =',status,': ',errtext

C  FITSIO usually generates an internal stack of error messages whenever
C  an error occurs.  These messages provide much more information on the
C  cause of the problem than can be provided by the single integer error
C  status value.  The FTGMSG subroutine retrieves the oldest message from
C  the stack and shifts any remaining messages on the stack down one
C  position.  FTGMSG is called repeatedly until a blank message is
C  returned, which indicates that the stack is empty.  Each error message
C  may be up to 80 characters in length.  Another subroutine, called
C  FTCMSG, is available to simply clear the whole error message stack in
C  cases where one is not interested in the contents.
      call ftgmsg(errmessage)
      do while (errmessage .ne. ' ')
          print *,errmessage
          call ftgmsg(errmessage)
      end do
      end
C *************************************************************************
      subroutine deletefile(filename,status)

C  A simple little routine to delete a FITS file

      integer status,unit,blocksize
      character*(*) filename

C  Simply return if status is greater than zero
      if (status .gt. 0)return

C  Get an unused Logical Unit Number to use to open the FITS file
      call ftgiou(unit,status)

C  Try to open the file, to see if it exists
      call ftopen(unit,filename,1,blocksize,status)

      if (status .eq. 0)then
C         file was opened;  so now delete it 
          call ftdelt(unit,status)
      else if (status .eq. 103)then
C         file doesn't exist, so just reset status to zero and clear errors
          status=0
          call ftcmsg
      else
C         there was some other error opening the file; delete the file anyway
          status=0
          call ftcmsg
          call ftdelt(unit,status)
      end if

C  Free the unit number for later reuse
      call ftfiou(unit, status)
      end
