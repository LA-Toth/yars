/*  rsclientrequest.cpp - request of the client

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

#include "rs/rsclientrequest.hh"

RS::ClientRequest::ClientRequest() : Header(), version("1.0") {
}

void RS::ClientRequest::setVersion(const std::string& version)
{
    this->version = version;
    this->firstLine ="GET " + this->imageName +  " RS/" + this->version;
}

void RS::ClientRequest::setImageName(const std::string& imageName)
{
    this->imageName= imageName;
    this->firstLine ="GET " + this->imageName +  " RS/" + this->version;
}


/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
