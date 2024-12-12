/***
*
*  Copyright (c) 1998, Valve LLC. All rights reserved.
*
*  This product contains software technology licensed from Id
*  Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*  All Rights Reserved.
*
****/

//
// bspfile.cpp
//

#include <math.h>

#include <extdll.h>
#include <h_export.h>

#include "cmdlib.h"
#include "mathlib.h"
#include "bspfile.h"
#include "scriplib.h"

//=============================================================================

// Make all of these dynamice for BSP_tool...

int      nummodels = 0;
dmodel_t* dmodels = NULL;

int      visdatasize = 0;
byte* dvisdata = NULL;

int      lightdatasize = 0;
byte* dlightdata = NULL;

int      texdatasize = 0;
byte* dtexdata = NULL;

int      entdatasize = 0;
char* dentdata = NULL;

int      numleafs = 0;
dleaf_t* dleafs = NULL;

int      numplanes = 0;
dplane_t* dplanes = NULL;

int      numvertexes = 0;
dvertex_t* dvertexes = NULL;

int      numnodes = 0;
dnode_t* dnodes = NULL;

int      numtexinfo = 0;
texinfo_t* texinfo = NULL;

int      numfaces = 0;
dface_t* dfaces = NULL;

int      numclipnodes = 0;
dclipnode_t* dclipnodes = NULL;

int      numedges = 0;
dedge_t* dedges = NULL;

int      nummarksurfaces = 0;
unsigned short* dmarksurfaces = NULL;

int      numsurfedges = 0;
int* dsurfedges = NULL;

int      num_entities = 0;
entity_t entities[MAX_MAP_ENTITIES];

#ifdef __linux__
unsigned _rotl(unsigned val, int shift)
{
	register unsigned hibit;        /* non-zero means hi bit set */
	register unsigned num = val;    /* number to rotate */

	shift &= 0x1f;                  /* modulo 32 -- this will also make
									   negative shifts work */
	while (shift--) {
		hibit = num & 0x80000000;   /* get high bit */
		num <<= 1;                  /* shift left one bit */
		if (hibit)
			num |= 1;               /* set lo bit if hi bit was set */
	}

	return num;
}
#endif

/*
===============
FastChecksum
===============
*/

int FastChecksum(char* buffer, int bytes)
{
	int   checksum = 0;

	while (bytes--)
	{
		checksum = _rotl(checksum, 4) ^ *(char*)buffer;
		buffer++;
	}

	return checksum;
}

/*
===============
CompressVis
===============
*/
int CompressVis(byte* vis, byte* dest)
{
	int   j;
	int   rep;
	int   visrow;
	byte* dest_p;

	dest_p = dest;
	visrow = (numleafs + 7) >> 3;

	for (j = 0; j < visrow; j++)
	{
		*dest_p++ = vis[j];
		if (vis[j])
			continue;

		rep = 1;
		for (j++; j < visrow; j++)
			if (vis[j] || rep == 255)
				break;
			else
				rep++;
		*dest_p++ = rep;
		j--;
	}

	return dest_p - dest;
}

/*
===================
DecompressVis
===================
*/
void DecompressVis(byte* in, byte* decompressed)
{
	int   c;
	byte* out;
	int   row;

	row = (numleafs + 7) >> 3;
	out = decompressed;

	do
	{
		if (*in)
		{
			*out++ = *in++;
			continue;
		}

		c = in[1];
		in += 2;
		while (c)
		{
			*out++ = 0;
			c--;
		}
	} while (out - decompressed < row);
}

//=============================================================================

