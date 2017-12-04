      program f77iterate_a

      external flux_rate
      integer ncols
      parameter (ncols=3)
      integer units(ncols), colnum(ncols), datatype(ncols)
      integer iotype(ncols), offset, rows_per_loop, status
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


      iunit = 15

      units(1) = iunit
      units(2) = iunit
      units(3) = iunit

C open the file
      fname = 'iter_a.fit'
      call ftopen(iunit,fname,1,blocksize,status)

C move to the HDU containing the rate table
      call ftmnhd(iunit, BINARY_TBL, 'RATE', 0, status)

C Select iotypes for column data
      iotype(1) = InputCol
      iotype(2) = InputCol
      iotype(3) = OutputCol

C Select desired datatypes for column data
      datatype(1) = TINT
      datatype(2) = TFLOAT
      datatype(3) = TFLOAT

C find the column number corresponding to each column
      call ftgcno( iunit, 0, 'counts', colnum(1), status )
      call ftgcno( iunit, 0, 'time', colnum(2), status )
      call ftgcno( iunit, 0, 'rate', colnum(3), status )

C use default optimum number of rows
      rows_per_loop = 0
      offset = 0

C apply the rate function to each row of the table
      print *, 'Calling iterator function...', status

C although colname is not being used, still need to send a string
C array in the function
      call ftiter( ncols, units, colnum, colname, datatype, iotype,
     &      offset, rows_per_loop, flux_rate, 3, status )

      call ftclos(iunit, status)
      stop
      end

C***************************************************************************
C   Sample iterator function that calculates the output flux 'rate' column
C   by dividing the input 'counts' by the 'time' column.
C   It also applies a constant deadtime correction factor if the 'deadtime'
C   keyword exists.  Finally, this creates or updates the 'LIVETIME'
C   keyword with the sum of all the individual integration times.
C***************************************************************************
      subroutine flux_rate(totalrows, offset, firstrow, nrows, ncols,
     &     units, colnum, datatype, iotype, repeat, status, userData,
     &     counts, interval, rate )

      integer totalrows, offset, firstrow, nrows, ncols
      integer units(ncols), colnum(ncols), datatype(ncols)
      integer iotype(ncols), repeat(ncols)
      integer userData

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

      integer counts(*)
      real interval(*),rate(*)

      integer ii, status
      character*80 comment

C**********************************************************************
C  must preserve these values between calls
      real deadtime, livetime
      common /fluxblock/ deadtime, livetime
C**********************************************************************

      if (status .ne. 0) return

C    --------------------------------------------------------
C      Initialization procedures: execute on the first call  
C    --------------------------------------------------------
      if (firstrow .eq. 1) then
         if (ncols .ne. 3) then
C     wrong number of columns
            status = -1
            return
         endif

         if (datatype(1).ne.TINT .or. datatype(2).ne.TFLOAT .or.
     &        datatype(3).ne.TFLOAT ) then
C     bad data type
            status = -2
            return
         endif

C     try to get the deadtime keyword value
         call ftgkye( units(1), 'DEADTIME', deadtime, comment, status )

         if (status.ne.0) then
C     default deadtime if keyword doesn't exist
            deadtime = 1.0
            status = 0
         elseif (deadtime .lt. 0.0 .or. deadtime .gt. 1.0) then
C     bad deadtime value
            status = -3
            return
         endif

         print *, 'deadtime = ', deadtime

         livetime = 0.0
      endif

C    --------------------------------------------
C      Main loop: process all the rows of data
C    --------------------------------------------
      
C     NOTE: 1st element of array is the null pixel value!
C     Loop over elements 2 to nrows+1, not 1 to nrows.
      
C     this version ignores null values

C     set the output null value to zero to ignore nulls */
      rate(1) = 0.0
      do 10 ii = 2,nrows+1
         if ( interval(ii) .gt. 0.0) then
           rate(ii) = counts(ii) / interval(ii) / deadtime
           livetime = livetime + interval(ii)
        else
C     Nonsensical negative time interval
           status = -3
           return
        endif
 10   continue

C    -------------------------------------------------------
C      Clean up procedures:  after processing all the rows  
C    -------------------------------------------------------

      if (firstrow + nrows - 1 .eq. totalrows) then
C     update the LIVETIME keyword value

         call ftukye( units(1),'LIVETIME', livetime, 3,
     &        'total integration time', status )
         print *,'livetime = ', livetime

      endif
 
      return
      end
