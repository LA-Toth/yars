/*  request_thread.hh - types for Incoming requests to restore an image...

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

#ifndef REQUEST_TRHEAD__HH
#define REQUEST_THREAD__HH


class ReceivedRequest
{
public:
    std::string file;
    uint32_t from;
    uint32_t to;
    unsigned char digest[16];
};



#ifdef HAS_REQUEST
# ifndef EXTERN_REQ
#   define EXTERN_REQ extern
# endif
EXTERN_REQ std::vector<ReceivedRequest> requests;
#endif

#endif


/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
