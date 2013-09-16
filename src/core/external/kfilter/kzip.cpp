/* This file is part of the KDE libraries
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002 Holger Schroeder <holger-kde@holgis.net>

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

/*
	This class implements a kioslave to access ZIP files from KDE.
	you can use it in QIODevice::ReadOnly or in QIODevice::WriteOnly mode, and it
	behaves just as expected (i hope ;-) ).
	It can also be used in QIODevice::ReadWrite mode, in this case one can
	append files to an existing zip archive. when you append new files, which
	are not yet in the zip, it works as expected, they are appended at the end.
	when you append a file, which is already in the file, the reference to the
	old file is dropped and the new one is added to the zip. but the
	old data from the file itself is not deleted, it is still in the
	zipfile. so when you want to have a small and garbagefree zipfile,
	just read the contents of the appended zipfile and write it to a new one
	in QIODevice::WriteOnly mode. especially take care of this, when you don't want
	to leak information of how intermediate versions of files in the zip
	were looking.
	For more information on the zip fileformat go to
	http://www.pkware.com/support/appnote.html .

*/

#include "kzip.h"
#include "kfilterdev.h"
#include "klimitediodevice.h"

#include <QtCore/QHash>
#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QDate>
#include <QtCore/QList>
#include <QDebug>

#include <zlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>

#ifdef Q_OS_WIN
#include "kdewin32/sys/stat.h"
#endif

const int max_path_len = 4095;	// maximum number of character a path may contain

static void transformToMsDos(const QDateTime& dt, char* buffer)
{
	if ( dt.isValid() )
	{
		const quint16 time =
			 ( dt.time().hour() << 11 )    // 5 bit hour
		   | ( dt.time().minute() << 5 )   // 6 bit minute
		   | ( dt.time().second() >> 1 );  // 5 bit double seconds

		buffer[0] = char(time);
		buffer[1] = char(time >> 8);

		const quint16 date =
			 ( ( dt.date().year() - 1980 ) << 9 ) // 7 bit year 1980-based
		   | ( dt.date().month() << 5 )           // 4 bit month
		   | ( dt.date().day() );                 // 5 bit day

		buffer[2] = char(date);
		buffer[3] = char(date >> 8);
	}
	else // !dt.isValid(), assume 1980-01-01 midnight
	{
		buffer[0] = 0;
		buffer[1] = 0;
		buffer[2] = 33;
		buffer[3] = 0;
	}
}

static time_t transformFromMsDos(const char* buffer)
{
	quint16 time = (uchar)buffer[0] | ( (uchar)buffer[1] << 8 );
	int h = time >> 11;
	int m = ( time & 0x7ff ) >> 5;
	int s = ( time & 0x1f ) * 2 ;
	QTime qt(h, m, s);

	quint16 date = (uchar)buffer[2] | ( (uchar)buffer[3] << 8 );
	int y = ( date >> 9 ) + 1980;
	int o = ( date & 0x1ff ) >> 5;
	int d = ( date & 0x1f );
	QDate qd(y, o, d);

	QDateTime dt( qd, qt );
	return dt.toTime_t();
}

// == parsing routines for zip headers

/** all relevant information about parsing file information */
struct ParseFileInfo {
  // file related info
  mode_t perm;			// permissions of this file
  time_t atime;			// last access time (UNIX format)
  time_t mtime;			// modification time (UNIX format)
  time_t ctime;			// creation time (UNIX format)
  int uid;			// user id (-1 if not specified)
  int gid;			// group id (-1 if not specified)
  QByteArray guessed_symlink;	// guessed symlink target
  int extralen;			// length of extra field

  // parsing related info
  bool exttimestamp_seen;	// true if extended timestamp extra field
				// has been parsed
  bool newinfounix_seen;	// true if Info-ZIP Unix New extra field has
				// been parsed

  ParseFileInfo() : perm(0100644), uid(-1), gid(-1), extralen(0),
	exttimestamp_seen(false), newinfounix_seen(false) {
	ctime = mtime = atime = time(0);
  }
};

/** updates the parse information with the given extended timestamp extra field.
  * @param buffer start content of buffer known to contain an extended
  *	timestamp extra field (without magic & size)
  * @param size size of field content (must not count magic and size entries)
  * @param islocal true if this is a local field, false if central
  * @param pfi ParseFileInfo object to be updated
  * @return true if processing was successful
  */
static bool parseExtTimestamp(const char *buffer, int size, bool islocal,
			ParseFileInfo &pfi) {
  if (size < 1) {
	qDebug() << "premature end of extended timestamp (#1)";
	return false;
  }/*end if*/
  int flags = *buffer;		// read flags
  buffer += 1;
  size -= 1;

  if (flags & 1) {		// contains modification time
	if (size < 4) {
	  qDebug() << "premature end of extended timestamp (#2)";
	  return false;
	}/*end if*/
	pfi.mtime = time_t((uchar)buffer[0] | (uchar)buffer[1] << 8
				| (uchar)buffer[2] << 16 | (uchar)buffer[3] << 24);
	buffer += 4;
	size -= 4;
  }/*end if*/
  // central extended field cannot contain more than the modification time
  // even if other flags are set
  if (!islocal) {
	pfi.exttimestamp_seen = true;
	return true;
  }/*end if*/

  if (flags & 2) {		// contains last access time
	if (size < 4) {
	  qDebug() << "premature end of extended timestamp (#3)";
	  return true;
	}/*end if*/
	pfi.atime = time_t((uchar)buffer[0] | (uchar)buffer[1] << 8
				| (uchar)buffer[2] << 16 | (uchar)buffer[3] << 24);
	buffer += 4;
	size -= 4;
  }/*end if*/

  if (flags & 4) {		// contains creation time
	if (size < 4) {
	  qDebug() << "premature end of extended timestamp (#4)";
	  return true;
	}/*end if*/
	pfi.ctime = time_t((uchar)buffer[0] | (uchar)buffer[1] << 8
				| (uchar)buffer[2] << 16 | (uchar)buffer[3] << 24);
	buffer += 4;
  }/*end if*/

  pfi.exttimestamp_seen = true;
  return true;
}

