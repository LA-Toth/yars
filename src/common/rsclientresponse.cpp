/*  rsclientresponse.hh - parsing the request (client side)

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


#include "rs/rsserverrequest.hh"
#include "helpers.hh"


RS::ClientResponse::ClientResponse(const RSParser::Parser& parser) :  error(-1)
{
    parseHeader(parser.getLines());
}
RS::ClientResponse::ClientResponse(const RSParser::Parser::strList& lines) :  error(-1)
{
    parseHeader(lines);
}
void RS::ClientResponse::parseHeader(const RSParser::Parser::strList& lines)
{
    using namespace Helpers;
    
    if (lines.size() < 3) {
	error = 3;
	return;
    }
    std::vector<std::string> strList;
    splitByEx(' ', lines[0], strList);
    
    // searching for errors
    if (strList.size() != 2 ||
	strList[0] != "RS/1.0") {
	error = 0;
	return;
    }
    version.assign(strList[0].begin() + 3, strList[0].end());
    statusText = strList[1];
    splitByEx(' ', statusText, strList);
    if (strList.size() != 2) {
	error = 0;
	return;
    }
    status = strList[0];
    statusText = strList[1];
    
    for (size_t i = 1; i!= lines.size(); ++i) {
	splitByEx(':', lines[i], strList);

	// searching for errors
	if (strList.size() != 2) {
	    error = i;
	    return;
	}
	
	strList[0] = trim(strList[0]);
	strList[1] = trim(strList[1]);
	if (strList[0].size() == 0 ||
	    strList[1].size() == 0) {
	    error = i;
	    return;
	}
	headers[strList[0]] = strList[1];
    }
}

std::string RS::ClientResponse::getHeader(const std::string& name, 
					 const std::string& defaultValue)
{
    std::map<std::string, std::string>::const_iterator p = headers.find(name);
    if (p != headers.end())
	return p->second;
    else
	return defaultValue;
}


/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
