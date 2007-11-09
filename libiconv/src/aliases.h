/* ANSI-C code produced by gperf version 3.0.1 */
/* Command-line: gperf -m 10 lib/aliases.gperf  */
/* Computed positions: -k'1,3-11,$' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

#line 1 "lib/aliases.gperf"
struct alias { int name; unsigned int encoding_index; };

#define TOTAL_KEYWORDS 329
#define MIN_WORD_LENGTH 2
#define MAX_WORD_LENGTH 45
#define MIN_HASH_VALUE 11
#define MAX_HASH_VALUE 849
/* maximum key range = 839, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
aliases_hash (register const char *str, register unsigned int len)
{
  static const unsigned short asso_values[] =
    {
      850, 850, 850, 850, 850, 850, 850, 850, 850, 850,
      850, 850, 850, 850, 850, 850, 850, 850, 850, 850,
      850, 850, 850, 850, 850, 850, 850, 850, 850, 850,
      850, 850, 850, 850, 850, 850, 850, 850, 850, 850,
      850, 850, 850, 850, 850,  12,  99, 850,  41,   2,
        7,   6,  56,   4,   3,  74,   8,  17, 181, 850,
      850, 850, 850, 850, 850,  18, 172,   5,  18,  59,
      123,  45, 100,   2, 227, 205, 135, 173,   4,   2,
       20, 850,   5,  58,  20, 143, 324, 131, 164,  13,
        5, 850, 850, 850, 850,  49, 850, 850, 850, 850,
      850, 850, 850, 850, 850, 850, 850, 850, 850, 850,
      850, 850, 850, 850, 850, 850, 850, 850, 850, 850,
      850, 850, 850, 850, 850, 850, 850, 850
    };
  register int hval = len;

  switch (hval)
    {
      default:
        hval += asso_values[(unsigned char)str[10]];
      /*FALLTHROUGH*/
      case 10:
        hval += asso_values[(unsigned char)str[9]];
      /*FALLTHROUGH*/
      case 9:
        hval += asso_values[(unsigned char)str[8]];
      /*FALLTHROUGH*/
      case 8:
        hval += asso_values[(unsigned char)str[7]];
      /*FALLTHROUGH*/
      case 7:
        hval += asso_values[(unsigned char)str[6]];
      /*FALLTHROUGH*/
      case 6:
        hval += asso_values[(unsigned char)str[5]];
      /*FALLTHROUGH*/
      case 5:
        hval += asso_values[(unsigned char)str[4]];
      /*FALLTHROUGH*/
      case 4:
        hval += asso_values[(unsigned char)str[3]];
      /*FALLTHROUGH*/
      case 3:
        hval += asso_values[(unsigned char)str[2]];
      /*FALLTHROUGH*/
      case 2:
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