/** updates the parse information with the given Info-ZIP Unix old extra field.
  * @param buffer start of content of buffer known to contain an Info-ZIP
  *	Unix old extra field (without magic & size)
  * @param size size of field content (must not count magic and size entries)
  * @param islocal true if this is a local field, false if central
  * @param pfi ParseFileInfo object to be updated
  * @return true if processing was successful
  */
static bool parseInfoZipUnixOld(const char *buffer, int size, bool islocal,
			ParseFileInfo &pfi) {
  // spec mandates to omit this field if one of the newer fields are available
  if (pfi.exttimestamp_seen || pfi.newinfounix_seen) return true;

  if (size < 8) {
	qDebug() << "premature end of Info-ZIP unix extra field old";
	return false;
  }/*end if*/

  pfi.atime = time_t((uchar)buffer[0] | (uchar)buffer[1] << 8
				| (uchar)buffer[2] << 16 | (uchar)buffer[3] << 24);
  buffer += 4;
  pfi.mtime = time_t((uchar)buffer[0] | (uchar)buffer[1] << 8
				| (uchar)buffer[2] << 16 | (uchar)buffer[3] << 24);
  buffer += 4;
  if (islocal && size >= 12) {
	pfi.uid = (uchar)buffer[0] | (uchar)buffer[1] << 8;
	buffer += 2;
	pfi.gid = (uchar)buffer[0] | (uchar)buffer[1] << 8;
	buffer += 2;
  }/*end if*/
  return true;
}

#if 0 // not needed yet
/** updates the parse information with the given Info-ZIP Unix new extra field.
  * @param buffer start of content of buffer known to contain an Info-ZIP
  *		Unix new extra field (without magic & size)
  * @param size size of field content (must not count magic and size entries)
  * @param islocal true if this is a local field, false if central
  * @param pfi ParseFileInfo object to be updated
  * @return true if processing was successful
  */
static bool parseInfoZipUnixNew(const char *buffer, int size, bool islocal,
			ParseFileInfo &pfi) {
  if (!islocal) {	// contains nothing in central field
	pfi.newinfounix = true;
	return true;
  }/*end if*/

  if (size < 4) {
	qDebug() << "premature end of Info-ZIP unix extra field new";
	return false;
  }/*end if*/

  pfi.uid = (uchar)buffer[0] | (uchar)buffer[1] << 8;
  buffer += 2;
  pfi.gid = (uchar)buffer[0] | (uchar)buffer[1] << 8;
  buffer += 2;

  pfi.newinfounix = true;
  return true;
}
#endif

/**
 * parses the extra field
 * @param buffer start of buffer where the extra field is to be found
 * @param size size of the extra field
 * @param islocal true if this is part of a local header, false if of central
 * @param pfi ParseFileInfo object which to write the results into
 * @return true if parsing was successful
 */
static bool parseExtraField(const char *buffer, int size, bool islocal,
			ParseFileInfo &pfi) {
  // extra field in central directory doesn't contain useful data, so we
  // don't bother parsing it
  if (!islocal) return true;

  while (size >= 4) {	// as long as a potential extra field can be read
	int magic = (uchar)buffer[0] | (uchar)buffer[1] << 8;
	buffer += 2;
	int fieldsize = (uchar)buffer[0] | (uchar)buffer[1] << 8;
	buffer += 2;
	size -= 4;

	if (fieldsize > size) {
	  //qDebug() << "fieldsize: " << fieldsize << " size: " << size;
	  qDebug() << "premature end of extra fields reached";
	  break;
	}/*end if*/

	switch (magic) {
	  case 0x5455:		// extended timestamp
		if (!parseExtTimestamp(buffer, fieldsize, islocal, pfi)) return false;
	break;
	  case 0x5855:		// old Info-ZIP unix extra field
		if (!parseInfoZipUnixOld(buffer, fieldsize, islocal, pfi)) return false;
	break;
#if 0	// not needed yet
	  case 0x7855:		// new Info-ZIP unix extra field
		if (!parseInfoZipUnixNew(buffer, fieldsize, islocal, pfi)) return false;
	break;
#endif
	  default:
		/* ignore everything else */;
	}/*end switch*/

	buffer += fieldsize;
	size -= fieldsize;
  }/*wend*/
  return true;
}

////////////////////////////////////////////////////////////////////////
/////////////////////////// KZip ///////////////////////////////////////
////////////////////////////////////////////////////////////////////////

class KZip::KZipPrivate
{
public:
	KZipPrivate()
		: m_crc( 0 ),
		  m_currentFile( 0 ),
		  m_currentDev( 0 ),
		  m_compression( 8 ),
		  m_extraField( KZip::NoExtraField ),
	  m_offset( 0 )
	{}

	unsigned long           m_crc;         // checksum
	KZipFileEntry*          m_currentFile; // file currently being written
	QIODevice*              m_currentDev;  // filterdev used to write to the above file
	QList<KZipFileEntry*> m_fileList;    // flat list of all files, for the index (saves a recursive method ;)
	int                     m_compression;
	KZip::ExtraField        m_extraField;
	// m_offset holds the offset of the place in the zip,
	// where new data can be appended. after openarchive it points to 0, when in
	// writeonly mode, or it points to the beginning of the central directory.
	// each call to writefile updates this value.
	unsigned int            m_offset;
};

KZip::KZip( const QString& fileName )
	: KArchive( fileName ),d(new KZipPrivate)
{
}

KZip::KZip( QIODevice * dev )
	: KArchive( dev ),d(new KZipPrivate)
{
}

KZip::~KZip()
{
	//qDebug() << this;
	if( isOpen() )
		close();
	delete d;
}

bool KZip::openArchive( QIODevice::OpenMode mode )
{
	//qDebug();
	d->m_fileList.clear();

	if ( mode == QIODevice::WriteOnly )
		return true;

	char buffer[47];

	// Check that it's a valid ZIP file
	// KArchive::open() opened the underlying device already.

	uint offset = 0; // holds offset, where we read
	int n;

	// contains information gathered from the local file headers
	QHash<QByteArray, ParseFileInfo> pfi_map;

	QIODevice* dev = device();

	// We set a bool for knowing if we are allowed to skip the start of the file
	bool startOfFile = true;

	for (;;) // repeat until 'end of entries' signature is reached
	{
		//qDebug() << "loop starts";
		//qDebug() << "dev->pos() now : " << dev->pos();
		n = dev->read( buffer, 4 );

		if (n < 4)
		{
			qWarning() << "Invalid ZIP file. Unexpected end of file. (#1)";

			return false;
		}

		if ( !memcmp( buffer, "PK\5\6", 4 ) ) // 'end of entries'
		{
		//qDebug() << "PK56 found end of archive";
			startOfFile = false;
		break;
	}

	if ( !memcmp( buffer, "PK\3\4", 4 ) ) // local file header
		{
		//qDebug() << "PK34 found local file header";
			startOfFile = false;
			// can this fail ???
		dev->seek( dev->pos() + 2 ); // skip 'version needed to extract'

		// read static header stuff
			n = dev->read( buffer, 24 );
		if (n < 24) {
				qWarning() << "Invalid ZIP file. Unexpected end of file. (#4)";
				return false;
		}

		int gpf = (uchar)buffer[0];	// "general purpose flag" not "general protection fault" ;-)
		int compression_mode = (uchar)buffer[2] | (uchar)buffer[3] << 8;
		time_t mtime = transformFromMsDos( buffer+4 );

		qint64 compr_size = (uchar)buffer[12] | (uchar)buffer[13] << 8
					| (uchar)buffer[14] << 16 | (uchar)buffer[15] << 24;
		qint64 uncomp_size = (uchar)buffer[16] | (uchar)buffer[17] << 8
					| (uchar)buffer[18] << 16 | (uchar)buffer[19] << 24;
		int namelen = (uchar)buffer[20] | (uchar)buffer[21] << 8;
		int extralen = (uchar)buffer[22] | (uchar)buffer[23] << 8;

			/*
		qDebug() << "general purpose bit flag: " << gpf;
		qDebug() << "compressed size: " << compr_size;
		qDebug() << "uncompressed size: " << uncomp_size;
		qDebug() << "namelen: " << namelen;
		qDebug() << "extralen: " << extralen;
		qDebug() << "archive size: " << dev->size();
		*/

		// read fileName
			Q_ASSERT( namelen > 0 );
		QByteArray fileName = dev->read(namelen);
			if ( fileName.size() < namelen ) {
				qWarning() << "Invalid ZIP file. Name not completely read (#2)";
		return false;
		}

		ParseFileInfo pfi;
		pfi.mtime = mtime;

			// read and parse the beginning of the extra field,
			// skip rest of extra field in case it is too long
			unsigned int extraFieldEnd = dev->pos() + extralen;
		pfi.extralen = extralen;
		int handledextralen = qMin(extralen, (int)sizeof buffer);

			//if ( handledextralen )
			//    qDebug() << "handledextralen: " << handledextralen;

		n = dev->read(buffer, handledextralen);
		// no error msg necessary as we deliberately truncate the extra field
		if (!parseExtraField(buffer, handledextralen, true, pfi))
		{
			qWarning() << "Invalid ZIP File. Broken ExtraField.";
			return false;
		}

			// jump to end of extra field
			dev->seek( extraFieldEnd );

		// we have to take care of the 'general purpose bit flag'.
			// if bit 3 is set, the header doesn't contain the length of
			// the file and we look for the signature 'PK\7\8'.
			if ( gpf & 8 )
			{
			// here we have to read through the compressed data to find
		// the next PKxx
			qDebug() << "trying to seek for next PK78";
				bool foundSignature = false;

				while (!foundSignature)
				{
					n = dev->read( buffer, 1 );
					if (n < 1)
					{
						qWarning() << "Invalid ZIP file. Unexpected end of file. (#2)";
						return false;
					}

					if ( buffer[0] != 'P' )
						continue;

					n = dev->read( buffer, 3 );
					if (n < 3)
					{
						qWarning() << "Invalid ZIP file. Unexpected end of file. (#3)";
						return false;
					}

					// we have to detect three magic tokens here:
			// PK34 for the next local header in case there is no data descriptor
			// PK12 for the central header in case there is no data descriptor
			// PK78 for the data descriptor in case it is following the compressed data

			if ( buffer[0] == 'K' && buffer[1] == 7 && buffer[2] == 8 )
					{
						foundSignature = true;
						dev->seek( dev->pos() + 12 ); // skip the 'data_descriptor'
					}
			else if ( ( buffer[0] == 'K' && buffer[1] == 1 && buffer[2] == 2 )
				 || ( buffer[0] == 'K' && buffer[1] == 3 && buffer[2] == 4 ) )
					{
						foundSignature = true;
						dev->seek( dev->pos() - 4 ); // go back 4 bytes, so that the magic bytes can be found...
					}
					else if ( buffer[0] == 'P' || buffer[1] == 'P' || buffer[2] == 'P' )
					{
						// We have another P character so we must go back a little to check if it is a magic
						dev->seek( dev->pos() - 3 );
					}

				}
			}
			else
			{
			// here we skip the compressed data and jump to the next header
			//qDebug() << "general purpose bit flag indicates, that local file header contains valid size";
		// check if this could be a symbolic link
		if (compression_mode == NoCompression
				&& uncomp_size <= max_path_len
			&& uncomp_size > 0) {
			// read content and store it
					// If it's not a symlink, then we'll just discard the data for now.
			pfi.guessed_symlink = dev->read(uncomp_size);
			if (pfi.guessed_symlink.size() < uncomp_size) {
			qWarning() << "Invalid ZIP file. Unexpected end of file. (#5)";
			return false;
			}
		} else {

				if ( compr_size > dev->size() )
			{
				// here we cannot trust the compressed size, so scan through the compressed
			// data to find the next header
			bool foundSignature = false;

			while (!foundSignature)
			{
				n = dev->read( buffer, 1 );
				if (n < 1)
				{
					qWarning() << "Invalid ZIP file. Unexpected end of file. (#2)";
					return false;
				}

				if ( buffer[0] != 'P' )
					continue;

				n = dev->read( buffer, 3 );
				if (n < 3)
				{
					qWarning() << "Invalid ZIP file. Unexpected end of file. (#3)";
					return false;
				}

				// we have to detect three magic tokens here:
				// PK34 for the next local header in case there is no data descriptor
				// PK12 for the central header in case there is no data descriptor
				// PK78 for the data descriptor in case it is following the compressed data

				if ( buffer[0] == 'K' && buffer[1] == 7 && buffer[2] == 8 )
				{
					foundSignature = true;
					dev->seek( dev->pos() + 12 ); // skip the 'data_descriptor'
				}

				if ( ( buffer[0] == 'K' && buffer[1] == 1 && buffer[2] == 2 )
					|| ( buffer[0] == 'K' && buffer[1] == 3 && buffer[2] == 4 ) )
				{
					foundSignature = true;
					dev->seek( dev->pos() - 4 );
					// go back 4 bytes, so that the magic bytes can be found
					// in the next cycle...
				}
			}
			}
			else
			{
// 			qDebug() << "before interesting dev->pos(): " << dev->pos();
			bool success = dev->seek( dev->pos() + compr_size ); // can this fail ???
			static_cast<void>(success); // annoying unused variable warning
						Q_ASSERT( success ); // let's see...
/*			qDebug() << "after interesting dev->pos(): " << dev->pos();
			if ( success )
				qDebug() << "dev->at was successful... ";
			else
				qDebug() << "dev->at failed... ";*/
			}

		}

// not needed any more
/*                // here we calculate the length of the file in the zip
				// with headers and jump to the next header.
				uint skip = compr_size + namelen + extralen;
				offset += 30 + skip;*/
			}
			pfi_map.insert(fileName, pfi);
		}
		else if ( !memcmp( buffer, "PK\1\2", 4 ) ) // central block
		{
		//qDebug() << "PK12 found central block";
			startOfFile = false;

			// so we reached the central header at the end of the zip file
			// here we get all interesting data out of the central header
			// of a file
			offset = dev->pos() - 4;

			//set offset for appending new files
			if ( d->m_offset == 0L ) d->m_offset = offset;

			n = dev->read( buffer + 4, 42 );
			if (n < 42) {
				qWarning() << "Invalid ZIP file, central entry too short"; // not long enough for valid entry
				return false;
			}

			//int gpf = (uchar)buffer[9] << 8 | (uchar)buffer[10];
			//kDebug() << "general purpose flag=" << gpf;
			// length of the fileName (well, pathname indeed)
			int namelen = (uchar)buffer[29] << 8 | (uchar)buffer[28];
			Q_ASSERT( namelen > 0 );
			QByteArray bufferName = dev->read( namelen );
			if ( bufferName.size() < namelen )
				qWarning() << "Invalid ZIP file. Name not completely read";

			ParseFileInfo pfi = pfi_map.value( bufferName, ParseFileInfo() );

			QString name( QFile::decodeName(bufferName) );

			//qDebug() << "name: " << name;
			// only in central header ! see below.
			// length of extra attributes
			int extralen = (uchar)buffer[31] << 8 | (uchar)buffer[30];
			// length of comment for this file
			int commlen =  (uchar)buffer[33] << 8 | (uchar)buffer[32];
			// compression method of this file
			int cmethod =  (uchar)buffer[11] << 8 | (uchar)buffer[10];

			//qDebug() << "cmethod: " << cmethod;
			//qDebug() << "extralen: " << extralen;

			// crc32 of the file
			uint crc32 = (uchar)buffer[19] << 24 | (uchar)buffer[18] << 16 |
					   (uchar)buffer[17] << 8 | (uchar)buffer[16];

			// uncompressed file size
			uint ucsize = (uchar)buffer[27] << 24 | (uchar)buffer[26] << 16 |
				(uchar)buffer[25] << 8 | (uchar)buffer[24];
			// compressed file size
			uint csize = (uchar)buffer[23] << 24 | (uchar)buffer[22] << 16 |
				(uchar)buffer[21] << 8 | (uchar)buffer[20];

			// offset of local header
			uint localheaderoffset = (uchar)buffer[45] << 24 | (uchar)buffer[44] << 16 |
				(uchar)buffer[43] << 8 | (uchar)buffer[42];

			// some clever people use different extra field lengths
			// in the central header and in the local header... funny.
			// so we need to get the localextralen to calculate the offset
			// from localheaderstart to dataoffset
			int localextralen = pfi.extralen; // FIXME: this will not work if
							// no local header exists

			//qDebug() << "localextralen: " << localextralen;

			// offset, where the real data for uncompression starts
			uint dataoffset = localheaderoffset + 30 + localextralen + namelen; //comment only in central header

			//qDebug() << "esize: " << esize;
			//qDebug() << "eoffset: " << eoffset;
			//qDebug() << "csize: " << csize;

		int os_madeby = (uchar)buffer[5];
			bool isdir = false;
			int access = 0100644;

		if (os_madeby == 3) {	// good ole unix
			access = (uchar)buffer[40] | (uchar)buffer[41] << 8;
		}

			QString entryName;

			if ( name.endsWith( '/' ) ) // Entries with a trailing slash are directories
			{
				isdir = true;
				name = name.left( name.length() - 1 );
				if (os_madeby != 3) access = S_IFDIR | 0755;
		else Q_ASSERT(access & S_IFDIR);
			}

			int pos = name.lastIndexOf( '/' );
			if ( pos == -1 )
				entryName = name;
			else
				entryName = name.mid( pos + 1 );
			Q_ASSERT( !entryName.isEmpty() );

			KArchiveEntry* entry;
			if ( isdir )
			{
				QString path = QDir::cleanPath( name );
				const KArchiveEntry* ent = rootDir()->entry( path );
				if ( ent && ent->isDirectory() )
				{
					//qDebug() << "Directory already exists, NOT going to add it again";
					entry = 0;
				}
				else
				{
					entry = new KArchiveDirectory( this, entryName, access, (int)pfi.mtime, rootDir()->user(), rootDir()->group(), QString() );
					//qDebug() << "KArchiveDirectory created, entryName= " << entryName << ", name=" << name;
				}
		}
			else
			{
			QString symlink;
		if (S_ISLNK(access)) {
			symlink = QFile::decodeName(pfi.guessed_symlink);
		}
				entry = new KZipFileEntry( this, entryName, access, pfi.mtime,
					rootDir()->user(), rootDir()->group(),
					symlink, name, dataoffset,
					ucsize, cmethod, csize );
				static_cast<KZipFileEntry *>(entry)->setHeaderStart( localheaderoffset );
				static_cast<KZipFileEntry*>(entry)->setCRC32(crc32);
				//qDebug() << "KZipFileEntry created, entryName= " << entryName << ", name=" << name;
				d->m_fileList.append( static_cast<KZipFileEntry *>( entry ) );
			}

			if ( entry )
			{
				if ( pos == -1 )
				{
					rootDir()->addEntry(entry);
				}
				else
				{
					// In some tar files we can find dir/./file => call cleanPath
					QString path = QDir::cleanPath( name.left( pos ) );
					// Ensure container directory exists, create otherwise
					KArchiveDirectory * tdir = findOrCreate( path );
					tdir->addEntry(entry);
				}
			}

			//calculate offset to next entry
			offset += 46 + commlen + extralen + namelen;
			bool b = dev->seek(offset);
			Q_ASSERT( b );
			if ( !b )
			  return false;
		}
		else if ( startOfFile )
		{
			// The file does not start with any ZIP header (e.g. self-extractable ZIP files)
			// Therefore we need to find the first PK\003\004 (local header)
			qDebug() << "Try to skip start of file";
			startOfFile = false;
			bool foundSignature = false;

			while (!foundSignature)
			{
				n = dev->read( buffer, 1 );
				if (n < 1)
				{
					qWarning() << "Invalid ZIP file. Unexpected end of file. " ;
					return false;
				}

				if ( buffer[0] != 'P' )
					continue;

				n = dev->read( buffer, 3 );
				if (n < 3)
				{
					qWarning() << "Invalid ZIP file. Unexpected end of file. " ;
					return false;
				}

				// We have to detect the magic token for a local header: PK\003\004
				/*
				 * Note: we do not need to check the other magics, if the ZIP file has no
				 * local header, then it has not any files!
				 */
				if ( buffer[0] == 'K' && buffer[1] == 3 && buffer[2] == 4 )
				{
					foundSignature = true;
					dev->seek( dev->pos() - 4 ); // go back 4 bytes, so that the magic bytes can be found...
				}
				else if ( buffer[0] == 'P' || buffer[1] == 'P' || buffer[2] == 'P' )
				{
						// We have another P character so we must go back a little to check if it is a magic
					dev->seek( dev->pos() - 3 );
				}
			}
		}
		else
		{
			qWarning() << "Invalid ZIP file. Unrecognized header at offset " << offset;

			return false;
		}
	}
	//qDebug() << "*** done *** ";
	return true;
}

