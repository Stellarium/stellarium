C     This is a big and complicated program that tests most of
C     the fitsio routines.  This code does not represent
C     the most efficient method of reading or writing FITS files 
C     because this code is primarily designed to stress the fitsio
C     library routines.

      character asciisum*17
      character*3 cval
      character*1 xinarray(21), binarray(21), boutarray(21), bnul
      character colname*70, tdisp*40, nulstr*40
      character oskey*15
      character iskey*21
      character lstr*200   
      character  comm*73
      character*30 inskey(21)
      character*30 onskey(3)
      character filename*40, card*78, card2*78
      character keyword*8
      character value*68, comment*72
      character uchars*78
      character*15 ttype(10), tform(10), tunit(10)
      character*15 tblname
      character*15 binname
      character errmsg*75
      character*8  inclist(2),exclist(2)
      character*8 xctype,yctype,ctype
      character*18 kunit

      logical simple,extend,larray(42), larray2(42)
      logical olkey, ilkey, onlkey(3), inlkey(3), anynull

      integer*2 imgarray(19,30), imgarray2(10,20)
      integer*2         iinarray(21), ioutarray(21), inul

      integer naxes(3), pcount, gcount, npixels, nrows, rowlen
      integer existkeys, morekeys, keynum
      integer datastatus, hdustatus
      integer status, bitpix, naxis, block
      integer ii, jj, jjj, hdutype, hdunum, tfields
      integer nkeys, nfound, colnum, typecode, signval,nmsg
      integer repeat, offset, width, jnulval
      integer kinarray(21), koutarray(21), knul
      integer jinarray(21), joutarray(21), jnul
      integer ojkey, ijkey, otint
      integer onjkey(3), injkey(3)
      integer tbcol(5)
      integer iunit, tmpunit
      integer fpixels(2), lpixels(2), inc(2)

      real estatus, vers
      real einarray(21), eoutarray(21), enul, cinarray(42)
      real ofkey, oekey, iekey, onfkey(3),onekey(3), inekey(3)

      double precision dinarray(21),doutarray(21),dnul, minarray(42)
      double precision scale, zero
      double precision ogkey, odkey, idkey, otfrac, ongkey(3)
      double precision ondkey(3), indkey(3)
      double precision checksum, datsum
      double precision xrval,yrval,xrpix,yrpix,xinc,yinc,rot
      double precision xpos,ypos,xpix,ypix

      tblname = 'Test-ASCII'
      binname = 'Test-BINTABLE'
      onskey(1) = 'first string'
      onskey(2) = 'second string'
      onskey(3) = '        '
      oskey = 'value_string'
      inclist(1)='key*'
      inclist(2)='newikys'
      exclist(1)='key_pr*'
      exclist(2)='key_pkls'
      xctype='RA---TAN'
      yctype='DEC--TAN'

      olkey = .true.
      ojkey = 11
      otint = 12345678
      ofkey = 12.121212
      oekey = 13.131313
      ogkey = 14.1414141414141414D+00
      odkey = 15.1515151515151515D+00
      otfrac = .1234567890123456D+00
      onlkey(1) = .true.
      onlkey(2) = .false.
      onlkey(3) = .true.
      onjkey(1) = 11
      onjkey(2) = 12
      onjkey(3) = 13
      onfkey(1) = 12.121212
      onfkey(2) = 13.131313
      onfkey(3) = 14.141414
      onekey(1) = 13.131313
      onekey(2) = 14.141414
      onekey(3) = 15.151515
      ongkey(1) = 14.1414141414141414D+00
      ongkey(2) = 15.1515151515151515D+00
      ongkey(3) = 16.1616161616161616D+00
      ondkey(1) = 15.1515151515151515D+00
      ondkey(2) = 16.1616161616161616D+00
      ondkey(3) = 17.1717171717171717D+00

      tbcol(1) = 1
      tbcol(2) =  17
      tbcol(3) =  28
      tbcol(4) =  43
      tbcol(5) =  56
      status = 0

      call ftvers(vers)
      write(*,'(1x,A)') 'FITSIO TESTPROG'
      write(*, '(1x,A)')' '

      iunit = 15
      tmpunit = 16

      write(*,'(1x,A)') 'Try opening then closing a nonexistent file: '
      call ftopen(iunit, 'tq123x.kjl', 1, block, status)
      write(*,'(1x,A,2i4)')'  ftopen iunit, status (expect an error) ='
     & ,iunit, status
      call ftclos(iunit, status)
      write(*,'(1x,A,i4)')'  ftclos status = ', status
      write(*,'(1x,A)')' '

      call ftcmsg
      status = 0

      filename = 'testf77.fit'

C delete previous version of the file, if it exists 

      call ftopen(iunit, filename, 1, block, status)
      if (status .eq. 0)then
         call ftdelt(iunit, status)
      else
C        clear the error message stack
         call ftcmsg
      end if

      status = 0

C
C        #####################
C        #  create FITS file #
C        #####################
      

      call ftinit(iunit, filename, 1, status)
      write(*,'(1x,A,i4)')'ftinit create new file status = ', status
      write(*,'(1x,A)')' '

      if (status .ne. 0)go to 999

      simple = .true.
      bitpix = 32
      naxis = 2
      naxes(1) = 10
      naxes(2) = 2
      npixels = 20
      pcount = 0
      gcount = 1
      extend = .true.
      
