//
// BSP_tool - botman's Half-Life BSP utilities
//
// (http://planethalflife.com/botman/)
//
// paklib.h
//
// Copyright (C) 2001 - Jeffrey "botman" Broome
//
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// See the GNU General Public License for more details at:
// http://www.gnu.org/copyleft/gpl.html
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#ifndef PAKLIB_H
#define PAKLIB_H

/*
The format of PAK files is such. The first 12 bytes of the files are the
header. This is seperated into three, 4 byte blocks as follows:

-Header-

 - Signature (4 bytes of char, equals 'PACK' in ALL cases. If it doesn't then
			  it is not considered a pack file.)

 - Directory Offset (4 bytes, single integer. Specifies the position of the
					 first of X Directory areas in the file.)

 - Directory Length (4 bytes, single integer. Specifies the length of the X
					 dirextory areas.)

(Note: We can find the number X by dividing the length by the 64, because each
 directory section is 64 bytes. If the mod of this division is not zero then
 we have a corruption.)

-Directory section-

 - File name (56 bytes of char, specifies the name of the file pointed to by
			  the File Position data. Includes path info. ie maps/base1.bsp)

 - File Position (4  bytes, single integer. The first byte(address) of the
				  file named by File Name)

 - File Length (4  bytes, single integer. The length of the file named by
				File Name)

Notes: Normally, the header is at the start of the file and the X number of
directory areas at the very end. The file data is usually in between.

									 ________________________________
		   HEADER starts here --->  | - Signature         (4 bytes)  |
									| - Directory Offset  (4 bytes)  |
									| - Directory Length  (4 bytes)  |
									|________________________________|
		FILE DATA starts here --->  |                                |
									|                                |
									|         BINARY DATA            |
									|      (pointed to by the        |
									|       File Position data)      |
									|                                |
								  ~~~~~                            ~~~~~
								  ~~~~~                            ~~~~~
									|                                |
									|                                |
									|                                |
									|                                |
									|________________________________|
DIRECTORY SECTION starts here --->  | - File name         (56 bytes) |
									| - File Position     (4  bytes) |
									| - File Length       (4  bytes) |
									|________________________________|
									| - File name         (56 bytes) |
									| - File Position     (4  bytes) |
									| - File Length       (4  bytes) |
									|________________________________|
									|                                |
								  ~~~~~                            ~~~~~
								  ~~~~~                            ~~~~~
									|________________________________|
									| - File name         (56 bytes) |
									| - File Position     (4  bytes) |
									| - File Length       (4  bytes) |
									|________________________________|
*/

//
// pak reading
//

typedef struct
{
	char  identification[4];      // should be PACK or KCAP
	int   dir_offset;             // directory offset
	int   dir_length;             // directory length
} pakheader_t;

typedef struct
{
	char filename[56];            // PAK entry filename
	int file_pos;                 // PAK entry file position
	int file_length;              // PAK entry length
} pakinfo_t;

typedef struct pakconfig_s
{
	FILE* pakhandle;
	pakheader_t  pakheader;
	pakinfo_t* pakinfo;
	int          num_entries;
	struct pakconfig_s* next_config;
} pakconfig_t;

FILE* P_OpenPak(char* filename);
int P_ReadPakHeader(FILE* pakhandle, pakheader_t* pakheader);
void P_ReadPakInfo(FILE* pakhandle, long offset, int num_entries, pakinfo_t* pakinfo);
void P_ReadPakItem(FILE* pakhandle, pakinfo_t* pakinfo, void* buffer);
void Cmd_PakFile(void);
bool SearchPakFilename(char* filename, pakconfig_t** pakconfig, pakinfo_t** pakinfo);
bool LoadPakBSPFile(char* filename);

#endif
