/* This file is part of the KDE libraries
   Copyright (C) 2000-2005 David Faure <faure@kde.org>
   Copyright (C) 2003 Leo Savernik <l.savernik@aon.at>

   Moved from ktar.h by Roberto Teixeira <maragato@kde.org>

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
#ifndef KARCHIVE_H
#define KARCHIVE_H

#include <sys/types.h>

#include <QtCore/QDate>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QHash>

class KArchiveDirectory;
class KArchiveFile;

class KArchivePrivate;
/**
 * KArchive is a base class for reading and writing archives.
 * @short generic class for reading/writing archives
 * @author David Faure <faure@kde.org>
 */
class KArchive
{
protected:
    /**
     * Base constructor (protected since this is a pure virtual class).
     * @param fileName is a local path (e.g. "/tmp/myfile.ext"),
     * from which the archive will be read from, or into which the archive
     * will be written, depending on the mode given to open().
     */
    KArchive( const QString& fileName );

    /**
     * Base constructor (protected since this is a pure virtual class).
     * @param dev the I/O device where the archive reads its data
     * Note that this can be a file, but also a data buffer, a compression filter, etc.
     * For a file in writing mode it is better to use the other constructor
     * though, to benefit from the use of KSaveFile when saving.
     */
    KArchive( QIODevice * dev );

public:
    virtual ~KArchive();

    /**
     * Opens the archive for reading or writing.
     * Inherited classes might want to reimplement openArchive instead.
     * @param mode may be QIODevice::ReadOnly or QIODevice::WriteOnly
     * @see close
     */
    virtual bool open( QIODevice::OpenMode mode );

    /**
     * Closes the archive.
     * Inherited classes might want to reimplement closeArchive instead.
     *
     * @return true if close succeeded without problems
     * @see open
     */
    virtual bool close();

    /**
     * Checks whether the archive is open.
     * @return true if the archive is opened
     */
    bool isOpen() const;

    /**
     * Returns the mode in which the archive was opened
     * @return the mode in which the archive was opened (QIODevice::ReadOnly or QIODevice::WriteOnly)
     * @see open()
     */
    QIODevice::OpenMode mode() const;

    /**
     * The underlying device.
     * @return the underlying device.
     */
    QIODevice * device() const;

    /**
     * The name of the archive file, as passed to the constructor that takes a
     * fileName, or an empty string if you used the QIODevice constructor.
     * @return the name of the file, or QString() if unknown
     */
    QString fileName() const;

    /**
     * If an archive is opened for reading, then the contents
     * of the archive can be accessed via this function.
     * @return the directory of the archive
     */
    const KArchiveDirectory* directory() const;

    /**
     * Writes a local file into the archive. The main difference with writeFile,
     * is that this method minimizes memory usage, by not loading the whole file
     * into memory in one go.
     *
     * If @p fileName is a symbolic link, it will be written as is, i. e.
     * it will not be resolved before.
     * @param fileName full path to an existing local file, to be added to the archive.
     * @param destName the resulting name (or relative path) of the file in the archive.
     */
    bool addLocalFile( const QString& fileName, const QString& destName );

    /**
     * Writes a local directory into the archive, including all its contents, recursively.
     * Calls addLocalFile for each file to be added.
     *
     * Since KDE 3.2 it will also add a @p path that is a symbolic link to a
     * directory. The symbolic link will be dereferenced and the content of the
     * directory it is pointing to added recursively. However, symbolic links
     * *under* @p path will be stored as is.
     * @param path full path to an existing local directory, to be added to the archive.
     * @param destName the resulting name (or relative path) of the file in the archive.
     */
    bool addLocalDirectory( const QString& path, const QString& destName );

    enum { UnknownTime = static_cast<time_t>( -1 ) };

    /**
     * If an archive is opened for writing then you can add new directories
     * using this function. KArchive won't write one directory twice.
     *
     * This method also allows some file metadata to be set.
     * However, depending on the archive type not all metadata might be regarded.
     *
     * @param name the name of the directory
     * @param user the user that owns the directory
     * @param group the group that owns the directory
     * @param perm permissions of the directory
     * @param atime time the file was last accessed
     * @param mtime modification time of the file
     * @param ctime time of last status change
     */
    virtual bool writeDir( const QString& name, const QString& user, const QString& group,
                           mode_t perm = 040755, time_t atime = UnknownTime,
                           time_t mtime = UnknownTime, time_t ctime = UnknownTime );

