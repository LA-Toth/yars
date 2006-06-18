/*  checksums.hh - calculating checksum of part of a file
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

#ifndef CHECKSUMS__HH
#define CHECKSUMS__HH

#include "types/checksuminfo.hh"

/// calculates checksume of a part of a file. Return value indicates success
bool calculateChecksum(int fd, off64_t offset, size_t maxLength, ChecksumInfo* ci, size_t& readLength);

/// calculates every blocks's checksum, every block's size is <= block_size
bool calculateChecksum(int fd, ChecksumInfoList* checksums, size_t block_size);
/// calculates every block's checksum, block size is 1024*1024 bytes
bool calculateChecksum(int fd, ChecksumInfoList* checksums);

#endif

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */

