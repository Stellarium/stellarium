/*
 * Stellarium
 * Copyright (C) 2026 KainanP
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

#ifndef STELTEXTURELOADQUEUE_HPP
#define STELTEXTURELOADQUEUE_HPP

#include <QList>
#include <QSharedPointer>
#include <QWeakPointer>

//! @class StelTextureLoadQueue
//! Queue of textures waiting for a loading step (decoding in a loader thread, or upload to OpenGL).
//! Instead of first-in-first-out, takeNext() returns the texture with the highest number of
//! failed bind() calls, i.e. the texture that drawing code has asked for most often while it was
//! not yet available. Textures that are in view are bound every frame, so they overtake textures
//! that were asked for once and then went out of view. Ties are resolved in insertion order.
//! The priority is read when an entry is taken, so bind() calls made after push() are accounted for.
//! Entries are held by weak pointers: textures destroyed while queued are silently dropped.
//! This class is not thread-safe.
//! @tparam T a type providing <tt>unsigned getBindRequestCount() const</tt>, normally StelTexture.
template<typename T>
class StelTextureLoadQueue
{
public:
	void push(const QSharedPointer<T>& tex) { entries.append(tex); }

	bool isEmpty() const { return entries.isEmpty(); }

	//! Number of entries, including those whose texture has already been destroyed.
	int size() const { return static_cast<int>(entries.size()); }

	void clear() { entries.clear(); }

	//! Remove @p tex from the queue, if present.
	void remove(const T* tex)
	{
		for (qsizetype i = 0; i < entries.size(); ++i)
		{
			if (entries[i].toStrongRef().data() == tex)
			{
				entries.removeAt(i);
				return;
			}
		}
	}

	//! Remove and return the queued texture with the highest bind request count.
	//! @return a null pointer if no live texture is queued.
	QSharedPointer<T> takeNext()
	{
		QSharedPointer<T> best;
		qsizetype bestIndex = -1;
		unsigned bestCount = 0;
		for (qsizetype i = 0; i < entries.size(); )
		{
			const QSharedPointer<T> tex = entries[i].toStrongRef();
			if (!tex)
			{
				entries.removeAt(i);
				continue;
			}
			const unsigned count = tex->getBindRequestCount();
			if (!best || count > bestCount)
			{
				best = tex;
				bestIndex = i;
				bestCount = count;
			}
			++i;
		}
		if (best)
			entries.removeAt(bestIndex);
		return best;
	}

	//! @return the highest bind request count among the live queued textures, or 0 if there are none.
	unsigned highestBindRequestCount() const
	{
		unsigned highest = 0;
		for (const auto& entry : entries)
		{
			const QSharedPointer<T> tex = entry.toStrongRef();
			if (tex && tex->getBindRequestCount() > highest)
				highest = tex->getBindRequestCount();
		}
		return highest;
	}

private:
	QList<QWeakPointer<T>> entries;
};

#endif // STELTEXTURELOADQUEUE_HPP