/*
=============
SwapBSPFile

Byte swaps all data in a bsp file.
=============
*/
void SwapBSPFile(qboolean todisk)
{
	int   i, j, c;
	dmodel_t* d;
	dmiptexlump_t* mtl;

	// models
	for (i = 0; i < nummodels; i++)
	{
		d = &dmodels[i];

		for (j = 0; j < MAX_MAP_HULLS; j++)
			d->headnode[j] = LittleLong(d->headnode[j]);

		d->visleafs = LittleLong(d->visleafs);
		d->firstface = LittleLong(d->firstface);
		d->numfaces = LittleLong(d->numfaces);

		for (j = 0; j < 3; j++)
		{
			d->mins[j] = LittleFloat(d->mins[j]);
			d->maxs[j] = LittleFloat(d->maxs[j]);
			d->origin[j] = LittleFloat(d->origin[j]);
		}
	}

	//
	// vertexes
	//
	for (i = 0; i < numvertexes; i++)
	{
		for (j = 0; j < 3; j++)
			dvertexes[i].point[j] = LittleFloat(dvertexes[i].point[j]);
	}

	//
	// planes
	//
	for (i = 0; i < numplanes; i++)
	{
		for (j = 0; j < 3; j++)
			dplanes[i].normal[j] = LittleFloat(dplanes[i].normal[j]);
		dplanes[i].dist = LittleFloat(dplanes[i].dist);
		dplanes[i].type = LittleLong(dplanes[i].type);
	}

	//
	// texinfos
	//
	for (i = 0; i < numtexinfo; i++)
	{
		for (j = 0; j < 8; j++)
			texinfo[i].vecs[0][j] = LittleFloat(texinfo[i].vecs[0][j]);
		texinfo[i].miptex = LittleLong(texinfo[i].miptex);
		texinfo[i].flags = LittleLong(texinfo[i].flags);
	}

	//
	// faces
	//
	for (i = 0; i < numfaces; i++)
	{
		dfaces[i].texinfo = LittleShort(dfaces[i].texinfo);
		dfaces[i].planenum = LittleShort(dfaces[i].planenum);
		dfaces[i].side = LittleShort(dfaces[i].side);
		dfaces[i].lightofs = LittleLong(dfaces[i].lightofs);
		dfaces[i].firstedge = LittleLong(dfaces[i].firstedge);
		dfaces[i].numedges = LittleShort(dfaces[i].numedges);
	}

	//
	// nodes
	//
	for (i = 0; i < numnodes; i++)
	{
		dnodes[i].planenum = LittleLong(dnodes[i].planenum);
		for (j = 0; j < 3; j++)
		{
			dnodes[i].mins[j] = LittleShort(dnodes[i].mins[j]);
			dnodes[i].maxs[j] = LittleShort(dnodes[i].maxs[j]);
		}
		dnodes[i].children[0] = LittleShort(dnodes[i].children[0]);
		dnodes[i].children[1] = LittleShort(dnodes[i].children[1]);
		dnodes[i].firstface = LittleShort(dnodes[i].firstface);
		dnodes[i].numfaces = LittleShort(dnodes[i].numfaces);
	}

	//
	// leafs
	//
	for (i = 0; i < numleafs; i++)
	{
		dleafs[i].contents = LittleLong(dleafs[i].contents);
		for (j = 0; j < 3; j++)
		{
			dleafs[i].mins[j] = LittleShort(dleafs[i].mins[j]);
			dleafs[i].maxs[j] = LittleShort(dleafs[i].maxs[j]);
		}

		dleafs[i].firstmarksurface = LittleShort(dleafs[i].firstmarksurface);
		dleafs[i].nummarksurfaces = LittleShort(dleafs[i].nummarksurfaces);
		dleafs[i].visofs = LittleLong(dleafs[i].visofs);
	}

	//
	// clipnodes
	//
	for (i = 0; i < numclipnodes; i++)
	{
		dclipnodes[i].planenum = LittleLong(dclipnodes[i].planenum);
		dclipnodes[i].children[0] = LittleShort(dclipnodes[i].children[0]);
		dclipnodes[i].children[1] = LittleShort(dclipnodes[i].children[1]);
	}

	//
	// miptex
	//
	if (texdatasize)
	{
		mtl = (dmiptexlump_t*)dtexdata;
		if (todisk)
			c = mtl->nummiptex;
		else
			c = LittleLong(mtl->nummiptex);
		mtl->nummiptex = LittleLong(mtl->nummiptex);
		for (i = 0; i < c; i++)
			mtl->dataofs[i] = LittleLong(mtl->dataofs[i]);
	}

	//
	// marksurfaces
	//
	for (i = 0; i < nummarksurfaces; i++)
		dmarksurfaces[i] = LittleShort(dmarksurfaces[i]);

	//
	// surfedges
	//
	for (i = 0; i < numsurfedges; i++)
		dsurfedges[i] = LittleLong(dsurfedges[i]);

	//
	// edges
	//
	for (i = 0; i < numedges; i++)
	{
		dedges[i].v[0] = LittleShort(dedges[i].v[0]);
		dedges[i].v[1] = LittleShort(dedges[i].v[1]);
	}
}

dheader_t* header;

