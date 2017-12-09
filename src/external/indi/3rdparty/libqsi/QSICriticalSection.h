/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * qsilib
 * 
 */

#ifndef _QSI_CRITICAL_SECTION_H_
#define _QSI_CRITICAL_SECTION_H_
#include <pthread.h>

class QSICriticalSection
{
private:  
    pthread_mutex_t m_CS; 
public: 
    QSICriticalSection() 
	{ 
		pthread_mutex_init(&m_CS, NULL);
	} 
	~QSICriticalSection() 
	{ 
		pthread_mutex_destroy( &m_CS );
	} 
	void Lock() 
	{ 
		pthread_mutex_lock( &m_CS );
	} 
	void Unlock() 
	{ 
		pthread_mutex_unlock( &m_CS );
	} 
};

#endif // _QSI_CRITICAL_SECTION_H_
