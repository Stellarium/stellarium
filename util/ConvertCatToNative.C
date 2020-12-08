// Author and Copyright: Johannes Gajdosik, 2007
// License: GNU GPLv2 or later
// g++ -O2 ConvertCatToNative.C -o ConvertCatToNative

// If you use a compiler different from gcc - which I discourage -
// and want to use mmap star catalogue loading,
// you can use this program for converting stellarium star catalogoues
// into native format. Your resulting native format catalogue is not
// portable,  but is has the structs packed in a way that your compiler
// understands. And the stellarium binary, that you have build with your
// non-gcc compiler, will be able to load it.


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <iostream>
using namespace std;


typedef int Int32;
typedef unsigned int Uint32;
typedef short int Int16;
typedef unsigned short int Uint16;


#if (defined(__sgi) && defined(_COMPILER_VERSION) && !defined(__GNUC__))
#pragma pack(1)
#endif
struct Star1 { // 28 byte
  int hip:24;                  // 17 bits needed
  unsigned char component_ids; //  5 bits needed
  Int32 x0;                    // 32 bits needed
  Int32 x1;                    // 32 bits needed
  unsigned char b_v;           //  7 bits needed
  unsigned char mag;           //  8 bits needed
  Uint16 sp_int;               // 14 bits needed
  Int32 dx0,dx1,plx;
  void repack(bool from_be);
  void print(void);
}
#if defined(__GNUC__)
   __attribute__ ((__packed__))
#endif
;
#if (defined(__sgi) && defined(_COMPILER_VERSION) && !defined(__GNUC__))
#pragma pack(0)
#endif


#if (defined(__sgi) && defined(_COMPILER_VERSION) && !defined(__GNUC__))
#pragma pack(1)
#endif
struct Star2 {  // 10 byte
  int x0:20;
  int x1:20;
  int dx0:14;
  int dx1:14;
  unsigned int b_v:7;
  unsigned int mag:5;
  void repack(bool from_be);
  void print(void);
}
#if defined(__GNUC__)
   __attribute__ ((__packed__))
#endif
;
#if (defined(__sgi) && defined(_COMPILER_VERSION) && !defined(__GNUC__))
#pragma pack(0)
#endif


#if (defined(__sgi) && defined(_COMPILER_VERSION) && !defined(__GNUC__))
#pragma pack(1)
#endif
struct Star3 {  // 6 byte
  int x0:18;
  int x1:18;
  unsigned int b_v:7;
  unsigned int mag:5;
  void repack(bool from_be);
  void print(void);
}
#if defined(__GNUC__)
   __attribute__ ((__packed__))
#endif
;
#if (defined(__sgi) && defined(_COMPILER_VERSION) && !defined(__GNUC__))
#pragma pack(0)
#endif


static unsigned int bswap_32(unsigned int val) {
  return ((val & 0xff000000) >> 24) |
         ((val & 0x00ff0000) >>  8) |
         ((val & 0x0000ff00) <<  8) |
         ((val & 0x000000ff) << 24);
}


static
int UnpackBits(bool from_be,const char *addr,int bits_begin,
               const int bits_size) {
  assert(bits_size <= 32);
  while (bits_begin >= 8) {
    bits_begin -= 8;
    addr++;
  }
  const int bits_end = bits_begin + bits_size;
  int rval;
  if (from_be) {
    rval = (int)((( (( (((unsigned int)(unsigned char)(addr[0]))  << 8) |
                        ((unsigned int)(unsigned char)(addr[1]))) << 8) |
                        ((unsigned int)(unsigned char)(addr[2]))) << 8) |
                        ((unsigned int)(unsigned char)(addr[3])));
    if (bits_end <= 32) {
      if (bits_begin > 0) rval <<= bits_begin;
    } else {
      rval <<= bits_begin;
      unsigned int rval_lo = (unsigned char)(addr[4]);
      rval_lo >>= (8-bits_begin);
      rval |= rval_lo;
    }
    if (bits_size < 32) rval >>= (32-bits_size);
  } else {
    rval = (int)((( (( (((unsigned int)(unsigned char)(addr[3]))  << 8) |
                        ((unsigned int)(unsigned char)(addr[2]))) << 8) |
                        ((unsigned int)(unsigned char)(addr[1]))) << 8) |
                        ((unsigned int)(unsigned char)(addr[0])));
    if (bits_end <= 32) {
      if (bits_end < 32) rval <<= (32-bits_end);
      if (bits_size < 32) rval >>= (32-bits_size);
    } else {
      int rval_hi = addr[4];
      rval_hi <<= (64-bits_end);
      rval_hi >>= (32-bits_size);
      rval = ((unsigned int)rval) >> bits_begin;
      rval |= rval_hi;
    }
  }
  return rval;
}



