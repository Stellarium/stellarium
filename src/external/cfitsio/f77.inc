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

