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

//Vector algebra functions
#include "tess_algebra.h"
#include <math.h>
void vector_cross(const vector *a, const vector *b, vector *out)
{
    out->x = a->y * b->z - a->z * b->y;
    out->y = a->z * b->x - a->x * b->z;
    out->z = a->x * b->y - a->y * b->x;
}

float vector_dot(const vector *a, const vector *b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

void vector_normalize(vector *a)
{
    float mag = sqrt(vector_dot(a, a));
    if (mag != 0)
    {
        a->x /= mag;
        a->y /= mag;
        a->z /= mag;
    }
    else
    {
        a->x = 0.1;
        a->y = 0.1;
        a->z = 0.1;
    }
}

//--------------------------------------------------------------------
// Returns a heading (in degrees)
// given an acceleration vector a due to gravity, a magnetic vector m, and a facing vector p.
//--------------------------------------------------------------------
int get_heading(const vector *a, const vector *m, const vector *p, vector *headingVector)
{
    vector E;
    vector N;
    float k;
    float roll, pitch, heading;

    // cross magnetic vector (magnetic north + inclination) with "down" (acceleration vector) to produce "east"
    vector_cross(m, a, &E);
    vector_normalize(&E);

    // cross "down" with "east" to produce "north" (parallel to the ground)
    vector_cross(a, &E, &N);
    vector_normalize(&N);

    //Calcula Roll
    k = pow(a->y, 2) + pow(a->z, 2);
    if (k != 0)
        roll = atan(a->x / sqrt(k));
    roll = roll * 180 / M_PI;

    //Calcula Altura
    k = pow(a->x, 2) + pow(a->z, 2);
    if (k != 0)
        pitch = atan(a->y / sqrt(k));
    pitch = pitch * 180 / M_PI;

    //Calcula heading
    heading = atan2(vector_dot(&E, p), vector_dot(&N, p)) * 180 / M_PI;
    if (heading < 0)
        heading += 360;

    headingVector->x = roll;
    headingVector->y = pitch;
    headingVector->z = heading;

    return heading;
}

void MaxMin(vector *m_avg, vector *MagMax, vector *MagMin)
{
    if (m_avg->x > MagMax->x)
        MagMax->x = m_avg->x;
    if (m_avg->y > MagMax->y)
        MagMax->y = m_avg->y;
    if (m_avg->z > MagMax->z)
        MagMax->z = m_avg->z;

    if (m_avg->x < MagMin->x)
        MagMin->x = m_avg->x;
    if (m_avg->y < MagMin->y)
        MagMin->y = m_avg->y;
    if (m_avg->z < MagMin->z)
        MagMin->z = m_avg->z;
}

//--- resetea variables magnetometro --------
void magReset(vector *MagMax, vector *MagMin)
{
    MagMax->x = -5000;
    MagMax->y = -5000;
    MagMax->z = -5000;

    MagMin->x = 5000;
    MagMin->y = 5000;
    MagMin->z = 5000;
}
