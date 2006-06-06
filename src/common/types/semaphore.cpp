/*  semaphore.cpp - Wrapper for the pthread semaphore (mutex) functions

    Copyright (C) 2006 Laszlo Attila Toth
    
    Original version: CommonC++2 (GPL),
    Copyright (C) 1999-2005 Open Source Telecom Corporation.


    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */


#include "types/semaphore.hh"


#include <sys/time.h>


timespec *getTimeout(struct timespec *spec, timeout_t timer)
{
        static  struct timespec myspec;

        if(spec == NULL)
                spec = &myspec;

#ifdef  PTHREAD_GET_EXPIRATION_NP
        struct timespec offset;

        offset.tv_sec = timer / 1000;
        offset.tv_nsec = (timer % 1000) * 1000000;
        pthread_get_expiration_np(&offset, sec);
#else
        struct timeval current;

        gettimeofday(&current, NULL);
        spec->tv_sec = current.tv_sec + ((timer + current.tv_usec / 1000) / 1000);
        spec->tv_nsec = ((current.tv_usec / 1000 + timer) % 1000) * 1000000;

#endif
        return spec;
}


Semaphore::Semaphore(unsigned resource)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&_mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    if(pthread_cond_init(&_cond, NULL))
	throw SemaphoreException();

    _count = resource;
    _waiters = 0;
}

Semaphore::~Semaphore()
{
    pthread_cond_destroy(&_cond);
    pthread_mutex_destroy(&_mutex);
}


bool Semaphore::wait(timeout_t timeout)
{
    struct timespec ts;
    bool flag = true;
    int rc;
    pthread_mutex_lock(&_mutex);
    ++_waiters;

    if(_count)
	goto exiting;

    if(!timeout)
        {
	    while(_count == 0)
		pthread_cond_wait(&_cond, &_mutex);
	    goto exiting;
        }

    getTimeout(&ts, timeout);
    rc = pthread_cond_timedwait(&_cond, &_mutex, &ts);
    if(rc || _count == 0)
	flag = false;

 exiting:
    --_waiters;
    if(_count)
	--_count;
    pthread_mutex_unlock(&_mutex);
    return flag;
}

void Semaphore::post(void)
{
    pthread_mutex_lock(&_mutex);
    if(_waiters > 0)
	pthread_cond_signal(&_cond);
    ++_count;
    pthread_mutex_unlock(&_mutex);
}

int Semaphore::getValue(void)
{
    int value;
    
    pthread_mutex_lock(&_mutex);
    value = _count;
    pthread_mutex_unlock(&_mutex);
    return value;
}

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