    /**
     * Writes a symbolic link to the archive if supported.
     * The archive must be opened for writing.
     *
     * @param name name of symbolic link
     * @param target target of symbolic link
     * @param user the user that owns the directory
     * @param group the group that owns the directory
     * @param perm permissions of the directory
     * @param atime time the file was last accessed
     * @param mtime modification time of the file
     * @param ctime time of last status change
     */
    virtual bool writeSymLink(const QString &name, const QString &target,
                              const QString &user, const QString &group,
                              mode_t perm = 0120755, time_t atime = UnknownTime,
                              time_t mtime = UnknownTime, time_t ctime = UnknownTime );

    /**
     * If an archive is opened for writing then you can add a new file
     * using this function. If the file name is for example "mydir/test1" then
     * the directory "mydir" is automatically appended first if that did not
     * happen yet.
     *
     * This method also allows some file metadata to be
     * set. However, depending on the archive type not all metadata might be
     * regarded.
     * @param name the name of the file
     * @param user the user that owns the file
     * @param group the group that owns the file
     * @param data the data to write (@p size bytes)
     * @param size the size of the file
     * @param perm permissions of the file
     * @param atime time the file was last accessed
     * @param mtime modification time of the file
     * @param ctime time of last status change
     */
    virtual bool writeFile( const QString& name, const QString& user, const QString& group,
                            const char* data, qint64 size,
                            mode_t perm = 0100644, time_t atime = UnknownTime,
                            time_t mtime = UnknownTime, time_t ctime = UnknownTime );

    /**
     * Here's another way of writing a file into an archive:
     * Call prepareWriting(), then call writeData()
     * as many times as wanted then call finishWriting( totalSize ).
     * For tar.gz files, you need to know the size before hand, it is needed in the header!
     * For zip files, size isn't used.
     *
     * This method also allows some file metadata to be
     * set. However, depending on the archive type not all metadata might be
     * regarded.
     * @param name the name of the file
     * @param user the user that owns the file
     * @param group the group that owns the file
     * @param size the size of the file
     * @param perm permissions of the file
     * @param atime time the file was last accessed
     * @param mtime modification time of the file
     * @param ctime time of last status change
     */
    virtual bool prepareWriting( const QString& name, const QString& user,
                                 const QString& group, qint64 size,
                                 mode_t perm = 0100644, time_t atime = UnknownTime,
                                 time_t mtime = UnknownTime, time_t ctime = UnknownTime );

    /**
     * Write data into the current file - to be called after calling prepareWriting
     */
    virtual bool writeData( const char* data, qint64 size );

    /**
     * Call finishWriting after writing the data.
     * @param size the size of the file
     * @see prepareWriting()
     */
    virtual bool finishWriting( qint64 size );

protected:
    /**
     * Opens an archive for reading or writing.
     * Called by open.
     * @param mode may be QIODevice::ReadOnly or QIODevice::WriteOnly
     */
    virtual bool openArchive( QIODevice::OpenMode mode ) = 0;

    /**
     * Closes the archive.
     * Called by close.
     */
    virtual bool closeArchive() = 0;

    /**
     * Retrieves or create the root directory.
     * The default implementation assumes that openArchive() did the parsing,
     * so it creates a dummy rootdir if none was set (write mode, or no '/' in the archive).
     * Reimplement this to provide parsing/listing on demand.
     * @return the root directory
     */
    virtual KArchiveDirectory* rootDir();

    /**
     * Write a directory to the archive.
     * This virtual method must be implemented by subclasses.
     *
     * Depending on the archive type not all metadata might be used.
     *
     * @param name the name of the directory
     * @param user the user that owns the directory
     * @param group the group that owns the directory
     * @param perm permissions of the directory. Use 040755 if you don't have any other information.
     * @param atime time the file was last accessed
     * @param mtime modification time of the file
     * @param ctime time of last status change
     * @see writeDir
     */
    virtual bool doWriteDir( const QString& name, const QString& user, const QString& group,
                             mode_t perm, time_t atime, time_t mtime, time_t ctime ) = 0;