bool KZip::closeArchive()
{
	if ( ! ( mode() & QIODevice::WriteOnly ) )
	{
		//qDebug() << "readonly";
		return true;
	}

	//ReadWrite or WriteOnly
	//write all central dir file entries

	// to be written at the end of the file...
	char buffer[ 22 ]; // first used for 12, then for 22 at the end
	uLong crc = crc32(0L, Z_NULL, 0);

	qint64 centraldiroffset = device()->pos();
	//qDebug() << "closearchive: centraldiroffset: " << centraldiroffset;
	qint64 atbackup = centraldiroffset;
	QMutableListIterator<KZipFileEntry*> it( d->m_fileList );

	while(it.hasNext())
	{	//set crc and compressed size in each local file header
		it.next();
		if ( !device()->seek( it.value()->headerStart() + 14 ) )
			return false;
	//qDebug() << "closearchive setcrcandcsize: fileName:"
	//    << it.current()->path()
	//    << "encoding:" << it.current()->encoding();

		uLong mycrc = it.value()->crc32();
		buffer[0] = char(mycrc); // crc checksum, at headerStart+14
		buffer[1] = char(mycrc >> 8);
		buffer[2] = char(mycrc >> 16);
		buffer[3] = char(mycrc >> 24);

		int mysize1 = it.value()->compressedSize();
		buffer[4] = char(mysize1); // compressed file size, at headerStart+18
		buffer[5] = char(mysize1 >> 8);
		buffer[6] = char(mysize1 >> 16);
		buffer[7] = char(mysize1 >> 24);

		int myusize = it.value()->size();
		buffer[8] = char(myusize); // uncompressed file size, at headerStart+22
		buffer[9] = char(myusize >> 8);
		buffer[10] = char(myusize >> 16);
		buffer[11] = char(myusize >> 24);

		if ( device()->write( buffer, 12 ) != 12 )
			return false;
	}
	device()->seek( atbackup );

	it.toFront();
	while (it.hasNext())
	{
	it.next();
		//qDebug() << "fileName:" << it.current()->path()
		//              << "encoding:" << it.current()->encoding();

		QByteArray path = QFile::encodeName(it.value()->path());

	const int extra_field_len = 9;
		int bufferSize = extra_field_len + path.length() + 46;
		char* buffer = new char[ bufferSize ];

		memset(buffer, 0, 46); // zero is a nice default for most header fields

		const char head[] =
		{
			'P', 'K', 1, 2, // central file header signature
			0x14, 3,        // version made by (3 == UNIX)
			0x14, 0         // version needed to extract
		};

	// I do not know why memcpy is not working here
		//memcpy(buffer, head, sizeof(head));
		memmove(buffer, head, sizeof(head));

		buffer[ 10 ] = char(it.value()->encoding()); // compression method
		buffer[ 11 ] = char(it.value()->encoding() >> 8);

		transformToMsDos( it.value()->datetime(), &buffer[ 12 ] );

		uLong mycrc = it.value()->crc32();
		buffer[ 16 ] = char(mycrc); // crc checksum
		buffer[ 17 ] = char(mycrc >> 8);
		buffer[ 18 ] = char(mycrc >> 16);
		buffer[ 19 ] = char(mycrc >> 24);

		int mysize1 = it.value()->compressedSize();
		buffer[ 20 ] = char(mysize1); // compressed file size
		buffer[ 21 ] = char(mysize1 >> 8);
		buffer[ 22 ] = char(mysize1 >> 16);
		buffer[ 23 ] = char(mysize1 >> 24);

		int mysize = it.value()->size();
		buffer[ 24 ] = char(mysize); // uncompressed file size
		buffer[ 25 ] = char(mysize >> 8);
		buffer[ 26 ] = char(mysize >> 16);
		buffer[ 27 ] = char(mysize >> 24);

		buffer[ 28 ] = char(it.value()->path().length()); // fileName length
		buffer[ 29 ] = char(it.value()->path().length() >> 8);

	buffer[ 30 ] = char(extra_field_len);
	buffer[ 31 ] = char(extra_field_len >> 8);

	buffer[ 40 ] = char(it.value()->permissions());
	buffer[ 41 ] = char(it.value()->permissions() >> 8);

		int myhst = it.value()->headerStart();
		buffer[ 42 ] = char(myhst); //relative offset of local header
		buffer[ 43 ] = char(myhst >> 8);
		buffer[ 44 ] = char(myhst >> 16);
		buffer[ 45 ] = char(myhst >> 24);

		// file name
		strncpy( buffer + 46, path, path.length() );
	//qDebug() << "closearchive length to write: " << bufferSize;

	// extra field
	char *extfield = buffer + 46 + path.length();
	extfield[0] = 'U';
	extfield[1] = 'T';
	extfield[2] = 5;
	extfield[3] = 0;
	extfield[4] = 1 | 2 | 4;	// specify flags from local field
					// (unless I misread the spec)
	// provide only modification time
	unsigned long time = (unsigned long)it.value()->date();
	extfield[5] = char(time);
	extfield[6] = char(time >> 8);
	extfield[7] = char(time >> 16);
	extfield[8] = char(time >> 24);

		crc = crc32(crc, (Bytef *)buffer, bufferSize );
		bool ok = ( device()->write( buffer, bufferSize ) == bufferSize );
		delete[] buffer;
		if ( !ok )
			return false;
	}
	qint64 centraldirendoffset = device()->pos();
	//qDebug() << "closearchive: centraldirendoffset: " << centraldirendoffset;
	//qDebug() << "closearchive: device()->pos(): " << device()->pos();

	//write end of central dir record.
	buffer[ 0 ] = 'P'; //end of central dir signature
	buffer[ 1 ] = 'K';
	buffer[ 2 ] = 5;
	buffer[ 3 ] = 6;

	buffer[ 4 ] = 0; // number of this disk
	buffer[ 5 ] = 0;

	buffer[ 6 ] = 0; // number of disk with start of central dir
	buffer[ 7 ] = 0;

	int count = d->m_fileList.count();
	//qDebug() << "number of files (count): " << count;


	buffer[ 8 ] = char(count); // total number of entries in central dir of
	buffer[ 9 ] = char(count >> 8); // this disk

	buffer[ 10 ] = buffer[ 8 ]; // total number of entries in the central dir
	buffer[ 11 ] = buffer[ 9 ];

	int cdsize = centraldirendoffset - centraldiroffset;
	buffer[ 12 ] = char(cdsize); // size of the central dir
	buffer[ 13 ] = char(cdsize >> 8);
	buffer[ 14 ] = char(cdsize >> 16);
	buffer[ 15 ] = char(cdsize >> 24);

	//qDebug() << "end : centraldiroffset: " << centraldiroffset;
	//qDebug() << "end : centraldirsize: " << cdsize;

	buffer[ 16 ] = char(centraldiroffset); // central dir offset
	buffer[ 17 ] = char(centraldiroffset >> 8);
	buffer[ 18 ] = char(centraldiroffset >> 16);
	buffer[ 19 ] = char(centraldiroffset >> 24);

	buffer[ 20 ] = 0; //zipfile comment length
	buffer[ 21 ] = 0;

	if ( device()->write( buffer, 22 ) != 22 )
		return false;

	return true;
}

