/*
 * Stellarium Telescope Control Plug-in
 *
 * Copyright (C) 2006 Johannes Gajdosik
 * Copyright (C) 2009-2010 Bogdan Marinov
 *
 * This module was originally written by Johannes Gajdosik in 2006
 * as a core module of Stellarium. In 2009 it was significantly extended with
 * GUI features and later split as an external plug-in module by Bogdan Marinov.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "InterpolatedPosition.hpp"

InterpolatedPosition::InterpolatedPosition() :
		end_position(positions+(sizeof(positions)/sizeof(positions[0])))
{
	reset();
}

InterpolatedPosition::~InterpolatedPosition()
{
}

void InterpolatedPosition::reset()
{
	for (position_pointer = positions; position_pointer < end_position; position_pointer++)
	{
		position_pointer->server_micros = INT64_MAX;
		position_pointer->client_micros = INT64_MAX;
		position_pointer->pos[0] = 0.0;
		position_pointer->pos[1] = 0.0;
		position_pointer->pos[2] = 0.0;
		position_pointer->status = 0;
	}
	position_pointer = positions;
}

void InterpolatedPosition::add(Vec3d &position, qint64 clientTime, qint64 serverTime, int status)
{
	// remember the time and received position so that later we
	// will know where the telescope is pointing to:
	position_pointer++;
	if (position_pointer >= end_position)
		position_pointer = positions;

	position_pointer->pos = position;
	position_pointer->server_micros = serverTime;
	position_pointer->client_micros = clientTime;
	position_pointer->status = status;
}

Vec3d InterpolatedPosition::get(qint64 now) const
{
	if (position_pointer->client_micros == INT64_MAX)
	{
		return Vec3d(0,0,0);
	}

	const Position *p = position_pointer;
	do
	{
		const Position *pp = p;
		if (pp == positions) pp = end_position;
		pp--;
		if (pp->client_micros == INT64_MAX) break;
		if (pp->client_micros <= now && now <= p->client_micros)
		{
			if (pp->client_micros != p->client_micros)
			{
				Vec3d rval = p->pos * (now - pp->client_micros) + pp->pos * (p->client_micros - now);
				double f = rval.lengthSquared();
				if (f > 0.0)
				{
					return (1.0/std::sqrt(f))*rval;
				}
			}
			break;
		}
		p = pp;
	}
	while (p != position_pointer);

	return Vec3d(p->pos);
}
