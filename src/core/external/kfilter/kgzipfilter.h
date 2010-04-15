/* This file is part of the KDE libraries
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __kgzipfilter__h
#define __kgzipfilter__h

#include "kfilterbase.h"

/**
 * Internal class used by KFilterDev
 * @internal
 */
class KGzipFilter : public KFilterBase
{
public:
    KGzipFilter();
    virtual ~KGzipFilter();

    virtual void init( int mode );
    virtual int mode() const;
    virtual void terminate();
    virtual void reset();
    virtual bool readHeader();
    virtual bool writeHeader( const QByteArray & fileName );
    void writeFooter();
    virtual void setOutBuffer( char * data, uint maxlen );
    virtual void setInBuffer( const char * data, uint size );
    virtual int  inBufferAvailable() const;
    virtual int  outBufferAvailable() const;
    virtual Result uncompress();
    virtual Result compress( bool finish );

private:
    Result uncompress_noop();
    class Private;
    Private* const d;
};

#endif