bool KZip::doWriteDir( const QString&, const QString&, const QString&,
					   mode_t, time_t, time_t, time_t ) {
		return true;
}

bool KZip::doPrepareWriting(const QString &name, const QString &user,
							   const QString &group, qint64 /*size*/, mode_t perm,
							   time_t atime, time_t mtime, time_t ctime) {
	//qDebug();
	if ( !isOpen() )
	{
		qWarning( "KZip::writeFile: You must open the zip file before writing to it\n");
		return false;
	}

	if ( ! ( mode() & QIODevice::WriteOnly ) ) // accept WriteOnly and ReadWrite
	{
		qWarning( "KZip::writeFile: You must open the zip file for writing\n");
		return false;
	}

	Q_ASSERT( device() );

	// set right offset in zip.
	if ( !device()->seek( d->m_offset ) ) {
		qWarning() << "doPrepareWriting: cannot seek in ZIP file. Disk full?";
		return false;
	}

	// delete entries in the filelist with the same fileName as the one we want
	// to save, so that we don't have duplicate file entries when viewing the zip
	// with konqi...
	// CAUTION: the old file itself is still in the zip and won't be removed !!!
	QMutableListIterator<KZipFileEntry*> it( d->m_fileList );
	//qDebug() << "fileName to write: " << name;
	while(it.hasNext())
	{
		it.next();
		//qDebug() << "prepfileName: " << it.current()->path();
		if (name == it.value()->path() )
		{
			//qDebug() << "removing following entry: " << it.current()->path();
		delete it.value();
			it.remove();
		}

	}
	// Find or create parent dir
	KArchiveDirectory* parentDir = rootDir();
	QString fileName( name );
	int i = name.lastIndexOf( '/' );
	if ( i != -1 )
	{
		QString dir = name.left( i );
		fileName = name.mid( i + 1 );
		//qDebug() << "ensuring" << dir << "exists. fileName=" << fileName;
		parentDir = findOrCreate( dir );
	}

	// construct a KZipFileEntry and add it to list
	KZipFileEntry * e = new KZipFileEntry( this, fileName, perm, mtime, user, group, QString(),
										   name, device()->pos() + 30 + name.length(), // start
										   0 /*size unknown yet*/, d->m_compression, 0 /*csize unknown yet*/ );
	e->setHeaderStart( device()->pos() );
	//qDebug() << "wrote file start: " << e->position() << " name: " << name;
	parentDir->addEntry( e );

	d->m_currentFile = e;
	d->m_fileList.append( e );

	int extra_field_len = 0;
	if ( d->m_extraField == ModificationTime )
		extra_field_len = 17;	// value also used in finishWriting()

	// write out zip header
	QByteArray encodedName = QFile::encodeName(name);
	int bufferSize = extra_field_len + encodedName.length() + 30;
	//qDebug() << "bufferSize=" << bufferSize;
	char* buffer = new char[ bufferSize ];

	buffer[ 0 ] = 'P'; //local file header signature
	buffer[ 1 ] = 'K';
	buffer[ 2 ] = 3;
	buffer[ 3 ] = 4;

	buffer[ 4 ] = 0x14; // version needed to extract
	buffer[ 5 ] = 0;

	buffer[ 6 ] = 0; // general purpose bit flag
	buffer[ 7 ] = 0;

	buffer[ 8 ] = char(e->encoding()); // compression method
	buffer[ 9 ] = char(e->encoding() >> 8);

	transformToMsDos( e->datetime(), &buffer[ 10 ] );

	buffer[ 14 ] = 'C'; //dummy crc
	buffer[ 15 ] = 'R';
	buffer[ 16 ] = 'C';
	buffer[ 17 ] = 'q';

	buffer[ 18 ] = 'C'; //compressed file size
	buffer[ 19 ] = 'S';
	buffer[ 20 ] = 'I';
	buffer[ 21 ] = 'Z';

	buffer[ 22 ] = 'U'; //uncompressed file size
	buffer[ 23 ] = 'S';
	buffer[ 24 ] = 'I';
	buffer[ 25 ] = 'Z';

	buffer[ 26 ] = (uchar)(encodedName.length()); //fileName length
	buffer[ 27 ] = (uchar)(encodedName.length() >> 8);

	buffer[ 28 ] = (uchar)(extra_field_len); // extra field length
	buffer[ 29 ] = (uchar)(extra_field_len >> 8);

	// file name
	strncpy( buffer + 30, encodedName, encodedName.length() );

	// extra field
	if ( d->m_extraField == ModificationTime )
	{
		char *extfield = buffer + 30 + encodedName.length();
		// "Extended timestamp" header (0x5455)
		extfield[0] = 'U';
		extfield[1] = 'T';
		extfield[2] = 13; // data size
		extfield[3] = 0;
		extfield[4] = 1 | 2 | 4;	// contains mtime, atime, ctime

		extfield[5] = char(mtime);
		extfield[6] = char(mtime >> 8);
		extfield[7] = char(mtime >> 16);
		extfield[8] = char(mtime >> 24);

		extfield[9] = char(atime);
		extfield[10] = char(atime >> 8);
		extfield[11] = char(atime >> 16);
		extfield[12] = char(atime >> 24);

		extfield[13] = char(ctime);
		extfield[14] = char(ctime >> 8);
		extfield[15] = char(ctime >> 16);
		extfield[16] = char(ctime >> 24);
	}

	// Write header
	bool b = (device()->write( buffer, bufferSize ) == bufferSize );
	d->m_crc = 0L;
	delete[] buffer;

	Q_ASSERT( b );
	if (!b) {
		return false;
	}

	// Prepare device for writing the data
	// Either device() if no compression, or a KFilterDev to compress
	if ( d->m_compression == 0 ) {
		d->m_currentDev = device();
		return true;
	}

	d->m_currentDev = KFilterDev::device( device(), "application/x-gzip", false );
	Q_ASSERT( d->m_currentDev );
	if ( !d->m_currentDev ) {
		return false; // ouch
	}
	static_cast<KFilterDev *>(d->m_currentDev)->setSkipHeaders(); // Just zlib, not gzip

	b = d->m_currentDev->open( QIODevice::WriteOnly );
	Q_ASSERT( b );
	return b;
}

