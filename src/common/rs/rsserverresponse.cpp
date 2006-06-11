/*  rsserverresponse.cpp - Response of the server

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

#include "rs/rsserverresponse.hh"

void RS::ServerResponse::ServerResponse() : Header(), version("1.0") {
}

void RS::ServerResponse::setVersion(const std::string& version)
{
    this->version = version;
    this->firstLine = "RS/" + this->version + " " + this->status + " " + this->statusText;
}

void RS::ServerResponse::setStatus(const std::string& status)
{
    this->status = status;
    this->firstLine = "RS/" + this->version + " " + this->status + " " + this->statusText;
}

void RS::ServerResponse::setStatus(const std::string& statusText)
{
    this->statusText = statusText;
    this->firstLine = "RS/" + this->version + " " + this->status + " " + this->statusText;
}

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