    /**
     * Writes a symbolic link to the archive.
     * This virtual method must be implemented by subclasses.
     *
     * @param name name of symbolic link
     * @param target target of symbolic link
     * @param user the user that owns the directory
     * @param group the group that owns the directory
     * @param perm permissions of the directory
     * @param atime time the file was last accessed
     * @param mtime modification time of the file
     * @param ctime time of last status change
     * @see writeSymLink
     */
    virtual bool doWriteSymLink(const QString &name, const QString &target,
                                const QString &user, const QString &group,
                                mode_t perm, time_t atime, time_t mtime, time_t ctime) = 0;

    /**
     * This virtual method must be implemented by subclasses.
     *
     * Depending on the archive type not all metadata might be used.
     *
     * @param name the name of the file
     * @param user the user that owns the file
     * @param group the group that owns the file
     * @param size the size of the file
     * @param perm permissions of the file. Use 0100644 if you don't have any more specific permissions to set.
     * @param atime time the file was last accessed
     * @param mtime modification time of the file
     * @param ctime time of last status change
     * @see prepareWriting
     */
    virtual bool doPrepareWriting( const QString& name, const QString& user,
                                   const QString& group, qint64 size, mode_t perm,
                                   time_t atime, time_t mtime, time_t ctime ) = 0;

    /**
     * Called after writing the data.
     * This virtual method must be implemented by subclasses.
     *
     * @param size the size of the file
     * @see finishWriting()
     */
    virtual bool doFinishWriting( qint64 size ) = 0;

    /**
     * Ensures that @p path exists, create otherwise.
     * This handles e.g. tar files missing directory entries, like mico-2.3.0.tar.gz :)
     * @param path the path of the directory
     * @return the directory with the given @p path
     */
    KArchiveDirectory * findOrCreate( const QString & path );

    /**
     * Can be reimplemented in order to change the creation of the device
     * (when using the fileName constructor). By default this method uses
     * KSaveFile when saving, and a simple QFile on reading.
     * This method is called by open().
     */
    virtual bool createDevice( QIODevice::OpenMode mode );

    /**
     * Can be called by derived classes in order to set the underlying device.
     * Note that KArchive will -not- own the device, it must be deleted by the derived class.
     */
    void setDevice( QIODevice *dev );

    /**
     * Derived classes call setRootDir from openArchive,
     * to set the root directory after parsing an existing archive.
     */
    void setRootDir( KArchiveDirectory *rootDir );

protected:
    virtual void virtual_hook( int id, void* data );
private:
    KArchivePrivate* const d;
};

class KArchiveEntryPrivate;
/**
 * A base class for entries in an KArchive.
 * @short Base class for the archive-file's directory structure.
 *
 * @see KArchiveFile
 * @see KArchiveDirectory
 */
class KArchiveEntry
{
public:
    /**
     * Creates a new entry.
     * @param archive the entries archive
     * @param name the name of the entry
     * @param access the permissions in unix format
     * @param date the date (in seconds since 1970)
     * @param user the user that owns the entry
     * @param group the group that owns the entry
     * @param symlink the symlink, or QString()
     */
    KArchiveEntry( KArchive* archive, const QString& name, int access, int date,
                   const QString& user, const QString& group,
                   const QString& symlink );

    virtual ~KArchiveEntry();

    /**
     * Creation date of the file.
     * @return the creation date
     */
    QDateTime datetime() const;

    /**
     * Creation date of the file.
     * @return the creation date in seconds since 1970
     */
    int date() const;

    /**
     * Name of the file without path.
     * @return the file name without path
     */
    QString name() const;
    /**
     * The permissions and mode flags as returned by the stat() function
     * in st_mode.
     * @return the permissions
     */
    mode_t permissions() const;
    /**
     * User who created the file.
     * @return the owner of the file
     */
    QString user() const;
    /**
     * Group of the user who created the file.
     * @return the group of the file
     */
    QString group() const;

    /**
     * Symlink if there is one.
     * @return the symlink, or QString()
     */
    QString symLinkTarget() const;

    /**
     * Checks whether the entry is a file.
     * @return true if this entry is a file
     */
    virtual bool isFile() const;

    /**
     * Checks whether the entry is a directory.
     * @return true if this entry is a directory
     */
    virtual bool isDirectory() const;

protected:
    KArchive* archive() const;

protected:
    virtual void virtual_hook( int id, void* data );
private:
    KArchiveEntryPrivate* const d;
};

