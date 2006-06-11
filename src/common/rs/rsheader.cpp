/*  rsheader.cpp - generating HTTP-like header 

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

#include <string>
#include <map>


void RS::Header::setHeader(const std::string& name, const std::string& value)
{
    headers[name] = value;    
}
const std::string getLines() const
{
    std::string retVal = firstLine;
    if (firstLine.size()) 
	retVal += "\r\n";
    std::map<std::string, std::string>::const_iterator p = headers.begin();
    for (;p !=  headers.end(); ++p)
	retVal += p->first + ": " + p->second + "\r\n";
    return retVal + "\r\n";
}
std::string getHeader(const std::string& name, const std::string& defaultValue)
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
