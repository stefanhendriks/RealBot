/* Simply dumps the node files in ASCII and made some basic tests...

evyncke@hec.be, June 2004

Based on routines from Stefan H.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>
#include <entity_state.h>
#include "../bot.h"
#include "../NodeMachine.h"

extern char* Version;

/* Copy from NodeMachine.cpp by Stefan Hendricks */

tNode Nodes[MAX_NODES];     // Nodes
tInfoNode InfoNodes[MAX_NODES];       // Info for Nodes
int Meredians[MAX_MEREDIANS][MAX_MEREDIANS];    // Meredian lookup search for Nodes
int iMaxUsedNodes;

// Input: Vector, Output X and Y Meredians
void VectorToMeredian(Vector vOrigin, int* iX, int* iY)
{
	// Called for lookupt and for storing
	int iCoordX = abs(vOrigin.x + 8192.0);       // map height (converts from - to +)
	int iCoordY = abs(vOrigin.y + 8192.0);       // map width (converts from - to +)

	// Meredian:
	iCoordX = abs(iCoordX / SIZE_MEREDIAN);
	iCoordY = abs(iCoordY / SIZE_MEREDIAN);

	*iX = iCoordX;
	*iY = iCoordY;
}

void load(char* mapname)
{
	char filename[256];
	int i, j, n;

	strcpy(filename, mapname);
	strcat(filename, ".rbn");     // nodes file

	FILE* rbl;
	rbl = fopen(filename, "rb");

	if (rbl != NULL)
	{
		int iVersion;
		fread(&iVersion, sizeof(int), 1, rbl);

		// Version 1.0
		if (iVersion == FILE_NODE_VER1)
		{
			for (i = 0; i < MAX_NODES; i++)
			{
				fread(&Nodes[i].origin, sizeof(Vector), 1, rbl);
				for (n = 0; n < MAX_NEIGHBOURS; n++)
				{
					fread(&Nodes[i].iNeighbour[n], sizeof(int), 1, rbl);
				}

				// save bit flags
				fread(&Nodes[i].iNodeBits, sizeof(int), 1, rbl);

				if (Nodes[i].origin != Vector(9999, 9999, 9999))
					iMaxUsedNodes = i;
			}
		}
	}
	else {
		fprintf(stderr, "Cannot open file %s\n", filename);
		exit;
	}
	printf("%d nodes loaded out of %d.\n", iMaxUsedNodes, MAX_NODES);
	fclose(rbl);

	// Zero the Meredians table
	for (i = 0;i < MAX_MEREDIANS;i++)
		for (j = 0;j < MAX_MEREDIANS;j++)
			Meredians[i][j] = 0;

	// Add nodes to meredians
	for (i = 0; i < MAX_NODES; i++)
		if (Nodes[i].origin != Vector(9999, 9999, 9999))
		{
			int iX, iY;
			VectorToMeredian(Nodes[i].origin, &iX, &iY);
			if (iX > -1 && iY > -1)
				Meredians[iX][iY] ++;
		}
}

void PrintNodesPair(int iMe, int iNext, Vector Me, Vector Next)
{
	int MeX, MeY, NextX, NextY; // Meredians

	printf("  dist(%d,%d)=%.0f", iMe, iNext, (Next - Me).Length());
	VectorToMeredian(Me, &MeX, &MeY);
	VectorToMeredian(Next, &NextX, &NextY);
	if (abs(Me.z - Next.z) > 5) printf(", altitude diff=%d", abs(Me.z - Next.z));
	if ((MeX != NextX) && (MeY != NextY))
		printf(", Both meredians do not match!");
	else if ((MeX != NextX) || (MeY != NextY))
		printf(", One Meredian does not match!");
	printf("\n");
}

// Analyze the RBN file which has just been read...

void AnalyseNeighbours(void)
{
	int MeX, MeY, NextX, NextY; // Meredians
	int Count, i, j;
	int Histogram[MAX_NEIGHBOURS + 1]; // Will count the frequency of nodes having 0... N neighbours
	Vector Me, Next;
	float Distance;

	for (i = 0;i < MAX_NEIGHBOURS + 1;i++)
		Histogram[i] = 0;
	Count = 0;
	for (i = 0; i < iMaxUsedNodes; i++)
	{
		for (j = 0; (j < MAX_NEIGHBOURS) && (Nodes[i].iNeighbour[j] >= 0);j++)
			Count++;
		Histogram[j]++;
	}
	printf("There are %d neighbours (i.e. %.1f neighbour(s) per node out of %d)\n", Count, (float)Count / (float)iMaxUsedNodes, MAX_NEIGHBOURS);
	printf("Neighbours distribution\n");
	for (j = 0;j < MAX_NEIGHBOURS + 1;j++)
		printf("Nodes with %d neighbours: %d\n", j, Histogram[j]);

	if (Histogram[0]) {
		printf("Isolated nodes are at: ");
		for (i = 0; i < iMaxUsedNodes; i++)
			if (Nodes[i].iNeighbour[0] < 0)
				printf("%d(%.0f, %.0f, %.0f) ", i, Nodes[i].origin.x, Nodes[i].origin.y, Nodes[i].origin.z);
		printf("\n");
		printf("Two consecutive isolated nodes that were created one after the other (HIGHLY suspicious):\n");
		for (i = 0; i < iMaxUsedNodes - 1; i++)
			if (Nodes[i].iNeighbour[0] < 0 && Nodes[i + 1].iNeighbour[0] < 0) {
				Me = Nodes[i].origin;
				Next = Nodes[i + 1].origin;
				PrintNodesPair(i, i + 1, Me, Next);
			}
		for (i = 1; i < iMaxUsedNodes; i++)
			if (Nodes[i].iNeighbour[0] < 0) {
				Me = Nodes[i].origin;
				Next = Nodes[i - 1].origin;
				Distance = (Next - Me).Length();
				if (Distance > NODE_ZONE * 2) continue;
				PrintNodesPair(i, i - 1, Me, Next);
			}
		printf("Isolated nodes that are close to the next one (HIGHLY suspicious):\n");
		for (i = 0; i < iMaxUsedNodes - 1; i++)
			if (Nodes[i].iNeighbour[0] < 0) {
				Me = Nodes[i].origin;
				Next = Nodes[i + 1].origin;
				Distance = (Next - Me).Length();
				if (Distance > NODE_ZONE * 2) continue;
				PrintNodesPair(i, i + 1, Me, Next);
			}
	}
}

void AnalyseMeredians(void)
{
	int i, j;

	for (i = 0;i < MAX_MEREDIANS;i++)
		for (j = 0;j < MAX_MEREDIANS;j++)
			if (Meredians[i][j] >= MAX_NODES_IN_MEREDIANS) printf("Too many nodes on meredians(%d,%d): %d (max %d)\n",
				i, j, Meredians[i][j], MAX_NODES_IN_MEREDIANS);
}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage is %s mapname\n", argv[0]);
		exit;
	}
	printf("DumpNodes Version %s\nBy eric@vyncke.org\n", Version);
	load(argv[1]);
	AnalyseNeighbours();
	AnalyseMeredians();
}