static
unsigned int UnpackUBits(bool from_be,const char *addr,int bits_begin,
                         const int bits_size) {
  assert(bits_size <= 32);
  while (bits_begin >= 8) {
    bits_begin -= 8;
    addr++;
  }
  const int bits_end = bits_begin + bits_size;
  unsigned int rval;
  if (from_be) {
    rval = (( (( (((unsigned int)(unsigned char)(addr[0]))  << 8) |
                  ((unsigned int)(unsigned char)(addr[1]))) << 8) |
                  ((unsigned int)(unsigned char)(addr[2]))) << 8) |
                  ((unsigned int)(unsigned char)(addr[3]));
    if (bits_end <= 32) {
      if (bits_begin > 0) rval <<= bits_begin;
    } else {
      rval <<= bits_begin;
      unsigned int rval_lo = (unsigned char)(addr[4]);
      rval_lo >>= (8-bits_begin);
      rval |= rval_lo;
    }
    if (bits_size < 32) rval >>= (32-bits_size);
  } else {
    rval = (( (( (((unsigned int)(unsigned char)(addr[3]))  << 8) |
                  ((unsigned int)(unsigned char)(addr[2]))) << 8) |
                  ((unsigned int)(unsigned char)(addr[1]))) << 8) |
                  ((unsigned int)(unsigned char)(addr[0]));
    if (bits_end <= 32) {
      if (bits_begin > 0) rval >>= bits_begin;
    } else {
      unsigned int rval_hi = (unsigned char)(addr[4]);
      rval_hi <<= (32-bits_begin);
      rval = rval >> bits_begin;
      rval |= rval_hi;
    }
    if (bits_size < 32) rval &= ((((unsigned int)1)<<bits_size)-1);
  }
  return rval;
}





void Star1::repack(bool from_be) {
  const int _hip  = UnpackBits(from_be,(const char*)this, 0,24);
  const unsigned int _cids = UnpackUBits(from_be,(const char*)this,24, 8);
  const int _x0  = UnpackBits(from_be,(const char*)this,32,32);
  const int _x1  = UnpackBits(from_be,(const char*)this,64,32);
  const unsigned int _b_v = UnpackUBits(from_be,(const char*)this, 96, 8);
  const unsigned int _mag = UnpackUBits(from_be,(const char*)this,104, 8);
  const unsigned int _sp_int = UnpackUBits(from_be,(const char*)this,112,16);
  const int _dx0 = UnpackBits(from_be,(const char*)this,128,32);
  const int _dx1 = UnpackBits(from_be,(const char*)this,160,32);
  const int _plx = UnpackBits(from_be,(const char*)this,192,32);
//assert(hip == _hip);
//assert(component_ids == _cids);
//assert(x0 == _x0);
//assert(x1 == _x1);
//assert(b_v == _b_v);
//assert(mag == _mag);
//assert(sp_int == _sp_int);
//assert(dx0 == _dx0);
//assert(dx1 == _dx1);
//assert(plx == _plx);
  hip = _hip;
  component_ids = _cids;
  x0 = _x0;
  x1 = _x1;
  b_v = _b_v;
  mag = _mag;
  sp_int = _sp_int;
  dx0 = _dx0;
  dx1 = _dx1;
  plx = _plx;
}

void Star1::print(void) {
  cout << "hip: " << hip
       << ", component_ids: " << ((unsigned int)component_ids)
       << ", x0: " << x0
       << ", x1: " << x1
       << ", b_v: " << ((unsigned int)b_v)
       << ", mag: " << ((unsigned int)mag)
       << ", sp_int: " << sp_int
       << ", dx0: " << dx0
       << ", dx1: " << dx1
       << ", plx: " << plx
       << endl;
}


void Star2::repack(bool from_be) {
  const int _x0  = UnpackBits(from_be,(const char*)this, 0,20);
  const int _x1  = UnpackBits(from_be,(const char*)this,20,20);
  const int _dx0 = UnpackBits(from_be,(const char*)this,40,14);
  const int _dx1 = UnpackBits(from_be,(const char*)this,54,14);
  const unsigned int _b_v = UnpackUBits(from_be,(const char*)this,68, 7);
  const unsigned int _mag = UnpackUBits(from_be,(const char*)this,75, 5);
//assert(x0 == _x0);
//assert(x1 == _x1);
//assert(dx0 == _dx0);
//assert(dx1 == _dx1);
//assert(b_v == _b_v);
//assert(mag == _mag);
  x0 = _x0;
  x1 = _x1;
  dx0 = _dx0;
  dx1 = _dx1;
  b_v = _b_v;
  mag = _mag;
}

void Star2::print(void) {
  cout << "x0: " << x0
       << ", x1: " << x1
       << ", dx0: " << dx0
       << ", dx1: " << dx1
       << ", b_v: " << b_v
       << ", mag: " << mag
       << endl;
}


void Star3::repack(bool from_be) {
  const int _x0  = UnpackBits(from_be,(const char*)this, 0,18);
  const int _x1  = UnpackBits(from_be,(const char*)this,18,18);
  const unsigned int _b_v = UnpackUBits(from_be,(const char*)this,36, 7);
  const unsigned int _mag = UnpackUBits(from_be,(const char*)this,43, 5);
//assert(x0 == _x0);
//assert(x1 == _x1);
//assert(b_v == _b_v);
//assert(mag == _mag);
  x0 = _x0;
  x1 = _x1;
  b_v = _b_v;
  mag = _mag;
}

