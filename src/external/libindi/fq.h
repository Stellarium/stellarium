/* a fifo queue that never fills.
 * Copyright (C) 2005 Elwood C. Downey ecdowney@clearskyinstitute.com

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include <stddef.h>

typedef struct _FQ FQ;

extern FQ *newFQ(int grow);
extern void delFQ(FQ *q);
extern void pushFQ(FQ *q, void *e);
extern void *popFQ(FQ *q);
extern void *peekFQ(FQ *q);
extern void *peekiFQ(FQ *q, int i);
extern int nFQ(FQ *q);
extern void setMemFuncsFQ(void *(*newmalloc)(size_t size), void *(*newrealloc)(void *ptr, size_t size),
                          void (*newfree)(void *ptr));
