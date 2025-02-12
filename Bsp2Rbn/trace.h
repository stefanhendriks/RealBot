//
// BSP_tool - botman's Half-Life BSP utilities
//
// (http://planethalflife.com/botman/)
//
// trace.h
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

#ifndef TRACE_H
#define TRACE_H

typedef struct
{
	bool     allsolid;   /* if true, plane is not valid */
	bool     startsolid; /* if true, the initial point was in a solid area */
	float    fraction;   /* time completed, 1.0 = didn't hit anything */
	vec3_t   hitpos;     /* surface hit position (in solid) */
	vec3_t   endpos;     /* final position (not in solid) */
	int      contents;   /* contents of endpos */
} botman_trace_t;

int BotmanPointContents(const int nodenum, const vec3_t coord);
int BotmanPointContentsInHull(const int hullNumber, const vec3_t coord);
void BotmanTraceLine(vec3_t start, vec3_t end, botman_trace_t* trace);
dface_t* TraceLineFindFace(vec3_t start, botman_trace_t* tr);

#endif
