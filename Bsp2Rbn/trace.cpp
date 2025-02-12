//
// Ancilliary functions to handle tracelines, tracehulls, ...
// Based on BSP_tool - botman's Half-Life BSP utilities
// Based on Quake world.c
// Modified for hulls by Eric Vyncke
//
// (http://planethalflife.com/botman/)
//
// trace.cpp
//
// Copyright (C) 2001 - Jeffrey "botman" Broome
// Copyright (C) 2004 - Eric Vyncke
// evyncke@students.hec.be, Summer 2004
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation; either version 2 of the License, or
//   (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with this program; if not, write to the Free Software Foundation,
//   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include <extdll.h>
#include <h_export.h>

// Below is tricky and dirty required because HL SDK redefine vec3_t as class Vector which brings a lot of trouble
#undef vec3_t
typedef vec_t vec3_t[3];

#include "bspfile.h"
#include "mathlib.h"
#include "trace.h"

#define MAX(a,b) (a > b ? a : b)
#define MIN(a,b) (a < b ? a : b)

dnode_t* gNode;

// return the leaf node for a point in 3D space... for model rooted on nodenum
static dleaf_t* TracePointInHull0Leaf(const int num, const vec3_t point)
{
	vec_t    d;
	dnode_t* node;
	dplane_t* plane;
	int nodenum;

	nodenum = num;
	// walk through the tree to find the leaf for the point...
	while (nodenum >= 0)
	{
		node = &dnodes[nodenum];
		plane = &dplanes[node->planenum];
		d = DotProduct(point, plane->normal) - plane->dist;
		if (d > 0)
			nodenum = node->children[0];
		else
			nodenum = node->children[1];
	}

	gNode = node;
	return &dleafs[-nodenum - 1];
}

// return contents of a coordinate in 3D space... using hull > 0 (collision hulls)
static int TracePointInLeaf(const int hullNumber, const vec3_t point)
{
	int      nodenum;
	vec_t    d;
	dclipnode_t* node;
	dplane_t* plane;

	nodenum = dmodels[0].headnode[hullNumber];
	// walk through the tree to find the leaf for the point...
	while (nodenum >= 0)
	{
		node = &dclipnodes[nodenum];
		plane = &dplanes[node->planenum];
		d = DotProduct(point, plane->normal) - plane->dist;
		if (d > 0)
			nodenum = node->children[0];
		else
			nodenum = node->children[1];
	}
	return nodenum;
}

// find the contents of a coordinate in 3D space... using hull > 0 (collision hulls)
// Just a wrapper for TracePointInLeaf
int BotmanPointContentsInHull(const int hullNumber, const vec3_t coord)
{
	if (hullNumber == point_hull)
		return BotmanPointContents(dmodels[0].headnode[0], coord);

	return TracePointInLeaf(hullNumber, coord);
}

// find the contents of a coordinate in 3D space... using hull0 (visibility)
int BotmanPointContents(const int nodenum, const vec3_t coord)
{
	dleaf_t* leaf;

	leaf = TracePointInHull0Leaf(nodenum, coord);

	// return contents (CONTENTS_EMPTY, CONTENTS_SOLID, CONTENTS_WATER, etc.)
	return leaf->contents;
}

int UTIL_PointContents(const Vector& x)
{
	vec3_t point;
	int WorldContent, ModelContent;
	int ent_index, model_index;
	char* value;

	point[0] = x.x;
	point[1] = x.y;
	point[2] = x.z;

	// Find the content in visibility hull of the world (dmodels[0])
	WorldContent = BotmanPointContents(0, point);

	// If not empty, return the result
	if (WorldContent != CONTENTS_EMPTY)
		return WorldContent;

	// If CONTENTS_EMPTY, need to do further check for water...

	// loop through all the entities looking for "func_water"...
	// And check whether we are in the water!
	ent_index = -1;
	while ((ent_index = FindEntityByClassname(ent_index, "func_water")) != -1) {
		value = ValueForKey(&entities[ent_index], "model");
		if (value[0]) {
			sscanf(value, "*%d", &model_index);
			ModelContent = BotmanPointContents(dmodels[model_index].headnode[0], point);
			if (ModelContent == CONTENT_WATER) {
				//printf("***Found water !!! (%.0f,%.0f,%.0f)\n",point[0],point[1],point[2]) ;
				return ModelContent;
			}
		}
	}
	return WorldContent;
}

