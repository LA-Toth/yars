/*  config.cpp - These functions for config file parsing, checking etc

    Copyright (C) 2004,2006, Laszlo Attila Toth

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

#ifndef CONFIG__HH
#define CONFIG__HH

#include "confparser.hh"
#include "types/imageinfo.hh"

typedef struct Configuration
{
    ConfParser::ConfigParser parser;
    ImageInfoList infoList;
} Configuration;


#ifdef HAS_CONFIGURATION
# ifndef EXTERN_CFG
#   define EXTERN_CFG extern
# endif
EXTERN_CFG Configuration* configuration;
#endif

#endif


/** EMACS **
 * Local variables:
 * mode: c++
 * c-basic-offset: 4
 * End:
 */