class KArchiveFilePrivate;
/**
 * Represents a file entry in a KArchive.
 * @short A file in an archive.
 *
 * @see KArchive
 * @see KArchiveDirectory
 */
class KArchiveFile : public KArchiveEntry
{
public:
    /**
     * Creates a new file entry. Do not call this, KArchive takes care of it.
     * @param archive the entries archive
     * @param name the name of the entry
     * @param access the permissions in unix format
     * @param date the date (in seconds since 1970)
     * @param user the user that owns the entry
     * @param group the group that owns the entry
     * @param symlink the symlink, or QString()
     * @param pos the position of the file in the directory
     * @param size the size of the file
     */
    KArchiveFile( KArchive* archive, const QString& name, int access, int date,
                  const QString& user, const QString& group, const QString &symlink,
                  qint64 pos, qint64 size );

    /**
     * Destructor. Do not call this, KArchive takes care of it.
     */
    virtual ~KArchiveFile();

    /**
     * Position of the data in the [uncompressed] archive.
     * @return the position of the file
     */
    qint64 position() const;
    /**
     * Size of the data.
     * @return the size of the file
     */
    qint64 size() const;
    /**
     * Set size of data, usually after writing the file.
     * @param s the new size of the file
     */
    void setSize( qint64 s );

    /**
     * Returns the data of the file.
     * Call data() with care (only once per file), this data isn't cached.
     * @return the content of this file.
     */
    virtual QByteArray data() const;

    /**
     * This method returns QIODevice (internal class: KLimitedIODevice)
     * on top of the underlying QIODevice. This is obviously for reading only.
     *
     * WARNING: Note that the ownership of the device is being transferred to the caller,
     * who will have to delete it.
     *
     * The returned device auto-opens (in readonly mode), no need to open it.
     * @return the QIODevice of the file
     */
    virtual QIODevice *createDevice() const;

    /**
     * Checks whether this entry is a file.
     * @return true, since this entry is a file
     */
    virtual bool isFile() const;

    /**
     * Extracts the file to the directory @p dest
     * @param dest the directory to extract to
     */
    void copyTo(const QString& dest) const;

protected:
    virtual void virtual_hook( int id, void* data );
private:
    KArchiveFilePrivate* const d;
};

class KArchiveDirectoryPrivate;
/**
 * Represents a directory entry in a KArchive.
 * @short A directory in an archive.
 *
 * @see KArchive
 * @see KArchiveFile
 */
class KArchiveDirectory : public KArchiveEntry
{
public:
    /**
     * Creates a new directory entry.
     * @param archive the entries archive
     * @param name the name of the entry
     * @param access the permissions in unix format
     * @param date the date (in seconds since 1970)
     * @param user the user that owns the entry
     * @param group the group that owns the entry
     * @param symlink the symlink, or QString()
     */
    KArchiveDirectory( KArchive* archive, const QString& name, int access, int date,
                   const QString& user, const QString& group,
                   const QString& symlink);

    virtual ~KArchiveDirectory();

    /**
     * Returns a list of sub-entries.
     * Note that the list is not sorted, it's even in random order (due to using a hashtable).
     * Use sort() on the result to sort the list by filename.
     *
     * @return the names of all entries in this directory (filenames, no path).
     */
    QStringList entries() const;
    /**
     * Returns the entry with the given name.
     * @param name may be "test1", "mydir/test3", "mydir/mysubdir/test3", etc.
     * @return a pointer to the entry in the directory.
     */
    const KArchiveEntry* entry( const QString& name ) const;

    /**
     * @internal
     * Adds a new entry to the directory.
     */
    void addEntry( KArchiveEntry* );

    /**
     * Checks whether this entry is a directory.
     * @return true, since this entry is a directory
     */
    virtual bool isDirectory() const;

    /**
     * Extracts all entries in this archive directory to the directory
     * @p dest.
     * @param dest the directory to extract to
     * @param recursive if set to true, subdirectories are extracted as well
     */
     void copyTo(const QString& dest, bool recursive = true) const;

protected:
    virtual void virtual_hook( int id, void* data );
private:
    KArchiveDirectoryPrivate* const d;
};

#endif
