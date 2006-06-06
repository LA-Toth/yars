/*  checksuminfo.hh - ChecksumInfo and List types

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

#ifndef CHECKSUMINFO__HH
#define CHECKSUMINFO__HH

#include <vector>
#include <istream>
#include <ostream>

class ChecksumInfo
{
public:
    char digest[16];
    ChecksumInfo(const ChecksumInfo& info);
    ChecksumInfo(){ memset(digest,0, sizeof(digest));};
    ChecksumInfo& operator=(const ChecksumInfo& info);
};


bool operator==(const ChecksumInfo& lhs, const ChecksumInfo& rhs);
bool operator!=(const ChecksumInfo& lhs, const ChecksumInfo& rhs);

std::ostream& operator<<(std::ostream& os, const ChecksumInfo& rhs);
std::istream& operator>>(std::istream& is, ChecksumInfo& rhs);

class ChecksumInfoList
{
private:
    std::vector<ChecksumInfo> list;
public:
    ChecksumInfoList() { }
    ChecksumInfoList(const ChecksumInfoList& il);
    inline size_t size() const { return list.size(); };
    inline void add(ChecksumInfo& info) { list.push_back(info); }
    inline ChecksumInfo operator[](int idx) const { return list[idx]; }
    ChecksumInfoList& operator=(const ChecksumInfoList& info);  
    inline void clear() { list.clear(); }
};


bool operator==(const ChecksumInfoList& lhs, const ChecksumInfoList& rhs);
bool operator!=(const ChecksumInfoList& lhs, const ChecksumInfoList& rhs);

std::ostream& operator<<(std::ostream& os, const ChecksumInfoList& rhs);
std::istream& operator>>(std::istream& is, ChecksumInfoList& rhs);


#endif

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