C        ############################
C        #  write single keywords   #
C        ############################
      
      call ftphpr(iunit,simple, bitpix, naxis, naxes, 
     & 0,1,extend,status)

      call ftprec(iunit, 
     &'key_prec= ''This keyword was written by fxprec'' / '//
     & 'comment goes here',  status)

      write(*,'(1x,A)') 'test writing of long string keywords: '
      card = '1234567890123456789012345678901234567890'//
     & '12345678901234567890123456789012345'
      call ftpkys(iunit, 'card1', card, ' ', status)
      call ftgkey(iunit, 'card1', card2, comment, status)

      write(*,'(1x,A)') card
      write(*,'(1x,A)') card2
      
      card = '1234567890123456789012345678901234567890'//
     &  '123456789012345678901234''6789012345'
      call ftpkys(iunit, 'card2', card, ' ', status)
      call ftgkey(iunit, 'card2', card2, comment, status)
      write(*,'(1x,A)') card
      write(*,'(1x,A)') card2
      
      card = '1234567890123456789012345678901234567890'//
     &  '123456789012345678901234''''789012345'
      call ftpkys(iunit, 'card3', card, ' ', status)
      call ftgkey(iunit, 'card3', card2, comment, status)
      write(*,'(1x,A)') card
      write(*,'(1x,A)') card2
      
      card = '1234567890123456789012345678901234567890'//
     & '123456789012345678901234567''9012345'
      call ftpkys(iunit, 'card4', card, ' ', status)
      call ftgkey(iunit, 'card4', card2, comment, status)
      write(*,'(1x,A)') card
      write(*,'(1x,A)') card2

      call ftpkys(iunit, 'key_pkys', oskey, 'fxpkys comment', status)
      call ftpkyl(iunit, 'key_pkyl', olkey, 'fxpkyl comment', status)
      call ftpkyj(iunit, 'key_pkyj', ojkey, 'fxpkyj comment', status)
      call ftpkyf(iunit,'key_pkyf',ofkey,5, 'fxpkyf comment', status)
      call ftpkye(iunit,'key_pkye',oekey,6, 'fxpkye comment', status)
      call ftpkyg(iunit,'key_pkyg',ogkey,14, 'fxpkyg comment',status)
      call ftpkyd(iunit,'key_pkyd',odkey,14, 'fxpkyd comment',status)

      lstr='This is a very long string '//
     &   'value that is continued over more than one keyword.'

      call ftpkls(iunit,'key_pkls',lstr,'fxpkls comment',status)

      call ftplsw(iunit, status)
      call ftpkyt(iunit,'key_pkyt',otint,otfrac,'fxpkyt comment',
     & status)
      call ftpcom(iunit, 'This keyword was written by fxpcom.',
     &  status)
      call ftphis(iunit, 
     &'  This keyword written by fxphis (w/ 2 leading spaces).',
     &    status)

      call ftpdat(iunit, status)
      
      if (status .gt. 0)go to 999   

C
C        ###############################
C        #  write arrays of keywords   #
C        ###############################
      
      nkeys = 3

      comm = 'fxpkns comment&'
      call ftpkns(iunit, 'ky_pkns', 1, nkeys, onskey, comm, status)
      comm = 'fxpknl comment&'
      call ftpknl(iunit, 'ky_pknl', 1, nkeys, onlkey, comm, status)

      comm = 'fxpknj comment&'
      call ftpknj(iunit, 'ky_pknj', 1, nkeys, onjkey, comm, status)

      comm = 'fxpknf comment&'
      call ftpknf(iunit, 'ky_pknf', 1, nkeys, onfkey,5,comm,status)

      comm = 'fxpkne comment&'
      call ftpkne(iunit, 'ky_pkne', 1, nkeys, onekey,6,comm,status)

      comm = 'fxpkng comment&'
      call ftpkng(iunit, 'ky_pkng', 1, nkeys, ongkey,13,comm,status)

      comm = 'fxpknd comment&'
      call ftpknd(iunit, 'ky_pknd', 1, nkeys, ondkey,14,comm,status)
      
      if (status .gt. 0)go to 999
      
C        ############################
C        #  write generic keywords  #
C        ############################
      

      oskey = '1'
      call ftpkys(iunit, 'tstring', oskey, 'tstring comment',status)

      olkey = .true.
      call ftpkyl(iunit, 'tlogical', olkey, 'tlogical comment',
     &    status)

      ojkey = 11
      call ftpkyj(iunit, 'tbyte', ojkey, 'tbyte comment', status)

      ojkey = 21
      call ftpkyj(iunit, 'tshort', ojkey, 'tshort comment', status)

      ojkey = 31
      call ftpkyj(iunit, 'tint', ojkey, 'tint comment', status)

      ojkey = 41
      call ftpkyj(iunit, 'tlong', ojkey, 'tlong comment', status)

      oekey = 42
      call ftpkye(iunit, 'tfloat', oekey, 6,'tfloat comment', status)

      odkey = 82.D+00
      call ftpkyd(iunit, 'tdouble', odkey, 14, 'tdouble comment',
     &          status)

      if (status .gt. 0)go to 999
      write(*,'(1x,A)') 'Wrote all Keywords successfully '


C        ############################
C        #  write data              #
C        ############################
      
      
C define the null value (must do this before writing any data) 
      call ftpkyj(iunit,'BLANK',-99,
     & 'value to use for undefined pixels',   status)
      
C initialize arrays of values to write to primary array 
      do ii = 1, npixels
          boutarray(ii) = char(ii)
          ioutarray(ii) = ii
          joutarray(ii) = ii
          eoutarray(ii) = ii
          doutarray(ii) = ii
      end do      

C write a few pixels with each datatype 
C set the last value in each group of 4 as undefined 
      call ftpprb(iunit, 1,  1, 2, boutarray(1),  status)
      call ftppri(iunit, 1,  5, 2, ioutarray(5),  status)
      call ftpprj(iunit, 1,  9, 2, joutarray(9),  status)
      call ftppre(iunit, 1, 13, 2, eoutarray(13), status)
      call ftpprd(iunit, 1, 17, 2, doutarray(17), status)
      bnul = char(4)
      call ftppnb(iunit, 1,  3, 2, boutarray(3),   bnul, status)
      inul = 8
      call ftppni(iunit, 1,  7, 2, ioutarray(7),  inul, status)
      call ftppnj(iunit, 1, 11, 2, joutarray(11),  12, status)
      call ftppne(iunit, 1, 15, 2, eoutarray(15), 16., status)
      dnul = 20.
      call ftppnd(iunit, 1, 19, 2, doutarray(19), dnul, status)
      call ftppru(iunit, 1, 1, 1, status)

      if (status .gt. 0)then
          write(*,'(1x,A,I4)')'ftppnx status = ', status
          goto 999
      end if

      call ftflus(iunit, status)   
C flush all data to the disk file  
      write(*,'(1x,A,I4)')'ftflus status = ', status
      write(*,'(1x,A)')' '

      call ftghdn(iunit, hdunum)
      write(*,'(1x,A,I4)')'HDU number = ', hdunum

C        ############################
C        #  read data               #
C        ############################
      
     
C read back the data, setting null values = 99 
      write(*,'(1x,A)')
     &   'Values read back from primary array (99 = null pixel)'
      write(*,'(1x,A)') 
     &  'The 1st, and every 4th pixel should be undefined: '

      anynull = .false.
      bnul = char(99)
      call ftgpvb(iunit, 1,  1, 10, bnul, binarray, anynull, status)
      call ftgpvb(iunit, 1, 11, 10, bnul, binarray(11),anynull,status)

      do ii = 1,npixels
           iinarray(ii) = ichar(binarray(ii))
      end do

      write(*,1101) (iinarray(ii), ii = 1, npixels), anynull,
     &  ' (ftgpvb) '
1101  format(1x,20i3,l3,a)

      inul = 99
      call ftgpvi(iunit, 1, 1, npixels, inul, iinarray,anynull,status)

      write(*,1101) (iinarray(ii), ii = 1, npixels), anynull,
     &  ' (ftgpvi) '

      call ftgpvj(iunit, 1, 1, npixels, 99,  jinarray,anynull,status)

      write(*,1101) (jinarray(ii), ii = 1, npixels), anynull,
     &  ' (ftgpvj) '

      call ftgpve(iunit, 1, 1, npixels, 99., einarray,anynull,status)

      write(*,1102) (einarray(ii), ii = 1, npixels), anynull,
     &  ' (ftgpve) '

1102  format(2x,20f3.0,l2,a)

      dnul = 99.
      call ftgpvd(iunit, 1,  1, 10, dnul,  dinarray, anynull, status)
      call ftgpvd(iunit, 1, 11, 10, dnul,dinarray(11),anynull,status)

      write(*,1102) (dinarray(ii), ii = 1, npixels), anynull,
     &  ' (ftgpvd) '

      if (status .gt. 0)then
          write(*,'(1x,A,I4)')'ERROR: ftgpv_ status = ', status
          goto 999
      end if
      
      if (.not. anynull)then
         write(*,'(1x,A)') 'ERROR: ftgpv_ did not detect null values '
         go to 999
      end if
      
C reset the output null value to the expected input value 

      do ii = 4, npixels, 4      
          boutarray(ii) = char(99)
          ioutarray(ii) = 99
          joutarray(ii) = 99
          eoutarray(ii) = 99.
          doutarray(ii) = 99.
      end do

          ii = 1
          boutarray(ii) = char(99)
          ioutarray(ii) = 99
          joutarray(ii) = 99
          eoutarray(ii) = 99.
          doutarray(ii) = 99.

      
C compare the output with the input flag any differences 
      do ii = 1, npixels
     
         if (boutarray(ii) .ne. binarray(ii))then
             write(*,'(1x,A,2A2)') 'bout != bin ', boutarray(ii), 
     &      binarray(ii)
         end if

         if (ioutarray(ii) .ne. iinarray(ii))then
             write(*,'(1x,A,2I8)') 'bout != bin ', ioutarray(ii), 
     &      iinarray(ii)
         end if

         if (joutarray(ii) .ne. jinarray(ii))then
             write(*,'(1x,A,2I12)') 'bout != bin ', joutarray(ii), 
     &       jinarray(ii)
         end if

         if (eoutarray(ii) .ne. einarray(ii))then
             write(*,'(1x,A,2E15.3)') 'bout != bin ', eoutarray(ii),
     &       einarray(ii)
         end if
    
         if (doutarray(ii) .ne. dinarray(ii))then
             write(*,'(1x,A,2D20.6)') 'bout != bin ', doutarray(ii), 
     &       dinarray(ii)
         end if
      end do

      do ii = 1, npixels
        binarray(ii) = char(0)
        iinarray(ii) = 0
        jinarray(ii) = 0
        einarray(ii) = 0.
        dinarray(ii) = 0.
      end do      

      anynull = .false.
      call ftgpfb(iunit, 1,  1, 10, binarray, larray, anynull,status)
      call ftgpfb(iunit, 1, 11, 10, binarray(11), larray(11),
     & anynull, status)

      do ii = 1, npixels
        if (larray(ii))binarray(ii) = char(0)
      end do

      do ii = 1,npixels
           iinarray(ii) = ichar(binarray(ii))
      end do

      write(*,1101)(iinarray(ii),ii = 1,npixels),anynull,' (ftgpfb)'

      call ftgpfi(iunit, 1, 1, npixels, iinarray, larray, anynull,
     & status)

      do ii = 1, npixels
        if (larray(ii))iinarray(ii) = 0
      end do

      write(*,1101)(iinarray(ii),ii = 1,npixels),anynull,' (ftgpfi)'

      call ftgpfj(iunit, 1, 1, npixels, jinarray, larray, anynull,
     & status)

      do ii = 1, npixels
        if (larray(ii))jinarray(ii) = 0
      end do

      write(*,1101)(jinarray(ii),ii = 1,npixels),anynull,' (ftgpfj)'

      call ftgpfe(iunit, 1, 1, npixels, einarray, larray, anynull,
     & status)

      do ii = 1, npixels
        if (larray(ii))einarray(ii) = 0.
      end do

      write(*,1102)(einarray(ii),ii = 1,npixels),anynull,' (ftgpfe)'

      call ftgpfd(iunit, 1,  1, 10, dinarray, larray, anynull,status)
      call ftgpfd(iunit, 1, 11, 10, dinarray(11), larray(11),
     & anynull, status)

      do ii = 1, npixels
        if (larray(ii))dinarray(ii) = 0.
      end do

      write(*,1102)(dinarray(ii),ii = 1,npixels),anynull,' (ftgpfd)'

      if (status .gt. 0)then
          write(*,'(1x,A,I4)')'ERROR: ftgpf_ status = ', status
          go to 999
      end if

      if (.not. anynull)then
         write(*,'(1x,A)') 'ERROR: ftgpf_ did not detect null values'
         go to 999
      end if


C        ##########################################
C        #  close and reopen file multiple times  #
C        ##########################################
      

      do ii = 1, 10
         call ftclos(iunit, status)
        
         if (status .gt. 0)then
            write(*,'(1x,A,I4)')'ERROR in ftclos (1) = ', status
            go to 999
         end if

         call ftopen(iunit, filename, 1, block, status)

         if (status .gt. 0)then
            write(*,'(1x,A,I4)')'ERROR: ftopen open file status = ',
     &      status
            go to 999
         end if
      end do
      
      write(*,'(1x,A)') ' '
      write(*,'(1x,A)') 'Closed then reopened the FITS file 10 times.'
      write(*,'(1x,A)')' '

      call ftghdn(iunit, hdunum)
      write(*,'(1x,A,I4)')'HDU number = ', hdunum


C        ############################
C        #  read single keywords    #
C        ############################
      

      simple = .false.
      bitpix = 0
      naxis = 0
      naxes(1) = 0
      naxes(2) = 0
      pcount = -99
      gcount =  -99
      extend = .false.
      write(*,'(1x,A)') 'Read back keywords: '
      call ftghpr(iunit, 3, simple, bitpix, naxis, naxes, pcount,
     &       gcount, extend, status)
      write(*,'(1x,A,L4,4I4)')'simple, bitpix, naxis, naxes = ',
     &       simple, bitpix, naxis, naxes(1), naxes(2)
      write(*,'(1x,A,2I4,L4)')'  pcount, gcount, extend = ',
     &           pcount, gcount, extend

      call ftgrec(iunit, 9, card, status)
      write(*,'(1x,A)') card
      if (card(1:15) .ne. 'KEY_PREC= ''This')
     &    write(*,'(1x,A)') 'ERROR in ftgrec '

      call ftgkyn(iunit, 9, keyword, value, comment, status)
      write(*,'(1x,5A)') keyword,' ', value(1:35),' ', comment(1:20)

      if (keyword(1:8) .ne. 'KEY_PREC' )
     &    write(*,'(1x,2A)') 'ERROR in ftgkyn: ', keyword

      call ftgcrd(iunit, keyword, card, status)
      write(*,'(1x,A)') card

      if (keyword(1:8) .ne.  card(1:8) )
     &    write(*,'(1x,2A)') 'ERROR in ftgcrd: ', keyword

      call ftgkey(iunit, 'KY_PKNS1', value, comment, status)
      write(*,'(1x,5A)') 'KY_PKNS1 ',':', value(1:15),':', comment(1:16)

      if (value(1:14) .ne. '''first string''')
     &  write(*,'(1x,2A)') 'ERROR in ftgkey: ', value

      call ftgkys(iunit, 'key_pkys', iskey, comment, status)
      write(*,'(1x,5A,I4)')'KEY_PKYS ',':',iskey,':',comment(1:16),
     & status

      call ftgkyl(iunit, 'key_pkyl', ilkey, comment, status)
      write(*,'(1x,2A,L4,2A,I4)') 'KEY_PKYL ',':', ilkey,':', 
     &comment(1:16), status

      call ftgkyj(iunit, 'KEY_PKYJ', ijkey, comment, status)
      write(*,'(1x,2A,I4,2A,I4)') 'KEY_PKYJ ',':',ijkey,':', 
     &  comment(1:16), status

      call ftgkye(iunit, 'KEY_PKYJ', iekey, comment, status)
      write(*,'(1x,2A,f12.5,2A,I4)') 'KEY_PKYE ',':',iekey,':',
     & comment(1:16), status

      call ftgkyd(iunit, 'KEY_PKYJ', idkey, comment, status)
      write(*,'(1x,2A,F12.5,2A,I4)') 'KEY_PKYD ',':',idkey,':', 
     & comment(1:16), status

      if (ijkey .ne. 11 .or. iekey .ne. 11. .or. idkey .ne. 11.)
     &   write(*,'(1x,A,I4,2F5.1)') 'ERROR in ftgky(jed): ',
     & ijkey, iekey, idkey

      iskey= ' '
      call ftgkys(iunit, 'key_pkys', iskey, comment, status)
      write(*,'(1x,5A,I4)') 'KEY_PKYS ',':', iskey,':', comment(1:16),
     &  status

      ilkey = .false.
      call ftgkyl(iunit, 'key_pkyl', ilkey, comment, status)
      write(*,'(1x,2A,L4,2A,I4)') 'KEY_PKYL ',':', ilkey,':',
     &  comment(1:16), status

      ijkey = 0
      call ftgkyj(iunit, 'KEY_PKYJ', ijkey, comment, status)
      write(*,'(1x,2A,I4,2A,I4)') 'KEY_PKYJ ',':',ijkey,':', 
     & comment(1:16), status

      iekey = 0
      call ftgkye(iunit, 'KEY_PKYE', iekey, comment, status)
      write(*,'(1x,2A,f12.5,2A,I4)') 'KEY_PKYE ',':',iekey,':', 
     & comment(1:16), status

      idkey = 0
      call ftgkyd(iunit, 'KEY_PKYD', idkey, comment, status)
      write(*,'(1x,2A,F12.5,2A,I4)') 'KEY_PKYD ',':',idkey,':',
     & comment(1:16), status

      iekey = 0
      call ftgkye(iunit, 'KEY_PKYF', iekey, comment, status)
      write(*,'(1x,2A,f12.5,2A,I4)') 'KEY_PKYF ',':',iekey,':',
     & comment(1:16), status

      iekey = 0
      call ftgkye(iunit, 'KEY_PKYE', iekey, comment, status)
      write(*,'(1x,2A,f12.5,2A,I4)') 'KEY_PKYE ',':',iekey,':', 
     & comment(1:16), status

      idkey = 0
      call ftgkyd(iunit, 'KEY_PKYG', idkey, comment, status)
      write(*,'(1x,2A,f16.12,2A,I4)') 'KEY_PKYG ',':',idkey,':',
     & comment(1:16), status

      idkey = 0
      call ftgkyd(iunit, 'KEY_PKYD', idkey, comment, status)
      write(*,'(1x,2A,f16.12,2A,I4)') 'KEY_PKYD ',':',idkey,':', 
     & comment(1:16), status

      call ftgkyt(iunit, 'KEY_PKYT', ijkey, idkey, comment, status)
      write(*,'(1x,2A,i10,A,f16.14,A,I4)') 'KEY_PKYT  ',':',
     & ijkey,':', idkey, comment(1:16), status

      call ftpunt(iunit, 'KEY_PKYJ', 'km/s/Mpc', status)
      ijkey = 0
      call ftgkyj(iunit, 'KEY_PKYJ', ijkey, comment, status)
      write(*,'(1x,2A,I4,2A,I4)') 'KEY_PKYJ ',':',ijkey,':', 
     & comment(1:38), status
      call ftgunt(iunit,'KEY_PKYJ',kunit,status)
      write(*,'(1x,2A)') 'keyword unit=', kunit

      call ftpunt(iunit, 'KEY_PKYJ', ' ', status)
      ijkey = 0
      call ftgkyj(iunit, 'KEY_PKYJ', ijkey, comment, status)
      write(*,'(1x,2A,I4,2A,I4)') 'KEY_PKYJ ',':',ijkey,':', 
     & comment(1:38), status
      call ftgunt(iunit,'KEY_PKYJ',kunit,status)
      write(*,'(1x,2A)') 'keyword unit=', kunit

      call ftpunt(iunit, 'KEY_PKYJ', 'feet/second/second', status)
      ijkey = 0
      call ftgkyj(iunit, 'KEY_PKYJ', ijkey, comment, status)
      write(*,'(1x,2A,I4,2A,I4)') 'KEY_PKYJ ',':',ijkey,':', 
     & comment(1:38), status
      call ftgunt(iunit,'KEY_PKYJ',kunit,status)
      write(*,'(1x,2A)') 'keyword unit=', kunit

      call ftgkys(iunit, 'key_pkls', lstr, comment, status)
      write(*,'(1x,2A)') 'KEY_PKLS long string value = ', lstr(1:50)
      write(*,'(1x,A)')lstr(51:120)

C get size and position in header 
      call ftghps(iunit, existkeys, keynum, status)
      write(*,'(1x,A,I4,A,I4)') 'header contains ', existkeys,
     & ' keywords; located at keyword ', keynum

C        ############################
C        #  read array keywords     #
C        ############################
      
      call ftgkns(iunit, 'ky_pkns', 1, 3, inskey, nfound, status)
      write(*,'(1x,4A)') 'ftgkns: ', inskey(1)(1:14), inskey(2)(1:14),
     &  inskey(3)(1:14)
      if (nfound .ne. 3 .or. status .gt. 0)
     &   write(*,'(1x,A,2I4)') ' ERROR in ftgkns ', nfound, status

      call ftgknl(iunit, 'ky_pknl', 1, 3, inlkey, nfound, status)
      write(*,'(1x,A,3L4)') 'ftgknl: ', inlkey(1), inlkey(2), inlkey(3)
      if (nfound .ne. 3 .or. status .gt. 0)
     &   write(*,'(1x,A,2I4)') ' ERROR in ftgknl ', nfound, status

      call ftgknj(iunit, 'ky_pknj', 1, 3, injkey, nfound, status)
      write(*,'(1x,A,3I4)') 'ftgknj: ', injkey(1), injkey(2), injkey(3)
      if (nfound .ne. 3 .or. status .gt. 0)
     &   write(*,'(1x,A,2I4)') ' ERROR in ftgknj ', nfound, status

      call ftgkne(iunit, 'ky_pkne', 1, 3, inekey, nfound, status)
      write(*,'(1x,A,3F10.5)') 'ftgkne: ',inekey(1),inekey(2),inekey(3)
      if (nfound .ne. 3 .or. status .gt. 0)
     &   write(*,'(1x,A,2I4)') ' ERROR in ftgkne ', nfound, status

      call ftgknd(iunit, 'ky_pknd', 1, 3, indkey, nfound, status)
      write(*,'(1x,A,3F10.5)') 'ftgknd: ',indkey(1),indkey(2),indkey(3)
      if (nfound .ne. 3 .or. status .gt. 0)
     &   write(*,'(1x,A,2I4)') ' ERROR in ftgknd ', nfound, status

      write(*,'(1x,A)')' '
      write(*,'(1x,A)')
     & 'Before deleting the HISTORY and DATE keywords...'
      do ii = 29, 32     
          call ftgrec(iunit, ii, card, status)
          write(*,'(1x,A)') card(1:8)
      end do

C don't print date value, so that 
C the output will always be the same 
      

C        ############################
C        #  delete keywords         #
C        ############################
      

      call ftdrec(iunit, 30, status)
      call ftdkey(iunit, 'DATE', status)

      write(*,'(1x,A)')' '
      write(*,'(1x,A)') 'After deleting the keywords... '
      do ii = 29, 30            
          call ftgrec(iunit, ii, card, status)
          write(*,'(1x,A)') card
      end do      

      if (status .gt. 0)
     &   write(*,'(1x,A)') ' ERROR deleting keywords '
      

C        ############################
C        #  insert keywords         #
C        ############################
      
      call ftirec(iunit,26,
     & 'KY_IREC = ''This keyword inserted by fxirec''',
     &   status)
      call ftikys(iunit, 'KY_IKYS', 'insert_value_string',
     &  'ikys comment', status)
      call ftikyj(iunit, 'KY_IKYJ', 49, 'ikyj comment', status)
      call ftikyl(iunit, 'KY_IKYL', .true., 'ikyl comment', status)
      call ftikye(iunit, 'KY_IKYE',12.3456,4,'ikye comment',status)
      odkey = 12.345678901234567D+00
      call ftikyd(iunit, 'KY_IKYD', odkey, 14,
     &  'ikyd comment', status)
      call ftikyf(iunit, 'KY_IKYF', 12.3456, 4, 'ikyf comment',
     & status)
      call ftikyg(iunit, 'KY_IKYG', odkey, 13,
     & 'ikyg comment', status)

      write(*,'(1x,A)')' '
      write(*,'(1x,A)') 'After inserting the keywords... '
      do ii = 25, 34
          call ftgrec(iunit, ii, card, status)
          write(*,'(1x,A)') card
      end do

      if (status .gt. 0)
     &   write(*,'(1x,A)') ' ERROR inserting keywords '
      