bool KZip::doFinishWriting( qint64 size )
{
	if ( d->m_currentFile->encoding() == 8 ) {
		// Finish
		(void)d->m_currentDev->write( 0, 0 );
		delete d->m_currentDev;
	}
	// If 0, d->m_currentDev was device() - don't delete ;)
	d->m_currentDev = 0L;

	Q_ASSERT( d->m_currentFile );
	//qDebug() << "fileName: " << d->m_currentFile->path();
	//qDebug() << "getpos (at): " << device()->pos();
	d->m_currentFile->setSize(size);
	int extra_field_len = 0;
	if ( d->m_extraField == ModificationTime )
		extra_field_len = 17;	// value also used in finishWriting()

	int csize = device()->pos() -
		d->m_currentFile->headerStart() - 30 -
		d->m_currentFile->path().length() - extra_field_len;
	d->m_currentFile->setCompressedSize(csize);
	//qDebug() << "usize: " << d->m_currentFile->size();
	//qDebug() << "csize: " << d->m_currentFile->compressedSize();
	//qDebug() << "headerstart: " << d->m_currentFile->headerStart();

	//qDebug() << "crc: " << d->m_crc;
	d->m_currentFile->setCRC32( d->m_crc );

	d->m_currentFile = 0L;

	// update saved offset for appending new files
	d->m_offset = device()->pos();
	return true;
}

