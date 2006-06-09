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
    int offset_eol; //< position _after_ eol can appear
public:
    char buffer[4097];
    std::string tempString;
    
    RSParserHelper() : hasEOL(false), posEOL(0), normalEOL(false), offset(0), offset_eol(0) {
	memset(buffer, 0, 4097);
    }
    inline int getOffset() const { return offset; }
    inline int maxReadableLength() const { return 4096 - offset; }

    void updateLength(int readLength) {
	if (!hasEOL) {
	    for (int i=offset_eol; i <  offset + readLength; ++i) {
		if (buffer[i] == '\r') {
		    // next must be '\n'
		    if (buffer[ ++i ] == '\n') {
			hasEOL = true;
			normalEOL = true;
			posEOL = i-1;
			break;
		    } // else: nothing: bug!!
		} else if (buffer[ i ] == '\n') { // bugous, doesn't work
		    hasEOL = true;
		    normalEOL = false;
		    posEOL = i;
		} 		
	    }
	}
	offset += readLength;
	if (!hasEOL) {
	    offset_eol += readLength;
	}
    }

    inline int getEOL() { return hasEOL ? posEOL : -1 ; }

    inline void removeLine() { removeLine(false); }
    inline void forcedRemoveLine() { removeLine(true); }

private:    
    void removeLine(bool forced) {
	if (!hasEOL && !forced || !offset) return;
	int len = posEOL + ( normalEOL ? 2 : 1);
	memmove(buffer, buffer + len,  4096 - len);
	offset -= len;
	offset_eol -= len;
	hasEOL = false;
	updateLength(0);
    }    
};

RSParser::RSParser(bool request) : helper(new  RSParserHelper())
{
}

RSParser::~RSParser()
{
    if (helper)
	delete helper;
}

int RSParser::parseHeader(int& fd, fd_set *readfds, fd_set* exceptfds)
{
    int retval = 1;
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
	    
	    while(helper->getOffset() && ((r = helper->getEOL()) > 0)) {
	        helper->buffer[ helper->getEOL() ] = 0;
		helper->tempString += helper->buffer;
		lines.push_back( helper->tempString );
		helper->tempString.clear();
		helper->removeLine();
	    }
	    if (helper->getOffset() == 0) {
		if (helper->getEOL() == 0) {
		    helper->forcedRemoveLine();
		    retval = 0;
		} else if(helper->getEOL() > 0) {
		    helper->buffer[ helper->getOffset() ] = 0;
		    helper->tempString += helper->buffer;
		    helper->forcedRemoveLine();
		    retval = 1;
		}
	    } else if (helper->getEOL() == 0) {
		helper->forcedRemoveLine();
		retval = 0;
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