struct stringpool_t
  {
    char stringpool_str11[sizeof("CN")];
    char stringpool_str15[sizeof("R8")];
    char stringpool_str17[sizeof("866")];
    char stringpool_str25[sizeof("862")];
    char stringpool_str26[sizeof("CP1361")];
    char stringpool_str27[sizeof("CP866")];
    char stringpool_str28[sizeof("CP1251")];
    char stringpool_str30[sizeof("CP1256")];
    char stringpool_str32[sizeof("CP1255")];
    char stringpool_str33[sizeof("CP1133")];
    char stringpool_str34[sizeof("ASCII")];
    char stringpool_str35[sizeof("CP862")];
    char stringpool_str36[sizeof("CP1253")];
    char stringpool_str37[sizeof("CHAR")];
    char stringpool_str38[sizeof("CP1252")];
    char stringpool_str39[sizeof("CP936")];
    char stringpool_str40[sizeof("CP1258")];
    char stringpool_str42[sizeof("C99")];
    char stringpool_str47[sizeof("CP932")];
    char stringpool_str49[sizeof("ISO-IR-6")];
    char stringpool_str54[sizeof("CP819")];
    char stringpool_str56[sizeof("ISO-IR-166")];
    char stringpool_str58[sizeof("ISO-IR-165")];
    char stringpool_str60[sizeof("ISO-IR-126")];
    char stringpool_str64[sizeof("ISO-IR-58")];
    char stringpool_str65[sizeof("ISO-IR-226")];
    char stringpool_str66[sizeof("ISO8859-1")];
    char stringpool_str68[sizeof("ISO8859-6")];
    char stringpool_str69[sizeof("ISO-IR-138")];
    char stringpool_str70[sizeof("ISO8859-5")];
    char stringpool_str71[sizeof("ISO8859-16")];
    char stringpool_str73[sizeof("ISO8859-15")];
    char stringpool_str74[sizeof("ISO8859-3")];
    char stringpool_str76[sizeof("ISO8859-2")];
    char stringpool_str77[sizeof("ISO8859-13")];
    char stringpool_str78[sizeof("ISO8859-8")];
    char stringpool_str79[sizeof("ISO-8859-1")];
    char stringpool_str80[sizeof("GB2312")];
    char stringpool_str81[sizeof("ISO-8859-6")];
    char stringpool_str82[sizeof("EUCCN")];
    char stringpool_str83[sizeof("ISO-8859-5")];
    char stringpool_str84[sizeof("ISO-8859-16")];
    char stringpool_str85[sizeof("ISO-IR-159")];
    char stringpool_str86[sizeof("ISO-8859-15")];
    char stringpool_str87[sizeof("ISO-8859-3")];
    char stringpool_str89[sizeof("ISO-8859-2")];
    char stringpool_str90[sizeof("ISO-8859-13")];
    char stringpool_str91[sizeof("ISO-8859-8")];
    char stringpool_str92[sizeof("ISO-IR-101")];
    char stringpool_str93[sizeof("850")];
    char stringpool_str95[sizeof("EUC-CN")];
    char stringpool_str96[sizeof("ISO8859-9")];
    char stringpool_str98[sizeof("ISO-IR-199")];
    char stringpool_str99[sizeof("CSASCII")];
    char stringpool_str100[sizeof("ISO646-CN")];
    char stringpool_str104[sizeof("CP850")];
    char stringpool_str105[sizeof("ISO-IR-203")];
    char stringpool_str106[sizeof("CP1250")];
    char stringpool_str107[sizeof("HZ")];
    char stringpool_str109[sizeof("ISO-8859-9")];
    char stringpool_str113[sizeof("CP950")];
    char stringpool_str114[sizeof("ISO-2022-CN")];
    char stringpool_str116[sizeof("ISO_8859-1")];
    char stringpool_str117[sizeof("CP949")];
    char stringpool_str118[sizeof("ISO_8859-6")];
    char stringpool_str119[sizeof("ISO-IR-148")];
    char stringpool_str120[sizeof("ISO_8859-5")];
    char stringpool_str121[sizeof("ISO_8859-16")];
    char stringpool_str122[sizeof("ISO-IR-109")];
    char stringpool_str123[sizeof("ISO_8859-15")];
    char stringpool_str124[sizeof("ISO_8859-3")];
    char stringpool_str125[sizeof("ISO_8859-16:2001")];
    char stringpool_str126[sizeof("ISO_8859-2")];
    char stringpool_str127[sizeof("ISO_8859-13")];
    char stringpool_str128[sizeof("ISO_8859-8")];
    char stringpool_str131[sizeof("ISO-IR-110")];
    char stringpool_str132[sizeof("ISO_8859-15:1998")];
    char stringpool_str134[sizeof("ISO-2022-CN-EXT")];
    char stringpool_str136[sizeof("CP1254")];
    char stringpool_str137[sizeof("ISO-IR-149")];
    char stringpool_str139[sizeof("L1")];
    char stringpool_str140[sizeof("L6")];
    char stringpool_str141[sizeof("L5")];
    char stringpool_str143[sizeof("L3")];
    char stringpool_str144[sizeof("L2")];
    char stringpool_str145[sizeof("L8")];
    char stringpool_str146[sizeof("ISO_8859-9")];
    char stringpool_str147[sizeof("ISO8859-10")];
    char stringpool_str153[sizeof("CSISO2022CN")];
    char stringpool_str155[sizeof("ISO-IR-179")];
    char stringpool_str156[sizeof("UHC")];
    char stringpool_str158[sizeof("ISO-IR-14")];
    char stringpool_str160[sizeof("ISO-8859-10")];
    char stringpool_str167[sizeof("CP367")];
    char stringpool_str168[sizeof("ISO_8859-10:1992")];
    char stringpool_str170[sizeof("ISO-IR-100")];
    char stringpool_str171[sizeof("LATIN1")];
    char stringpool_str172[sizeof("CP1257")];
    char stringpool_str173[sizeof("LATIN6")];
    char stringpool_str174[sizeof("ISO8859-4")];
    char stringpool_str175[sizeof("LATIN5")];
    char stringpool_str176[sizeof("TIS620")];
    char stringpool_str177[sizeof("ISO8859-14")];
    char stringpool_str178[sizeof("ELOT_928")];
    char stringpool_str179[sizeof("LATIN3")];
    char stringpool_str180[sizeof("SJIS")];
    char stringpool_str181[sizeof("LATIN2")];
    char stringpool_str183[sizeof("LATIN8")];
    char stringpool_str184[sizeof("ISO_8859-14:1998")];
    char stringpool_str185[sizeof("GEORGIAN-ACADEMY")];
    char stringpool_str186[sizeof("MAC")];
    char stringpool_str187[sizeof("ISO-8859-4")];
    char stringpool_str189[sizeof("TIS-620")];
    char stringpool_str190[sizeof("ISO-8859-14")];
    char stringpool_str191[sizeof("GB18030")];
    char stringpool_str192[sizeof("X0212")];
    char stringpool_str193[sizeof("L4")];
    char stringpool_str196[sizeof("ISO-IR-57")];
    char stringpool_str197[sizeof("ISO_8859-10")];
    char stringpool_str198[sizeof("IBM866")];
    char stringpool_str199[sizeof("ISO-IR-157")];
    char stringpool_str200[sizeof("ISO-IR-87")];
    char stringpool_str202[sizeof("ISO-IR-127")];
    char stringpool_str203[sizeof("US")];
    char stringpool_str204[sizeof("CP874")];
    char stringpool_str206[sizeof("IBM862")];
    char stringpool_str207[sizeof("MS936")];
    char stringpool_str210[sizeof("ISO8859-7")];
    char stringpool_str211[sizeof("L7")];
    char stringpool_str214[sizeof("LATIN-9")];
    char stringpool_str215[sizeof("ISO-IR-144")];
    char stringpool_str220[sizeof("L10")];
    char stringpool_str221[sizeof("X0201")];
    char stringpool_str222[sizeof("ROMAN8")];
    char stringpool_str223[sizeof("ISO-8859-7")];
    char stringpool_str224[sizeof("ISO_8859-4")];
    char stringpool_str225[sizeof("IBM819")];
    char stringpool_str226[sizeof("ARABIC")];
    char stringpool_str227[sizeof("ISO_8859-14")];
    char stringpool_str228[sizeof("GB_2312-80")];
    char stringpool_str229[sizeof("BIG5")];
    char stringpool_str231[sizeof("TIS620-0")];
    char stringpool_str232[sizeof("UCS-2")];
    char stringpool_str233[sizeof("X0208")];
    char stringpool_str238[sizeof("CSBIG5")];
    char stringpool_str239[sizeof("CSKOI8R")];
    char stringpool_str241[sizeof("GB_1988-80")];
    char stringpool_str242[sizeof("BIG-5")];
    char stringpool_str243[sizeof("KOI8-R")];
    char stringpool_str244[sizeof("IBM-CP1133")];
    char stringpool_str249[sizeof("JP")];
    char stringpool_str250[sizeof("US-ASCII")];
    char stringpool_str251[sizeof("CN-BIG5")];
    char stringpool_str252[sizeof("LATIN10")];
    char stringpool_str253[sizeof("CHINESE")];
    char stringpool_str255[sizeof("CSUNICODE11")];
    char stringpool_str257[sizeof("ISO-CELTIC")];
    char stringpool_str259[sizeof("CSGB2312")];
    char stringpool_str260[sizeof("ISO_8859-7")];
    char stringpool_str261[sizeof("CSISOLATIN1")];
    char stringpool_str263[sizeof("CSISOLATIN6")];
    char stringpool_str265[sizeof("CSISOLATIN5")];
    char stringpool_str266[sizeof("TIS620.2533-1")];
    char stringpool_str267[sizeof("MACCROATIAN")];
    char stringpool_str269[sizeof("CSISOLATIN3")];
    char stringpool_str270[sizeof("UNICODE-1-1")];
    char stringpool_str271[sizeof("CSISOLATIN2")];
    char stringpool_str273[sizeof("KOI8-T")];
    char stringpool_str274[sizeof("CSISOLATINCYRILLIC")];
    char stringpool_str275[sizeof("IBM850")];
    char stringpool_str276[sizeof("MS-ANSI")];
    char stringpool_str278[sizeof("TIS620.2529-1")];
    char stringpool_str279[sizeof("LATIN4")];
    char stringpool_str280[sizeof("GEORGIAN-PS")];
    char stringpool_str284[sizeof("EUCKR")];
    char stringpool_str285[sizeof("CSISOLATINARABIC")];
    char stringpool_str290[sizeof("ECMA-118")];
    char stringpool_str292[sizeof("UTF-16")];
    char stringpool_str295[sizeof("ARMSCII-8")];
    char stringpool_str297[sizeof("EUC-KR")];
    char stringpool_str298[sizeof("ISO-10646-UCS-2")];
    char stringpool_str299[sizeof("UTF-8")];
    char stringpool_str301[sizeof("KOREAN")];
    char stringpool_str302[sizeof("CYRILLIC")];
    char stringpool_str304[sizeof("UTF-32")];
    char stringpool_str305[sizeof("TIS620.2533-0")];
    char stringpool_str306[sizeof("CSUNICODE")];
    char stringpool_str310[sizeof("ISO_8859-5:1988")];
    char stringpool_str312[sizeof("ISO_8859-3:1988")];
    char stringpool_str314[sizeof("ISO_8859-8:1988")];
    char stringpool_str315[sizeof("LATIN7")];
    char stringpool_str316[sizeof("ISO-2022-KR")];
    char stringpool_str319[sizeof("KSC_5601")];
    char stringpool_str327[sizeof("MACTHAI")];
    char stringpool_str329[sizeof("CSUCS4")];
    char stringpool_str330[sizeof("UCS-4")];
    char stringpool_str331[sizeof("CSUNICODE11UTF7")];
    char stringpool_str332[sizeof("ISO_8859-9:1989")];
    char stringpool_str333[sizeof("CN-GB-ISOIR165")];
    char stringpool_str336[sizeof("EUCJP")];
    char stringpool_str338[sizeof("IBM367")];
    char stringpool_str339[sizeof("HP-ROMAN8")];
    char stringpool_str344[sizeof("ASMO-708")];
    char stringpool_str346[sizeof("ISO646-US")];
    char stringpool_str347[sizeof("ISO-10646-UCS-4")];
    char stringpool_str348[sizeof("UNICODE-1-1-UTF-7")];
    char stringpool_str349[sizeof("EUC-JP")];
    char stringpool_str350[sizeof("WCHAR_T")];
    char stringpool_str351[sizeof("EUCTW")];
    char stringpool_str352[sizeof("ISO-2022-JP-1")];
    char stringpool_str353[sizeof("CSHPROMAN8")];
    char stringpool_str354[sizeof("ISO646-JP")];
    char stringpool_str355[sizeof("CSISO2022KR")];
    char stringpool_str356[sizeof("TCVN")];
    char stringpool_str357[sizeof("ISO-2022-JP-2")];
    char stringpool_str362[sizeof("ISO_8859-4:1988")];
    char stringpool_str364[sizeof("EUC-TW")];
    char stringpool_str365[sizeof("CSISO58GB231280")];
    char stringpool_str367[sizeof("MS-EE")];
    char stringpool_str368[sizeof("ISO-2022-JP")];
    char stringpool_str369[sizeof("CSISOLATIN4")];
    char stringpool_str372[sizeof("CSPC862LATINHEBREW")];
    char stringpool_str373[sizeof("NEXTSTEP")];
    char stringpool_str374[sizeof("ISO_8859-1:1987")];
    char stringpool_str375[sizeof("ISO_8859-6:1987")];
    char stringpool_str377[sizeof("CSIBM866")];
    char stringpool_str379[sizeof("ISO_8859-2:1987")];
    char stringpool_str380[sizeof("HZ-GB-2312")];
    char stringpool_str383[sizeof("WINDOWS-1251")];
    char stringpool_str384[sizeof("WINDOWS-1256")];
    char stringpool_str385[sizeof("WINDOWS-1255")];
    char stringpool_str386[sizeof("ECMA-114")];
    char stringpool_str387[sizeof("WINDOWS-1253")];
    char stringpool_str388[sizeof("WINDOWS-1252")];
    char stringpool_str389[sizeof("WINDOWS-1258")];
    char stringpool_str390[sizeof("GREEK8")];
    char stringpool_str392[sizeof("MACROMAN")];
    char stringpool_str393[sizeof("JIS_C6226-1983")];
    char stringpool_str395[sizeof("CSISO2022JP2")];
    char stringpool_str396[sizeof("WINDOWS-936")];
    char stringpool_str397[sizeof("JIS0208")];
    char stringpool_str399[sizeof("VISCII")];
    char stringpool_str402[sizeof("CSISO57GB1988")];
    char stringpool_str403[sizeof("KS_C_5601-1989")];
    char stringpool_str407[sizeof("CSISO2022JP")];
    char stringpool_str408[sizeof("CSVISCII")];
    char stringpool_str411[sizeof("CN-GB")];
    char stringpool_str412[sizeof("MACARABIC")];
    char stringpool_str422[sizeof("WINDOWS-1250")];
    char stringpool_str428[sizeof("MACROMANIA")];
    char stringpool_str429[sizeof("CSKSC56011987")];
    char stringpool_str430[sizeof("JIS_C6220-1969-RO")];
    char stringpool_str431[sizeof("UTF-7")];
    char stringpool_str434[sizeof("CSEUCKR")];
    char stringpool_str436[sizeof("CSISO14JISC6220RO")];
    char stringpool_str437[sizeof("WINDOWS-1254")];
    char stringpool_str438[sizeof("CSISO159JISX02121990")];
    char stringpool_str446[sizeof("ISO_8859-7:1987")];
    char stringpool_str447[sizeof("MACICELAND")];
    char stringpool_str455[sizeof("WINDOWS-1257")];
    char stringpool_str458[sizeof("GBK")];
    char stringpool_str460[sizeof("KS_C_5601-1987")];
    char stringpool_str461[sizeof("TCVN5712-1")];
    char stringpool_str463[sizeof("TCVN-5712")];
    char stringpool_str471[sizeof("UCS-2-INTERNAL")];
    char stringpool_str473[sizeof("MACINTOSH")];
    char stringpool_str478[sizeof("UNICODELITTLE")];
    char stringpool_str480[sizeof("UCS-2LE")];
    char stringpool_str483[sizeof("ANSI_X3.4-1986")];
    char stringpool_str485[sizeof("MS-CYRL")];
    char stringpool_str488[sizeof("ANSI_X3.4-1968")];
    char stringpool_str493[sizeof("CSISOLATINHEBREW")];
    char stringpool_str496[sizeof("MACCYRILLIC")];
    char stringpool_str498[sizeof("CSMACINTOSH")];
    char stringpool_str501[sizeof("CSEUCTW")];
    char stringpool_str503[sizeof("UNICODEBIG")];
    char stringpool_str510[sizeof("UCS-2-SWAPPED")];
    char stringpool_str511[sizeof("CSISOLATINGREEK")];
    char stringpool_str517[sizeof("UCS-2BE")];
    char stringpool_str519[sizeof("KOI8-U")];
    char stringpool_str520[sizeof("UCS-4-INTERNAL")];
    char stringpool_str521[sizeof("VISCII1.1-1")];
    char stringpool_str525[sizeof("KOI8-RU")];
    char stringpool_str529[sizeof("UCS-4LE")];
    char stringpool_str533[sizeof("MS-HEBR")];
    char stringpool_str537[sizeof("EXTENDED_UNIX_CODE_PACKED_FORMAT_FOR_JAPANESE")];
    char stringpool_str544[sizeof("UTF-16LE")];
    char stringpool_str547[sizeof("MULELAO-1")];
    char stringpool_str552[sizeof("UTF-32LE")];
    char stringpool_str558[sizeof("MACCENTRALEUROPE")];
    char stringpool_str559[sizeof("UCS-4-SWAPPED")];
    char stringpool_str561[sizeof("WINDOWS-874")];
    char stringpool_str563[sizeof("ISO_646.IRV:1991")];
    char stringpool_str566[sizeof("UCS-4BE")];
    char stringpool_str569[sizeof("SHIFT-JIS")];
    char stringpool_str571[sizeof("JIS_X0212")];
    char stringpool_str577[sizeof("MS-ARAB")];
    char stringpool_str578[sizeof("GREEK")];
    char stringpool_str581[sizeof("UTF-16BE")];
    char stringpool_str587[sizeof("JISX0201-1976")];
    char stringpool_str589[sizeof("UTF-32BE")];
    char stringpool_str591[sizeof("JAVA")];
    char stringpool_str600[sizeof("JIS_X0201")];
    char stringpool_str604[sizeof("HEBREW")];
    char stringpool_str606[sizeof("SHIFT_JIS")];
    char stringpool_str612[sizeof("JIS_X0208")];
    char stringpool_str623[sizeof("CSISO87JISX0208")];
    char stringpool_str624[sizeof("JIS_X0212-1990")];
    char stringpool_str629[sizeof("JIS_X0208-1983")];
    char stringpool_str651[sizeof("TCVN5712-1:1993")];
    char stringpool_str663[sizeof("CSSHIFTJIS")];
    char stringpool_str664[sizeof("JIS_X0208-1990")];
    char stringpool_str683[sizeof("MACUKRAINE")];
    char stringpool_str688[sizeof("MS_KANJI")];
    char stringpool_str689[sizeof("CSHALFWIDTHKATAKANA")];
    char stringpool_str694[sizeof("JOHAB")];
    char stringpool_str708[sizeof("CSPC850MULTILINGUAL")];
    char stringpool_str713[sizeof("JIS_X0212.1990-0")];
    char stringpool_str714[sizeof("BIG5HKSCS")];
    char stringpool_str727[sizeof("BIG5-HKSCS")];
    char stringpool_str764[sizeof("MACGREEK")];
    char stringpool_str770[sizeof("MS-TURK")];
    char stringpool_str771[sizeof("MS-GREEK")];
    char stringpool_str791[sizeof("BIGFIVE")];
    char stringpool_str804[sizeof("BIG-FIVE")];
    char stringpool_str821[sizeof("MACTURKISH")];
    char stringpool_str843[sizeof("WINBALTRIM")];
    char stringpool_str844[sizeof("MACHEBREW")];
    char stringpool_str849[sizeof("CSEUCPKDFMTJAPANESE")];
  };
