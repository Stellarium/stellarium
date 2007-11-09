/*
 * Copyright (C) 1999-2001 Free Software Foundation, Inc.
 * This file is part of the GNU LIBICONV Library.
 *
 * The GNU LIBICONV Library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * The GNU LIBICONV Library is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the GNU LIBICONV Library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation, Inc., 59 Temple Place -
 * Suite 330, Boston, MA 02111-1307, USA.
 */

/*
 * GB18030
 */

/*
 * GB18030, as implemented in glibc-2.2, is an extension of GBK (= CP936).
 * It adds the following ranges:
 * 1. Two-byte range
 *    0xA2E3, 0xA8BF, 0xA98A..0xA995, 0xFE50..0xFE9F
 * 2. Four-byte range
 *    0x{81..84}{30..39}{81..FE}{30..39}
 *    Most of Unicode plane 1 in Unicode order.
 *    Start: 0x81308130 = 0x0080
 *    End:   0x8431A439 = 0xFFFF
 * 3. Four-byte range
 *    0x{90..E3}{30..39}{81..FE}{30..39}
 *    Unicode plane 2..16 in Unicode order.
 *    Start: 0x90308130 = 0x010000
 *    End:   0xE3329A35 = 0x10FFFF
 */

#include "gb18030ext.h"
#include "gb18030uni.h"

static int
gb18030_mbtowc (conv_t conv, ucs4_t *pwc, const unsigned char *s, int n)
{
  int ret;

  /* Code set 0 (ASCII) */
  if (*s < 0x80)
    return ascii_mbtowc(conv,pwc,s,n);

  /* Code set 1 (GBK extended) */
  ret = gbk_mbtowc(conv,pwc,s,n);
  if (ret != RET_ILSEQ)
    return ret;

  ret = gb18030ext_mbtowc(conv,pwc,s,n);
  if (ret != RET_ILSEQ)
    return ret;

  /* Code set 2 (remainder of Unicode U+0000..U+FFFF) */
  ret = gb18030uni_mbtowc(conv,pwc,s,n);
  if (ret != RET_ILSEQ)
    return ret;

  /* Code set 3 (Unicode U+10000..U+10FFFF) */
  {
    unsigned char c1 = s[0];
    if (c1 >= 0x90 && c1 <= 0xe3) {
      if (n >= 2) {
        unsigned char c2 = s[1];
        if (c2 >= 0x30 && c2 <= 0x39) {
          if (n >= 3) {
            unsigned char c3 = s[2];
            if (c3 >= 0x81 && c3 <= 0xfe) {
              if (n >= 4) {
                unsigned char c4 = s[3];
                if (c4 >= 0x30 && c4 <= 0x39) {
                  unsigned int i = (((c1 - 0x90) * 10 + (c2 - 0x30)) * 126 + (c3 - 0x81)) * 10 + (c4 - 0x30);
                  if (i >= 0 && i < 0x100000) {
                    *pwc = (ucs4_t) (0x10000 + i);
                    return 4;
                  }
                }
                return RET_ILSEQ;
              }
              return RET_TOOFEW(0);
            }
            return RET_ILSEQ;
          }
          return RET_TOOFEW(0);
        }
        return RET_ILSEQ;
      }
      return RET_TOOFEW(0);
    }
    return RET_ILSEQ;
  }
}

static int
gb18030_wctomb (conv_t conv, unsigned char *r, ucs4_t wc, int n)
{
  int ret;

  /* Code set 0 (ASCII) */
  ret = ascii_wctomb(conv,r,wc,n);
  if (ret != RET_ILUNI)
    return ret;

  /* Code set 1 (GBK extended) */
  ret = gbk_wctomb(conv,r,wc,n);
  if (ret != RET_ILUNI)
    return ret;

  ret = gb18030ext_wctomb(conv,r,wc,n);
  if (ret != RET_ILUNI)
    return ret;

  /* Code set 2 (remainder of Unicode U+0000..U+FFFF) */
  ret = gb18030uni_wctomb(conv,r,wc,n);
  if (ret != RET_ILUNI)
    return ret;

  /* Code set 3 (Unicode U+10000..U+10FFFF) */
  if (n >= 4) {
    if (wc >= 0x10000 && wc < 0x110000) {
      unsigned int i = wc - 0x10000;
      r[3] = (i % 10) + 0x30; i = i / 10;
      r[2] = (i % 126) + 0x81; i = i / 126;
      r[1] = (i % 10) + 0x30; i = i / 10;
      r[0] = i + 0x90;
      return 4;
    }
    return RET_ILUNI;
  }
  return RET_TOOSMALL;
}
