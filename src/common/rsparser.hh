/*  rsparser.hh - HTTP-like header parser

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

#ifndef RSPARSER__HH
#define RSPARSER__HH

#include <string>
#include <map>

/// contains the buffer, etc.
class RSParserHelper;

class RSParser {
    RSParserHelper * helper;
    bool error;
    std::string name;
    int version; /// for example:  0x0103 represents 1.3
    int status;
    int statusText;
    std::map<std::string, std::string> lines;
public:
    RSParser(bool request);
    ~RSParser();
    /// must be called after each select (if FD_ISSET can be true)
    bool headerParser(int fd, fd_set *readfds, fd_set *writefds, fd_set *exceptfds);
    std::string hasError() const;

    std::string getHeader(const std::string& name) const; 
    int  getVersion() const {return version; } /// same as in HTTP (HTTP version)
    std::string getStatus() const { return status ; } /// same as in HTTP
    std::string getStatusText() const {return statusText; }/// same as in HTTP
    std::string getName() const { return name;} /// image name, in HTTP: requested URI
};

#endif


/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