static const struct stringpool_t stringpool_contents =
  {
    "CN",
    "R8",
    "866",
    "862",
    "CP1361",
    "CP866",
    "CP1251",
    "CP1256",
    "CP1255",
    "CP1133",
    "ASCII",
    "CP862",
    "CP1253",
    "CHAR",
    "CP1252",
    "CP936",
    "CP1258",
    "C99",
    "CP932",
    "ISO-IR-6",
    "CP819",
    "ISO-IR-166",
    "ISO-IR-165",
    "ISO-IR-126",
    "ISO-IR-58",
    "ISO-IR-226",
    "ISO8859-1",
    "ISO8859-6",
    "ISO-IR-138",
    "ISO8859-5",
    "ISO8859-16",
    "ISO8859-15",
    "ISO8859-3",
    "ISO8859-2",
    "ISO8859-13",
    "ISO8859-8",
    "ISO-8859-1",
    "GB2312",
    "ISO-8859-6",
    "EUCCN",
    "ISO-8859-5",
    "ISO-8859-16",
    "ISO-IR-159",
    "ISO-8859-15",
    "ISO-8859-3",
    "ISO-8859-2",
    "ISO-8859-13",
    "ISO-8859-8",
    "ISO-IR-101",
    "850",
    "EUC-CN",
    "ISO8859-9",
    "ISO-IR-199",
    "CSASCII",
    "ISO646-CN",
    "CP850",
    "ISO-IR-203",
    "CP1250",
    "HZ",
    "ISO-8859-9",
    "CP950",
    "ISO-2022-CN",
    "ISO_8859-1",
    "CP949",
    "ISO_8859-6",
    "ISO-IR-148",
    "ISO_8859-5",
    "ISO_8859-16",
    "ISO-IR-109",
    "ISO_8859-15",
    "ISO_8859-3",
    "ISO_8859-16:2001",
    "ISO_8859-2",
    "ISO_8859-13",
    "ISO_8859-8",
    "ISO-IR-110",
    "ISO_8859-15:1998",
    "ISO-2022-CN-EXT",
    "CP1254",
    "ISO-IR-149",
    "L1",
    "L6",
    "L5",
    "L3",
    "L2",
    "L8",
    "ISO_8859-9",
    "ISO8859-10",
    "CSISO2022CN",
    "ISO-IR-179",
    "UHC",
    "ISO-IR-14",
    "ISO-8859-10",
    "CP367",
    "ISO_8859-10:1992",
    "ISO-IR-100",
    "LATIN1",
    "CP1257",
    "LATIN6",
    "ISO8859-4",
    "LATIN5",
    "TIS620",
    "ISO8859-14",
    "ELOT_928",
    "LATIN3",
    "SJIS",
    "LATIN2",
    "LATIN8",
    "ISO_8859-14:1998",
    "GEORGIAN-ACADEMY",
    "MAC",
    "ISO-8859-4",
    "TIS-620",
    "ISO-8859-14",
    "GB18030",
    "X0212",
    "L4",
    "ISO-IR-57",
    "ISO_8859-10",
    "IBM866",
    "ISO-IR-157",
    "ISO-IR-87",
    "ISO-IR-127",
    "US",
    "CP874",
    "IBM862",
    "MS936",
    "ISO8859-7",
    "L7",
    "LATIN-9",
    "ISO-IR-144",
    "L10",
    "X0201",
    "ROMAN8",
    "ISO-8859-7",
    "ISO_8859-4",
    "IBM819",
    "ARABIC",
    "ISO_8859-14",
    "GB_2312-80",
    "BIG5",
    "TIS620-0",
    "UCS-2",
    "X0208",
    "CSBIG5",
    "CSKOI8R",
    "GB_1988-80",
    "BIG-5",
    "KOI8-R",
    "IBM-CP1133",
    "JP",
    "US-ASCII",
    "CN-BIG5",
    "LATIN10",
    "CHINESE",
    "CSUNICODE11",
    "ISO-CELTIC",
    "CSGB2312",
    "ISO_8859-7",
    "CSISOLATIN1",
    "CSISOLATIN6",
    "CSISOLATIN5",
    "TIS620.2533-1",
    "MACCROATIAN",
    "CSISOLATIN3",
    "UNICODE-1-1",
    "CSISOLATIN2",
    "KOI8-T",
    "CSISOLATINCYRILLIC",
    "IBM850",
    "MS-ANSI",
    "TIS620.2529-1",
    "LATIN4",
    "GEORGIAN-PS",
    "EUCKR",
    "CSISOLATINARABIC",
    "ECMA-118",
    "UTF-16",
    "ARMSCII-8",
    "EUC-KR",
    "ISO-10646-UCS-2",
    "UTF-8",
    "KOREAN",
    "CYRILLIC",
    "UTF-32",
    "TIS620.2533-0",
    "CSUNICODE",
    "ISO_8859-5:1988",
    "ISO_8859-3:1988",
    "ISO_8859-8:1988",
    "LATIN7",
    "ISO-2022-KR",
    "KSC_5601",
    "MACTHAI",
    "CSUCS4",
    "UCS-4",
    "CSUNICODE11UTF7",
    "ISO_8859-9:1989",
    "CN-GB-ISOIR165",
    "EUCJP",
    "IBM367",
    "HP-ROMAN8",
    "ASMO-708",
    "ISO646-US",
    "ISO-10646-UCS-4",
    "UNICODE-1-1-UTF-7",
    "EUC-JP",
    "WCHAR_T",
    "EUCTW",
    "ISO-2022-JP-1",
    "CSHPROMAN8",
    "ISO646-JP",
    "CSISO2022KR",
    "TCVN",
    "ISO-2022-JP-2",
    "ISO_8859-4:1988",
    "EUC-TW",
    "CSISO58GB231280",
    "MS-EE",
    "ISO-2022-JP",
    "CSISOLATIN4",
    "CSPC862LATINHEBREW",
    "NEXTSTEP",
    "ISO_8859-1:1987",
    "ISO_8859-6:1987",
    "CSIBM866",
    "ISO_8859-2:1987",
    "HZ-GB-2312",
    "WINDOWS-1251",
    "WINDOWS-1256",
    "WINDOWS-1255",
    "ECMA-114",
    "WINDOWS-1253",
    "WINDOWS-1252",
    "WINDOWS-1258",
    "GREEK8",
    "MACROMAN",
    "JIS_C6226-1983",
    "CSISO2022JP2",
    "WINDOWS-936",
    "JIS0208",
    "VISCII",
    "CSISO57GB1988",
    "KS_C_5601-1989",
    "CSISO2022JP",
    "CSVISCII",
    "CN-GB",
    "MACARABIC",
    "WINDOWS-1250",
    "MACROMANIA",
    "CSKSC56011987",
    "JIS_C6220-1969-RO",
    "UTF-7",
    "CSEUCKR",
    "CSISO14JISC6220RO",
    "WINDOWS-1254",
    "CSISO159JISX02121990",
    "ISO_8859-7:1987",
    "MACICELAND",
    "WINDOWS-1257",
    "GBK",
    "KS_C_5601-1987",
    "TCVN5712-1",
    "TCVN-5712",
    "UCS-2-INTERNAL",
    "MACINTOSH",
    "UNICODELITTLE",
    "UCS-2LE",
    "ANSI_X3.4-1986",
    "MS-CYRL",
    "ANSI_X3.4-1968",
    "CSISOLATINHEBREW",
    "MACCYRILLIC",
    "CSMACINTOSH",
    "CSEUCTW",
    "UNICODEBIG",
    "UCS-2-SWAPPED",
    "CSISOLATINGREEK",
    "UCS-2BE",
    "KOI8-U",
    "UCS-4-INTERNAL",
    "VISCII1.1-1",
    "KOI8-RU",
    "UCS-4LE",
    "MS-HEBR",
    "EXTENDED_UNIX_CODE_PACKED_FORMAT_FOR_JAPANESE",
    "UTF-16LE",
    "MULELAO-1",
    "UTF-32LE",
    "MACCENTRALEUROPE",
    "UCS-4-SWAPPED",
    "WINDOWS-874",
    "ISO_646.IRV:1991",
    "UCS-4BE",
    "SHIFT-JIS",
    "JIS_X0212",
    "MS-ARAB",
    "GREEK",
    "UTF-16BE",
    "JISX0201-1976",
    "UTF-32BE",
    "JAVA",
    "JIS_X0201",
    "HEBREW",
    "SHIFT_JIS",
    "JIS_X0208",
    "CSISO87JISX0208",
    "JIS_X0212-1990",
    "JIS_X0208-1983",
    "TCVN5712-1:1993",
    "CSSHIFTJIS",
    "JIS_X0208-1990",
    "MACUKRAINE",
    "MS_KANJI",
    "CSHALFWIDTHKATAKANA",
    "JOHAB",
    "CSPC850MULTILINGUAL",
    "JIS_X0212.1990-0",
    "BIG5HKSCS",
    "BIG5-HKSCS",
    "MACGREEK",
    "MS-TURK",
    "MS-GREEK",
    "BIGFIVE",
    "BIG-FIVE",
    "MACTURKISH",
    "WINBALTRIM",
    "MACHEBREW",
    "CSEUCPKDFMTJAPANESE"
  };
