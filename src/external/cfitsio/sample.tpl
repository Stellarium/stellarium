# sample template - create 9 HDUs in one FITS file

# syntax :

# everything which starts with a hashmark is ignored
# the same for empty lines

# one can use \include filename to include other files
# equal sign after keyword name is optional
# \group must be terminated by \end
# xtension is terminated by \group, xtension or EOF
# First HDU of type image may be defined using "SIMPLE T"
# group may contain other groups and xtensions
# keywords may be indented, but indentation is limited to max 7chars.

# template parser processes all keywords, makes substitutions
# when necessary (hashmarks -> index), converts keyword names
# to uppercase and writes keywords to file.
# For string keywords, parser uses CFITSIO long string routines
# to store string values longer than 72 characters. Parser can
# read/process lines of any length, as long as there is enough memory.
# For a very limited set of keywords (like NAXIS1 for binary tables)
# template parser ignores values specified in template file
# (one should not specify NAXIS1 for binary tables) and computes and
# writes values respective to table structure.
# number of rows in binary/ascii tables can be specified with NAXIS2

# if the 1st HDU is not defined with "SIMPLE T" and is defined with
# xtension image/asciitable/bintable then dummy primary HDU is
# created by parser.

simple	t
 bitpix		16
 naxis		1
 naxis1		10
COMMENT
 comment  
 sdsdf / keyword without value (null type)
        if line begins with 8+ spaces everything is a comment

xtension image
 bitpix		16
 naxis		1
 naxis1		10
 QWERW		F / dfg dfgsd fg - boolean keyword
 FFFSDS45	3454345 /integer_or_real keyword
 SSSDFS34	32345.453   / real keyword
 adsfd34	(234234.34,2342342.3) / complex keyword - no space between ()
 SDFDF#		adfasdfasdfdfcvxccvzxcvcvcxv / autoindexed keyword, here idx=1
 SDFD#		'asf dfa dfad df dfad f ad fadfdaf dfdfa df loooooong keyyywoooord - reaaalllly verrrrrrrrrryy loooooooooong' / comment is max 80 chars
 history        history record, spaces (all but 1st) after keyname are copied
 SDFDF#		strg_value_without_spaces / autoindexed keyword, here idx=2
 comment        comment record, spaces (all but 1st) after keyname are copied
 strg45		'sdfasdfadfffdfasdfasdfasdf &'
 continue   'sdfsdfsdfsd fsdf' / 3 spaces must follow CONTINUE keyword


xtension image
 bitpix		16
 naxis		1
 naxis1		10

\group
 
 xtension image
  bitpix	16
  naxis		1
  naxis1	10

# create group inside group

 \group

# one can specify additional columns in group HDU. The first column
# specified will have index 7 however, since the first 6 columns are occupied
# by grouping table itself.
# Please note, that it is not allowed to specify EXTNAME keyword as an
# additional keyword for group HDU, since parser automatically writes
# EXTNAME = GROUPING keyword.

  TFORM#	13A
  TTYPE#	ADDIT_COL_IN_GRP_HDU
  TFORM#	1E
  TTYPE#	REAL_COLUMN
  COMMENT sure, there is always place for comments

# the following specifies empty ascii table (0 cols / 0 rows)

  xtension asciitable

 \end
 
\end

# one do not have to specify all NAXISn keywords. If not specified
# NAXISn equals zero.

xtension image
 bitpix	16
 naxis	1
# naxis1	10

# the following tells how to set number of rows in binary table
# note also that the last line in template file does not have to
# have LineFeed character as the last one.

xtension bintable
naxis2	 10
EXTNAME	asdjfhsdkf
TTYPE#   MEMBER_XTENSION
TFORM#   8A
TTYPE#   MEMBER_2
TFORM#   8U
TTYPE#   MEMBER_3
TFORM#   8V
TTYPE#   MEMBER_NAME
TFORM#   32A
TDIM#	 '(8,4)'
TTYPE#   MEMBER_VERSION
TFORM#   1J
TNULL#   0