C        ############################
C        #  modify keywords         #
C        ############################
      
      call ftmrec(iunit, 25,
     & 'COMMENT   This keyword was modified by fxmrec', status)
      call ftmcrd(iunit, 'KY_IREC', 
     & 'KY_MREC = ''This keyword was modified by fxmcrd''', status)
      call ftmnam(iunit, 'KY_IKYS', 'NEWIKYS', status)

      call ftmcom(iunit,'KY_IKYJ','This is a modified comment',
     & status)
      call ftmkyj(iunit, 'KY_IKYJ', 50, '&', status)
      call ftmkyl(iunit, 'KY_IKYL', .false., '&', status)
      call ftmkys(iunit, 'NEWIKYS', 'modified_string', '&', status)
      call ftmkye(iunit, 'KY_IKYE', -12.3456, 4, '&', status)
      odkey = -12.345678901234567D+00

      call ftmkyd(iunit, 'KY_IKYD', odkey, 14, 
     & 'modified comment', status)
      call ftmkyf(iunit, 'KY_IKYF', -12.3456, 4, '&', status)
      call ftmkyg(iunit,'KY_IKYG', odkey,13,'&',status)

      write(*,'(1x,A)')' '
      write(*,'(1x,A)') 'After modifying the keywords... '
      do ii = 25, 34
          call ftgrec(iunit, ii, card, status)
          write(*,'(1x,A)') card
      end do
      
      if (status .gt. 0)then
         write(*,'(1x,A)') ' ERROR modifying keywords '
         go to 999
      end if
      
C        ############################
C        #  update keywords         #
C        ############################
      
      call ftucrd(iunit, 'KY_MREC', 
     & 'KY_UCRD = ''This keyword was updated by fxucrd''',
     &         status)

      call ftukyj(iunit, 'KY_IKYJ', 51, '&', status)
      call ftukyl(iunit, 'KY_IKYL', .true., '&', status)
      call ftukys(iunit, 'NEWIKYS', 'updated_string', '&', status)
      call ftukye(iunit, 'KY_IKYE', -13.3456, 4, '&', status)
      odkey = -13.345678901234567D+00

      call ftukyd(iunit, 'KY_IKYD',odkey , 14, 
     & 'modified comment', status)
      call ftukyf(iunit, 'KY_IKYF', -13.3456, 4, '&', status)
      call ftukyg(iunit, 'KY_IKYG', odkey, 13, '&', status)

      write(*,'(1x,A)')' '
      write(*,'(1x,A)') 'After updating the keywords... '
      do ii = 25, 34
          call ftgrec(iunit, ii, card, status)
          write(*,'(1x,A)') card
      end do

      if (status .gt. 0)then
         write(*,'(1x,A)') ' ERROR modifying keywords '
         go to 999
      end if

C     move to top of header and find keywords using wild cards 
      call ftgrec(iunit, 0, card, status)

      write(*,'(1x,A)')' '
      write(*,'(1x,A)')
     &  'Keywords found using wildcard search (should be 9)...'
      nfound = -1
91    nfound = nfound +1
      call ftgnxk(iunit, inclist, 2, exclist, 2, card, status)
      if (status .eq. 0)then
          write(*,'(1x,A)') card
          go to 91
      end if

      if (nfound .ne. 9)then
          write(*,'(1x,A)')
     &    'ERROR reading keywords using wildcards (ftgnxk)'
         go to 999
      end if
      status = 0

C        ############################
C        #  create binary table     #
C        ############################
      
      tform(1) = '15A'
      tform(2) = '1L'
      tform(3) = '16X'
      tform(4) = '1B'
      tform(5) = '1I'
      tform(6) = '1J'
      tform(7) = '1E'
      tform(8) = '1D'
      tform(9) = '1C'
      tform(10)= '1M'

      ttype(1) = 'Avalue'
      ttype(2) = 'Lvalue'
      ttype(3) = 'Xvalue'
      ttype(4) = 'Bvalue'
      ttype(5) = 'Ivalue'
      ttype(6) = 'Jvalue'
      ttype(7) = 'Evalue'
      ttype(8) = 'Dvalue'
      ttype(9) = 'Cvalue'
      ttype(10)= 'Mvalue'

      tunit(1) = ' '
      tunit(2) = 'm**2'
      tunit(3) = 'cm'
      tunit(4) = 'erg/s'
      tunit(5) = 'km/s'
      tunit(6) = ' '
      tunit(7) = ' '
      tunit(8) = ' '
      tunit(9) = ' '
      tunit(10)= ' '

      nrows = 21
      tfields = 10
      pcount = 0

      call ftibin(iunit, nrows, tfields, ttype, tform, tunit, 
     & binname, pcount, status)
      write(*,'(1x,A)')' '
      write(*,'(1x,A,I4)') 'ftibin status = ', status
      call ftghdn(iunit, hdunum)
      write(*,'(1x,A,I4)') 'HDU number = ', hdunum

C get size and position in header, and reserve space for more keywords 
      call ftghps(iunit, existkeys, keynum, status)
      write(*,'(1x,A,I4,A,I4)') 'header contains ',existkeys,
     & ' keywords located at keyword ', keynum

      morekeys = 40
      call fthdef(iunit, morekeys, status)
      call ftghsp(iunit, existkeys, morekeys, status)
      write(*,'(1x,A,I4,A,I4,A)') 'header contains ', existkeys,
     &' keywords with room for ', morekeys,' more'