bool KZip::doWriteSymLink(const QString &name, const QString &target,
						  const QString &user, const QString &group,
						  mode_t perm, time_t atime, time_t mtime, time_t ctime) {
  // reassure that symlink flag is set, otherwise strange things happen on
  // extraction
  perm |= S_IFLNK;
  Compression c = compression();
  setCompression(NoCompression);	// link targets are never compressed

  if (!doPrepareWriting(name, user, group, 0, perm, atime, mtime, ctime)) {
	qWarning() << "prepareWriting failed";
	setCompression(c);
	return false;
  }

  QByteArray symlink_target = QFile::encodeName(target);
  if (!writeData(symlink_target, symlink_target.length())) {
	qWarning() << "writeData failed";
	setCompression(c);
	return false;
  }

  if (!finishWriting(symlink_target.length())) {
	qWarning() << "finishWriting failed";
	setCompression(c);
	return false;
  }

  setCompression(c);
  return true;
}

void KZip::virtual_hook( int id, void* data )
{
	KArchive::virtual_hook( id, data );
}

bool KZip::writeData(const char * data, qint64 size)
{
	Q_ASSERT( d->m_currentFile );
	Q_ASSERT( d->m_currentDev );
	if (!d->m_currentFile || !d->m_currentDev) {
		return false;
	}

	// crc to be calculated over uncompressed stuff...
	// and they didn't mention it in their docs...
	d->m_crc = crc32(d->m_crc, (const Bytef *) data , size);

	qint64 written = d->m_currentDev->write( data, size );
	//qDebug() << "wrote" << size << "bytes.";
	return written == size;
}

