      program f77iterate_c
C
C    This example program illustrates how to use the CFITSIO iterator function.
C
C    This program creates a 2D histogram of the X and Y columns of an event
C    list.  The 'main' routine just creates the empty new image, then executes
C    the 'writehisto' work function by calling the CFITSIO iterator function.
C
C    'writehisto' opens the FITS event list that contains the X and Y columns.
C    It then calls a second work function, calchisto, (by recursively calling
C    the CFITSIO iterator function) which actually computes the 2D histogram.

C     external work function to be passed to the iterator
      external writehisto

      integer ncols
      parameter (ncols=1)
      integer units(ncols), colnum(ncols), datatype(ncols)
      integer iotype(ncols), offset, n_per_loop, status
      character*70 colname(ncols)

      integer naxes(2), ounit, blocksize
      character*80 fname
      logical exists

C     include f77.inc -------------------------------------
C     Codes for FITS extension types
      integer IMAGE_HDU, ASCII_TBL, BINARY_TBL
      parameter (
     &     IMAGE_HDU  = 0,
     &     ASCII_TBL  = 1,
     &     BINARY_TBL = 2  )

C     Codes for FITS table data types

      integer TBIT,TBYTE,TLOGICAL,TSTRING,TSHORT,TINT
      integer TFLOAT,TDOUBLE,TCOMPLEX,TDBLCOMPLEX
      parameter (
     &     TBIT        =   1,
     &     TBYTE       =  11,
     &     TLOGICAL    =  14,
     &     TSTRING     =  16,
     &     TSHORT      =  21,
     &     TINT        =  31,
     &     TFLOAT      =  42,
     &     TDOUBLE     =  82,
     &     TCOMPLEX    =  83,
     &     TDBLCOMPLEX = 163  )

C     Codes for iterator column types

      integer InputCol, InputOutputCol, OutputCol
      parameter (
     &     InputCol       = 0,
     &     InputOutputCol = 1,
     &     OutputCol      = 2  )
C     End of f77.inc -------------------------------------

C**********************************************************************
C     Need to make these variables available to the 2 work functions
      integer xsize,ysize,xbinsize,ybinsize
      common /histcomm/ xsize,ysize,xbinsize,ybinsize
C**********************************************************************

      status = 0

      xsize = 480
      ysize = 480
      xbinsize = 32
      ybinsize = 32

      fname = 'histoimg.fit'
      ounit = 15

C     delete previous version of the file if it exists
      inquire(file=fname,exist=exists)
      if( exists ) then
         open(ounit,file=fname,status='old')
         close(ounit,status='delete')
      endif
 99   blocksize = 2880

C     create new output image
      call ftinit(ounit,fname,blocksize,status)

      naxes(1) = xsize
      naxes(2) = ysize

C     create primary HDU
      call ftiimg(ounit,32,2,naxes,status)

      units(1) = ounit

C     Define column as TINT and Output
      datatype(1) = TINT
      iotype(1) = OutputCol

C     force whole array to be passed at one time
      n_per_loop = -1
      offset = 0

C     execute the function to create and write the 2D histogram
      print *,'Calling writehisto iterator work function... ',status

      call ftiter( ncols, units, colnum, colname, datatype, iotype,
     &      offset, n_per_loop, writehisto, 0, status )

      call ftclos(ounit, status)

C     print out error messages if problem
      if (status.ne.0) then
         call ftrprt('STDERR', status)
      else
        print *,'Program completed successfully.'
      endif

      stop
      end

C--------------------------------------------------------------------------
C
C   Sample iterator function.
C
C   Iterator work function that writes out the 2D histogram.
C   The histogram values are calculated by another work function, calchisto.
C
C--------------------------------------------------------------------------
      subroutine writehisto(totaln, offset, firstn, nvalues, narrays,
     &     units_out, colnum_out, datatype_out, iotype_out, repeat,
     &     status, userData, histogram )

      integer totaln,offset,firstn,nvalues,narrays,status
      integer units_out(narrays),colnum_out(narrays)
      integer datatype_out(narrays),iotype_out(narrays)
      integer repeat(narrays)
      integer histogram(*), userData

      external calchisto
      integer ncols
      parameter (ncols=2)
      integer units(ncols), colnum(ncols), datatype(ncols)
      integer iotype(ncols), rowoffset, rows_per_loop
      character*70 colname(ncols)

      integer iunit, blocksize
      character*80 fname

C     include f77.inc -------------------------------------
C     Codes for FITS extension types
      integer IMAGE_HDU, ASCII_TBL, BINARY_TBL
      parameter (
     &     IMAGE_HDU  = 0,
     &     ASCII_TBL  = 1,
     &     BINARY_TBL = 2  )

C     Codes for FITS table data types

      integer TBIT,TBYTE,TLOGICAL,TSTRING,TSHORT,TINT
      integer TFLOAT,TDOUBLE,TCOMPLEX,TDBLCOMPLEX
      parameter (
     &     TBIT        =   1,
     &     TBYTE       =  11,
     &     TLOGICAL    =  14,
     &     TSTRING     =  16,
     &     TSHORT      =  21,
     &     TINT        =  31,
     &     TFLOAT      =  42,
     &     TDOUBLE     =  82,
     &     TCOMPLEX    =  83,
     &     TDBLCOMPLEX = 163  )