int CopyLump(int lump, void** dest, int size)
{
	int   length, ofs;
	int   num;

	length = header->lumps[lump].filelen;
	ofs = header->lumps[lump].fileofs;

	if (length % size)
		Error("LoadBSPFile: odd lump size");

	num = length / size;

	if (*dest == NULL)
	{
		// allocate memory for this lump...
		*dest = malloc(num * size);

		if (*dest == NULL)
			Error("Error allocating memory for BSP file lump!\n");
	}

	memcpy(*dest, (byte*)header + ofs, length);

	return num;
}

/*
=============
LoadBSPFile
=============
*/
void LoadBSPFile(char* filename)
{
	int   i;

	//
	// load the file header
	//
	if (LoadFile(filename, (void**)&header) < sizeof(dheader_t))
		Error("File %s is too short", filename);

	if (header == NULL)
		Error("Cannot read file %s", filename);

	// swap the header
	for (i = 0; i < sizeof(dheader_t) / 4; i++)
		((int*)header)[i] = LittleLong(((int*)header)[i]);

	if (header->version != BSPVERSION)
		Error("%s is version %i, not %i", filename, header->version, BSPVERSION);

	nummodels = CopyLump(LUMP_MODELS, (void**)&dmodels, sizeof(dmodel_t));
	numvertexes = CopyLump(LUMP_VERTEXES, (void**)&dvertexes, sizeof(dvertex_t));
	numplanes = CopyLump(LUMP_PLANES, (void**)&dplanes, sizeof(dplane_t));
	numleafs = CopyLump(LUMP_LEAFS, (void**)&dleafs, sizeof(dleaf_t));
	numnodes = CopyLump(LUMP_NODES, (void**)&dnodes, sizeof(dnode_t));
	numtexinfo = CopyLump(LUMP_TEXINFO, (void**)&texinfo, sizeof(texinfo_t));
	numclipnodes = CopyLump(LUMP_CLIPNODES, (void**)&dclipnodes, sizeof(dclipnode_t));
	numfaces = CopyLump(LUMP_FACES, (void**)&dfaces, sizeof(dface_t));
	nummarksurfaces = CopyLump(LUMP_MARKSURFACES, (void**)&dmarksurfaces, sizeof(dmarksurfaces[0]));
	numsurfedges = CopyLump(LUMP_SURFEDGES, (void**)&dsurfedges, sizeof(dsurfedges[0]));
	numedges = CopyLump(LUMP_EDGES, (void**)&dedges, sizeof(dedge_t));

	texdatasize = CopyLump(LUMP_TEXTURES, (void**)&dtexdata, 1);
	visdatasize = CopyLump(LUMP_VISIBILITY, (void**)&dvisdata, 1);
	lightdatasize = CopyLump(LUMP_LIGHTING, (void**)&dlightdata, 1);
	entdatasize = CopyLump(LUMP_ENTITIES, (void**)&dentdata, 1);

	free(header);    // everything has been copied out

 //
 // swap everything
 //
	SwapBSPFile(false);

	//   dmodels_checksum = FastChecksum( (char *)dmodels, nummodels*sizeof(dmodels[0]) );
	//   dvertexes_checksum = FastChecksum( (char *)dvertexes, numvertexes*sizeof(dvertexes[0]) );
	//   dplanes_checksum = FastChecksum( (char *)dplanes, numplanes*sizeof(dplanes[0]) );
	//   dleafs_checksum = FastChecksum( (char *)dleafs, numleafs*sizeof(dleafs[0]) );
	//   dnodes_checksum = FastChecksum( (char *)dnodes, numnodes*sizeof(dnodes[0]) );
	//   texinfo_checksum = FastChecksum( (char *)texinfo, numtexinfo*sizeof(texinfo[0]) );
	//   dclipnodes_checksum = FastChecksum( (char *)dclipnodes, numclipnodes*sizeof(dclipnodes[0]) );
	//   dfaces_checksum = FastChecksum( (char *)dfaces, numfaces*sizeof(dfaces[0]) );
	//   dmarksurfaces_checksum = FastChecksum( (char *)dmarksurfaces, nummarksurfaces*sizeof(dmarksurfaces[0]) );
	//   dsurfedges_checksum = FastChecksum( (char *)dsurfedges, numsurfedges*sizeof(dsurfedges[0]) );
	//   dedges_checksum = FastChecksum( (char *)dedges, numedges*sizeof(dedges[0]) );
	//   dtexdata_checksum = FastChecksum( (char *)dtexdata, numedges*sizeof(dtexdata[0]) );
	//   dvisdata_checksum = FastChecksum( (char *)dvisdata, visdatasize*sizeof(dvisdata[0]) );
	//   dlightdata_checksum = FastChecksum( (char *)dlightdata, lightdatasize*sizeof(dlightdata[0]) );
	//   dentdata_checksum = FastChecksum( (char *)dentdata, entdatasize*sizeof(dentdata[0]) );
}

