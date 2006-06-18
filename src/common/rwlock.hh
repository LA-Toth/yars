/*  configlock.hh - rwlock.cpp - lock functions (in classes) for exclusive write, multiple read

    Copyright (C) 2006 Laszlo Attila Toth

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

#ifndef RWLOCK__HH
#define RWLOCK__HH

namespace RWLock {

  class SemaphoreSet;

  class Lock;
  /// multiple reader allowed
  class Reader
  {
    SemaphoreSet * _set;
    Reader(SemaphoreSet *set);
  public:
    ~Reader();
    friend class Lock;
  };
  
  
  /// only one writer allowed - higher priority
  class Writer
  {
    SemaphoreSet * _set;
    Writer(SemaphoreSet * set);
  public:
    ~Writer();
    friend class Lock;
  };

  class Lock {
    SemaphoreSet *_set;
  public:
    Lock();
    ~Lock();
    Reader getReader();
    Writer getWriter();
  };

  template <typename T>
  class LockProtected {
  public:
    Lock lock;
    T data;
  };
};
#endif
