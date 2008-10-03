/*
 * The 3D Studio File Format Library
 * Copyright (C) 1996-2007 by Jan Eric Kyprianidis <www.kyprianidis.com>
 * All rights reserved.
 *
 * This program is  free  software;  you can redistribute it and/or modify it
 * under the terms of the  GNU Lesser General Public License  as published by 
 * the  Free Software Foundation;  either version 2.1 of the License,  or (at 
 * your option) any later version.
 *
 * This  program  is  distributed in  the  hope that it will  be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or  FITNESS FOR A  PARTICULAR PURPOSE.  See the  GNU Lesser General Public  
 * License for more details.
 *
 * You should  have received  a copy of the GNU Lesser General Public License
 * along with  this program;  if not, write to the  Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: ease.c,v 1.6 2007/06/15 09:33:19 jeh Exp $
 */
#include <lib3ds/ease.h>


/*!
 * \defgroup ease Ease
 */


/*!
 * \ingroup ease
 */
Lib3dsFloat
lib3ds_ease(Lib3dsFloat fp, Lib3dsFloat fc, Lib3dsFloat fn,
  Lib3dsFloat ease_from, Lib3dsFloat ease_to)
{
  Lib3dsDouble s,step;
  Lib3dsDouble tofrom;
  Lib3dsDouble a;

  s=step=(Lib3dsFloat)(fc-fp)/(fn-fp);
  tofrom=ease_to+ease_from;
  if (tofrom!=0.0) {
    if (tofrom>1.0) {
      ease_to=(Lib3dsFloat)(ease_to/tofrom);
      ease_from=(Lib3dsFloat)(ease_from/tofrom);
    }
    a=1.0/(2.0-(ease_to+ease_from));

    if (step<ease_from) s=a/ease_from*step*step;
    else {
      if ((1.0-ease_to)<=step) {
        step=1.0-step;
        s=1.0-a/ease_to*step*step;
      }
      else {
        s=((2.0*step)-ease_from)*a;
      }
    }
  }
  return((Lib3dsFloat)s);
}