void Star3::print(void) {
  cout << "x0: " << x0
       << ", x1: " << x1
       << ", b_v: " << b_v
       << ", mag: " << mag
       << endl;
}






#define FILE_MAGIC 0x835f040a
#define FILE_MAGIC_OTHER_ENDIAN 0x0a045f83
#define FILE_MAGIC_NATIVE 0x835f040b
#define MAX_MAJOR_FILE_VERSION 0

template <class Star>
void Convert(FILE *f_in,FILE *f_out,bool from_be,unsigned int nr_of_stars) {
  for (;nr_of_stars>0;nr_of_stars--) {
    Star s;
    if (sizeof(Star) != fread(&s,1,sizeof(Star),f_in)) {
      printf("Convert: read failed\n");
      exit(-1);
    }
    s.repack(from_be);
    if (sizeof(Star) != fwrite(&s,1,sizeof(Star),f_out)) {
      printf("Convert: write failed\n");
      exit(-1);
    }
  }
}

static inline
int ReadInt(FILE *f,unsigned int &x) {
  const int rval = (4 == fread(&x,1,4,f)) ? 0 : -1;
  return rval;
}

static
void WriteInt(FILE *f,int x) {
  if (4!=fwrite(&x,1,4,f)) {
    printf("WriteInt: fwrite failed\n");
    exit(-1);
  }
}

static
void PerformConversion(const char *fname_in,const char *fname_out) {
  FILE *f_in = fopen(fname_in,"rb");
  FILE *f_out = fopen(fname_out,"wb");
  if (f_in == 0) {
    fprintf(stderr,"fopen(%s) failed\n",fname_in);
    return;
  }
  if (f_out == 0) {
    fprintf(stderr,"fopen(%s) failed\n",fname_out);
    return;
  }
  printf("Reading %s: ",fname_in);
  unsigned int magic,major,minor,type,level,mag_min,mag_range,mag_steps;
  if (ReadInt(f_in,magic) < 0 ||
      ReadInt(f_in,type) < 0 ||
      ReadInt(f_in,major) < 0 ||
      ReadInt(f_in,minor) < 0 ||
      ReadInt(f_in,level) < 0 ||
      ReadInt(f_in,mag_min) < 0 ||
      ReadInt(f_in,mag_range) < 0 ||
      ReadInt(f_in,mag_steps) < 0) {
    printf("bad file\n");
    return;
  }
  const bool byte_swap = (magic == FILE_MAGIC_OTHER_ENDIAN);
  if (byte_swap) {
      // ok, FILE_MAGIC_OTHER_ENDIAN, must swap
    printf("byteswap ");
    type = bswap_32(type);
    major = bswap_32(major);
    minor = bswap_32(minor);
    level = bswap_32(level);
    mag_min = bswap_32(mag_min);
    mag_range = bswap_32(mag_range);
    mag_steps = bswap_32(mag_steps);
  } else if (magic == FILE_MAGIC) {
      // ok, FILE_MAGIC
  } else {
    printf("no .cat or .bcat star catalogue file\n");
    return;
  }
  const bool from_be =
#ifdef WORDS_BIGENDIAN
  // need for byte_swap on a BE machine means that catalog is LE
                !byte_swap
#else
  // need for byte_swap on a LE machine means that catalog is BE
                byte_swap
#endif
  ;
  printf("type: %u major: %u minor: %u level: %u"
         " mag_min: %d mag_range: %u mag_steps: %u; ",
         type,major,minor,level,(int)mag_min,mag_range,mag_steps);
  if (major > MAX_MAJOR_FILE_VERSION) {
    printf("unsupported version\n");
    return;
  }
  WriteInt(f_out,FILE_MAGIC_NATIVE);
  WriteInt(f_out,type);
  WriteInt(f_out,major);
  WriteInt(f_out,minor);
  WriteInt(f_out,level);
  WriteInt(f_out,mag_min);
  WriteInt(f_out,mag_range);
  WriteInt(f_out,mag_steps);

  const unsigned int nr_of_zones = (20<<(level<<1)); // 20*4^level
  unsigned int nr_of_stars = 0;
  for (unsigned int i=0;i<nr_of_zones;i++) {
    unsigned int x;
    ReadInt(f_in,x);
    if (byte_swap) x = bswap_32(x);
    WriteInt(f_out,x);
    nr_of_stars += x;
  }

  switch (type) {
    default:
      printf("bad file type\n");
      return;
    case 0:
        assert(sizeof(Star1) == 28);
        Convert<Star1>(f_in,f_out,from_be,nr_of_stars);
      break;
    case 1:
        assert(sizeof(Star2) == 10);
        Convert<Star2>(f_in,f_out,from_be,nr_of_stars);
      break;
    case 2:
        assert(sizeof(Star3) == 6);
        Convert<Star3>(f_in,f_out,from_be,nr_of_stars);
      break;
  }
  fclose(f_out);
  fclose(f_in);
  printf("conversion successful\n");
}

int main(int argc,char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s input_catalogue_file output_catalogue_file\n",argv[0]);
    return 1;
  }
  PerformConversion(argv[1],argv[2]);
  return 0;
}

