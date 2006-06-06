/*  semaphore.hh - Wrapper for the pthread semaphore (mutex) functions

    Copyright (C) 2006, Laszlo Attila Toth

    Original version is in CommonC++2 (GPL):
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


#ifndef SEMAPHORE__HH
#define SEMAPHORE__HH

#include <pthread.h>


typedef unsigned long   timeout_t;

class SemaphoreException {};

/**
 *     @author David Sugar <dyfet@ostel.com>
 */
class Semaphore
{
private:
    unsigned _count, _waiters;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
    
public:
public:
    /**
     * The initial value of the semaphore can be specified.  An initial
     * value is often used When used to lock a finite resource or to
     * specify the maximum number of thread instances that can access a
     * specified resource.
     *
     * @param resource specify initial resource count or 1 default.
     */
    Semaphore(unsigned resource = 1);
  
    /**
     * Destroying a semaphore also removes any system resources
     * associated with it.  If a semaphore has threads currently waiting
     * on it, those threads will all continue when a semaphore is
     * destroyed.
     */
    virtual ~Semaphore();
  

    /**
     * Wait is used to keep a thread held until the semaphore counter
     * is greater than 0.  If the current thread is held, then another
     * thread must increment the semaphore.  Once the thread is accepted,
     * the semaphore is automatically decremented, and the thread
     * continues execution.
     *
     * The pthread semaphore object does not support a timed "wait", and
     * hence to maintain consistancy, neither the posix nor win32 source
     * trees support "timed" semaphore objects.
     *
     * @return false if timed out
     * @param timeout period in milliseconds to wait
     * @see #post
     */
    bool wait(timeout_t timeout = 0);
    
    void P() { wait(); }
    
    /**
     * Posting to a semaphore increments its current value and releases
     * the first thread waiting for the semaphore if it is currently at
     * 0.  Interestingly, there is no support to increment a semaphore by
     * any value greater than 1 to release multiple waiting threads in
     * either pthread or the win32 API.  Hence, if one wants to release
     * a semaphore to enable multiple threads to execute, one must perform
     * multiple post operations.
     *
     * @see #wait
     */
    void post(void);
    
    void V() { post(); };
    
    // FIXME: how implement getValue for posix compatibility ?
    /**
     * Get the current value of a semaphore.
     *
     * @return current value.
     */
    int getValue(void);  
};

/**
 * The SemaphoreLock class is used to protect a section of code through
 * a semaphore so that only x instances of the member function may
 * execute concurrently.
 *
 * A common use is
 *
 * void func_to_protect()
 * {
 *   SemaphoreLock lock(semaphore);
 *   ... operation ...
 * }
 *
 * NOTE: do not declare variable as "SemaohoreLock (semaphore)", the
 * mutex will be released at statement end.
 *
 * @author David Sugar <dyfet@gnu.org>
 * @short Semaphore automatic locker for protected access.
 */
class SemaphoreLock
{
private:
    Semaphore& sem;

public:
    /**
     * Wait for the semaphore
     */
    SemaphoreLock( Semaphore& _sem ) : sem( _sem )
    { sem.wait(); }
    /**
     * Post the semaphore automatically
     */
    // this should be not-virtual
    ~SemaphoreLock()
    { sem.post(); }
};


#endif
/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
