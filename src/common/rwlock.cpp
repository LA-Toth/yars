/*  rwlock.cpp - lock functions (in classes) for exclusive write, multiple read

    used by path expressions:
    path requestread, requestwrite end;
    path { openwrite }, openread end;
    path { openread2; read }, write end;
    requestread: begin openread end;
    openread: begin openread2 end;
    openread2: begin end;
    requestwrite: begin openwrite end;
    openwrite: begin write end;
    READ: begin reuqestread; read end;
    WRITE: begin requestwrite end;

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

#include "types/semaphore.hh"
#include "rwlock.hh"

namespace RWLock {
  class SemaphoreSet {
    Semaphore s1, s2, s3, s4, s5;
    int c1, c2;
  public:
    void requestRead() {    
      s1.P();
      OpenRead();
      s1.V();
    }
    void requestWrite() {
      s1.P();
      OpenWrite();
      s1.V();
    }

    void endWrite() {
      s4.V();
      VV(c1, s3, s2);
    }

    void endRead()
    {
      VV (c2, s5, s4);
    }
  private:
    void OpenRead() {
      s2.P();
      OpenRead2();
      s2.V();
    }

    void OpenWrite() {
      PP(c1, s3, s2);
      // virtual code
      //write();
      //VV(c1, s3, s2);
    
      // real code
      s4.P();
    }
  
    void OpenRead2() {
      PP(c2, s5, s4);
      // s6.V();

      // code of read():
      // s6.P();
      // body of read
      // VV (c2, s5, s4);
    }
  
    void PP(int &counter, Semaphore &s2, Semaphore &s1) {
      s2.P();
      ++counter;
      if (counter == 1 ) s1.P();
      s2.V();
    }
    void VV(int &counter, Semaphore &s2, Semaphore &s1) {
      s2.P();
      --counter;
      if (counter == 0 ) s1.V();
      s2.V();
    }
  };
};

RWLock::Lock::Lock()
{
  this->_set = new SemaphoreSet; 
}

RWLock::Lock::~Lock()
{
  delete this->_set;
}

RWLock::Reader RWLock::Lock::getReader()
{
  return Reader(_set);
}

RWLock::Writer RWLock::Lock::getWriter()
{
  return Writer(_set);
}

RWLock::Reader::Reader(SemaphoreSet * set)
{
  this->_set = set;
  _set->requestRead();
}

RWLock::Reader::~Reader()
{
  _set->endRead();
}

RWLock::Writer::Writer(SemaphoreSet * set)
{
  this->_set = set;
  _set->requestWrite();
}

RWLock::Writer::~Writer()
{
  _set->endWrite();
}