//============================================================================

FILE* wadfile;
dheader_t   outheader;

void AddLump(int lumpnum, void* data, int len)
{
	lump_t* lump;

	lump = &header->lumps[lumpnum];

	lump->fileofs = LittleLong(ftell(wadfile));
	lump->filelen = LittleLong(len);
	SafeWrite(wadfile, data, (len + 3) & ~3);
}

/*
=============
WriteBSPFile

Swaps the bsp file in place, so it should not be referenced again
=============
*/
void WriteBSPFile(char* filename)
{
	header = &outheader;
	memset(header, 0, sizeof(dheader_t));

	SwapBSPFile(true);

	header->version = LittleLong(BSPVERSION);

	wadfile = SafeOpenWrite(filename);
	SafeWrite(wadfile, header, sizeof(dheader_t)); // overwritten later

	AddLump(LUMP_PLANES, dplanes, numplanes * sizeof(dplane_t));
	AddLump(LUMP_LEAFS, dleafs, numleafs * sizeof(dleaf_t));
	AddLump(LUMP_VERTEXES, dvertexes, numvertexes * sizeof(dvertex_t));
	AddLump(LUMP_NODES, dnodes, numnodes * sizeof(dnode_t));
	AddLump(LUMP_TEXINFO, texinfo, numtexinfo * sizeof(texinfo_t));
	AddLump(LUMP_FACES, dfaces, numfaces * sizeof(dface_t));
	AddLump(LUMP_CLIPNODES, dclipnodes, numclipnodes * sizeof(dclipnode_t));
	AddLump(LUMP_MARKSURFACES, dmarksurfaces, nummarksurfaces * sizeof(dmarksurfaces[0]));
	AddLump(LUMP_SURFEDGES, dsurfedges, numsurfedges * sizeof(dsurfedges[0]));
	AddLump(LUMP_EDGES, dedges, numedges * sizeof(dedge_t));
	AddLump(LUMP_MODELS, dmodels, nummodels * sizeof(dmodel_t));

	AddLump(LUMP_LIGHTING, dlightdata, lightdatasize);
	AddLump(LUMP_VISIBILITY, dvisdata, visdatasize);
	AddLump(LUMP_ENTITIES, dentdata, entdatasize);
	AddLump(LUMP_TEXTURES, dtexdata, texdatasize);

	fseek(wadfile, 0, SEEK_SET);
	SafeWrite(wadfile, header, sizeof(dheader_t));
	fclose(wadfile);
}

//============================================================================

//#define ENTRIES(a)   (sizeof(a)/sizeof(*(a)))
//#define ENTRYSIZE(a) (sizeof(*(a)))

int ArrayUsage(char* szItem, int items, int maxitems, int itemsize)
{
	float percentage = maxitems ? items * (float)100.0 / maxitems : (float)0.0;

	printf("%-12s  %7i/%-7i  %7i/%-7i  (%4.1f%%)",
		szItem, items, maxitems, items * itemsize, maxitems * itemsize, percentage);
	if (percentage > 80.0)
		printf("VERY FULL!\n");
	else if (percentage > 95.0)
		printf("SIZE DANGER!\n");
	else if (percentage > 99.9)
		printf("SIZE OVERFLOW!!!\n");
	else
		printf("\n");
	return items * itemsize;
}

int GlobUsage(char* szItem, int itemstorage, int maxstorage)
{
	float percentage = maxstorage ? itemstorage * (float)100.0 / maxstorage : (float)0.0;
	printf("%-12s     [variable]    %7i/%-7i  (%4.1f%%)",
		szItem, itemstorage, maxstorage, percentage);
	if (percentage > 80.0)
		printf("VERY FULL!\n");
	else if (percentage > 95.0)
		printf("SIZE DANGER!\n");
	else if (percentage > 99.9)
		printf("SIZE OVERFLOW!!!\n");
	else
		printf("\n");
	return itemstorage;
}

