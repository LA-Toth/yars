/*  imageinfo.cpp - ImageInfo and List types

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

#include <arpa/inet.h>
#include "types/imageinfo.hh"

ImageInfo::ImageInfo(): size(0), blockSize(0), lastBlockSize(0), lastModified(0)
{
}

ImageInfo::ImageInfo(const ImageInfo& info) {
    name = info.name;
    memcpy(digest,info.digest, sizeof(digest));
    size = info.size;
    blockSize = info.blockSize;
    lastBlockSize = info.lastBlockSize;
    checksums = info.checksums;
}

ImageInfo& ImageInfo::operator=(const ImageInfo& info) {
    name = info.name;
    memcpy(digest,info.digest, sizeof(digest));
    size = info.size;
    blockSize = info.blockSize;
    lastBlockSize = info.lastBlockSize;
    checksums = info.checksums;
    return *this;
}


bool operator==(const ImageInfo& lhs, const ImageInfo& rhs)
{
    return  !(lhs != rhs);
}
bool operator!=(const ImageInfo& lhs, const ImageInfo& rhs)
{
    return  lhs.name != rhs.name || memcmp(lhs.digest, rhs.digest, 16) ||
	rhs.size != lhs.size || lhs.blockSize !=  rhs.blockSize ||
	rhs.blockSize != lhs.blockSize ||
	rhs.checksums != lhs.checksums;
}
std::ostream& operator<<(std::ostream& os, const ImageInfo& rhs)
{
    int32_t tmp;
    os << rhs.name << ((char)0);
    os.write(rhs.digest, sizeof(rhs.digest));
    tmp = htonl(rhs.size & 0xffffffff);
    os.write((char*)&tmp, sizeof(int32_t));
    tmp = htonl((rhs.size >>32) & 0xffffffff);
    os.write((char*)&tmp, sizeof(int32_t));
    tmp = htonl(rhs.blockSize);
    os.write((char*)&tmp, sizeof(int32_t));
    tmp = htonl(rhs.lastBlockSize);
    os.write((char*)&tmp, sizeof(int32_t));
    tmp = htonl(rhs.lastModified);
    os.write((char*)&tmp, sizeof(int32_t));
    os << rhs.checksums;
    return os;  
}

std::istream& operator>>(std::istream& is, ImageInfo& rhs)
{
    std::getline(is, rhs.name, (char)0);
    int32_t tmp;
    int64_t tmp64;
    is.read(rhs.digest, sizeof(rhs.digest));
    is.read((char*)&tmp, sizeof(int32_t));
    rhs.size = ntohl(tmp);
    is.read((char*)&tmp, sizeof(int32_t));
    tmp64 = ((int64_t)(ntohl(tmp)) << 32);
    rhs.size |= tmp64;
    is.read((char*)&tmp, sizeof(int32_t));
    rhs.blockSize = ntohl(tmp);
    is.read((char*)&tmp, sizeof(int32_t));
    rhs.lastBlockSize = ntohl(tmp);
    is.read((char*)&tmp, sizeof(int32_t));
    rhs.lastModified = ntohl(tmp);
    is >> rhs.checksums;
    return is;  
}

ImageInfoList::ImageInfoList(const ImageInfoList& info)
{
    for (std::vector<ImageInfo>::const_iterator it = info.list.begin(); it != info.list.end(); ++it) {
	list.push_back( *it );
    }
}
ImageInfoList& ImageInfoList::operator=(const ImageInfoList& info)
{
    list.clear();
    for (std::vector<ImageInfo>::const_iterator it = info.list.begin(); it != info.list.end(); ++it) {
	list.push_back( *it );
    }
    return *this;
}

bool operator==(const ImageInfoList& lhs, const ImageInfoList& rhs)
{
    if (lhs.size() != rhs.size()) return false;
    for (size_t i=0; i!= lhs.size(); ++i) {
	if (lhs[i] != rhs[i]) return false;
    }
    return true;
}
bool operator!=(const ImageInfoList& lhs, const ImageInfoList& rhs) {
    return ! (lhs != rhs);
}

std::ostream& operator<<(std::ostream& os, const ImageInfoList& rhs)
{
    int32_t tmp = htonl(rhs.size());
    os.write((char*)(&tmp), sizeof(int32_t));
    for (size_t i=0; i!= rhs.size(); ++i) {
	os << rhs[i];
    }
    return os;  
}

std::istream& operator>>(std::istream& is, ImageInfoList& rhs)
{
    int32_t tmp;
    is.read((char*)(&tmp), sizeof(int32_t));
    tmp = ntohl(tmp);
    ImageInfo info;
    rhs.clear();
    for (int i=0; i!= tmp; ++i) {
	is >> info;
	rhs.add(info);
    }
    return is;  
}

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
