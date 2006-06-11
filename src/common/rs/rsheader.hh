/*  rsheader.hh - generating HTTP-like header 

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

#ifndef RSHEADER__HH
#define RSHEADER__HH

#include <string>
#include <map>


namespace RS {
    
    class Header {
	std::map<std::sting, std::string> headers;
    protected:
	std::string firstLine;
    public:
	void setHeader(const std::string& name, std::string& value);
	const std::string getLines() const; 
	std::string getHeader(const std::string& name) {return getHeader(name, ""); }
	std::string getHeader(const std::string& name, const std::string& defaultValue);
    };
}

#endif

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