C define null value for int cols 
      call fttnul(iunit, 4, 99, status)   
      call fttnul(iunit, 5, 99, status)
      call fttnul(iunit, 6, 99, status)

      call ftpkyj(iunit, 'TNULL4', 99, 'value for undefined pixels',
     &  status)
      call ftpkyj(iunit, 'TNULL5', 99, 'value for undefined pixels',
     &  status)
      call ftpkyj(iunit, 'TNULL6', 99, 'value for undefined pixels',
     & status)

      naxis = 3
      naxes(1) = 1
      naxes(2) = 2
      naxes(3) = 8
      call ftptdm(iunit, 3, naxis, naxes, status)

      naxis = 0
      naxes(1) = 0
      naxes(2) = 0
      naxes(3) = 0
      call ftgtdm(iunit, 3, 3, naxis, naxes, status)
      call ftgkys(iunit, 'TDIM3', iskey, comment, status)
      write(*,'(1x,2A,4I4)') 'TDIM3 = ', iskey, naxis, naxes(1),
     &      naxes(2), naxes(3)

C force header to be scanned (not required) 
      call ftrdef(iunit, status)  
    
C        ############################
C        #  write data to columns   #
C        ############################
         
C initialize arrays of values to write to table 
      signval = -1
      do ii = 1, 21
          signval = signval * (-1)
          boutarray(ii) = char(ii)
          ioutarray(ii) = (ii) * signval
          joutarray(ii) = (ii) * signval
          koutarray(ii) = (ii) * signval
          eoutarray(ii) = (ii) * signval
          doutarray(ii) = (ii) * signval
      end do

      call ftpcls(iunit, 1, 1, 1, 3, onskey, status)  
C write string values 
      call ftpclu(iunit, 1, 4, 1, 1, status)  
C write null value 

      larray(1) = .false.
      larray(2) =.true.
      larray(3) = .false.
      larray(4) = .false.
      larray(5) =.true.
      larray(6) =.true.
      larray(7) = .false.
      larray(8) = .false.
      larray(9) = .false.
      larray(10) =.true.
      larray(11) =.true.
      larray(12) = .true.
      larray(13) = .false.
      larray(14) = .false.
      larray(15) =.false.
      larray(16) =.false.
      larray(17) = .true.
      larray(18) = .true.
      larray(19) = .true.
      larray(20) = .true.
      larray(21) =.false.
      larray(22) =.false.
      larray(23) =.false.
      larray(24) =.false.
      larray(25) =.false.
      larray(26) = .true.
      larray(27) = .true.
      larray(28) = .true.
      larray(29) = .true.
      larray(30) = .true.
      larray(31) =.false.
      larray(32) =.false.
      larray(33) =.false.
      larray(34) =.false.
      larray(35) =.false.
      larray(36) =.false.

C write bits
      call ftpclx(iunit, 3, 1, 1, 36, larray, status) 

C loop over cols 4 - 8 
      do ii = 4, 8   
          call ftpclb(iunit, ii, 1, 1, 2, boutarray, status)
          if (status .eq. 412) status = 0

          call ftpcli(iunit, ii, 3, 1, 2, ioutarray(3), status) 
          if (status .eq. 412) status = 0

          call ftpclj(iunit, ii, 5, 1, 2, koutarray(5), status) 
          if (status .eq. 412) status = 0

          call ftpcle(iunit, ii, 7, 1, 2, eoutarray(7), status)
          if (status .eq. 412)status = 0

          call ftpcld(iunit, ii, 9, 1, 2, doutarray(9), status)
          if (status .eq. 412)status = 0

C write null value 
          call ftpclu(iunit, ii, 11, 1, 1, status)  
      end do

      call ftpclc(iunit,  9, 1, 1, 10, eoutarray, status)
      call ftpclm(iunit, 10, 1, 1, 10, doutarray, status)

C loop over cols 4 - 8 
      do ii = 4, 8
          bnul = char(13)
          call ftpcnb(iunit, ii, 12, 1, 2, boutarray(12),bnul,status)
          if (status .eq. 412) status = 0
          inul=15
          call ftpcni(iunit, ii, 14, 1, 2, ioutarray(14),inul,status) 
          if (status .eq. 412) status = 0
          call ftpcnj(iunit, ii, 16, 1, 2, koutarray(16), 17, status) 
          if (status .eq. 412) status = 0
          call ftpcne(iunit, ii, 18, 1, 2, eoutarray(18), 19.,status)
          if (status .eq. 412) status = 0
          dnul = 21.
          call ftpcnd(iunit, ii, 20, 1, 2, doutarray(20),dnul,status)
          if (status .eq. 412) status = 0
      end do
      
C write logicals
      call ftpcll(iunit, 2, 1, 1, 21, larray, status) 
C write null value 
      call ftpclu(iunit, 2, 11, 1, 1, status)  
      write(*,'(1x,A,I4)') 'ftpcl_ status = ', status
      if (status .gt. 0)go to 999
      
C        #########################################
C        #  get information about the columns    #
C        #########################################
      
      write(*,'(1x,A)')' '
      write(*,'(1x,A)')
     & 'Find the column numbers a returned status value'//
     & ' of 237 is'
      write(*,'(1x,A)') 
     & 'expected and indicates that more than one column'//
     & ' name matches'
      write(*,'(1x,A)')'the input column name template.'//
     & '  Status = 219 indicates that'
      write(*,'(1x,A)') 'there was no matching column name.'

      call ftgcno(iunit, 0, 'Xvalue', colnum, status)
      write(*,'(1x,A,I4,A,I4)') 'Column Xvalue is number', colnum,
     &' status =',status

219   continue
      if (status .ne. 219)then
        call ftgcnn(iunit, 1, '*ue', colname, colnum, status)
        write(*,'(1x,3A,I4,A,I4)') 'Column ',colname(1:6),' is number', 
     &   colnum,' status = ',  status
        go to 219
      end if

      status = 0

      write(*,'(1x,A)')' '
      write(*,'(1x,A)') 'Information about each column: '

      do ii = 1, tfields
        call ftgtcl(iunit, ii, typecode, repeat, width, status)
        call ftgbcl(iunit,ii,ttype,tunit,cval,repeat,scale,
     &        zero, jnulval, tdisp, status)

        write(*,'(1x,A,3I4,5A,2F8.2,I12,A)')
     &  tform(ii)(1:3), typecode, repeat, width,' ',
     &  ttype(1)(1:6),' ',tunit(1)(1:6), cval, scale, zero, jnulval,
     &  tdisp(1:8)
      end do

      write(*,'(1x,A)') ' '

C        ###############################################
C        #  insert ASCII table before the binary table #
C        ###############################################

      call ftmrhd(iunit, -1, hdutype, status)
      if (status .gt. 0)goto 999

      tform(1) = 'A15'
      tform(2) = 'I10'
      tform(3) = 'F14.6'
      tform(4) = 'E12.5'
      tform(5) = 'D21.14'

      ttype(1) = 'Name'
      ttype(2) = 'Ivalue'
      ttype(3) = 'Fvalue'
      ttype(4) = 'Evalue'
      ttype(5) = 'Dvalue'

      tunit(1) = ' '
      tunit(2) = 'm**2'
      tunit(3) = 'cm'
      tunit(4) = 'erg/s'
      tunit(5) = 'km/s'

      rowlen = 76
      nrows = 11
      tfields = 5

      call ftitab(iunit, rowlen, nrows, tfields, ttype, tbcol, 
     & tform, tunit, tblname, status)
      write(*,'(1x,A,I4)') 'ftitab status = ', status
      call ftghdn(iunit, hdunum)
      write(*,'(1x,A,I4)') 'HDU number = ', hdunum

C define null value for int cols 
      call ftsnul(iunit, 1, 'null1', status)   
      call ftsnul(iunit, 2, 'null2', status)
      call ftsnul(iunit, 3, 'null3', status)
      call ftsnul(iunit, 4, 'null4', status)
      call ftsnul(iunit, 5, 'null5', status)
 
      call ftpkys(iunit, 'TNULL1', 'null1',
     & 'value for undefined pixels', status)
      call ftpkys(iunit, 'TNULL2', 'null2',
     & 'value for undefined pixels', status)
      call ftpkys(iunit, 'TNULL3', 'null3',
     & 'value for undefined pixels', status)
      call ftpkys(iunit, 'TNULL4', 'null4',
     & 'value for undefined pixels', status)
      call ftpkys(iunit, 'TNULL5', 'null5',
     & 'value for undefined pixels', status)

      if (status .gt. 0) goto 999
      
C        ############################
C        #  write data to columns   #
C        ############################
           
C initialize arrays of values to write to table 
      do ii = 1,21     
          boutarray(ii) = char(ii)
          ioutarray(ii) = ii
          joutarray(ii) = ii
          eoutarray(ii) = ii
          doutarray(ii) = ii
      end do      

C write string values 
      call ftpcls(iunit, 1, 1, 1, 3, onskey, status)  
C write null value 
      call ftpclu(iunit, 1, 4, 1, 1, status)  

      do ii = 2,5 
C loop over cols 2 - 5       
          call ftpclb(iunit, ii, 1, 1, 2, boutarray, status)  
C char array 
          if (status .eq. 412) status = 0
             
          call ftpcli(iunit, ii, 3, 1, 2, ioutarray(3), status)  
C short array 
          if (status .eq. 412) status = 0
             
          call ftpclj(iunit, ii, 5, 1, 2, joutarray(5), status)  
C long array 
          if (status .eq. 412)status = 0
              
          call ftpcle(iunit, ii, 7, 1, 2, eoutarray(7), status)  
C float array 
          if (status .eq. 412) status = 0
             
          call ftpcld(iunit, ii, 9, 1, 2, doutarray(9), status)  
C double array 
          if (status .eq. 412) status = 0

          call ftpclu(iunit, ii, 11, 1, 1, status)  
C write null value 
      end do
      write(*,'(1x,A,I4)') 'ftpcl_ status = ', status
      write(*,'(1x,A)')' '

C        ################################
C        #  read data from ASCII table  #
C        ################################
      
      call ftghtb(iunit, 99, rowlen, nrows, tfields, ttype, tbcol, 
     &       tform, tunit, tblname, status)

      write(*,'(1x,A,3I3,2A)')
     & 'ASCII table: rowlen, nrows, tfields, extname:',
     & rowlen, nrows, tfields,' ',tblname

      do ii = 1,tfields
        write(*,'(1x,A,I4,3A)') 
     & ttype(ii)(1:7), tbcol(ii),' ',tform(ii)(1:7), tunit(ii)(1:7)
      end do

      nrows = 11
      call ftgcvs(iunit, 1, 1, 1, nrows, 'UNDEFINED', inskey,
     &   anynull, status)
      bnul = char(99)
      call ftgcvb(iunit, 2, 1, 1, nrows, bnul, binarray,
     & anynull, status)
      inul = 99
      call ftgcvi(iunit, 2, 1, 1, nrows, inul, iinarray,
     & anynull, status)
      call ftgcvj(iunit, 3, 1, 1, nrows, 99, jinarray,
     & anynull, status)
      call ftgcve(iunit, 4, 1, 1, nrows, 99., einarray,
     & anynull, status)
      dnul = 99.
      call ftgcvd(iunit, 5, 1, 1, nrows, dnul, dinarray,
     & anynull, status)

      write(*,'(1x,A)')' '
      write(*,'(1x,A)') 'Data values read from ASCII table: '
      do ii = 1, nrows
        jj = ichar(binarray(ii))
        write(*,1011) inskey(ii), jj,
     &   iinarray(ii), jinarray(ii), einarray(ii), dinarray(ii)
1011    format(1x,a15,3i3,1x,2f3.0)
      end do

      call ftgtbs(iunit, 1, 20, 78, uchars, status)
      write(*,'(1x,A)')' '
      write(*,'(1x,A)') uchars
      call ftptbs(iunit, 1, 20, 78, uchars, status)
      
C        #########################################
C        #  get information about the columns    #
C        #########################################

      call ftgcno(iunit, 0, 'name', colnum, status)
      write(*,'(1x,A)')' '
      write(*,'(1x,A,I4,A,I4)')
     &  'Column name is number',colnum,' status = ', status

