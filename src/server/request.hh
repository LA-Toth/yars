/*  request.hh

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

#ifndef REQUEST__HH
#define REQUEST__HH

#include <string>
#include "rwlock.hh"

typedef struct RequestInfo
{
    std::string name;
    int from;
    int to;
    int max;
} RequestInfo;


#ifndef EXTERN_REQUEST
extern RWLock::LockProtected< std::vector<RequestInfo> > requests;
#else
RWLock::LockProtected< std::vector<RequestInfo> > requests;
#endif

#endif


/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