#define stringpool ((const char *) &stringpool_contents)

static const struct alias aliases[] =
  {
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1},
#line 274 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str11, ei_iso646_cn},
    {-1}, {-1}, {-1},
#line 222 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str15, ei_hp_roman8},
    {-1},
#line 203 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str17, ei_cp866},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 199 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str25, ei_cp862},
#line 336 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str26, ei_johab},
#line 201 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str27, ei_cp866},
#line 170 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str28, ei_cp1251},
    {-1},
#line 185 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str30, ei_cp1256},
    {-1},
#line 182 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str32, ei_cp1255},
#line 230 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str33, ei_cp1133},
#line 13 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str34, ei_ascii},
#line 197 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str35, ei_cp862},
#line 176 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str36, ei_cp1253},
#line 339 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str37, ei_local_char},
#line 173 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str38, ei_cp1252},
#line 309 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str39, ei_ces_gbk},
#line 191 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str40, ei_cp1258},
    {-1},
#line 51 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str42, ei_c99},
    {-1}, {-1}, {-1}, {-1},
#line 297 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str47, ei_cp932},
    {-1},
#line 16 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str49, ei_ascii},
    {-1}, {-1}, {-1}, {-1},
#line 57 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str54, ei_iso8859_1},
    {-1},
#line 238 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str56, ei_tis620},
    {-1},