2190  continue
      if (status .ne. 219)then
        if (status .gt. 0 .and. status .ne. 237)go to 999

        call ftgcnn(iunit, 1, '*ue', colname, colnum, status)
        write(*,'(1x,3A,I4,A,I4)')
     & 'Column ',colname(1:6),' is number',colnum,' status = ',status
        go to 2190
      end if
   
      status = 0

      do ii = 1, tfields       
        call ftgtcl(iunit, ii, typecode, repeat, width, status)
        call ftgacl(iunit, ii, ttype, tbcol,tunit,tform, 
     &   scale,zero, nulstr, tdisp, status)

        write(*,'(1x,A,3I4,2A,I4,2A,2F10.2,3A)')
     & tform(ii)(1:7), typecode, repeat, width,' ',
     &  ttype(1)(1:6), tbcol(1), ' ',tunit(1)(1:5),
     &  scale, zero, ' ', nulstr(1:6), tdisp(1:2)

      end do

      write(*,'(1x,A)') ' '

C        ###############################################
C        #  test the insert/delete row/column routines #
C        ###############################################
      
      call ftirow(iunit, 2, 3, status)
      if (status .gt. 0) goto 999

      nrows = 14
      call ftgcvs(iunit, 1, 1, 1, nrows, 'UNDEFINED',
     & inskey,   anynull, status)
      call ftgcvb(iunit, 2, 1, 1, nrows, bnul, binarray,
     & anynull, status)
      call ftgcvi(iunit, 2, 1, 1, nrows, inul, iinarray,
     & anynull, status)
      call ftgcvj(iunit, 3, 1, 1, nrows, 99, jinarray,
     & anynull, status)
      call ftgcve(iunit, 4, 1, 1, nrows, 99., einarray,
     & anynull, status)
      call ftgcvd(iunit, 5, 1, 1, nrows, dnul, dinarray,
     & anynull, status)

      write(*,'(1x,A)')' '
      write(*,'(1x,A)')'Data values after inserting 3 rows after row 2:'
      do ii = 1, nrows     
        jj = ichar(binarray(ii))
        write(*,1011) inskey(ii), jj,
     &   iinarray(ii), jinarray(ii), einarray(ii), dinarray(ii)
      end do

      call ftdrow(iunit, 10, 2, status)

      nrows = 12
      call ftgcvs(iunit, 1, 1, 1, nrows, 'UNDEFINED', inskey,  
     & anynull, status)
      call ftgcvb(iunit, 2, 1, 1, nrows, bnul, binarray, anynull,
     & status)
      call ftgcvi(iunit, 2, 1, 1, nrows, inul, iinarray, anynull,
     & status)
      call ftgcvj(iunit, 3, 1, 1, nrows, 99, jinarray, anynull,
     & status)
      call ftgcve(iunit, 4, 1, 1, nrows, 99., einarray, anynull,
     & status)
      call ftgcvd(iunit, 5, 1, 1, nrows, dnul, dinarray, anynull,
     & status)

      write(*,'(1x,A)')' '
      write(*,'(1x,A)') 'Data values after deleting 2 rows at row 10: '
      do ii = 1, nrows    
        jj = ichar(binarray(ii))
        write(*,1011)  inskey(ii), jj,
     &       iinarray(ii), jinarray(ii), einarray(ii), dinarray(ii)
      end do
      call ftdcol(iunit, 3, status)

      call ftgcvs(iunit, 1, 1, 1, nrows, 'UNDEFINED', inskey, 
     &  anynull, status)
      call ftgcvb(iunit, 2, 1, 1, nrows, bnul, binarray, anynull,
     & status)
      call ftgcvi(iunit, 2, 1, 1, nrows, inul, iinarray, anynull,
     & status)
      call ftgcve(iunit, 3, 1, 1, nrows, 99., einarray, anynull,
     & status)
      call ftgcvd(iunit, 4, 1, 1, nrows, dnul, dinarray, anynull,
     & status)

      write(*,'(1x,A)')' '
      write(*,'(1x,A)') 'Data values after deleting column 3: '
      do ii = 1,nrows
        jj = ichar(binarray(ii))
        write(*,1012) inskey(ii), jj,
     &       iinarray(ii), einarray(ii), dinarray(ii)
1012    format(1x,a15,2i3,1x,2f3.0)

      end do

      call fticol(iunit, 5, 'INSERT_COL', 'F14.6', status)

      call ftgcvs(iunit, 1, 1, 1, nrows, 'UNDEFINED', inskey,
     &   anynull, status)
      call ftgcvb(iunit, 2, 1, 1, nrows, bnul, binarray, anynull,
     & status)
      call ftgcvi(iunit, 2, 1, 1, nrows, inul, iinarray, anynull,
     & status)
      call ftgcve(iunit, 3, 1, 1, nrows, 99., einarray, anynull,
     & status)
      call ftgcvd(iunit, 4, 1, 1, nrows, dnul, dinarray, anynull,
     & status)
      call ftgcvj(iunit, 5, 1, 1, nrows, 99, jinarray, anynull,
     & status)

      write(*,'(1x,A)')' '
      write(*,'(1x,A)') ' Data values after inserting column 5: '
      do ii = 1, nrows
        jj = ichar(binarray(ii))
        write(*,1013) inskey(ii), jj,
     &       iinarray(ii), einarray(ii), dinarray(ii) , jinarray(ii)
1013    format(1x,a15,2i3,1x,2f3.0,i2)

      end do

C        ################################
C        #  read data from binary table #
C        ################################
      

      call ftmrhd(iunit, 1, hdutype, status)
      if (status .gt. 0)go to 999
      call ftghdn(iunit, hdunum)
      write(*,'(1x,A,I4)') 'HDU number = ', hdunum

      call ftghsp(iunit, existkeys, morekeys, status)
      write(*,'(1x,A)')' '
      write(*,'(1x,A)')'Moved to binary table'
      write(*,'(1x,A,I4,A,I4,A)') 'header contains ',existkeys,
     & ' keywords with room for ',morekeys,' more '

      call ftghbn(iunit, 99, nrows, tfields, ttype, 
     &        tform, tunit, binname, pcount, status)

      write(*,'(1x,A)')' '
      write(*,'(1x,A,2I4,A,I4)') 
     & 'Binary table: nrows, tfields, extname, pcount:',
     &        nrows, tfields, binname, pcount

      do ii = 1,tfields
        write(*,'(1x,3A)') ttype(ii), tform(ii), tunit(ii)
      end do

      do ii = 1, 40
          larray(ii) = .false.
      end do

      write(*,'(1x,A)')' '
      write(*,'(1x,A)') 'Data values read from binary table: '
      write(*,'(1x,A)') ' Bit column (X) data values:   '

      call ftgcx(iunit, 3, 1, 1, 36, larray, status)
      write(*,1014) (larray(ii), ii = 1,40)
1014  format(1x,8l1,' ',8l1,' ',8l1,' ',8l1,' ',8l1)

      nrows = 21
      do ii = 1, nrows
        larray(ii) = .false.
        xinarray(ii) = ' '
        binarray(ii) = ' '
        iinarray(ii) = 0 
        kinarray(ii) = 0
        einarray(ii) = 0. 
        dinarray(ii) = 0.
        cinarray(ii * 2 -1) = 0. 
        minarray(ii * 2 -1) = 0.
        cinarray(ii * 2 ) = 0. 
        minarray(ii * 2 ) = 0.
      end do      

      write(*,'(1x,A)') '  '
      call ftgcvs(iunit, 1, 4, 1, 1, ' ',  inskey,   anynull,status)
      if (ichar(inskey(1)(1:1)) .eq. 0)inskey(1)=' '
      write(*,'(1x,2A)') 'null string column value (should be blank):',
     &        inskey(1)

      call ftgcvs(iunit, 1, 1, 1, nrows, 'NOT DEFINED',  inskey,
     &   anynull, status)
      call ftgcl( iunit, 2, 1, 1, nrows, larray, status)
      bnul = char(98)
      call ftgcvb(iunit, 3, 1, 1,nrows,bnul, xinarray,anynull,status)
      call ftgcvb(iunit, 4, 1, 1,nrows,bnul, binarray,anynull,status)
      inul = 98
      call ftgcvi(iunit, 5, 1, 1,nrows,inul, iinarray,anynull,status)
      call ftgcvj(iunit, 6, 1, 1, nrows, 98, kinarray,anynull,status)
      call ftgcve(iunit, 7, 1, 1, nrows, 98.,einarray,anynull,status)
      dnul = 98.
      call ftgcvd(iunit, 8, 1, 1, nrows,dnul,dinarray,anynull,status)
      call ftgcvc(iunit, 9, 1, 1, nrows, 98.,cinarray,anynull,status)
      call ftgcvm(iunit,10, 1, 1, nrows,dnul,minarray,anynull,status)

      write(*,'(1x,A)')' '
      write(*,'(1x,A)') 'Read columns with ftgcv_: '
      do ii = 1,nrows
        jj = ichar(xinarray(ii))
        jjj = ichar(binarray(ii))
      write(*,1201)inskey(ii),larray(ii),jj,jjj,iinarray(ii),
     & kinarray(ii), einarray(ii), dinarray(ii), cinarray(ii * 2 -1), 
     &cinarray(ii * 2 ), minarray(ii * 2 -1), minarray(ii * 2 )
      end do
1201  format(1x,a14,l4,4i4,6f5.0)

      do ii = 1, nrows
        larray(ii) = .false.
        xinarray(ii) = ' '
        binarray(ii) = ' '
        iinarray(ii) = 0 
        kinarray(ii) = 0
        einarray(ii) = 0. 
        dinarray(ii) = 0.
        cinarray(ii * 2 -1) = 0. 
        minarray(ii * 2 -1) = 0.
        cinarray(ii * 2 ) = 0. 
        minarray(ii * 2 ) = 0.
      end do      

      call ftgcfs(iunit, 1, 1, 1, nrows, inskey,   larray2, anynull,
     & status)
C     put blanks in strings if they are undefined.  (contain nulls)
      do ii = 1, nrows
         if (larray2(ii))inskey(ii) = ' '
      end do

      call ftgcfl(iunit, 2, 1, 1, nrows, larray,   larray2, anynull,
     & status)
      call ftgcfb(iunit, 3, 1, 1, nrows, xinarray, larray2, anynull,
     & status)
      call ftgcfb(iunit, 4, 1, 1, nrows, binarray, larray2, anynull,
     & status)
      call ftgcfi(iunit, 5, 1, 1, nrows, iinarray, larray2, anynull,
     & status)
      call ftgcfj(iunit, 6, 1, 1, nrows, kinarray, larray2, anynull,
     & status)
      call ftgcfe(iunit, 7, 1, 1, nrows, einarray, larray2, anynull,
     & status)
      call ftgcfd(iunit, 8, 1, 1, nrows, dinarray, larray2, anynull,
     & status)
      call ftgcfc(iunit, 9, 1, 1, nrows, cinarray, larray2, anynull,
     & status)
      call ftgcfm(iunit, 10,1, 1, nrows, minarray, larray2, anynull,
     & status)

      write(*,'(1x,A)')' '
      write(*,'(1x,A)') ' Read columns with ftgcf_: '
      do ii = 1, 10
        jj = ichar(xinarray(ii))
        jjj = ichar(binarray(ii))
      write(*,1201)
     & inskey(ii),larray(ii),jj,jjj,iinarray(ii),
     & kinarray(ii), einarray(ii), dinarray(ii), cinarray(ii * 2 -1), 
     & cinarray(ii * 2 ), minarray(ii * 2 -1), minarray(ii * 2)
      end do

      do ii = 11, 21