// trace a line from start to end, fill in trace_t structure with result...

// Should rather use Quake SV_RecursiveHullCheck()
void BotmanTraceLine(vec3_t start, vec3_t end, botman_trace_t* tr)
{
	dleaf_t* startleaf, * endleaf;
	int      numsteps, totalsteps;
	vec3_t   move, step, position;
	float    dist, trace_dist;

	memset(tr, 0, sizeof(botman_trace_t));

	if ((start[0] < -4095) || (start[0] > 4095) ||
		(start[1] < -4095) || (start[1] > 4095) ||
		(start[2] < -4095) || (start[2] > 4095))
	{
		// start beyond edge of world is INVALID!!!
		fprintf(stderr, "TraceLine: start point beyond edge of world!\n");
	}

	if (end[0] > 4095.0f)
	{
		float percent = 4095.0f / end[0];
		end[1] = end[1] * percent;
		end[2] = end[2] * percent;
		end[0] = 4095.0f;
	}

	if (end[1] > 4095.0f)
	{
		float percent = 4095.0f / end[1];
		end[0] = end[0] * percent;
		end[2] = end[2] * percent;
		end[1] = 4095.0f;
	}

	if (end[2] > 4095.0f)
	{
		float percent = 4095.0f / end[2];
		end[0] = end[0] * percent;
		end[1] = end[1] * percent;
		end[2] = 4095.0f;
	}

	if (end[0] < -4095.0f)
	{
		float percent = 4095.0f / end[0];
		end[1] = end[1] * percent;
		end[2] = end[2] * percent;
		end[0] = -4095.0f;
	}

	if (end[1] < -4095.0f)
	{
		float percent = 4095.0f / end[1];
		end[0] = end[0] * percent;
		end[2] = end[2] * percent;
		end[1] = -4095.0f;
	}

	if (end[2] < -4095.0f)
	{
		float percent = 4095.0f / end[2];
		end[0] = end[0] * percent;
		end[1] = end[1] * percent;
		end[2] = -4095.0f;
	}

	// find the starting and ending leafs...
	startleaf = TracePointInHull0Leaf(0, start);
	endleaf = TracePointInHull0Leaf(0, end);

	// set endpos, fraction and contents to the default (trace completed)
	VectorCopy(end, tr->endpos);
	tr->fraction = 1.0f;
	tr->contents = endleaf->contents;

	if (startleaf->contents == CONTENTS_SOLID)
		tr->startsolid = TRUE;

	// is start and end leaf the same (couldn't possibly hit the world)...
	if (startleaf == endleaf) {
		if (startleaf->contents == CONTENTS_SOLID)
			tr->allsolid = TRUE;
		return;
	}

	// get the length of each interation of the loop...
	VectorSubtract(end, start, move);
	dist = (float)VectorLength(move);

	// determine the number of steps from start to end...
	if (dist > 1.0f)
		numsteps = totalsteps = (int)dist + 1;
	else
		numsteps = totalsteps = 1;

	// calculate the length of the step vector...
	VectorScale(move, (float)2 / numsteps, step);

	VectorCopy(start, position);

	while (numsteps)
	{
		VectorAdd(position, step, position);

		endleaf = TracePointInHull0Leaf(0, position);

		if ((endleaf->contents == CONTENTS_SOLID) ||  // we hit something solid...
			(endleaf->contents == CONTENTS_SKY))      // we hit the sky
		{
			vec3_t hitpos;

			VectorCopy(position, hitpos);

			// store the hit position
			VectorCopy(position, tr->hitpos);

			// back off one step before solid
			VectorSubtract(position, step, position);

			// store the end position and end position contents
			VectorCopy(position, tr->endpos);
			tr->contents = endleaf->contents;

			VectorSubtract(position, start, move);
			trace_dist = (float)VectorLength(move);
			tr->fraction = trace_dist / dist;

			break;  // break out of while loop
		}

		numsteps--;
	}
}