/*
=============
PrintBSPFileSizes

Dumps info about current file
=============
*/
void PrintBSPFileSizes(void)
{
	int numtextures = texdatasize ? ((dmiptexlump_t*)dtexdata)->nummiptex : 0;
	int totalmemory = 0;

	printf("\n");
	printf("Object names  Objects/Maxobjs  Memory / Maxmem  Fullness\n");
	printf("------------  ---------------  ---------------  --------\n");

	totalmemory += ArrayUsage("models", nummodels, MAX_MAP_MODELS, sizeof(dmodel_t));
	totalmemory += ArrayUsage("planes", numplanes, MAX_MAP_PLANES, sizeof(dplane_t));
	totalmemory += ArrayUsage("vertexes", numvertexes, MAX_MAP_VERTS, sizeof(dvertex_t));
	totalmemory += ArrayUsage("nodes", numnodes, MAX_MAP_NODES, sizeof(dnode_t));
	totalmemory += ArrayUsage("texinfos", numtexinfo, MAX_MAP_TEXINFO, sizeof(texinfo_t));
	totalmemory += ArrayUsage("faces", numfaces, MAX_MAP_FACES, sizeof(dface_t));
	totalmemory += ArrayUsage("clipnodes", numclipnodes, MAX_MAP_CLIPNODES, sizeof(dclipnode_t));
	totalmemory += ArrayUsage("leaves", numleafs, MAX_MAP_LEAFS, sizeof(dleaf_t));
	totalmemory += ArrayUsage("marksurfaces", nummarksurfaces, MAX_MAP_MARKSURFACES, sizeof(unsigned short));
	totalmemory += ArrayUsage("surfedges", numsurfedges, MAX_MAP_SURFEDGES, sizeof(int));
	totalmemory += ArrayUsage("edges", numedges, MAX_MAP_EDGES, sizeof(dedge_t));

	totalmemory += GlobUsage("texdata", texdatasize, MAX_MAP_MIPTEX);
	totalmemory += GlobUsage("lightdata", lightdatasize, MAX_MAP_LIGHTING);
	totalmemory += GlobUsage("visdata", visdatasize, MAX_MAP_VISIBILITY);
	totalmemory += GlobUsage("entdata", entdatasize, MAX_MAP_ENTSTRING);

	printf("=== Total BSP file data space used: %d bytes ===\n", totalmemory);
}

/*
=================
ParseEpair
=================
*/
epair_t* ParseEpair(void)
{
	epair_t* e;

	e = (epair_t*)malloc(sizeof(epair_t));
	memset(e, 0, sizeof(epair_t));

	if (strlen(token) >= MAX_KEY - 1)
		Error("ParseEpar: token too long");
	e->key = copystring(token);
	GetToken(false);
	if (strlen(token) >= MAX_VALUE - 1)
		Error("ParseEpar: token too long");
	e->value = copystring(token);

	return e;
}

void DumpEntity(int i)
{
	epair_t* e;
	entity_t* mapent;

	if ((i < 0) || (i >= num_entities)) return;
	mapent = &entities[i];
	e = mapent->epairs;
	printf("Entity #%d:\n", i);
	while (e != NULL) {
		printf("\t%s = %s\n", e->key, e->value);
		e = e->next;
	}
}

/*
================
ParseEntity
================
*/
qboolean ParseEntity(void)
{
	epair_t* e;
	entity_t* mapent;

	if (!GetToken(true))
		return false;

	if (strcmp(token, "{"))
		Error("ParseEntity: { not found");

	if (num_entities == MAX_MAP_ENTITIES)
		Error("num_entities == MAX_MAP_ENTITIES");

	mapent = &entities[num_entities];
	num_entities++;

	do
	{
		if (!GetToken(true))
			Error("ParseEntity: EOF without closing brace");
		if (!strcmp(token, "}"))
			break;
		e = ParseEpair();
		e->next = mapent->epairs;
		mapent->epairs = e;
	} while (1);

	return true;
}

/*
================
ParseEntities

Parses the dentdata string into entities
================
*/
void ParseEntities(void)
{
	num_entities = 0;
	ParseFromMemory(dentdata, entdatasize);

	while (ParseEntity())
	{
	}
}