void KZip::setCompression( Compression c )
{
	d->m_compression = ( c == NoCompression ) ? 0 : 8;
}

KZip::Compression KZip::compression() const
{
   return ( d->m_compression == 8 ) ? DeflateCompression : NoCompression;
}

void KZip::setExtraField( ExtraField ef )
{
	d->m_extraField = ef;
}

KZip::ExtraField KZip::extraField() const
{
	return d->m_extraField;
}

////////////////////////////////////////////////////////////////////////
////////////////////// KZipFileEntry////////////////////////////////////
////////////////////////////////////////////////////////////////////////
class KZipFileEntry::KZipFileEntryPrivate
{
public:
	KZipFileEntryPrivate()
	: crc(0),
	  compressedSize(0),
	  headerStart(0),
	  encoding(0)
	{}
	unsigned long crc;
	qint64        compressedSize;
	qint64        headerStart;
	int           encoding;
	QString       path;
};

KZipFileEntry::KZipFileEntry(KZip* zip, const QString& name, int access, int date,
							 const QString& user, const QString& group, const QString& symlink,
							 const QString& path, qint64 start, qint64 uncompressedSize,
							 int encoding, qint64 compressedSize)
 : KArchiveFile(zip, name, access, date, user, group, symlink, start, uncompressedSize ),
   d(new KZipFileEntryPrivate)
{
	d->path = path;
	d->encoding = encoding;
	d->compressedSize = compressedSize;
}

KZipFileEntry::~KZipFileEntry()
{
	delete d;
}

int KZipFileEntry::encoding() const
{
	return d->encoding;
}

qint64 KZipFileEntry::compressedSize() const
{
	return d->compressedSize;
}

void KZipFileEntry::setCompressedSize(qint64 compressedSize)
{
	d->compressedSize = compressedSize;
}

void KZipFileEntry::setHeaderStart(qint64 headerstart)
{
	d->headerStart = headerstart;
}

qint64 KZipFileEntry::headerStart() const
{
	return d->headerStart;
}

unsigned long KZipFileEntry::crc32() const
{
	return d->crc;
}

void KZipFileEntry::setCRC32(unsigned long crc32)
{
	d->crc=crc32;
}

const QString &KZipFileEntry::path() const
{
	return d->path;
}

QByteArray KZipFileEntry::data() const
{
	QIODevice* dev = createDevice();
	QByteArray arr;
	if ( dev ) {
		arr = dev->readAll();
		delete dev;
	}
	return arr;
}

QIODevice* KZipFileEntry::createDevice() const
{
	//qDebug() << "creating iodevice limited to pos=" << position() << ", csize=" << compressedSize();
	// Limit the reading to the appropriate part of the underlying device (e.g. file)
	KLimitedIODevice* limitedDev = new KLimitedIODevice( archive()->device(), position(), compressedSize() );
	if ( encoding() == 0 || compressedSize() == 0 ) // no compression (or even no data)
		return limitedDev;

	if ( encoding() == 8 )
	{
		// On top of that, create a device that uncompresses the zlib data
		QIODevice* filterDev = KFilterDev::device( limitedDev, "application/x-gzip" );
		if ( !filterDev )
			return 0L; // ouch
		static_cast<KFilterDev *>(filterDev)->setSkipHeaders(); // Just zlib, not gzip
		bool b = filterDev->open( QIODevice::ReadOnly );
		static_cast<void>(b); // annoying unused variable warning
		Q_ASSERT( b );
		return filterDev;
	}

	qWarning() << "This zip file contains files compressed with method"
			  << encoding() << ", this method is currently not supported by KZip,"
			  << "please use a command-line tool to handle this file.";
	return 0L;
}
