// Author and Copyright: Johannes Gajdosik, 2007
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
//

/*
g++ -O2 ParseNomad.C -o ParseNomad
*/

#undef NDEBUG

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

const char *nomad_path = "/sda1/nomad1";

struct Short_NOMAD_Record {
  int ra,spd,pm_ra,pm_spd;
  short int b,v,r;
  unsigned short int flags;
};

#define WRITE_SIZE 10000

#define FILE_SIZE_LIMIT 1160000000

class Accumulator {
public:
  Accumulator(void);
  ~Accumulator(void);
  void add(int ra,int spd,int pm_ra,int pm_spd,
           short int b,short int v,short int r,
           unsigned short int flags);
private:
  void flush(bool close_in_any_case);
  Short_NOMAD_Record *buff;
  Short_NOMAD_Record *p;
  int bytes_written; // in current file
  int files_written;
  FILE *f;
};

Accumulator::Accumulator(void) {
  buff = new Short_NOMAD_Record[WRITE_SIZE];
  assert(buff);
  p = buff;
  bytes_written = 0;
  files_written = 0;
  f = 0;
}

Accumulator::~Accumulator(void) {
  flush(true);
  delete buff;
}

void Accumulator::add(int ra,int spd,int pm_ra,int pm_spd,
                      short int b,short int v,short int r,
                      unsigned short int flags) {
  p->ra = ra;
  p->spd = spd;
  p->pm_ra = pm_ra;
  p->pm_spd = pm_spd;
  p->b = b;
  p->v = v;
  p->r = r;
  p->flags = flags;
  p++;
  if (p >= buff+WRITE_SIZE) flush(false);
}

void Accumulator::flush(bool close_in_any_case) {
  if (f == 0) {
    char fname[1024];
    snprintf(fname,sizeof(fname),"Nomad%02d.sml",files_written);
    f = fopen(fname,"wb");
    if (f == 0) {
      fprintf(stderr,"could not open %s for writing\n",fname);
      exit(1);
    }
  }
  if (p > buff) {
    const int rc = fwrite(buff,sizeof(Short_NOMAD_Record),p-buff,f);
    if (rc != p-buff) {
      fprintf(stderr,"fwrite failed\n");
      exit(1);
    }
    bytes_written += (p-buff)*sizeof(Short_NOMAD_Record);
    p = buff;
  }
  if (bytes_written >= FILE_SIZE_LIMIT || close_in_any_case) {
    fclose(f);
    f = 0;
    bytes_written = 0;
    files_written++;
  }
}



const unsigned short int SHORT_ASTSRCBIT0 = 0x0001; // Astrometry source bit 0
const unsigned short int SHORT_ASTSRCBIT1 = 0x0002; // Astrometry source bit 1
const unsigned short int SHORT_ASTSRCBIT2 = 0x0004; // Astrometry source bit 2
const unsigned short int SHORT_UBBIT   = 0x0008;
const unsigned short int SHORT_TMBIT   = 0x0010;
const unsigned short int SHORT_XRBIT   = 0x0020;
const unsigned short int SHORT_IUCBIT  = 0x0040;
const unsigned short int SHORT_ITYBIT  = 0x0080;
const unsigned short int SHORT_OMAGBIT = 0x0100;
const unsigned short int SHORT_EMAGBIT = 0x0200;
const unsigned short int SHORT_TMONLY  = 0x0400;
const unsigned short int SHORT_SPIKE   = 0x0800;
const unsigned short int SHORT_TYCONF  = 0x1000;
const unsigned short int SHORT_BSCONF  = 0x2000;
const unsigned short int SHORT_BSART   = 0x4000;
const unsigned short int SHORT_USEME   = 0x8000;

struct NOMAD_Record {
  int ra,spd,dev_ra,dev_spd;
  int pm_ra,pm_spd,dev_pm_ra,dev_pm_spd;
  int epoch_ra,epoch_spd;
  int b,v,r,j,h,k;
  int usno_id,_two_mass_id,yb6_id,ucac2_id,tycho2_id;
  int flags;
};

#define READ_SIZE 100000
static NOMAD_Record buff[READ_SIZE];
const int ASTSRCBIT0 = 0x00000001; // Astrometry source bit 0
const int ASTSRCBIT1 = 0x00000002; // Astrometry source bit 1
const int ASTSRCBIT2 = 0x00000004; // Astrometry source bit 2