/*
================
UnparseEntities

Generates the dentdata string from all the entities
================
*/
void UnparseEntities(void)
{
	char* buf, * end;
	epair_t* ep;
	char  line[2048];
	int   i;

	buf = dentdata;
	end = buf;
	*end = 0;

	for (i = 0; i < num_entities; i++)
	{
		ep = entities[i].epairs;
		if (!ep)
			continue;   // ent got removed

		strcat(end, "{\n");
		end += 2;

		for (ep = entities[i].epairs; ep; ep = ep->next)
		{
			sprintf(line, "\"%s\" \"%s\"\n", ep->key, ep->value);
			strcat(end, line);
			end += strlen(line);
		}
		strcat(end, "}\n");
		end += 2;

		if (end > buf + MAX_MAP_ENTSTRING)
			Error("Entity text too long");
	}
	entdatasize = end - buf + 1;
}

void SetKeyValue(entity_t* ent, char* key, char* value)
{
	epair_t* ep;
	epair_t* prev_ep = NULL;

	for (ep = ent->epairs; ep; ep = ep->next)
	{
		if (!strcmp(ep->key, key))
		{
			free(ep->value);
			ep->value = copystring(value);
			return;
		}
	}

	prev_ep = NULL;
	for (ep = ent->epairs; ep; prev_ep = ep, ep = ep->next)
		;  // get to the end of the linked list

	ep = (epair_t*)malloc(sizeof(*ep));
	memset(ep, 0, sizeof(ep));

	if (prev_ep)
		prev_ep->next = ep;  // link it at the end
	else
		ent->epairs = ep;  // link it at the beginning (first and only)

	ep->key = copystring(key);
	ep->value = copystring(value);
}

void RemoveKey(entity_t* ent, char* key)
{
	epair_t* ep;
	epair_t* prev_ep = NULL;

	for (ep = ent->epairs; ep; ep = ep->next)
	{
		if (!strcmp(ep->key, key))
		{
			free(ep->value);
			free(ep->key);
			if (prev_ep)
			{
				prev_ep->next = ep->next;
				free(ep);
			}
			else
			{
				ent->epairs = ep->next;
				free(ep);
			}
			return;
		}
		prev_ep = ep;
	}
}

int FindEntityByClassname(int index, const char* classname)
{
	epair_t* ep;

	// index should be -1 to start at first entity...
	index++;

	while (index < num_entities)
	{
		ep = entities[index].epairs;

		while (ep)
		{
			if ((strcmp(ep->key, "classname") == 0) &&
				(strcmp(ep->value, classname) == 0))
			{
				return index;
			}

			ep = ep->next;
		}

		index++;
	}

	return -1;  // entity not found
}

int FindEntityByWildcard(int index, const char* classname, int length)
{
	epair_t* ep;

	// index should be -1 to start at first entity...
	index++;

	while (index < num_entities)
	{
		ep = entities[index].epairs;

		while (ep)
		{
			if ((strcmp(ep->key, "classname") == 0) &&
				(strncmp(ep->value, classname, length) == 0))
			{
				return index;
			}

			ep = ep->next;
		}

		index++;
	}

	return -1;  // entity not found
}

char* ValueForKey(entity_t* ent, char* key)
{
	epair_t* ep;

	for (ep = ent->epairs; ep; ep = ep->next)
		if (!strcmp(ep->key, key))
			return ep->value;
	return "";
}

vec_t FloatForKey(entity_t* ent, char* key)
{
	char* k;

	k = ValueForKey(ent, key);
	return (float)atof(k);
}

void GetVectorForKey(entity_t* ent, char* key, vec3_t vec)
{
	char* k;
	double   v1, v2, v3;

	k = ValueForKey(ent, key);
	// scanf into doubles, then assign, so it is vec_t size independent
	v1 = v2 = v3 = 0;
	sscanf(k, "%lf %lf %lf", &v1, &v2, &v3);
	vec[0] = (float)v1;
	vec[1] = (float)v2;
	vec[2] = (float)v3;
}

void FreeEntities(void)
{
	int i;
	epair_t* pEpair, * pEpairNext;

	for (i = 0; i < num_entities; i++)
	{
		pEpair = entities[i].epairs;

		while (pEpair)
		{
			pEpairNext = pEpair->next;
			free(pEpair->key);
			free(pEpair->value);
			free(pEpair);
			pEpair = pEpairNext;
		}

		//      num_entities = 0; EVY
	}
	num_entities = 0; // EVY: should rather be here

	for (i = 0; i < MAX_MAP_ENTITIES; i++)
		entities[i].epairs = NULL;
}