#line 280 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str58, ei_isoir165},
    {-1},
#line 106 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str60, ei_iso8859_7},
    {-1}, {-1}, {-1},
#line 277 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str64, ei_gb2312},
#line 159 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str65, ei_iso8859_16},
#line 62 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str66, ei_iso8859_1},
    {-1},
#line 102 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str68, ei_iso8859_6},
#line 116 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str69, ei_iso8859_8},
#line 93 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str70, ei_iso8859_5},
#line 162 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str71, ei_iso8859_16},
    {-1},
#line 155 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str73, ei_iso8859_15},
#line 78 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str74, ei_iso8859_3},
    {-1},
#line 70 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str76, ei_iso8859_2},
#line 141 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str77, ei_iso8859_13},
#line 119 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str78, ei_iso8859_8},
#line 53 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str79, ei_iso8859_1},
#line 305 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str80, ei_euc_cn},
#line 94 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str81, ei_iso8859_6},
#line 304 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str82, ei_euc_cn},
#line 87 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str83, ei_iso8859_5},
#line 156 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str84, ei_iso8859_16},
#line 269 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str85, ei_jisx0212},
#line 150 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str86, ei_iso8859_15},
#line 71 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str87, ei_iso8859_3},
    {-1},
#line 63 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str89, ei_iso8859_2},
#line 136 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str90, ei_iso8859_13},
#line 113 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str91, ei_iso8859_8},
#line 66 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str92, ei_iso8859_2},
#line 195 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str93, ei_cp850},
    {-1},
