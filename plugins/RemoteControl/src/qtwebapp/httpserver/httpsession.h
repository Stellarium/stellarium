/**
  @file
  @author Stefan Frings
*/

#ifndef HTTPSESSION_H
#define HTTPSESSION_H

#include <QByteArray>
#include <QVariant>
#include <QReadWriteLock>
#include "httpglobal.h"

/**
  This class stores data for a single HTTP session.
  A session can store any number of key/value pairs. This class uses implicit
  sharing for read and write access. This class is thread safe.
  @see HttpSessionStore should be used to create and get instances of this class.
*/

class DECLSPEC HttpSession {

public:

    /**
      Constructor.
      @param canStore The session can store data, if this parameter is true.
      Otherwise all calls to set() and remove() do not have any effect.
     */
    HttpSession(bool canStore=false);

    /**
      Copy constructor. Creates another HttpSession object that shares the
      data of the other object.
    */
    HttpSession(const HttpSession& other);

    /**
      Copy operator. Detaches from the current shared data and attaches to
      the data of the other object.
    */
    HttpSession& operator= (const HttpSession& other);


    /**
      Destructor. Detaches from the shared data.
    */
    virtual ~HttpSession();

    /** Get the unique ID of this session. This method is thread safe. */
    QByteArray getId() const;

    /**
      Null sessions cannot store data. All calls to set() and remove() 
      do not have any effect.This method is thread safe.
    */
    bool isNull() const;

    /** Set a value. This method is thread safe. */
    void set(const QByteArray& key, const QVariant& value);

    /** Remove a value. This method is thread safe. */
    void remove(const QByteArray& key);

    /** Get a value. This method is thread safe. */
    QVariant get(const QByteArray& key) const;

    /** Check if a key exists. This method is thread safe. */
    bool contains(const QByteArray& key) const;

    /**
      Get a copy of all data stored in this session.
      Changes to the session do not affect the copy and vice versa.
      This method is thread safe.
    */
    QMap<QByteArray,QVariant> getAll() const;

    /**
      Get the timestamp of last access. That is the time when the last
      HttpSessionStore::getSession() has been called.
      This method is thread safe.
    */
    qint64 getLastAccess() const;

    /**
      Set the timestamp of last access, to renew the timeout period.
      Called by  HttpSessionStore::getSession().
      This method is thread safe.
    */
    void setLastAccess();

private:

    struct HttpSessionData {

        /** Unique ID */
        QByteArray id;

        /** Timestamp of last access, set by the HttpSessionStore */
        qint64 lastAccess;

        /** Reference counter */
        int refCount;

        /** Used to synchronize threads */
        QReadWriteLock lock;

        /** Storage for the key/value pairs; */
        QMap<QByteArray,QVariant> values;

    };

    /** Pointer to the shared data. */
    HttpSessionData* dataPtr;

};

#endif // HTTPSESSION_H
