/*  helpers.cpp - Small functions

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


#include "helpers.hh"
  
void Helpers::split(const std::string& buf,std::vector<std::string>& vec)
{
    vec.clear();
    if (buf.size() == 0) return;
    std::string::const_iterator p1=buf.begin(),p2;
    std::string str;
    
    while(1) {
	while(p1 != buf.end() && (*p1==' ' || *p1=='\t'))
	    ++p1; // first non-whitespace char
	if (p1 == buf.end()) break;

	p2=p1;
	while (p2 != buf.end() && *p2 != ' '&& *p2 != '\t')  {  // a valid name
	    ++p2; 
	}
	str.assign(p1,p2);
	vec.push_back(str);
	p1 = p2;
    }
}
void Helpers::splitBy(const char delimiter, const std::string& buf,std::vector<std::string>& vec) 
{
    vec.clear();
    if (buf.size() == 0) return;
    std::string::const_iterator p1=buf.begin(),p2;
    std::string str;
    
    while(1) {
	p2=p1; 
	while (p2 != buf.end() && *p2 != delimiter)  {  // a valid name
	    ++p2; 
	}
	str.assign(p1,p2);
	vec.push_back(str);

	if (p2 == buf.end()) break;
	p1 = p2 +1;
    }
}
void Helpers::splitByEx(const char delimiter, const std::string& buf,std::vector<std::string>& vec) 
{
    vec.clear();
    if (buf.size() == 0) return;
    std::string::const_iterator p1=buf.begin(),p2;
    std::string str;
    
    p2=p1; 
    while (p2 != buf.end() && *p2 != delimiter)  {  // a valid name
	++p2; 
    }
    str.assign(p1,p2);
    vec.push_back(str);
    
    if (p2 == buf.end()) return;
    str.assign(p2 + 1, buf.end());
    vec.push_back(str);
    
}

std::string Helpers::trimEx(const std::string& str)
{
    std::string str2;
    std::string::const_iterator p1 = str.begin(), p2;	
    while (p1 != str.end() && *p1 == ' ') ++p1;
    p2 = p1;
    while (p2 != str.end() && *p2 != ' ') ++p2;
    str2.assign(p1, p2);
    return str2;
}

std::string Helpers::trimLeft(const std::string& str)
{
    std::string str2;
    std::string::const_iterator p1 = str.begin(), p2;	
    while (p1 != str.end() && *p1 == ' ') ++p1;
    p2 = str.end();
    str2.assign(p1, p2);
    return str2;
}


/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
