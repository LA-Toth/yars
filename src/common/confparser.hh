/* confparser.hh - configuration file parser

    Copyright (C) 2004, 2006 Laszlo Attila Toth

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

#ifndef CONFPARSER_V2_H
#define CONFPARSER_V2_H

#include <string>
#include <map>

namespace ConfParser {
    
    /* EXCEPTION CLASS - TEMPORARY */

    class Exception {
	std::string msg;
	
    public:
	Exception(){}
	Exception(std::string& msg) { this->msg = msg; }
	
	void setMsg(std::string& msg) { this->msg = msg; }
	std::string getMsg() const {return msg;}
    };

    class exFileOpenErr:public Exception {
    public:
	exFileOpenErr(){}
	exFileOpenErr(std::string& msg):Exception(msg){}
    };

    /* configfile parser
       line: <comment>| <blank> | <option>
       comment : {<whitespaces>} '#' <string> NL
       blank: {<whitespaces>} NL
       option: <name> <whietspaces> <eq>  <whitespaces> <value> NL
       NL: new line
       whitespaces: 0 or more space or tab
       name: [A-Za-z_][A-Za-z0-9_]*
       value: string, that can contain any characters. First whitespace
       characters must be escaped (\x20, etc).C-style escape sequences.
       eq: 0 or 1 equal sign(=)
    */
       

    class ConfigParser {
	std::string filename;
	std::map<std::string,std::string> data;
	bool parse();  // parse filename
    public:
	ConfigParser();
	ConfigParser(const std::string& file);
	ConfigParser(const char* file);

	bool parse(const std::string& file);
	bool parse(const char* file);
	
	std::string getValue(const char* name) const;
	std::string getValue(const std::string& name) const;
	std::string getValueDefault(const std::string& name, const std::string& defaultValue) const;

	char * getValue(const char* name,char* buffer,const int maxlen) const;
	char * getValue(const std::string& name,char* buffer,const int maxlen) const;
	
	class exFileParseErr:public Exception {
	public:
	    exFileParseErr():Exception(){}
	    exFileParseErr(std::string& msg):Exception(msg){}
	};
	class exFileMultiItemErr:public exFileParseErr {
	public:
	    exFileMultiItemErr():exFileParseErr(){}
	    exFileMultiItemErr(std::string& msg):exFileParseErr(msg){}
	};
    };


}; // namespace ConfParser


#endif 

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
