      program f77iterate_b

C     external work function is passed to the iterator
      external str_iter

      integer ncols
      parameter (ncols=2)
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

      status = 0

      fname = 'iter_b.fit'
      iunit = 15

C     both columns are in the same FITS file
      units(1) = iunit
      units(2) = iunit

C     open the file and move to the correct extension
      call ftopen(iunit,fname,1,blocksize,status)
      call ftmnhd(iunit, BINARY_TBL, 'iter_test', 0, status)

C     define the desired columns by name
      colname(1) = 'Avalue'
      colname(2) = 'Lvalue'

C     leave column numbers undefined
      colnum(1) = 0
      colnum(2) = 0  

C     define the desired datatype for each column: TSTRING & TLOGICAL
      datatype(1) = TSTRING
      datatype(2) = TLOGICAL

C     define whether columns are input, input/output, or output only
C     Both in/out
      iotype(1) = InputOutputCol
      iotype(2) = InputOutputCol
 
C     use default optimum number of rows and process all the rows
      rows_per_loop = 0
      offset = 0

C     apply the  function to each row of the table
      print *,'Calling iterator function...', status

      call ftiter( ncols, units, colnum, colname, datatype, iotype,
     &      offset, rows_per_loop, str_iter, 0, status )

      call ftclos(iunit, status)

C     print out error messages if problem
      if (status.ne.0) call ftrprt('STDERR', status)
      stop
      end

C--------------------------------------------------------------------------
C
C   Sample iterator function.
C
C--------------------------------------------------------------------------
      subroutine str_iter(totalrows, offset, firstrow, nrows, ncols,
     &     units, colnum, datatype, iotype, repeat, status, 
     &     userData, stringCol, logicalCol )

      integer totalrows,offset,firstrow,nrows,ncols,status
      integer units(*),colnum(*),datatype(*),iotype(*),repeat(*)
      integer userData
      character*(*) stringCol(*)
      logical logicalCol(*)

      integer ii

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

      if (status .ne. 0) return

C    --------------------------------------------------------
C      Initialization procedures: execute on the first call  
C    --------------------------------------------------------
      if (firstrow .eq. 1) then
         if (ncols .ne. 2) then
            status = -1
            return
         endif
         
         if (datatype(1).ne.TSTRING .or. datatype(2).ne.TLOGICAL) then
            status = -2
            return
         endif
         
         print *,'Total rows, No. rows = ',totalrows, nrows
         
      endif
      
C     -------------------------------------------
C       Main loop: process all the rows of data 
C     -------------------------------------------
      
C     NOTE: 1st element of array is the null pixel value!
C     Loop over elements 2 to nrows+1, not 1 to nrows.
      
      do 10 ii=2,nrows+1
         print *, stringCol(ii), logicalCol(ii)
         if( logicalCol(ii) ) then
            logicalCol(ii) = .false.
            stringCol(ii) = 'changed to false'
         else
            logicalCol(ii) = .true.
            stringCol(ii) = 'changed to true'
         endif
 10   continue
      
C     -------------------------------------------------------
C     Clean up procedures:  after processing all the rows  
C     -------------------------------------------------------
      
      if (firstrow + nrows - 1 .eq. totalrows) then
C     no action required in this case
      endif
      
      return
      end
      
