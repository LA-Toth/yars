/*  checksuminfo.cpp - ChecksumInfo and List types

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
#include "types/checksuminfo.hh"

ChecksumInfo::ChecksumInfo(const ChecksumInfo& info) {
    memcpy(digest,info.digest, sizeof(digest));
}

ChecksumInfo& ChecksumInfo::operator=(const ChecksumInfo& info) {
    memcpy(digest,info.digest, sizeof(digest));
    return *this;
}


bool operator==(const ChecksumInfo& lhs, const ChecksumInfo& rhs)
{
    return  !(lhs != rhs);
}
bool operator!=(const ChecksumInfo& lhs, const ChecksumInfo& rhs)
{
    return memcmp(lhs.digest, rhs.digest, 16);
}

std::ostream& operator<<(std::ostream& os, const ChecksumInfo& rhs)
{
    os.write(rhs.digest, sizeof(rhs.digest));
    return os;  
}

std::istream& operator>>(std::istream& is, ChecksumInfo& rhs)
{
    is.read(rhs.digest, sizeof(rhs.digest));
    return is;  
}

ChecksumInfoList::ChecksumInfoList(const ChecksumInfoList& info)
{
    for (std::vector<ChecksumInfo>::const_iterator it = info.list.begin(); it != info.list.end(); ++it) {
	list.push_back( *it );
    }
}
ChecksumInfoList& ChecksumInfoList::operator=(const ChecksumInfoList& info)
{
    list.clear();
    for (std::vector<ChecksumInfo>::const_iterator it = info.list.begin(); it != info.list.end(); ++it) {
	list.push_back( *it );
    }
    return *this;
}

bool operator==(const ChecksumInfoList& lhs, const ChecksumInfoList& rhs)
{
    if (lhs.size() != rhs.size()) return false;
    for (size_t i=0; i!= lhs.size(); ++i) {
	if (lhs[i] != rhs[i]) return false;
    }
    return true;
}
bool operator!=(const ChecksumInfoList& lhs, const ChecksumInfoList& rhs) {
    return ! (lhs != rhs);
}


std::ostream& operator<<(std::ostream& os, const ChecksumInfoList& rhs)
{
    int32_t tmp = htonl(rhs.size());
    os.write((char*)(&tmp), sizeof(int32_t));
    for (size_t i=0; i!= rhs.size(); ++i) {
	os << rhs[i];
    }
    return os;  
}

std::istream& operator>>(std::istream& is, ChecksumInfoList& rhs)
{
    int32_t tmp;
    is.read((char*)(&tmp), sizeof(int32_t));
    tmp = ntohl(tmp);
    ChecksumInfo info;
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