#line 303 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str95, ei_euc_cn},
#line 127 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str96, ei_iso8859_9},
    {-1},
#line 145 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str98, ei_iso8859_14},
#line 22 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str99, ei_ascii},
#line 272 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str100, ei_iso646_cn},
    {-1}, {-1}, {-1},
#line 193 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str104, ei_cp850},
#line 153 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str105, ei_iso8859_15},
#line 167 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str106, ei_cp1250},
#line 316 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str107, ei_hz},
    {-1},
#line 120 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str109, ei_iso8859_9},
    {-1}, {-1}, {-1},
#line 327 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str113, ei_cp950},
#line 313 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str114, ei_iso2022_cn},
    {-1},
#line 54 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str116, ei_iso8859_1},
#line 333 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str117, ei_cp949},
#line 95 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str118, ei_iso8859_6},
#line 123 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str119, ei_iso8859_9},
#line 88 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str120, ei_iso8859_5},
#line 157 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str121, ei_iso8859_16},
#line 74 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str122, ei_iso8859_3},
#line 151 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str123, ei_iso8859_15},
#line 72 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str124, ei_iso8859_3},
#line 158 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str125, ei_iso8859_16},
#line 64 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str126, ei_iso8859_2},
#line 137 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str127, ei_iso8859_13},
#line 114 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str128, ei_iso8859_8},
    {-1}, {-1},
#line 82 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str131, ei_iso8859_4},
#line 152 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str132, ei_iso8859_15},
    {-1},
#line 315 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str134, ei_iso2022_cn_ext},
    {-1},
#line 179 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str136, ei_cp1254},
#line 285 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str137, ei_ksc5601},
    {-1},
#line 60 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str139, ei_iso8859_1},
#line 133 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str140, ei_iso8859_10},
#line 125 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str141, ei_iso8859_9},
    {-1},
#line 76 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str143, ei_iso8859_3},
#line 68 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str144, ei_iso8859_2},
#line 147 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str145, ei_iso8859_14},
#line 121 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str146, ei_iso8859_9},
#line 135 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str147, ei_iso8859_10},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 314 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str153, ei_iso2022_cn},
    {-1},
#line 138 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str155, ei_iso8859_13},
#line 334 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str156, ei_cp949},
    {-1},
#line 250 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str158, ei_iso646_jp},
    {-1},
#line 128 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str160, ei_iso8859_10},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 19 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str167, ei_ascii},
#line 130 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str168, ei_iso8859_10},
    {-1},
#line 56 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str170, ei_iso8859_1},
#line 59 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str171, ei_iso8859_1},
#line 188 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str172, ei_cp1257},
#line 132 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str173, ei_iso8859_10},
#line 86 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str174, ei_iso8859_4},
#line 124 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str175, ei_iso8859_9},
#line 233 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str176, ei_tis620},
#line 149 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str177, ei_iso8859_14},
#line 108 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str178, ei_iso8859_7},
#line 75 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str179, ei_iso8859_3},
#line 294 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str180, ei_sjis},
#line 67 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str181, ei_iso8859_2},
    {-1},
#line 146 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str183, ei_iso8859_14},
#line 144 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str184, ei_iso8859_14},
#line 226 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str185, ei_georgian_academy},
#line 207 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str186, ei_mac_roman},
#line 79 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str187, ei_iso8859_4},
    {-1},
