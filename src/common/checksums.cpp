/*  checksums.cpp - calculating checksum of part of a file
    Copyright (C) 2006, Laszlo Attila Toth

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


#include <sys/types.h>
#include <unistd.h>
#include "md5.h"
#include "checksums.hh"

bool calculateChecksum(int fd, off64_t offset, size_t maxLen, ChecksumInfo* ci, size_t& readLen) {
    off64_t offs;
    offs = lseek64(fd, offset, SEEK_SET);
    if (offs != offset) {
	return false;
    }
    char *buf = new char[maxLen];
    memset(buf, 0, maxLen);
    offs = 1; readLen = 0;
    while (offs > 0 &&  readLen <  maxLen) {
	if ((offs = read(fd, buf, maxLen)) < 0)
	    return false;
	readLen += offs;
    }

    md5_buffer(buf, readLen, ci->digest);
  
    return true;
}

bool calculateChecksum(int fd, ChecksumInfoList* checksums, size_t blockSize) {
    size_t readLen = 0;
    off64_t offset = 0;
    ChecksumInfo ci;

    checksums->clear();
    do {
	if (!calculateChecksum(fd, offset, blockSize, &ci, readLen))
	    return false;
	checksums->add(ci);
	offset += readLen;
    } while (readLen == blockSize);
    return true;
}

/// calculates every block's checksum, block size is 1024*1024 bytes
bool calculateChecksum(int fd, ChecksumInfoList* checksums) {
    return calculateChecksum(fd, checksums, 1024*1024);
}


/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
