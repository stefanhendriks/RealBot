//
// BSP_tool - botman's Half-Life BSP utilities
//
// (http://planethalflife.com/botman/)
//
// world.cpp
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

#include <extdll.h>
#include <h_export.h>

#include "cmdlib.h"
#include "mathlib.h"
#include "bspfile.h"
#include "paklib.h"
#include "entity.h"
#include "world.h"

World::World(void)
{
}

World::~World(void)
{
	FreeWorld();
}

void World::FreeWorld(void)
{
	FreeEntities();

	if (dmodels)
	{
		free(dmodels);
		dmodels = NULL;
		nummodels = 0;
	}

	if (dvisdata)
	{
		free(dvisdata);
		dvisdata = NULL;
		visdatasize = 0;
	}

	if (dlightdata)
	{
		free(dlightdata);
		dlightdata = NULL;
		lightdatasize = 0;
	}

	if (dtexdata)
	{
		free(dtexdata);
		dtexdata = NULL;
		texdatasize = 0;
	}

	if (dentdata)
	{
		free(dentdata);
		dentdata = NULL;
		entdatasize = 0;
	}

	if (dleafs)
	{
		free(dleafs);
		dleafs = NULL;
		numleafs = 0;
	}

	if (dplanes)
	{
		free(dplanes);
		dplanes = NULL;
		numplanes = 0;
	}

	if (dvertexes)
	{
		free(dvertexes);
		dvertexes = NULL;
		numvertexes = 0;
	}

	if (dnodes)
	{
		free(dnodes);
		dnodes = NULL;
		numnodes = 0;
	}

	if (texinfo)
	{
		free(texinfo);
		texinfo = NULL;
		numtexinfo = 0;
	}

	if (dfaces)
	{
		free(dfaces);
		dfaces = NULL;
		numfaces = 0;
	}

	if (dclipnodes)
	{
		free(dclipnodes);
		dclipnodes = NULL;
		numclipnodes = 0;
	}

	if (dedges)
	{
		free(dedges);
		dedges = NULL;
		numedges = 0;
	}

	if (dmarksurfaces)
	{
		free(dmarksurfaces);
		dmarksurfaces = NULL;
		nummarksurfaces = 0;
	}

	if (dsurfedges)
	{
		free(dsurfedges);
		dsurfedges = NULL;
		numsurfedges = 0;
	}
}

void World::LoadBSP(char* bspfile)
{
	char bsp_filename[256];
	char pathname[256];
	bool bsp_found;
	int index, mod_index;
	char modname[256];
	int len;

	bsp_found = FALSE;

	strcpy(bspname, bspfile);

	if (FileTime(bspname) != -1)  // does the specified file exist?
		LoadBSPFile(bspname);
	else
		fprintf(stderr, "Cannot load file %s\n", bspname);

	ParseEntities();

	LoadEntVars();
}