#define TWO_PI 6.2831853f
#define DELTA 0.001f

// find the face where the traceline hit...
dface_t* TraceLineFindFace(vec3_t start, botman_trace_t* tr)
{
	vec3_t v_intersect, v_normalized, v_temp;
	dface_t* return_face = NULL;
	float min_diff = 9999.9f;

	VectorSubtract(tr->endpos, start, v_normalized);
	VectorNormalize(v_normalized);

	dleaf_t* endleaf = TracePointInHull0Leaf(0, tr->endpos);

	unsigned short* p = dmarksurfaces + endleaf->firstmarksurface;

	// find a plane with endpos on one side and hitpos on the other side...
	for (int i = 0; i < endleaf->nummarksurfaces; i++)
	{
		int face_idx = *p++;

		dface_t* face = &dfaces[face_idx];

		dplane_t* plane = &dplanes[face->planenum];

		float d1 = DotProduct(tr->endpos, plane->normal) - plane->dist;
		float d2 = DotProduct(tr->hitpos, plane->normal) - plane->dist;

		if ((d1 > 0 && d2 <= 0) || (d1 <= 0 && d2 > 0))
		{
			// found a plane, find the intersection point in the plane...

			vec3_t plane_origin, v_angle1, v_angle2;

			VectorScale(plane->normal, plane->dist, plane_origin);

			float dist = DistanceToIntersection(start, v_normalized, plane_origin, plane->normal);

			if (dist < 0)
				return NULL;  // can't find intersection

			VectorScale(v_normalized, dist, v_temp);
			VectorAdd(start, v_temp, v_intersect);

			// loop through all of the vertexes of all the edges of this face and
			// find the angle between vertex-n, v_intersect and vertex-n+1, then add
			// all these angles together.  if the sum of these angles is 360 degrees
			// (or 2 PI radians), then the intersect point lies within that polygon.

			float angle_sum = 0.0f;

			// loop though all of the edges, getting the vertexes...
			for (int edge_index = 0; edge_index < face->numedges; edge_index++)
			{
				vec3_t vertex1, vertex2;

				// get the coordinates of the vertex of this edge...
				int edge = dsurfedges[face->firstedge + edge_index];

				if (edge < 0)
				{
					edge = -edge;
					dedge_t* e = &dedges[edge];
					VectorCopy(dvertexes[e->v[1]].point, vertex1);
					VectorCopy(dvertexes[e->v[0]].point, vertex2);
				}
				else
				{
					dedge_t* e = &dedges[edge];
					VectorCopy(dvertexes[e->v[0]].point, vertex1);
					VectorCopy(dvertexes[e->v[1]].point, vertex2);
				}

				// now create vectors from the vertexes to the plane intersect point...
				VectorSubtract(vertex1, v_intersect, v_angle1);
				VectorSubtract(vertex2, v_intersect, v_angle2);

				VectorNormalize(v_angle1);
				VectorNormalize(v_angle2);

				// find the angle between these vectors...
				float angle = DotProduct(v_angle1, v_angle2);

				angle = (float)acos(angle);

				angle_sum += angle;

				edge++;
			}

			// is the sum of the angles 360 degrees (2 PI)?...
			if ((angle_sum >= (TWO_PI - DELTA)) && (angle_sum <= (TWO_PI + DELTA)))
			{
				// find the difference between the sum and 2 PI...
				float diff = (float)fabs(angle_sum - TWO_PI);

				if (diff < min_diff)  // is this the BEST so far?...
				{
					min_diff = diff;
					return_face = face;
				}
			}
		}
	}

	return return_face;
}