const int UBBIT   = 0x00001000; // 469314198  UBBIT   Fails Blaise's test for USNO-B1.0 star
const int TMBIT   = 0x00002000; // 123508070  TMBIT   Fails Roc's test for clean 2MASS star
const int YBBIT   = 0x00004000; //         0  YBBIT   (unused)
const int UCBIT   = 0x00008000; //         0  UCBIT   (unused)
const int TYBIT   = 0x00010000; //    484333  TYBIT   Astrometry comes from Tycho2 cat
const int XRBIT   = 0x00020000; //      2289  XRBIT   Alt correlations for same (RA,Dec)
const int ITMBIT  = 0x00040000; //         0  ITMBIT  Alt correlations for same 2MASS ID
const int IUCBIT  = 0x00080000; //        19  IUCBIT  Alt correlations for same UCAC-2 ID
const int ITYBIT  = 0x00100000; //        45  ITYBIT  Alt correlations for same Tycho2 ID
const int OMAGBIT = 0x00200000; //  45066488  OMAGBIT Blue magnitude from O (not J) plate
const int EMAGBIT = 0x00400000; //  87873547  EMAGBIT Red magnitude from E (not F) plate
const int TMONLY  = 0x00800000; //  61787163  TMONLY  Object found only in 2MASS cat
const int HIPAST  = 0x01000000; //    120242  HIPAST  Ast from Hipparcos (not Tycho2) cat
const int SPIKE   = 0x02000000; //  18031305  SPIKE   USNO-B1.0 diffraction spike bit set
const int TYCONF  = 0x04000000; //      4468  TYCONF  Tycho2 confusion flag
const int BSCONF  = 0x08000000; //   1076296  BSCONF  Bright star has nearby faint source
const int BSART   = 0x10000000; //    161579  BSART   Faint source is bright star artifact
const int USEME   = 0x20000000; //  16117123  USEME   Recommended astrometric standard
const int EXCAT   = 0x40000000; //         0  EXCAT   External, non-astrometric object

int main(int argc,char *argv[]) {
  Accumulator accu;
  int total_read = 0;
  char fname[1024];
  for (int i=0;i<180;i++) {
    for (int j=0;j<10;j++) {
      const int rc = snprintf(fname,sizeof(fname),
                              "%s/%03d/m%03d%d.cat",nomad_path,i,i,j);
      assert (rc < sizeof(fname));
      FILE *f;
      f = fopen(fname,"rb");
      if (f == 0) {
        fprintf(stderr,"Could not open file \"%s\".\n",fname);
        exit(2);
      }
      int count = 0;
      do {
        count = fread(buff,sizeof(NOMAD_Record),READ_SIZE,f);
        total_read += count;
        printf("\rfread(%s,...) returned %6d, total_read = %8d",
               fname,count,total_read);
        fflush(stdout);
        for (int k=0;k<count;k++) {
          if ((buff[k].v <= 19500 ||
               buff[k].b <= 19500 ||
               buff[k].r <= 19500) &&
              buff[k].tycho2_id == 0) {
            if (buff[k].flags & (YBBIT|UCBIT|ITMBIT|TYBIT|HIPAST|EXCAT)) {
              fprintf(stderr,"\n %03d%d,%d: error 1\n",i,j,k);
            } else if (buff[k].v <= 0 || buff[k].b <= 0 || buff[k].r <= 0) {
              fprintf(stderr,"\n %03d%d,%d: error 2\n",i,j,k);
            } else {
              unsigned short flags = 0;
              if (buff[k].flags & ASTSRCBIT0) flags |= SHORT_ASTSRCBIT0;
              if (buff[k].flags & ASTSRCBIT1) flags |= SHORT_ASTSRCBIT1;
              if (buff[k].flags & ASTSRCBIT2) flags |= SHORT_ASTSRCBIT2;
              if (buff[k].flags & UBBIT  ) flags |= SHORT_UBBIT;
              if (buff[k].flags & TMBIT  ) flags |= SHORT_TMBIT;
              if (buff[k].flags & XRBIT  ) flags |= SHORT_XRBIT;
              if (buff[k].flags & IUCBIT ) flags |= SHORT_IUCBIT;
              if (buff[k].flags & ITYBIT ) flags |= SHORT_ITYBIT;
              if (buff[k].flags & OMAGBIT) flags |= SHORT_OMAGBIT;
              if (buff[k].flags & EMAGBIT) flags |= SHORT_EMAGBIT;
              if (buff[k].flags & TMONLY ) flags |= SHORT_TMONLY;
              if (buff[k].flags & SPIKE  ) flags |= SHORT_SPIKE;
              if (buff[k].flags & TYCONF ) flags |= SHORT_TYCONF;
              if (buff[k].flags & BSCONF ) flags |= SHORT_BSCONF;
              if (buff[k].flags & BSART  ) flags |= SHORT_BSART;
              if (buff[k].flags & USEME  ) flags |= SHORT_USEME;
              
              accu.add(buff[k].ra,buff[k].spd,buff[k].pm_ra,buff[k].pm_spd,
                       (short int)(buff[k].b > 30000 ? 30000 : buff[k].b),
                       (short int)(buff[k].v > 30000 ? 30000 : buff[k].v),
                       (short int)(buff[k].r > 30000 ? 30000 : buff[k].r),
                       flags);
            }
          }
        }
      } while (count == READ_SIZE);
      printf("\n");
      fclose(f);
    }
  }

  return 0;
}
