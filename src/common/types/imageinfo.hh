/*  imageinfo.hh - ImageInfo and List types

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

#ifndef IMAGEINFO__HH
#define IMAGEINFO__HH

#include <vector>
#include <istream>
#include <ostream>

#include "types/checksuminfo.hh"


    class ImageInfo
    {
    public:
	std::string name;
	char  digest[16];
	int64_t size;
	int32_t blockSize, lastBlockSize;
	time_t lastModified;
	ChecksumInfoList checksums;
	ImageInfo();
	ImageInfo(const ImageInfo& info);
	ImageInfo& operator=(const ImageInfo& info);
    };

bool operator==(const ImageInfo& lhs, const ImageInfo& rhs);
bool operator!=(const ImageInfo& lhs, const ImageInfo& rhs);

std::ostream& operator<<(std::ostream& os, const ImageInfo& rhs);
std::istream& operator>>(std::istream& is, ImageInfo& rhs);

class ImageInfoList
{
private:
    std::vector<ImageInfo> list;
public:
    ImageInfoList() {}
    ImageInfoList(const ImageInfoList& il);
    inline size_t size() const { return list.size(); };
    inline void add(ImageInfo& info) { list.push_back(info); }
    inline ImageInfo operator[](int idx) const { return list[idx]; }
    ImageInfoList& operator=(const ImageInfoList& info);  
    inline void clear() { list.clear(); }
};

bool operator==(const ImageInfoList& lhs, const ImageInfoList& rhs);
bool operator!=(const ImageInfoList& lhs, const ImageInfoList& rhs);

std::ostream& operator<<(std::ostream& os, const ImageInfoList& rhs);
std::istream& operator>>(std::istream& is, ImageInfoList& rhs);

#endif

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
