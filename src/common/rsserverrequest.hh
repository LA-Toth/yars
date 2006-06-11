/*  rsserverrequest.hh - parsing the request (server side)

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

#ifndef RSSERVERREQUEST__HH
#define RSSERVERREQUEST__HH

#include "rs/rsparser.hh"
#include <map>

namespace RS
{
    class ServerRequest {
	void parseHeader(const std::vector<std::string>& lines);
	int error;
	std::map<std::string, std::string> headers;
	std::string method;
	std::string url;
	std::string version;
    public:
	ServerRequest(const RSParser::Parser& parser);
	ServerRequest(const RSParser::Parser::strList& lines);
	std::string getMethod() const { return method; }
	std::string getURL() const { return url; }
	std::string getVersion() const {return version; }
	std::string getHeader(const std::string& name) {return getHeader(name, ""); }
	std::string getHeader(const std::string& name, const std::string& defaultValue);
	int getError() { return error; }
	bool hasError() { return error !=  -1; }
    };
}
#endif


/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
