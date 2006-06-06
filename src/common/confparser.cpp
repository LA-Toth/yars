/* confparser.cpp - configuration file parser
   
   Copyright (C) 2004,2006 Laszlo Attila Toth <panther@elte.hu> 
   
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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include "confparser.hh"
#include <fstream>

ConfParser::ConfigParser::ConfigParser():filename("") {
    data.clear();
}
ConfParser::ConfigParser::ConfigParser(const std::string& file){
    parse(file);
}

ConfParser::ConfigParser::ConfigParser(const char* file){
    parse(file);
}
bool ConfParser::ConfigParser::parse(const std::string& file)
{
    filename = file;
    return parse();
}
bool ConfParser::ConfigParser::parse(const char* file){
    filename = file;
    return parse();
}
std::string ConfParser::ConfigParser::getValue(const char* name) const
{
    std::map<std::string,std::string>::const_iterator p = data.find(name);
    if (p == data.end()) return "";
    else return p->second;
}
std::string ConfParser::ConfigParser::getValue(const std::string& name) const
{
    std::map<std::string,std::string>::const_iterator p = data.find(name);
    if (p == data.end()) return "";
    else return p->second;
}
std::string ConfParser::ConfigParser::getValueDefault(const std::string& name, const std::string& defaultValue) const
{
    std::map<std::string,std::string>::const_iterator p = data.find(name);
    if (p == data.end()) return defaultValue;
    else return p->second;
}

char * ConfParser::ConfigParser::getValue(const char* name,char* buffer,const int maxlen) const
{
    std::string x=name;
    std::map<std::string,std::string>::const_iterator p = data.find(x);
    if (p != data.end()) {
	if (buffer) {
	    p->second.copy(buffer,maxlen-1);
	    buffer[p->second.length()>=(unsigned)maxlen?maxlen:p->second.length()]=0;
	}	    
    }
    else if (buffer && maxlen >0)
	buffer[0]=0;
    return buffer;
}
char * ConfParser::ConfigParser::getValue(const std::string& name,char* buffer,const int maxlen) const
{
    std::map<std::string,std::string>::const_iterator p = data.find(name);
    if (p != data.end()) {
	if (buffer) {
	    p->second.copy(buffer,maxlen-1);
	    buffer[p->second.length()>=(unsigned)maxlen?maxlen:p->second.length()]=0;
	}	    
    }
    else if (buffer && maxlen >0)
	buffer[0]=0;
    return buffer;
}

bool ConfParser::ConfigParser::parse() {
    std::ifstream f(filename.c_str());
    if (!f) return false;
    data.clear();
    std::string buf;
    while(std::getline(f,buf)) {
	//parsing this line
	std::string::iterator p1=buf.begin(),p2;
	while(p1 != buf.end() && (*p1==' ' || *p1=='\t'))++p1; // first non-whitespace char
	
	if (p1==buf.end() || *p1 =='#') continue; // EOL or comment -ignore whole line
	
	p2=p1;
	while (*p2 != ' '&& *p2 != '\t' && *p2!= '=')  { // name. EOL not allowed
	    if (p2 == buf.end()) return false; //throw exFileParseErr(buf);
	    ++p2; 
	}

	std::string name(p1,p2);

	while ((*p2==' ' || *p2 == '=' || *p2 == '\t')) {  // spaces between name and value. EOL not allowed
	    if (p2 == buf.end()) return false; //throw exFileParseErr(buf);
	    ++p2;
	}
	if (p2 == buf.end()) return false; //throw exFileParseErr(buf);
	
	std::string value(p2,buf.end());
       
	std::map<std::string,std::string>::const_iterator p = data.find(name); // multiple entries are invalid
	if (p != data.end()) return false; //throw exFileMultiItemErr(buf);
	data[name]=value;	
	
    }
   
    f.close();
    return true; 
}

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