#line 232 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str189, ei_tis620},
#line 142 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str190, ei_iso8859_14},
#line 312 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str191, ei_gb18030},
#line 268 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str192, ei_jisx0212},
#line 84 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str193, ei_iso8859_4},
    {-1}, {-1},
#line 273 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str196, ei_iso646_cn},
#line 129 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str197, ei_iso8859_10},
#line 202 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str198, ei_cp866},
#line 131 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str199, ei_iso8859_10},
#line 262 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str200, ei_jisx0208},
    {-1},
#line 97 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str202, ei_iso8859_6},
#line 21 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str203, ei_ascii},
#line 239 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str204, ei_cp874},
    {-1},
#line 198 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str206, ei_cp862},
#line 310 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str207, ei_ces_gbk},
    {-1}, {-1},
#line 112 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str210, ei_iso8859_7},
#line 140 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str211, ei_iso8859_13},
    {-1}, {-1},
#line 154 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str214, ei_iso8859_15},
#line 90 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str215, ei_iso8859_5},
    {-1}, {-1}, {-1}, {-1},
#line 161 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str220, ei_iso8859_16},
#line 255 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str221, ei_jisx0201},
#line 221 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str222, ei_hp_roman8},
#line 103 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str223, ei_iso8859_7},
#line 80 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str224, ei_iso8859_4},
#line 58 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str225, ei_iso8859_1},
#line 100 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str226, ei_iso8859_6},
#line 143 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str227, ei_iso8859_14},
#line 276 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str228, ei_gb2312},
#line 321 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str229, ei_ces_big5},
    {-1},
#line 234 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str231, ei_tis620},
#line 24 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str232, ei_ucs2},
#line 261 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str233, ei_jisx0208},
    {-1}, {-1}, {-1}, {-1},
#line 326 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str238, ei_ces_big5},
#line 164 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str239, ei_koi8_r},
    {-1},
#line 271 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str241, ei_iso646_cn},
#line 322 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str242, ei_ces_big5},
#line 163 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str243, ei_koi8_r},
#line 231 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str244, ei_cp1133},
    {-1}, {-1}, {-1}, {-1},
#line 251 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str249, ei_iso646_jp},
#line 12 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str250, ei_ascii},
#line 325 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str251, ei_ces_big5},
#line 160 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str252, ei_iso8859_16},
#line 279 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str253, ei_gb2312},
    {-1},
#line 30 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str255, ei_ucs2be},
    {-1},
#line 148 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str257, ei_iso8859_14},
    {-1},
#line 307 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str259, ei_euc_cn},
#line 104 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str260, ei_iso8859_7},
#line 61 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str261, ei_iso8859_1},
    {-1},
#line 134 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str263, ei_iso8859_10},
    {-1},
#line 126 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str265, ei_iso8859_9},
#line 237 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str266, ei_tis620},
#line 211 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str267, ei_mac_croatian},
    {-1},
#line 77 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str269, ei_iso8859_3},
#line 29 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str270, ei_ucs2be},
#line 69 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str271, ei_iso8859_2},
    {-1},
#line 228 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str273, ei_koi8_t},
#line 92 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str274, ei_iso8859_5},
#line 194 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str275, ei_cp850},
#line 175 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str276, ei_cp1252},
    {-1},
#line 235 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str278, ei_tis620},
#line 83 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str279, ei_iso8859_4},
#line 227 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str280, ei_georgian_ps},
    {-1}, {-1}, {-1},
#line 331 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str284, ei_euc_kr},
#line 101 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str285, ei_iso8859_6},
    {-1}, {-1}, {-1}, {-1},
#line 107 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str290, ei_iso8859_7},
    {-1},
#line 38 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str292, ei_utf16},
    {-1}, {-1},
#line 225 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str295, ei_armscii_8},
    {-1},
#line 330 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str297, ei_euc_kr},
#line 25 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str298, ei_ucs2},
#line 23 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str299, ei_utf8},
    {-1},
#line 287 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str301, ei_ksc5601},
#line 91 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str302, ei_iso8859_5},
    {-1},
#line 41 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str304, ei_utf32},
#line 236 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str305, ei_tis620},
#line 26 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str306, ei_ucs2},
    {-1}, {-1}, {-1},
#line 89 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str310, ei_iso8859_5},
    {-1},
#line 73 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str312, ei_iso8859_3},
    {-1},
#line 115 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str314, ei_iso8859_8},
#line 139 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str315, ei_iso8859_13},
#line 337 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str316, ei_iso2022_kr},
    {-1}, {-1},
#line 282 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str319, ei_ksc5601},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 219 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str327, ei_mac_thai},
    {-1},
#line 35 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str329, ei_ucs4},
#line 33 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str330, ei_ucs4},
#line 46 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str331, ei_utf7},
#line 122 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str332, ei_iso8859_9},
#line 281 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str333, ei_isoir165},
    {-1}, {-1},
#line 289 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str336, ei_euc_jp},
    {-1},
#line 20 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str338, ei_ascii},
#line 220 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str339, ei_hp_roman8},
    {-1}, {-1}, {-1}, {-1},
#line 99 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str344, ei_iso8859_6},
    {-1},
#line 14 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str346, ei_ascii},
#line 34 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str347, ei_ucs4},
#line 45 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str348, ei_utf7},
#line 288 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str349, ei_euc_jp},
#line 340 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str350, ei_local_wchar_t},
#line 319 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str351, ei_euc_tw},
#line 300 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str352, ei_iso2022_jp1},
#line 223 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str353, ei_hp_roman8},
#line 249 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str354, ei_iso646_jp},
#line 338 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str355, ei_iso2022_kr},
#line 244 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str356, ei_tcvn},
#line 301 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str357, ei_iso2022_jp2},
    {-1}, {-1}, {-1}, {-1},
#line 81 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str362, ei_iso8859_4},
    {-1},
#line 318 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str364, ei_euc_tw},
#line 278 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str365, ei_gb2312},
    {-1},
#line 169 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str367, ei_cp1250},
#line 298 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str368, ei_iso2022_jp},
#line 85 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str369, ei_iso8859_4},
    {-1}, {-1},
#line 200 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str372, ei_cp862},
#line 224 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str373, ei_nextstep},
#line 55 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str374, ei_iso8859_1},
#line 96 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str375, ei_iso8859_6},
    {-1},
#line 204 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str377, ei_cp866},
    {-1},
#line 65 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str379, ei_iso8859_2},
#line 317 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str380, ei_hz},
    {-1}, {-1},
