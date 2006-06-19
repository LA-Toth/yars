/*  bitvectex.cpp - BitVectorEx with special features for the Client

    Copyright (C) 2004, 2006, Laszlo Attila Toth

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

#include <string>
#include <iostream>
#include "types/bitvectex.hh"

using std::cout;

BitVectorEx::BitVectorEx(const int& siz): size(siz),e(-1) {
    data = new char[(size+7)>>3];
    memset(data, 0, (size+7)>>3);
}

BitVectorEx::~BitVectorEx(){
    delete[]  data;
}

void BitVectorEx::set(const int& idx) {
    if (idx == e+1) {
	write(2,".",1);
    }else{
	static char s[45];
	write(2,s,sprintf(s,"|%d|",idx-e));	
    }
   
    e = idx;
    data [idx>>3] |= 1<<(idx&7);
}

bool BitVectorEx::operator[](const int& idx) const{
    return 1 << (idx&7) & data[idx>>3];
}

void BitVectorEx::intv(int&x,int&y) const{
    x=0;
    while (x!=size && operator[](x)) ++x;
    if (x==size) {
	y = -1;
	return;
    }
    y = x;
    while ( (y!=(size-1)) && !operator[](y+1)) ++y;
}

void BitVectorEx::intv1(int&x,int&y) const{
    x=0;
    while (x!=size && !operator[](x)) ++x;
    if (x==size) {
	y = -1;
	return;
    }
    y = x;
    while ( (y!=(size-1)) && operator[](y+1)) ++y;
}

/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
