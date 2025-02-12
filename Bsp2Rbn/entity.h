//
// BSP_tool - botman's Half-Life BSP utilities
//
// (http://planethalflife.com/botman/)
//
// entity.h
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

#ifndef STUDIO_MODEL_H
#include "studio_model.h"
#endif

#ifndef ENTITY_H
#define ENTITY_H

typedef struct Botman_entvars_s
{
	char     classname[64];
	vec3_t   origin;
	vec3_t   angles;

	int      rendermode;
	float    renderamt;
	vec3_t   rendercolor;
	int      renderfx;

	int      brush_model_index;
	StudioModel* studio_model;
} Botman_entvars_t;

extern int Botman_num_entvars;
extern Botman_entvars_t Botman_entvars[];

//bool GetEntityKeyValue(char *key, char *value);
void LoadEntVars(void);
//bool FindEntity(int *index);
//bool FindEntityByClassname(int *index, const char *classname);
//bool FindEntityByWildcard(int *index, const char *classname, int length);

void InitSpawnPoint(void);

#endif