#line 171 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str383, ei_cp1251},
#line 186 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str384, ei_cp1256},
#line 183 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str385, ei_cp1255},
#line 98 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str386, ei_iso8859_6},
#line 177 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str387, ei_cp1253},
#line 174 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str388, ei_cp1252},
#line 192 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str389, ei_cp1258},
#line 109 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str390, ei_iso8859_7},
    {-1},
#line 205 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str392, ei_mac_roman},
#line 263 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str393, ei_jisx0208},
    {-1},
#line 302 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str395, ei_iso2022_jp2},
#line 311 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str396, ei_ces_gbk},
#line 260 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str397, ei_jisx0208},
    {-1},
#line 241 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str399, ei_viscii},
    {-1}, {-1},
#line 275 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str402, ei_iso646_cn},
#line 284 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str403, ei_ksc5601},
    {-1}, {-1}, {-1},
#line 299 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str407, ei_iso2022_jp},
#line 243 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str408, ei_viscii},
    {-1}, {-1},
#line 306 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str411, ei_euc_cn},
#line 218 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str412, ei_mac_arabic},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 168 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str422, ei_cp1250},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 212 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str428, ei_mac_romania},
#line 286 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str429, ei_ksc5601},
#line 248 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str430, ei_iso646_jp},
#line 44 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str431, ei_utf7},
    {-1}, {-1},
#line 332 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str434, ei_euc_kr},
    {-1},
#line 252 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str436, ei_iso646_jp},
#line 180 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str437, ei_cp1254},
#line 270 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str438, ei_jisx0212},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 105 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str446, ei_iso8859_7},
#line 210 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str447, ei_mac_iceland},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 189 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str455, ei_cp1257},
    {-1}, {-1},
#line 308 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str458, ei_ces_gbk},
    {-1},
#line 283 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str460, ei_ksc5601},
#line 246 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str461, ei_tcvn},
    {-1},
#line 245 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str463, ei_tcvn},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 47 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str471, ei_ucs2internal},
    {-1},
#line 206 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str473, ei_mac_roman},
    {-1}, {-1}, {-1}, {-1},
#line 32 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str478, ei_ucs2le},
    {-1},
#line 31 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str480, ei_ucs2le},
    {-1}, {-1},
#line 18 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str483, ei_ascii},
    {-1},
#line 172 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str485, ei_cp1251},
    {-1}, {-1},
#line 17 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str488, ei_ascii},
    {-1}, {-1}, {-1}, {-1},
#line 118 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str493, ei_iso8859_8},
    {-1}, {-1},
#line 213 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str496, ei_mac_cyrillic},
    {-1},
#line 208 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str498, ei_mac_roman},
    {-1}, {-1},
#line 320 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str501, ei_euc_tw},
    {-1},
#line 28 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str503, ei_ucs2be},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 48 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str510, ei_ucs2swapped},
#line 111 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str511, ei_iso8859_7},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 27 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str517, ei_ucs2be},
    {-1},
#line 165 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str519, ei_koi8_u},
#line 49 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str520, ei_ucs4internal},
#line 242 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str521, ei_viscii},
    {-1}, {-1}, {-1},
#line 166 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str525, ei_koi8_ru},
    {-1}, {-1}, {-1},
#line 37 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str529, ei_ucs4le},
    {-1}, {-1}, {-1},
#line 184 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str533, ei_cp1255},
    {-1}, {-1}, {-1},
#line 290 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str537, ei_euc_jp},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 40 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str544, ei_utf16le},
    {-1}, {-1},
#line 229 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str547, ei_mulelao},
    {-1}, {-1}, {-1}, {-1},
#line 43 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str552, ei_utf32le},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 209 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str558, ei_mac_centraleurope},
#line 50 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str559, ei_ucs4swapped},
    {-1},
#line 240 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str561, ei_cp874},
    {-1},
#line 15 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str563, ei_ascii},
    {-1}, {-1},
#line 36 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str566, ei_ucs4be},
    {-1}, {-1},
#line 293 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str569, ei_sjis},
    {-1},
#line 265 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str571, ei_jisx0212},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 187 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str577, ei_cp1256},
#line 110 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str578, ei_iso8859_7},
    {-1}, {-1},
#line 39 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str581, ei_utf16be},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 254 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str587, ei_jisx0201},
    {-1},
#line 42 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str589, ei_utf32be},
    {-1},
#line 52 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str591, ei_java},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 253 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str600, ei_jisx0201},
    {-1}, {-1}, {-1},
#line 117 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str604, ei_iso8859_8},
    {-1},
#line 292 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str606, ei_sjis},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 257 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str612, ei_jisx0208},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1},
#line 264 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str623, ei_jisx0208},
#line 267 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str624, ei_jisx0212},
    {-1}, {-1}, {-1}, {-1},
#line 258 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str629, ei_jisx0208},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1},
#line 247 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str651, ei_tcvn},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1},
#line 296 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str663, ei_sjis},
#line 259 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str664, ei_jisx0208},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 214 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str683, ei_mac_ukraine},
    {-1}, {-1}, {-1}, {-1},
#line 295 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str688, ei_sjis},
#line 256 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str689, ei_jisx0201},
    {-1}, {-1}, {-1}, {-1},
#line 335 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str694, ei_johab},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1},
#line 196 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str708, ei_cp850},
    {-1}, {-1}, {-1}, {-1},
#line 266 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str713, ei_jisx0212},
#line 329 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str714, ei_big5hkscs},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1},
#line 328 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str727, ei_big5hkscs},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 215 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str764, ei_mac_greek},
    {-1}, {-1}, {-1}, {-1}, {-1},
#line 181 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str770, ei_cp1254},
#line 178 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str771, ei_cp1253},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1},
#line 324 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str791, ei_ces_big5},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1},
#line 323 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str804, ei_ces_big5},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
#line 216 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str821, ei_mac_turkish},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1}, {-1},
    {-1}, {-1}, {-1},
#line 190 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str843, ei_cp1257},
#line 217 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str844, ei_mac_hebrew},
    {-1}, {-1}, {-1}, {-1},
#line 291 "lib/aliases.gperf"
    {(int)(long)&((struct stringpool_t *)0)->stringpool_str849, ei_euc_jp}
  };

#ifdef __GNUC__
__inline
#endif
const struct alias *
aliases_lookup (register const char *str, register unsigned int len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = aliases_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register int o = aliases[key].name;
          if (o >= 0)
            {
              register const char *s = o + stringpool;

              if (*str == *s && !strcmp (str + 1, s + 1))
                return &aliases[key];
            }
        }
    }
  return 0;
}