static void ConvertTraceResult(botman_trace_t* tr, TraceResult* ptr)
{
	ptr->fAllSolid = tr->allsolid;
	ptr->fStartSolid = tr->startsolid;
	ptr->fInOpen = tr->contents == CONTENTS_EMPTY; // TO BE FIXED
	ptr->fInWater = tr->contents == CONTENTS_WATER; // ????
	ptr->flFraction = tr->fraction;
	ptr->vecEndPos[0] = tr->endpos[0];
	ptr->vecEndPos[1] = tr->endpos[1];
	ptr->vecEndPos[2] = tr->endpos[2];
	ptr->flPlaneDist = 0.0f; // TO BE FIXED ?
	ptr->vecPlaneNormal[0] = 0; // TO BE FIXED
	ptr->vecPlaneNormal[1] = 0; // TO BE FIXED
	ptr->vecPlaneNormal[2] = 0; // TO BE FIXED
	ptr->pHit = NULL; // TO BE FIXED
	ptr->iHitgroup = 0;
}

/*
==================
SV_HullPointContents

==================
*/
static int SV_HullPointContents(int nodenum, vec3_t p)
{
	float		d;
	dclipnode_t* node;
	dplane_t* plane;

	while (nodenum >= 0)
	{
		if (nodenum > numclipnodes) {
			fprintf(stderr, "SV_HullPointContents: bad node number");
			exit(1);
		}

		node = &dclipnodes[nodenum];
		plane = &dplanes[node->planenum];

		d = DotProduct(plane->normal, p) - plane->dist;
		if (d < 0)
			nodenum = node->children[1];
		else
			nodenum = node->children[0];
	}
	return nodenum;
}

// 1/32 epsilon to keep floating point happy
#define	DIST_EPSILON	(0.03125)

/*
==================
SV_RecursiveHullCheck

==================
*/
qboolean SV_RecursiveHullCheck(int hullNumber, // Id of hull, 1 = human, ...
	int nodenum,  // Clip node to analyse from
	float p1f, float p2f, // Initialized to 0 & 1
	vec3_t p1, vec3_t p2, // P1 = start, P2 = end
	TraceResult* trace)
{
	dclipnode_t* node;
	dplane_t* plane;
	float		t1, t2;
	float		frac;
	int			i;
	vec3_t		mid;
	int			side;
	float		midf;

	if ((hullNumber < human_hull) || (hullNumber > head_hull)) {
		fprintf(stderr, "Wrong hullNumber in SV_RecursiveHullCheck\n");
		exit(1);
	}

	// check for empty
	if (nodenum < 0)
	{
		if (nodenum != CONTENTS_SOLID)
		{
			trace->fAllSolid = false;
			if (nodenum == CONTENTS_EMPTY)
				trace->fInOpen = true;
			else
				trace->fInWater = true;
		}
		else
			trace->fStartSolid = true;
		return true;		// empty
	}

	if (nodenum > numclipnodes) {
		fprintf(stderr, "SV_RecursiveHullCheck: bad node number");
		exit(1);
	}

	//
	// find the point distances
	//
	node = &dclipnodes[nodenum];
	if ((node->planenum > numplanes) || (node->planenum < 0)) {
		fprintf(stderr, "SV_RecursiveHullCheck: bad plane number");
		exit(1);
	}
	plane = &dplanes[node->planenum];

	t1 = DotProduct(plane->normal, p1) - plane->dist;
	t2 = DotProduct(plane->normal, p2) - plane->dist;

	// Check whether P1 & P2 are on the same side of the plane
	// Then, continue to check in this smaller space
	if (t1 >= 0 && t2 >= 0)
		return SV_RecursiveHullCheck(hullNumber, node->children[0], p1f, p2f, p1, p2, trace);
	if (t1 < 0 && t2 < 0)
		return SV_RecursiveHullCheck(hullNumber, node->children[1], p1f, p2f, p1, p2, trace);

	// P1 & P2 are on opposite side of the plane...
	if (t1 == t2) {
		fprintf(stderr, "SV_RecursiveHullCheck: same distance from plane");
		exit(1);
	}
	// put the crosspoint DIST_EPSILON pixels on the near side
	if (t1 < 0.0)
		frac = (t1 + DIST_EPSILON) / (t1 - t2);
	else
		frac = (t1 - DIST_EPSILON) / (t1 - t2);
	if (frac < 0.0)
		frac = 0.0;
	if (frac > 1.0)
		frac = 1.0;

	midf = p1f + (p2f - p1f) * frac;
	for (i = 0; i < 3; i++)
		mid[i] = p1[i] + frac * (p2[i] - p1[i]);

	side = (t1 < 0);

	// move up to the node
	if (!SV_RecursiveHullCheck(hullNumber, node->children[side], p1f, midf, p1, mid, trace))
		return false;

	if (SV_HullPointContents(node->children[side ^ 1], mid) != CONTENTS_SOLID)
		// go past the node
		return SV_RecursiveHullCheck(hullNumber, node->children[side ^ 1], midf, p2f, mid, p2, trace);

	if (trace->fAllSolid)
		return false;		// never got out of the solid area

//==================
// the other side of the node is solid, this is the impact point
//==================
	if (!side)
	{
		VectorCopy(plane->normal, trace->vecPlaneNormal);
		trace->flPlaneDist = plane->dist;
	}
	else
	{
		VectorSubtract(vec3_origin, plane->normal, trace->vecPlaneNormal);
		trace->flPlaneDist = -plane->dist;
	}

	// If mid is SOLID, move mid backwards until it is in EMPTY space
	while (SV_HullPointContents(dmodels[0].headnode[hullNumber], mid) == CONTENTS_SOLID)
	{
		frac -= 0.1;
		if (frac < 0.0) // shouldn't really happen, but does occasionally
		{
			//			fprintf(stderr,"SV_RecursiveHullCheck(): backup past 0\n");
			break; // Exit loop immediately
		}
		// Adjust mid to reflect the value of frac
		midf = p1f + (p2f - p1f) * frac;
		for (i = 0; i < 3; i++)
			mid[i] = p1[i] + frac * (p2[i] - p1[i]);
	}

	trace->flFraction = midf;
	VectorCopy(mid, trace->vecEndPos);

	return false;
}

