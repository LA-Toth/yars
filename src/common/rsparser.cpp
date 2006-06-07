/*  rsparser.cpp - HTTP-like header parser

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

#include <sys/select.h>
#include <sys/socket.h>
#include <errno.h>
#include "rsparser.hh"


class RSParserHelper
{
    bool hasEOL; //< has EOL char(s)
    int posEOL;  //< position of EOL 
    bool normalEOL; //< is "\r\n" or simply "\n"
    int offset;  //< position of new data
public:
    char buffer[4096];
    
    RSParserHelper() : hasEOL(false), posEOL(0), normalEOL(false), offset(0) {
	memset(buffer, 0, 4096);
    }
    inline int getOffset() const { return offset; }
    inline int maxReadableLength() const { return 4096 - offset; }

    void updateLength(int readLength) {
	if (!hasEOL) {
	    for (int i=0; i!= readLength; ++i) {
		if (buffer[offset + i] == '\r') {
		    // next must be '\n'
		    if (buffer[offset + ++i ] == '\n') {
			hasEOL = true;
			normalEOL = true;
			posEOL = i-1;
			break;
		    } // else: nothing: bug!!
		} else if (buffer[offset + i ] == '\n') {
			hasEOL = true;
			normalEOL = false;
			posEOL = i;
		} 		
	    }
	}
	offset += readLength;
    }

    inline int getEOL() { return hasEOL ? posEOL : -1 ; }

    inline void removeLine() { removeLine(false); }
    inline void forcedRemoveLine() { removeLine(true); }
private:    
    void removeLine(bool forced) {
	if (!hasEOL && !forced) return;
	int len = posEOL + ( normalEOL ? 2 : 1);
	memmove(buffer, buffer + len,  4096 - len);
	offset -= len;
	hasEOL = false;
    }    
};

RSParser::RSParser(bool request) : helper(new  RSParserHelper()), isRequest(request),
				   firstLine(true), error(false), 
				   version (0x0100), status(0)
{
}

RSParser::~RSParser()
{
    if (helper)
	delete helper;
}

int RSParser::parseHeader(int fd, fd_set *readfds, fd_set* exceptfds, bool searchEndOfHeader)
{
    int retval = 0;
    if (FD_ISSET(fd, exceptfds)) {
	char c;
	errno = 0;
	int r = recv (fd, &c, 1, MSG_OOB);
	if (r < 1) {
	    FD_CLR(fd, readfds);
	    shutdown (fd, SHUT_RDWR); 
	    close (fd);
	    fd = -1;
	    return -1;
	}
    } 
    if (FD_ISSET(fd, readfds)) {
	int r = read(fd, helper->buffer + helper->getOffset(), helper->maxReadableLength());
	if (r < 1) {
	    FD_CLR(fd, readfds);
	    shutdown (fd, SHUT_RDWR); 
	    close (fd);
	    fd = -1;
	    return -1;
	}  else {
	    helper->updateLength(r);

	    if (searchEndOfHeader) {
		while (helper->getOffset() > 0) {
		    if (helper->getEOL() == 0) return 0;
		    helper->removeLine();
		}
		return 0;
	    }

	    while(helper->getOffset() && ((r = helper->getEOL()) >= 0)) {
		if (firstLine) {
		    if ((!isRequest && memcmp(helper->buffer, "RS/1.0 ", 7)) ||
			(isRequest &&  memcmp(helper->buffer, "GET ", 4))) {
			// error!!! drop the whole header
			retval = 2;
		    } else {
			if (isRequest) {
			    helper->buffer[helper->getEOL()] = 0;
			    name = helper->buffer + 4;
			    std::string::const_iterator p1=name.begin(),p2;
			    while(p1 != name.end() && *p1!=' ')++p1;
			    if (p1 == name.end()) {
				retval = 2;
			    } else {
				std::string n((p2=name.begin()), p1);
				name = n;
			    }				
			} else {
			    helper->buffer[helper->getEOL()] = 0;
			    name = helper->buffer + 7; 
			    std::string::iterator p1=name.begin(),p2;
			    while(p1 != name.end() && *p1!=' ')++p1;
			    std::string n(name.begin(), p1);
			    if ((status = atoi(n.c_str())) == 0) {
				retval = 2;
			    } else {
				std::string n(--p1,name.end());
				statusText = n;
			    }
			}		
		    }
		} else if(!retval)  {// no error
		    if (helper->getEOL() == 0) {
			helper->removeLine();					
			return 1;
		    }
		    helper->buffer[helper->getEOL()] = 0;
		    std::string buf = helper->buffer;
		    std::string::iterator p1=buf.begin(),p2;
		    while(p1 != buf.end() && *p1!=':')++p1;
		    p2=p1;
		    if (p2 == buf.end() || *++p1 != ' ') {
			retval = 2;
		    } else {
			std::string name(buf.begin(),p2);
			++p1;
			std::string value(p1,buf.end());
			std::map<std::string,std::string>::const_iterator p = lines.find(name); // multiple entries are invalid
			if (p != lines.end()) {
			    retval = 2;
			    lines[name] = value;
			}
		    }		    
		}
		helper->removeLine();
	    }
	} 
    }
    return retval;
}


/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