C     Codes for iterator column types

      integer InputCol, InputOutputCol, OutputCol
      parameter (
     &     InputCol       = 0,
     &     InputOutputCol = 1,
     &     OutputCol      = 2  )
C     End of f77.inc -------------------------------------

C**********************************************************************
C     Need to make these variables available to the 2 work functions
      integer xsize,ysize,xbinsize,ybinsize
      common /histcomm/ xsize,ysize,xbinsize,ybinsize
C**********************************************************************

      if (status .ne. 0) return

C     name of FITS table
      fname = 'iter_c.fit'
      iunit = 16

C     do sanity checking of input values
      if (totaln .ne. nvalues) then
C     whole image must be passed at one time
         status = -1
         return
      endif

      if (narrays .ne. 1) then
C     number of images is incorrect
         status = -2
         return
      endif

      if (datatype_out(1) .ne. TINT) then
C     input array has wrong data type
         status = -3
         return
      endif

C     open the file and move to the table containing the X and Y columns
      call ftopen(iunit,fname,0,blocksize,status)
      call ftmnhd(iunit, BINARY_TBL, 'EVENTS', 0, status)
      if (status) return
   
C     both the columns are in the same FITS file
      units(1) = iunit
      units(2) = iunit

C     desired datatype for each column: TINT
      datatype(1) = TINT
      datatype(2) = TINT

C     names of the columns
      colname(1) = 'X'
      colname(2) = 'Y'

C     leave column numbers undefined
      colnum(1) = 0
      colnum(2) = 0

C     define whether columns are input, input/output, or output only
C     Both input
      iotype(1) = InputCol
      iotype(1) = InputCol
 
C     take default number of rows per iteration
      rows_per_loop = 0
      rowoffset = 0

C     calculate the histogram
      print *,'Calling calchisto iterator work function... ', status

      call ftiter( ncols, units, colnum, colname, datatype, iotype,
     &      rowoffset, rows_per_loop, calchisto, histogram, status )

      call ftclos(iunit,status)
      return
      end

C--------------------------------------------------------------------------
C
C   Iterator work function that calculates values for the 2D histogram.
C
C--------------------------------------------------------------------------
      subroutine calchisto(totalrows, offset, firstrow, nrows, ncols,
     &     units, colnum, datatype, iotype, repeat, status, 
     &     histogram, xcol, ycol )

      integer totalrows,offset,firstrow,nrows,ncols,status
      integer units(ncols),colnum(ncols),datatype(ncols)
      integer iotype(ncols),repeat(ncols)
      integer histogram(*),xcol(*),ycol(*)
C     include f77.inc -------------------------------------
C     Codes for FITS extension types
      integer IMAGE_HDU, ASCII_TBL, BINARY_TBL
      parameter (
     &     IMAGE_HDU  = 0,
     &     ASCII_TBL  = 1,
     &     BINARY_TBL = 2  )

C     Codes for FITS table data types

      integer TBIT,TBYTE,TLOGICAL,TSTRING,TSHORT,TINT
      integer TFLOAT,TDOUBLE,TCOMPLEX,TDBLCOMPLEX
      parameter (
     &     TBIT        =   1,
     &     TBYTE       =  11,
     &     TLOGICAL    =  14,
     &     TSTRING     =  16,
     &     TSHORT      =  21,
     &     TINT        =  31,
     &     TFLOAT      =  42,
     &     TDOUBLE     =  82,
     &     TCOMPLEX    =  83,
     &     TDBLCOMPLEX = 163  )

C     Codes for iterator column types

      integer InputCol, InputOutputCol, OutputCol
      parameter (
     &     InputCol       = 0,
     &     InputOutputCol = 1,
     &     OutputCol      = 2  )
C     End of f77.inc -------------------------------------

      integer ii, ihisto, xbin, ybin

C**********************************************************************
C     Need to make these variables available to the 2 work functions
      integer xsize,ysize,xbinsize,ybinsize
      common /histcomm/ xsize,ysize,xbinsize,ybinsize
C**********************************************************************

      if (status .ne. 0) return

C    --------------------------------------------------------
C      Initialization procedures: execute on the first call  
C    --------------------------------------------------------
      if (firstrow .eq. 1) then
C     do sanity checking of input values

         if (ncols .ne. 2) then
C     number of arrays is incorrect
            status = -4
            return
         endif

         if (datatype(1).ne.TINT .or. datatype(2).ne.TINT) then
C     wrong datatypes
            status = -5
            return
         endif

C     initialize the histogram image pixels = 0, including null value
         do 10 ii = 1, xsize * ysize + 1
            histogram(ii) = 0
 10     continue

      endif

C     ------------------------------------------------------------------
C       Main loop: increment the 2D histogram at position of each event 
C     ------------------------------------------------------------------

      do 20 ii=2,nrows+1
        xbin = xcol(ii) / xbinsize
        ybin = ycol(ii) / ybinsize

        ihisto = ( ybin * xsize ) + xbin + 2
        histogram(ihisto) = histogram(ihisto) + 1
 20   continue

      return
      end

