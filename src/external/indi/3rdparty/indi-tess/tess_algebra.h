/*
    NACHO MAS 2013 (mas.ignacio at gmail.com)
    Driver for TESS custom hardware. See README

    Base on tutorial_Four
    Copyright (C) 2010 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#ifndef inditess_algebra_H
#define inditess_algebra_H

typedef struct
{
    float x;
    float y;
    float z;
} vector;

int get_heading(const vector *a, const vector *m, const vector *p, vector *headingVector);
void vector_cross(const vector *a, const vector *b, vector *out);
float vector_dot(const vector *a, const vector *b);
void vector_normalize(vector *a);
void MaxMin(void);
void magReset(vector MagMax, vector MagMin);

#endif
