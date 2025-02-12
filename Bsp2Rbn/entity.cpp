//
// BSP_tool - botman's Half-Life BSP utilities
//
// (http://planethalflife.com/botman/)
//
// entity.cpp
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

#include <math.h>

#include <extdll.h>
#include <h_export.h>

// chierie de /home/evyncke/cstrike/Realbot/HLSDK/multiplayer/cl_dll/util_vector.h definissant vec3_t comme Vector

#include "cmdlib.h"
#include "mathlib.h"
#include "bspfile.h"
#include "entity.h"

vec3_t spawn_point;
float spawn_point_yaw;

epair_t* pEpair = NULL;

int Botman_num_entvars = 0;
Botman_entvars_t Botman_entvars[MAX_MAP_ENTITIES];

void LoadEntVars(void)
{
	int ent_index = 0;
	char* value;

	while (ent_index < num_entities)
	{
		value = ValueForKey(&entities[ent_index], "classname");

		if (value[0])
		{
			strcpy(Botman_entvars[Botman_num_entvars].classname, value);

			// initialize the default entvars fields...
			Botman_entvars[Botman_num_entvars].origin[0] = 0.0f;
			Botman_entvars[Botman_num_entvars].origin[1] = 0.0f;
			Botman_entvars[Botman_num_entvars].origin[2] = 0.0f;

			Botman_entvars[Botman_num_entvars].angles[0] = 0.0f;
			Botman_entvars[Botman_num_entvars].angles[1] = 0.0f;
			Botman_entvars[Botman_num_entvars].angles[2] = 0.0f;

			Botman_entvars[Botman_num_entvars].rendermode = 0;
			Botman_entvars[Botman_num_entvars].renderamt = 1.0f;
			Botman_entvars[Botman_num_entvars].rendercolor[0] = 1.0f;
			Botman_entvars[Botman_num_entvars].rendercolor[1] = 1.0f;
			Botman_entvars[Botman_num_entvars].rendercolor[2] = 1.0f;
			Botman_entvars[Botman_num_entvars].renderfx = 0;

			Botman_entvars[Botman_num_entvars].brush_model_index = 0;

			Botman_entvars[Botman_num_entvars].studio_model = NULL;

			value = ValueForKey(&entities[ent_index], "origin");
			if (value[0])
			{
				sscanf(value, "%f %f %f", &Botman_entvars[Botman_num_entvars].origin[0],
					&Botman_entvars[Botman_num_entvars].origin[1],
					&Botman_entvars[Botman_num_entvars].origin[2]);
			}

			value = ValueForKey(&entities[ent_index], "angle");
			if (value[0])
			{
				// set the yaw angle...
				sscanf(value, "%f", &Botman_entvars[Botman_num_entvars].angles[1]);
			}

			value = ValueForKey(&entities[ent_index], "renderamt");
			if (value[0])
			{
				int  n_renderamt;

				sscanf(value, "%d", &n_renderamt);
				Botman_entvars[Botman_num_entvars].renderamt = n_renderamt / 255.0f;
			}

			value = ValueForKey(&entities[ent_index], "rendercolor");
			if (value[0])
			{
				int  n_color_r, n_color_b, n_color_g;

				sscanf(value, "%d %d %d", &n_color_r, &n_color_g, &n_color_b);
				Botman_entvars[Botman_num_entvars].rendercolor[0] = n_color_r / 255.0f;
				Botman_entvars[Botman_num_entvars].rendercolor[1] = n_color_g / 255.0f;
				Botman_entvars[Botman_num_entvars].rendercolor[2] = n_color_b / 255.0f;
			}

			value = ValueForKey(&entities[ent_index], "model");
			if (value[0])
			{
				if (sscanf(value, "*%d", &Botman_entvars[Botman_num_entvars].brush_model_index) == 1)
				{
					dmodel_t* model;

					// calculate the origin for this brush model...
					model = &dmodels[Botman_entvars[Botman_num_entvars].brush_model_index];

					Botman_entvars[Botman_num_entvars].origin[0] = (model->mins[0] + model->maxs[0]) / 2.0f;
					Botman_entvars[Botman_num_entvars].origin[1] = (model->mins[1] + model->maxs[1]) / 2.0f;
					Botman_entvars[Botman_num_entvars].origin[2] = (model->mins[2] + model->maxs[2]) / 2.0f;
				}
			}

			if ((strcmp(Botman_entvars[Botman_num_entvars].classname, "func_button") == 0) ||
				(strcmp(Botman_entvars[Botman_num_entvars].classname, "func_door") == 0))
			{
				// always render func_button and func_door entities...
				Botman_entvars[Botman_num_entvars].renderamt = 255;
			}

			Botman_num_entvars++;
		}

		ent_index++;
	}
}

void InitSpawnPoint(void)
{
	int ent_index;
	int count = 0;
	char* value;
	int pick, loop;

	spawn_point[0] = 0.0;
	spawn_point[1] = 0.0;
	spawn_point[2] = 0.0;
	spawn_point_yaw = 0.0;

	//   if (config.spawnpoint[0] == 0)
	return;  // no spawn points configured, just return
}