void UTIL_TraceHull(const Vector& vecStart, const Vector& vecEnd,
	IGNORE_MONSTERS igmon, int hullNumber, edict_t* pentIgnore,
	TraceResult* ptr)
{
	vec3_t start, end;
	botman_trace_t tr;

	start[0] = vecStart.x;
	start[1] = vecStart.y;
	start[2] = vecStart.z;
	end[0] = vecEnd.x;
	end[1] = vecEnd.y;
	end[2] = vecEnd.z;
	memset(&tr, 0, sizeof(botman_trace_t));
	tr.fraction = 1.0;
	if (hullNumber == point_hull) {
		BotmanTraceLine(start, end, &tr);
		ConvertTraceResult(&tr, ptr);
	}
	else {
		TraceResult EntTrace;
		int ent_index, model_index;
		char* value;

		// Set the TraceResult default values
		memset(ptr, 0, sizeof(TraceResult));
		ptr->flFraction = 1.0;
		ptr->fAllSolid = true;
		SV_RecursiveHullCheck(hullNumber, dmodels[0].headnode[hullNumber],
			0.0, 1.0, start, end, ptr);

		// loop through all the entities looking for "func_wall"...
		// And check for collisions with them...
		ent_index = -1;
		while ((ent_index = FindEntityByClassname(ent_index, "func_wall")) != -1) {
			value = ValueForKey(&entities[ent_index], "model");
			if (value[0]) {
				sscanf(value, "*%d", &model_index);
				memset(&EntTrace, 0, sizeof(TraceResult));
				EntTrace.flFraction = 1.0;
				EntTrace.fAllSolid = true;
				SV_RecursiveHullCheck(hullNumber, dmodels[model_index].headnode[hullNumber],
					0.0, 1.0, start, end, &EntTrace);
				if (EntTrace.flFraction < ptr->flFraction)
					memcpy(ptr, &EntTrace, sizeof(TraceResult));
			}
		}
	}
	return;
}