C don't try to print the NaN values 
        jj = ichar(xinarray(ii))
        jjj = ichar(binarray(ii))
        write(*,1201) inskey(ii), larray(ii), jj,
     &    jjj, iinarray(ii)
      end do
      
      call ftprec(iunit,'key_prec= '// 
     &'''This keyword was written by f_prec'' / comment here',
     & status)

C        ###############################################
C        #  test the insert/delete row/column routines #
C        ###############################################
      
      call ftirow(iunit, 2, 3, status)
         if (status .gt. 0) go to 999

      nrows = 14
      call ftgcvs(iunit, 1, 1, 1, nrows, 'NOT DEFINED',  inskey,
     & anynull, status)
      call ftgcvb(iunit, 4, 1, 1, nrows,bnul,binarray,anynull,status)
      call ftgcvi(iunit, 5, 1, 1, nrows,inul,iinarray,anynull,status)
      call ftgcvj(iunit, 6, 1, 1, nrows, 98, jinarray,anynull,status)
      call ftgcve(iunit, 7, 1, 1, nrows, 98.,einarray,anynull,status)
      call ftgcvd(iunit, 8, 1, 1, nrows,dnul,dinarray,anynull,status)

      write(*,'(1x,A)')' '
      write(*,'(1x,A)')'Data values after inserting 3 rows after row 2:'
      do ii = 1, nrows
        jj = ichar(binarray(ii))
        write(*,1202)  inskey(ii), jj,
     &      iinarray(ii), jinarray(ii), einarray(ii), dinarray(ii)
      end do      
1202  format(1x,a14,3i4,2f5.0)

      call ftdrow(iunit, 10, 2, status)
          if (status .gt. 0)goto 999

      nrows = 12
      call ftgcvs(iunit, 1, 1, 1, nrows, 'NOT DEFINED',  inskey,
     &   anynull, status)
      call ftgcvb(iunit, 4, 1, 1, nrows,bnul,binarray,anynull,status)
      call ftgcvi(iunit, 5, 1, 1, nrows,inul,iinarray,anynull,status)
      call ftgcvj(iunit, 6, 1, 1, nrows, 98,jinarray,anynull,status)
      call ftgcve(iunit, 7, 1, 1, nrows, 98.,einarray,anynull,status)
      call ftgcvd(iunit, 8, 1, 1, nrows,dnul,dinarray,anynull,status)

      write(*,'(1x,A)')' '
      write(*,'(1x,A)') 'Data values after deleting 2 rows at row 10: '
      do ii = 1, nrows
        jj = ichar(binarray(ii))
        write(*,1202) inskey(ii), jj,
     &       iinarray(ii), jinarray(ii), einarray(ii), dinarray(ii)
      end do

      call ftdcol(iunit, 6, status)

      call ftgcvs(iunit, 1, 1, 1, nrows, 'NOT DEFINED',  inskey,
     &   anynull, status)
      call ftgcvb(iunit, 4, 1, 1, nrows,bnul,binarray,anynull,status)
      call ftgcvi(iunit, 5, 1, 1, nrows,inul,iinarray,anynull,status)
      call ftgcve(iunit, 6, 1, 1, nrows, 98.,einarray,anynull,status)
      call ftgcvd(iunit, 7, 1, 1, nrows,dnul,dinarray,anynull,status)

      write(*,'(1x,A)')' '
      write(*,'(1x,A)') 'Data values after deleting column 6: '
      do ii = 1, nrows
        jj = ichar(binarray(ii))      
        write(*,1203) inskey(ii), jj,
     &       iinarray(ii), einarray(ii), dinarray(ii)
1203  format(1x,a14,2i4,2f5.0)

      end do
      call fticol(iunit, 8, 'INSERT_COL', '1E', status)

      call ftgcvs(iunit, 1, 1, 1, nrows, 'NOT DEFINED',  inskey,
     &   anynull, status)
      call ftgcvb(iunit, 4, 1, 1, nrows,bnul,binarray,anynull,status)
      call ftgcvi(iunit, 5, 1, 1, nrows,inul,iinarray,anynull,status)
      call ftgcve(iunit, 6, 1, 1, nrows, 98.,einarray,anynull,status)
      call ftgcvd(iunit, 7, 1, 1, nrows,dnul,dinarray,anynull,status)
      call ftgcvj(iunit, 8, 1, 1, nrows, 98,jinarray,anynull,status)

      write(*,'(1x,A)')' '
      write(*,'(1x,A)') 'Data values after inserting column 8: '
      do ii = 1, nrows
        jj = ichar(binarray(ii))
        write(*,1204) inskey(ii), jj,
     &    iinarray(ii), einarray(ii), dinarray(ii) , jinarray(ii)
1204  format(1x,a14,2i4,2f5.0,i3)
      end do
      call ftpclu(iunit, 8, 1, 1, 10, status)

      call ftgcvs(iunit, 1, 1, 1, nrows, 'NOT DEFINED',  inskey,
     &   anynull, status)
      call ftgcvb(iunit, 4,1,1,nrows,bnul,binarray,anynull,status)
      call ftgcvi(iunit, 5,1,1,nrows,inul,iinarray,anynull,status)
      call ftgcve(iunit, 6,1,1,nrows,98., einarray,anynull,status)
      call ftgcvd(iunit, 7,1,1,nrows,dnul, dinarray,anynull,status)
      call ftgcvj(iunit, 8,1,1,nrows,98, jinarray,anynull, status)

      write(*,'(1x,A)')' '
      write(*,'(1x,A)') 
     &  'Values after setting 1st 10 elements in column 8 = null: '
      do ii = 1, nrows
        jj = ichar(binarray(ii))
        write(*,1204) inskey(ii), jj,
     &      iinarray(ii), einarray(ii), dinarray(ii) , jinarray(ii)
      end do      

C        ####################################################
C        #  insert binary table following the primary array #
C        ####################################################
   
      call ftmahd(iunit,  1, hdutype, status)

      tform(1) = '15A'
      tform(2) = '1L'
      tform(3) = '16X'
      tform(4) = '1B'
      tform(5) = '1I'
      tform(6) = '1J'
      tform(7) = '1E'
      tform(8) = '1D'
      tform(9) = '1C'
      tform(10)= '1M'

      ttype(1) = 'Avalue'
      ttype(2) = 'Lvalue'
      ttype(3) = 'Xvalue'
      ttype(4) = 'Bvalue'
      ttype(5) = 'Ivalue'
      ttype(6) = 'Jvalue'
      ttype(7) = 'Evalue'
      ttype(8) = 'Dvalue'
      ttype(9) = 'Cvalue'
      ttype(10)= 'Mvalue'

      tunit(1)= ' '
      tunit(2)= 'm**2'
      tunit(3)= 'cm'
      tunit(4)= 'erg/s'
      tunit(5)= 'km/s'
      tunit(6)= ' '
      tunit(7)= ' '
      tunit(8)= ' '
      tunit(9)= ' '
      tunit(10)= ' '

      nrows = 20
      tfields = 10
      pcount = 0

      call ftibin(iunit, nrows, tfields, ttype, tform, tunit, 
     & binname, pcount, status)
      write(*,'(1x,A)')' '
      write(*,'(1x,A,I4)') 'ftibin status = ', status
      call ftghdn(iunit, hdunum)
      write(*,'(1x,A,I4)') 'HDU number = ', hdunum

      call ftpkyj(iunit, 'TNULL4', 77, 
     & 'value for undefined pixels', status)
      call ftpkyj(iunit, 'TNULL5', 77, 
     & 'value for undefined pixels', status)
      call ftpkyj(iunit, 'TNULL6', 77, 
     & 'value for undefined pixels', status)

      call ftpkyj(iunit, 'TSCAL4', 1000, 'scaling factor', status)
      call ftpkyj(iunit, 'TSCAL5', 1, 'scaling factor', status)
      call ftpkyj(iunit, 'TSCAL6', 100, 'scaling factor', status)

      call ftpkyj(iunit, 'TZERO4', 0, 'scaling offset', status)
      call ftpkyj(iunit, 'TZERO5', 32768, 'scaling offset', status)
      call ftpkyj(iunit, 'TZERO6', 100, 'scaling offset', status)

      call fttnul(iunit, 4, 77, status)   
C define null value for int cols 
      call fttnul(iunit, 5, 77, status)
      call fttnul(iunit, 6, 77, status)
      
C set scaling 
      scale=1000.
      zero = 0.
      call fttscl(iunit, 4, scale, zero, status)   
      scale=1.
      zero = 32768.
      call fttscl(iunit, 5, scale, zero, status)
      scale=100.
      zero = 100.
      call fttscl(iunit, 6, scale, zero, status)

C  for some reason, it is still necessary to call ftrdef at this point
      call ftrdef(iunit,status)

C        ############################
C        #  write data to columns   #
C        ############################
           
C initialize arrays of values to write to table 
 
      joutarray(1) = 0
      joutarray(2) = 1000
      joutarray(3) = 10000
      joutarray(4) = 32768
      joutarray(5) = 65535


      do ii = 4,6
      
          call ftpclj(iunit, ii, 1, 1, 5, joutarray, status) 
          if (status .eq. 412)then
              write(*,'(1x,A,I4)') 'Overflow writing to column  ', ii
              status = 0
          end if

          call ftpclu(iunit, ii, 6, 1, 1, status)  
C write null value 
      end do

      do jj = 4,6  
        call ftgcvj(iunit, jj, 1,1,6, -999,jinarray,anynull,status)
        write(*,'(1x,6I6)') (jinarray(ii), ii=1,6)
      end do

      write(*,'(1x,A)') ' '
      
C turn off scaling, and read the unscaled values 
      scale = 1.
      zero = 0.
      call fttscl(iunit, 4, scale, zero, status)   
      call fttscl(iunit, 5, scale, zero, status)
      call fttscl(iunit, 6, scale, zero, status)

      do jj = 4,6
        call ftgcvj(iunit, jj,1,1,6,-999,jinarray,anynull,status)       
            write(*,'(1x,6I6)') (jinarray(ii), ii = 1,6)
      end do

      if (status .gt. 0)go to 999

C        ######################################################
C        #  insert image extension following the binary table #
C        ######################################################
      
      bitpix = -32
      naxis = 2
      naxes(1) = 15
      naxes(2) = 25
      call ftiimg(iunit, bitpix, naxis, naxes, status)
      write(*,'(1x,A)')' '
      write(*,'(1x,A,I4)') 
     & ' Create image extension: ftiimg status = ', status
      call ftghdn(iunit, hdunum)
      write(*,'(1x,A,I4)') 'HDU number = ', hdunum

      do jj = 0,29
        do ii = 0,18
          imgarray(ii+1,jj+1) = (jj * 10) + ii
        end do
      end do

      call ftp2di(iunit, 1, 19, naxes(1),naxes(2),imgarray,status)
      write(*,'(1x,A)') ' '
      write(*,'(1x,A,I4)')'Wrote whole 2D array: ftp2di status =',
     &      status

      do jj =1, 30
        do ii = 1, 19
          imgarray(ii,jj) = 0
        end do        
      end do
      
      call ftg2di(iunit,1,0,19,naxes(1),naxes(2),imgarray,anynull,
     &       status)
      write(*,'(1x,A)')' '
      write(*,'(1x,A,I4)')'Read whole 2D array: ftg2di status =',status

      do jj =1, 30
        write (*,1301)(imgarray(ii,jj),ii=1,19)
1301    format(1x,19I4)
      end do

        write(*,'(1x,A)') ' '
      

      do jj =1, 30
        do ii = 1, 19
          imgarray(ii,jj) = 0
        end do        
      end do
      
      do jj =0, 19
        do ii = 0, 9
          imgarray2(ii+1,jj+1) = (jj * (-10)) - ii
        end do        
      end do

      fpixels(1) = 5
      fpixels(2) = 5
      lpixels(1) = 14
      lpixels(2) = 14
      call ftpssi(iunit, 1, naxis, naxes, fpixels, lpixels, 
     &     imgarray2, status)
      write(*,'(1x,A)')' '
      write(*,'(1x,A,I4)')'Wrote subset 2D array: ftpssi status =',
     & status

      call ftg2di(iunit,1,0,19,naxes(1), naxes(2),imgarray,anynull,
     &        status)
      write(*,'(1x,A)')' '
      write(*,'(1x,A,I4)')'Read whole 2D array: ftg2di status =',status

      do jj =1, 30
        write (*,1301)(imgarray(ii,jj),ii=1,19)
      end do
      write(*,'(1x,A)') ' '


      fpixels(1) = 2
      fpixels(2) = 5
      lpixels(1) = 10
      lpixels(2) = 8
      inc(1) = 2
      inc(2) = 3

      do jj = 1,30    
        do ii = 1, 19
          imgarray(ii,jj) = 0
        end do
      end do
      
      call ftgsvi(iunit, 1, naxis, naxes, fpixels, lpixels, inc, 0,
     &       imgarray, anynull, status)
      write(*,'(1x,A)')' '
      write(*,'(1x,A,I4)')
     & 'Read subset of 2D array: ftgsvi status = ',status

      write(*,'(1x,10I5)')(imgarray(ii,1),ii = 1,10)

      
C        ###########################################################
C        #  insert another image extension                         #
C        #  copy the image extension to primary array of tmp file. #
C        #  then delete the tmp file, and the image extension      #
C        ###########################################################
      
      bitpix = 16
      naxis = 2
      naxes(1) = 15
      naxes(2) = 25
      call ftiimg(iunit, bitpix, naxis, naxes, status)
      write(*,'(1x,A)') ' '
      write(*,'(1x,A,I4)')'Create image extension: ftiimg status =',
     &   status
      call ftrdef(iunit, status)
      call ftghdn(iunit, hdunum)
      write(*,'(1x,A,I4)') 'HDU number = ', hdunum


      filename = 't1q2s3v4.tmp'
      call ftinit(tmpunit, filename, 1, status)
      write(*,'(1x,A,I4)')'Create temporary file: ftinit status = ',
     & status

      call ftcopy(iunit, tmpunit, 0, status)
      write(*,'(1x,A)') 
     &  'Copy image extension to primary array of tmp file.'
      write(*,'(1x,A,I4)')'ftcopy status = ',status


      call ftgrec(tmpunit, 1, card, status)
      write(*,'(1x,A)')  card
      call ftgrec(tmpunit, 2, card, status)
      write(*,'(1x,A)')  card
      call ftgrec(tmpunit, 3, card, status)
      write(*,'(1x,A)')  card
      call ftgrec(tmpunit, 4, card, status)
      write(*,'(1x,A)')  card
      call ftgrec(tmpunit, 5, card, status)
      write(*,'(1x,A)')  card
      call ftgrec(tmpunit, 6, card, status)
      write(*,'(1x,A)')  card

      call ftdelt(tmpunit, status)
      write(*,'(1x,A,I4)')'Delete the tmp file: ftdelt status =',status
      call ftdhdu(iunit, hdutype, status)
      write(*,'(1x,A,2I4)')
     &  'Delete the image extension hdutype, status =',
     &         hdutype, status
      call ftghdn(iunit, hdunum)
      write(*,'(1x,A,I4)') 'HDU number = ', hdunum

      
C        ###########################################################
C        #  append bintable extension with variable length columns #
C        ###########################################################
      
      call ftcrhd(iunit, status)
      write(*,'(1x,A,I4)') 'ftcrhd status = ', status

      tform(1)= '1PA'
      tform(2)= '1PL'
      tform(3)= '1PB' 
C Fortran FITSIO doesn't support  1PX 
      tform(4)= '1PB'
      tform(5)= '1PI'
      tform(6)= '1PJ'
      tform(7)= '1PE'
      tform(8)= '1PD'
      tform(9)= '1PC'
      tform(10)= '1PM'

      ttype(1)= 'Avalue'
      ttype(2)= 'Lvalue'
      ttype(3)= 'Xvalue'
      ttype(4)= 'Bvalue'
      ttype(5)= 'Ivalue'
      ttype(6)= 'Jvalue'
      ttype(7)= 'Evalue'
      ttype(8)= 'Dvalue'
      ttype(9)= 'Cvalue'
      ttype(10)= 'Mvalue'

      tunit(1)= ' '
      tunit(2)= 'm**2'
      tunit(3)= 'cm'
      tunit(4)= 'erg/s'
      tunit(5)= 'km/s'
      tunit(6)= ' '
      tunit(7)= ' '
      tunit(8)= ' '
      tunit(9)= ' '
      tunit(10)= ' '

      nrows = 20
      tfields = 10
      pcount = 0

      call ftphbn(iunit, nrows, tfields, ttype, tform, 
     & tunit, binname, pcount, status)
      write(*,'(1x,A,I4)')'Variable length arrays: ftphbn status =',
     & status
      call ftpkyj(iunit, 'TNULL4', 88, 'value for undefined pixels',
     & status)
      call ftpkyj(iunit, 'TNULL5', 88, 'value for undefined pixels',
     & status)
      call ftpkyj(iunit, 'TNULL6', 88, 'value for undefined pixels',
     & status)

C        ############################
C        #  write data to columns   #
C        ############################
            
C initialize arrays of values to write to table 
      iskey='abcdefghijklmnopqrst'

      do ii = 1, 20
      
          boutarray(ii) = char(ii)
          ioutarray(ii) = ii
          joutarray(ii) = ii
          eoutarray(ii) = ii
          doutarray(ii) = ii
      end do

      larray(1) = .false.
      larray(2) = .true.
      larray(3) = .false.
      larray(4) = .false.
      larray(5) = .true.
      larray(6) = .true.
      larray(7) = .false.
      larray(8) = .false.
      larray(9) = .false.
      larray(10) = .true.
      larray(11) = .true.
      larray(12) = .true.
      larray(13) = .false.
      larray(14) = .false.
      larray(15) = .false.
      larray(16) = .false.
      larray(17) = .true.
      larray(18) = .true.
      larray(19) = .true.
      larray(20) = .true.

C      inskey(1) = iskey(1:1)
      inskey(1) = ' '

        call ftpcls(iunit, 1, 1, 1, 1, inskey, status)  
C write string values 
        call ftpcll(iunit, 2, 1, 1, 1, larray, status)  
C write logicals 
        call ftpclx(iunit, 3, 1, 1, 1, larray, status)  
C write bits 
        call ftpclb(iunit, 4, 1, 1, 1, boutarray, status)
        call ftpcli(iunit, 5, 1, 1, 1, ioutarray, status) 
        call ftpclj(iunit, 6, 1, 1, 1, joutarray, status) 
        call ftpcle(iunit, 7, 1, 1, 1, eoutarray, status)
        call ftpcld(iunit, 8, 1, 1, 1, doutarray, status)

      do ii = 2, 20   
C loop over rows 1 - 20 
      
        inskey(1) =  iskey(1:ii)
        call ftpcls(iunit, 1, ii, 1, ii, inskey, status)  
C write string values 

        call ftpcll(iunit, 2, ii, 1, ii, larray, status)  
C write logicals 
        call ftpclu(iunit, 2, ii, ii-1, 1, status)

        call ftpclx(iunit, 3, ii, 1, ii, larray, status)  
C write bits 

        call ftpclb(iunit, 4, ii, 1, ii, boutarray, status)
        call ftpclu(iunit, 4, ii, ii-1, 1, status)

        call ftpcli(iunit, 5, ii, 1, ii, ioutarray, status) 
        call ftpclu(iunit, 5, ii, ii-1, 1, status)

        call ftpclj(iunit, 6, ii, 1, ii, joutarray, status) 
        call ftpclu(iunit, 6, ii, ii-1, 1, status)

        call ftpcle(iunit, 7, ii, 1, ii, eoutarray, status)
        call ftpclu(iunit, 7, ii, ii-1, 1, status)

        call ftpcld(iunit, 8, ii, 1, ii, doutarray, status)
        call ftpclu(iunit, 8, ii, ii-1, 1, status)
      end do

C     it is no longer necessary to update the PCOUNT keyword;
C     FITSIO now does this automatically when the HDU is closed.
C     call ftmkyj(iunit,'PCOUNT',4446, '&',status)
      write(*,'(1x,A,I4)') 'ftpcl_ status = ', status

C        #################################
C        #  close then reopen this HDU   #
C        #################################

       call ftmrhd(iunit, -1, hdutype, status)
       call ftmrhd(iunit,  1, hdutype, status)

C        #############################
C        #  read data from columns   #
C        #############################
      

      call ftgkyj(iunit, 'PCOUNT', pcount, comm, status)
      write(*,'(1x,A,I4)') 'PCOUNT = ', pcount
      
C initialize the variables to be read 
      inskey(1) =' '
      iskey = ' '

      do jj = 1, ii
          larray(jj) = .false.
          boutarray(jj) = char(0)
          ioutarray(jj) = 0
          joutarray(jj) = 0
          eoutarray(jj) = 0
          doutarray(jj) = 0
      end do      

      call ftghdn(iunit, hdunum)
      write(*,'(1x,A,I4)') 'HDU number = ', hdunum

      do ii = 1, 20   
C loop over rows 1 - 20 
      
        do jj = 1, ii
          larray(jj) = .false.
          boutarray(jj) = char(0)
          ioutarray(jj) = 0
          joutarray(jj) = 0
          eoutarray(jj) = 0
          doutarray(jj) = 0
        end do      

        call ftgcvs(iunit, 1, ii, 1,1,iskey,inskey,anynull,status)
        write(*,'(1x,2A,I4)') 'A  ', inskey(1), status

        call ftgcl( iunit, 2, ii, 1, ii, larray, status) 
        write(*,1400)'L',status,(larray(jj),jj=1,ii)
1400    format(1x,a1,i3,20l3)
1401    format(1x,a1,21i3)

        call ftgcx(iunit, 3, ii, 1, ii, larray, status)
        write(*,1400)'X',status,(larray(jj),jj=1,ii)

        bnul = char(99)
        call ftgcvb(iunit, 4, ii, 1,ii,bnul,boutarray,anynull,status)
        do jj = 1,ii
          jinarray(jj) = ichar(boutarray(jj))
        end do
        write(*,1401)'B',(jinarray(jj),jj=1,ii),status

        inul = 99
        call ftgcvi(iunit, 5, ii, 1,ii,inul,ioutarray,anynull,status)
        write(*,1401)'I',(ioutarray(jj),jj=1,ii),status

        call ftgcvj(iunit, 6, ii, 1, ii,99,joutarray,anynull,status)
        write(*,1401)'J',(joutarray(jj),jj=1,ii),status

        call ftgcve(iunit, 7, ii, 1,ii,99.,eoutarray,anynull,status)
        estatus=status
        write(*,1402)'E',(eoutarray(jj),jj=1,ii),estatus
1402    format(1x,a1,1x,21f3.0)

        dnul = 99.
        call ftgcvd(iunit, 8, ii,1,ii,dnul,doutarray,anynull,status)
        estatus=status
        write(*,1402)'D',(doutarray(jj),jj=1,ii),estatus

        call ftgdes(iunit, 8, ii, repeat, offset, status)
        write(*,'(1x,A,2I5)')'Column 8 repeat and offset =',
     &       repeat,offset
      end do

C        #####################################
C        #  create another image extension   #
C        #####################################
      

      bitpix = 32
      naxis = 2
      naxes(1) = 10
      naxes(2) = 2
      npixels = 20
 
      call ftiimg(iunit, bitpix, naxis, naxes, status)
      write(*,'(1x,A)')' '
      write(*,'(1x,A,I4)')'Create image extension: ftiimg status =',
     &       status
      
C initialize arrays of values to write to primary array 
      do ii = 1, npixels
          boutarray(ii) = char(ii * 2 -2)
          ioutarray(ii) = ii * 2 -2
          joutarray(ii) = ii * 2 -2
          koutarray(ii) = ii * 2 -2
          eoutarray(ii) = ii * 2 -2
          doutarray(ii) = ii * 2 -2
      end do      

C write a few pixels with each datatype 
      call ftpprb(iunit, 1, 1,  2, boutarray(1),  status)
      call ftppri(iunit, 1, 3,  2, ioutarray(3),  status)
      call ftpprj(iunit, 1, 5,  2, koutarray(5),  status)
      call ftppri(iunit, 1, 7,  2, ioutarray(7),  status)
      call ftpprj(iunit, 1, 9,  2, joutarray(9),  status)
      call ftppre(iunit, 1, 11, 2, eoutarray(11), status)
      call ftpprd(iunit, 1, 13, 2, doutarray(13), status)
      write(*,'(1x,A,I4)') 'ftppr status = ', status

      
C read back the pixels with each datatype 
      bnul = char(0)
      inul = 0
      knul = 0
      jnul = 0
      enul = 0.
      dnul = 0.

      call ftgpvb(iunit, 1,  1,  14, bnul, binarray, anynull, status)
      call ftgpvi(iunit, 1,  1,  14, inul, iinarray, anynull, status)
      call ftgpvj(iunit, 1,  1,  14, knul, kinarray, anynull, status)
      call ftgpvj(iunit, 1,  1,  14, jnul, jinarray, anynull, status)
      call ftgpve(iunit, 1,  1,  14, enul, einarray, anynull, status)
      call ftgpvd(iunit, 1,  1,  14, dnul, dinarray, anynull, status)

      write(*,'(1x,A)')' '
      write(*,'(1x,A)')
     &   'Image values written with ftppr and read with ftgpv:'
      npixels = 14
      do jj = 1,ii
          joutarray(jj) = ichar(binarray(jj))
      end do

      write(*,1501)(joutarray(ii),ii=1,npixels),anynull,'(byte)'
1501  format(1x,14i3,l3,1x,a)
      write(*,1501)(iinarray(ii),ii=1,npixels),anynull,'(short)'
      write(*,1501)(kinarray(ii),ii=1,npixels),anynull,'(int)'
      write(*,1501)(jinarray(ii),ii=1,npixels),anynull,'(long)'
      write(*,1502)(einarray(ii),ii=1,npixels),anynull,'(float)'
      write(*,1502)(dinarray(ii),ii=1,npixels),anynull,'(double)'
1502  format(2x,14f3.0,l2,1x,a)

C      ##########################################
C      #  test world coordinate system routines #
C      ##########################################

      xrval = 45.83D+00
      yrval =  63.57D+00
      xrpix =  256.D+00
      yrpix =  257.D+00
      xinc =   -.00277777D+00
      yinc =   .00277777D+00

C     write the WCS keywords 
C     use example values from the latest WCS document 
      call ftpkyd(iunit, 'CRVAL1', xrval, 10, 'comment', status)
      call ftpkyd(iunit, 'CRVAL2', yrval, 10, 'comment', status)
      call ftpkyd(iunit, 'CRPIX1', xrpix, 10, 'comment', status)
      call ftpkyd(iunit, 'CRPIX2', yrpix, 10, 'comment', status)
      call ftpkyd(iunit, 'CDELT1', xinc, 10, 'comment', status)
      call ftpkyd(iunit, 'CDELT2', yinc, 10, 'comment', status)
C     call ftpkyd(iunit, 'CROTA2', rot, 10, 'comment', status) 
      call ftpkys(iunit, 'CTYPE1', xctype, 'comment', status)
      call ftpkys(iunit, 'CTYPE2', yctype, 'comment', status)
      write(*,'(1x,A)')' '
      write(*,'(1x,A,I4)')'Wrote WCS keywords status =', status

C     reset value, to make sure they are reread correctly
      xrval =  0.D+00
      yrval =  0.D+00
      xrpix =  0.D+00
      yrpix =  0.D+00
      xinc =   0.D+00
      yinc =   0.D+00
      rot =    67.D+00

      call ftgics(iunit, xrval, yrval, xrpix,
     &      yrpix, xinc, yinc, rot, ctype, status)
      write(*,'(1x,A,I4)')'Read WCS keywords with ftgics status =',
     &  status

      xpix = 0.5D+00
      ypix = 0.5D+00

      call ftwldp(xpix,ypix,xrval,yrval,xrpix,yrpix,xinc,yinc,
     &     rot,ctype, xpos, ypos,status)

      write(*,'(1x,A,2f8.3)')'  CRVAL1, CRVAL2 =', xrval,yrval
      write(*,'(1x,A,2f8.3)')'  CRPIX1, CRPIX2 =', xrpix,yrpix
      write(*,'(1x,A,2f12.8)')'  CDELT1, CDELT2 =', xinc,yinc
      write(*,'(1x,A,f8.3,2A)')'  Rotation =',rot,' CTYPE =',ctype
      write(*,'(1x,A,I4)')'Calculated sky coord. with ftwldp status =',
     &   status
      write(*,6501)xpix,ypix,xpos,ypos
6501  format('  Pixels (',f10.6,f10.6,') --> (',f10.6,f10.6,') Sky')

      call ftxypx(xpos,ypos,xrval,yrval,xrpix,yrpix,xinc,yinc,
     &     rot,ctype, xpix, ypix,status)
      write(*,'(1x,A,I4)')
     & 'Calculated pixel coord. with ftxypx status =', status
      write(*,6502)xpos,ypos,xpix,ypix
6502  format('  Sky (',f10.6,f10.6,') --> (',f10.6,f10.6,') Pixels')

     
C        ######################################
C        #  append another ASCII table        #
C        ######################################
      

      tform(1)= 'A15'
      tform(2)= 'I11'
      tform(3)= 'F15.6'
      tform(4)= 'E13.5'
      tform(5)= 'D22.14'

      tbcol(1)= 1
      tbcol(2)= 17
      tbcol(3)= 29
      tbcol(4)= 45
      tbcol(5)= 59
      rowlen = 80

      ttype(1)= 'Name'
      ttype(2)= 'Ivalue'
      ttype(3)= 'Fvalue'
      ttype(4)= 'Evalue'
      ttype(5)= 'Dvalue'

      tunit(1)= ' '
      tunit(2)= 'm**2'
      tunit(3)= 'cm'
      tunit(4)= 'erg/s'
      tunit(5)= 'km/s'

      nrows = 11
      tfields = 5
      tblname = 'new_table'

      call ftitab(iunit, rowlen, nrows, tfields, ttype, tbcol, 
     & tform, tunit, tblname, status)
      write(*,'(1x,A)') ' '
      write(*,'(1x,A,I4)') 'ftitab status = ', status

      call ftpcls(iunit, 1, 1, 1, 3, onskey, status)  
C write string values 

C initialize arrays of values to write to primary array 
      
      do ii = 1,npixels
          boutarray(ii) = char(ii * 3 -3)
          ioutarray(ii) = ii * 3 -3
          joutarray(ii) = ii * 3 -3
          koutarray(ii) = ii * 3 -3
          eoutarray(ii) = ii * 3 -3
          doutarray(ii) = ii * 3 -3
      end do

      do ii = 2,5 
C loop over cols 2 - 5 
      
          call ftpclb(iunit,  ii, 1, 1, 2, boutarray,  status) 
          call ftpcli(iunit,  ii, 3, 1, 2,ioutarray(3),status)
          call ftpclj(iunit,  ii, 5, 1, 2,joutarray(5),status)
          call ftpcle(iunit,  ii, 7, 1, 2,eoutarray(7),status)
          call ftpcld(iunit,  ii, 9, 1, 2,doutarray(9),status)
      end do
      write(*,'(1x,A,I4)') 'ftpcl status = ', status
      
C read back the pixels with each datatype 
      call ftgcvb(iunit,   2, 1, 1, 10, bnul, binarray,anynull,
     & status)
      call ftgcvi(iunit,  2, 1, 1, 10, inul, iinarray,anynull,
     & status)
      call ftgcvj(iunit,    3, 1, 1, 10, knul, kinarray,anynull,
     & status)
      call ftgcvj(iunit,    3, 1, 1, 10, jnul, jinarray,anynull,
     & status)
      call ftgcve(iunit,   4, 1, 1, 10, enul, einarray,anynull,
     & status)
      call ftgcvd(iunit, 5, 1, 1, 10, dnul, dinarray,anynull,
     & status)

      write(*,'(1x,A)') 
     &'Column values written with ftpcl and read with ftgcl: '
      npixels = 10
      do ii = 1,npixels
         joutarray(ii) = ichar(binarray(ii))
      end do
      write(*,1601)(joutarray(ii),ii = 1, npixels),anynull,'(byte) '
      write(*,1601)(iinarray(ii),ii = 1, npixels),anynull,'(short) '
      write(*,1601)(kinarray(ii),ii = 1, npixels),anynull,'(int) '
      write(*,1601)(jinarray(ii),ii = 1, npixels),anynull,'(long) '
      write(*,1602)(einarray(ii),ii = 1, npixels),anynull,'(float) '
      write(*,1602)(dinarray(ii),ii = 1, npixels),anynull,'(double) '
1601  format(1x,10i3,l3,1x,a)
1602  format(2x,10f3.0,l2,1x,a)
      
C        ###########################################################
C        #  perform stress test by cycling thru all the extensions #
C        ###########################################################
      write(*,'(1x,A)')' '
      write(*,'(1x,A)')'Repeatedly move to the 1st 4 HDUs of the file: '

      do ii = 1,10
        call ftmahd(iunit,  1, hdutype, status)
        call ftghdn(iunit, hdunum)
        call ftmrhd(iunit,  1, hdutype, status)
        call ftghdn(iunit, hdunum)
        call ftmrhd(iunit,  1, hdutype, status)
        call ftghdn(iunit, hdunum)
        call ftmrhd(iunit,  1, hdutype, status)
        call ftghdn(iunit, hdunum)
        call ftmrhd(iunit, -1, hdutype, status)
        call ftghdn(iunit, hdunum)
        if (status .gt. 0) go to 999
      end do
      
      write(*,'(1x,A)') ' '

      checksum = 1234567890.D+00
      call ftesum(checksum, .false., asciisum)
      write(*,'(1x,A,F13.1,2A)')'Encode checksum: ',checksum,' -> ',
     &  asciisum
      checksum = 0
      call ftdsum(asciisum, 0, checksum)
      write(*,'(1x,3A,F13.1)') 'Decode checksum: ',asciisum,' -> ',
     & checksum

      call ftpcks(iunit, status)

C         don't print the CHECKSUM value because it is different every day
C         because the current date is in the comment field.

         call ftgcrd(iunit, 'CHECKSUM', card, status)
C         write(*,'(1x,A)') card

      call ftgcrd(iunit, 'DATASUM', card, status)
      write(*,'(1x,A)') card(1:22)

      call ftgcks(iunit, datsum, checksum, status)
      write(*,'(1x,A,F13.1,I4)') 'ftgcks data checksum, status = ',
     &         datsum, status

      call ftvcks(iunit, datastatus, hdustatus, status) 
      write(*,'(1x,A,3I4)')'ftvcks datastatus, hdustatus, status =  ',
     &          datastatus, hdustatus, status
 
      call ftprec(iunit,
     & 'new_key = ''written by fxprec'' / to change checksum',status)
      call ftucks(iunit, status)
      write(*,'(1x,A,I4)') 'ftupck status = ', status

      call ftgcrd(iunit, 'DATASUM', card, status)
      write(*,'(1x,A)') card(1:22)
      call ftvcks(iunit, datastatus, hdustatus, status) 
      write(*,'(1x,A,3I4)') 'ftvcks datastatus, hdustatus, status =  ',
     &          datastatus, hdustatus, status
 
C        delete the checksum keywords, so that the FITS file is always
C        the same, regardless of the date of when testprog is run.
      
      call ftdkey(iunit, 'CHECKSUM', status)
      call ftdkey(iunit, 'DATASUM',  status)


C        ############################
C        #  close file and quit     #
C        ############################
      

999   continue  
C jump here on error 

      call ftclos(iunit, status) 
      write(*,'(1x,A,I4)') 'ftclos status = ', status
      write(*,'(1x,A)')' '

      write(*,'(1x,A)')
     &  'Normally, there should be 8 error messages on the'
      write(*,'(1x,A)') 'stack all regarding ''numerical overflows'':'

      call ftgmsg(errmsg)
      nmsg = 0

998   continue
      if (errmsg .ne. ' ')then      
          write(*,'(1x,A)') errmsg
          nmsg = nmsg + 1
          call ftgmsg(errmsg)
          go to 998
      end if

      if (nmsg .ne. 8)write(*,'(1x,A)')
     & ' WARNING: Did not find the expected 8 error messages!'

      call ftgerr(status, errmsg)
      write(*,'(1x,A)')' '
      write(*,'(1x,A,I4,2A)') 'Status =', status,': ', errmsg(1:50)
      end
