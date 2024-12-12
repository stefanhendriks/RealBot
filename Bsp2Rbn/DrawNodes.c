// Based on Pierre-Marie Batty bmpfile.cpp

// Tuned for Realbot .RBN files by evyncke@students.hec.be, June 2004

// RACC - AI development project for first-person shooter games derived from Valve's Half-Life
// (http://www.racc-ai.com/)
//
// The game to engine interfacing code is based on the work done by Jeffrey 'botman' Broome
// (http://planethalflife.com/botman/)
//
// This project is partially based on the work done by Eric Bieschke in his BSDbot
// (http://gamershomepage.com/csbot/)
//
// This project is partially based on the work done by Brendan "Spyro" McCarthy in his ODD Bot
// (http://oddbot.hlfusion.com/)
//
// This project is partially based on the work done by Alistair 'eLiTe' Stewart in his TEAMbot
// (http://www.planethalflife.com/teambot/)
//
// The BMP writing functions in this file come primarily from botman's BSP slicer utility
// (http://planethalflife.com/botman/)
//
// Rational Autonomous Cybernetic Commandos AI
//
// bmpfile.cpp
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>
#include <entity_state.h>

extern char* Version;

// width and height of the debug bitmap image
#define DEBUG_BMP_WIDTH 2048
#define DEBUG_BMP_HEIGHT 2048

float scalex, scaley, scale;
char* bmp_buffer;

#include "../bot.h"
#include "../NodeMachine.h"

extern char* Version;

/* Copy from NodeMachine.cpp by Stefan Hendricks */

tNode Nodes[MAX_NODES];     // Nodes
tInfoNode InfoNodes[MAX_NODES];       // Info for Nodes
tMeredian Meredians[MAX_MEREDIANS][MAX_MEREDIANS];    // Meredian lookup search for Nodes
int iMaxUsedNodes;
float maxx, maxy, minx, miny;

void load(char* mapname)
{
	char filename[256];
	int i, n;

	// Set Directory name
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
}

void InitDebugBitmap(void)
{
	// this function allocates memory and clears the debug bitmap buffer

	if (bmp_buffer)
		free(bmp_buffer); // reliability check, free BMP buffer if already allocated
	bmp_buffer = NULL;
	bmp_buffer = (char*)malloc(DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT); // allocate memory
	if (bmp_buffer == NULL) {
		fprintf(stderr, "InitDebugBitmap(): unable to allocate %d kbytes for BMP buffer!\n", DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT / 1024);
		exit(1);
	}

	memset(bmp_buffer, 14, DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT); // Set all to white
	return; // yes, it's as simple as that
}

void DrawPoint(const Vector v, unsigned char color)
{
	int offset, fraction, x0, y0;
	float scalex, scaley, scale;

	if (bmp_buffer == NULL)
	{
		fprintf(stderr, "DrawLineInDebugBitmap(): function called with NULL BMP buffer!\n");
		return; // reliability check: cancel if bmp buffer unallocated
	}

	// first compute the X and Y divider scale, and take the greatest of both
	scalex = (maxx - minx) / DEBUG_BMP_WIDTH;
	scaley = (maxy - miny) / DEBUG_BMP_WIDTH;
	if (scalex > scaley)
		scale = scalex + scalex / 100; // add a little offset (margin) for safety
	else
		scale = scaley + scaley / 100; // add a little offset (margin) for safety

	 // translate the world coordinates in image pixel coordinates
	x0 = (int)((v.x - minx) / scale);
	y0 = (int)((v.y - miny) / scale);

	offset = y0 * DEBUG_BMP_WIDTH + x0;
	if ((offset < 0) || (offset >= DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT)) {
		fprintf(stderr, "DrawLineInDebugBitmap(): bad BMP buffer index %d (range 0 - %d)\n", offset, DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT);
		exit(1);
	}

	bmp_buffer[offset] = color; // draw the point itself
	if (offset + 1 < DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT) bmp_buffer[offset + 1] = color; // make a small star on the right
	if (offset - 1 >= 0) bmp_buffer[offset - 1] = color; // make a small star on the left
	if (offset + DEBUG_BMP_WIDTH < DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT) bmp_buffer[offset + DEBUG_BMP_WIDTH] = color;
	if (offset - DEBUG_BMP_WIDTH >= 0) bmp_buffer[offset - DEBUG_BMP_WIDTH] = color; // make a small star above
}

void DrawLineInDebugBitmap(const Vector v_from, const Vector v_to, unsigned char color)
{
	// blind copy of botman's Bresenham(). This function prints a vector line into a bitmap dot
	// matrix. The dot matrix (bmp_buffer) is a global array. The size of the bitmap is always
	// assumed to be DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT pixels (currently 2000 * 2000 to fit with
	// the size of the universe, with an adaptative unit scale, up to 1 pixel = 10 vector units).

	int x0, y0, x1, y1;
	int dx, stepx, dy, stepy;
	int offset, fraction;

	if (bmp_buffer == NULL)
	{
		fprintf(stderr, "DrawLineInDebugBitmap(): function called with NULL BMP buffer!\n");
		return; // reliability check: cancel if bmp buffer unallocated
	}

	// translate the world coordinates in image pixel coordinates
	x0 = (int)((v_from.x - minx) / scale);
	y0 = (int)((v_from.y - miny) / scale);
	x1 = (int)((v_to.x - minx) / scale);
	y1 = (int)((v_to.y - miny) / scale);

	dx = (x1 - x0) * 2;
	dy = (y1 - y0) * 2;
	if (dx < 0) { dx = -dx;  stepx = -1; }
	else stepx = 1;
	if (dy < 0) { dy = -dy;  stepy = -1; }
	else stepy = 1;

	offset = y0 * DEBUG_BMP_WIDTH + x0;
	if ((offset < 0) || (offset >= DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT)) {
		fprintf(stderr, "DrawLineInDebugBitmap(): bad BMP buffer index %d (range 0 - %d)\n", offset, DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT);
		exit(1);
	}

	bmp_buffer[offset] = color; // draw the first point of the line

	// is the line rather horizontal than vertical ? We need to know this to determine the step
	// advance in the Bresenham grid, either we draw y = f(x), or x = f(y).
	if (dx > dy)
	{
		// the line is rather horizontal, we can draw it safely for incremental values of x

		fraction = 2 * dy - dx; // fraction of height in x0 pixel's 'square' where y0 should be

		// while we've not reached the end of the segment...
		while (x0 != x1)
		{
			// if y0 should rather be drawn on a different height than its previous height...
			if (fraction >= 0)
			{
				y0 += stepy; // draw it one pixel aside, then (depending on line orientation)
				fraction -= 2 * dx; // and reset its fraction (Bresenham, not sure I get the math)
			}
			x0 += stepx; // in either case, draw x0 one pixel aside its previous position
			fraction += 2 * dy; // and update y0's fraction (not sure I get the math - but whatever)

			// compute the offset in the BMP buffer corresponding to this point
			offset = y0 * DEBUG_BMP_WIDTH + x0;
			if ((offset < 0) || (offset >= DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT)) {
				fprintf(stderr, "DrawLineInDebugBitmap(): bad BMP buffer index %d (range 0 - %d)\n", offset, DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT);
				exit(1);
			}

			bmp_buffer[offset] = color; // set this point to have the specified color
		}
	}
	else
	{
		// else the line is rather vertical, we NEED to draw it for incremental values of y (if we
		// did it for incremental values of x instead, we would drop half the pixels).

		fraction = 2 * dx - dy; // fraction of width in y0 pixel's 'square' where x0 should be

		// while we've not reached the end of the segment...
		while (y0 != y1)
		{
			// if x0 should rather be drawn on a different width than its previous width...
			if (fraction >= 0)
			{
				x0 += stepx; // draw it one pixel aside, then (depending on line orientation)
				fraction -= 2 * dy; // and reset its fraction (Bresenham, not sure I get the math)
			}
			y0 += stepy; // in either case, draw y0 one pixel aside its previous position
			fraction += 2 * dx; // and update x0's fraction (not sure I get the math - but whatever)

			// compute the offset in the BMP buffer corresponding to this point
			offset = y0 * DEBUG_BMP_WIDTH + x0;
			if ((offset < 0) || (offset >= DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT)) {
				fprintf(stderr, "DrawLineInDebugBitmap(): bad BMP buffer index %d (range 0 - %d)\n", offset, DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT);
				exit(1);
			}
			bmp_buffer[offset] = color; // set this point to have the specified color
		}
	}

	return; // finished, segment has been printed into the BMP dot matrix
}

void WriteDebugBitmap(const char* mapname)
{
	// this function writes the debug bitmap image buffer in a .BMP file to disk. The format is
	// 256 color and 2000 * 2000 pixels. The center of the world being roughly the center of the
	// bitmap. The bitmap is stored in the file specified by 'filename' (which can be a relative
	// path from the Half-Life base directory).
	char filename[256];
	FILE* fp;
	int data_start, file_size;
	unsigned long dummy;

	if (bmp_buffer == NULL)
	{
		fprintf(stderr, "WriteDebugBitmap(): function called with NULL BMP buffer!\n");
		return; // reliability check: cancel if bmp buffer unallocated
	}

	// open (or create) the .bmp file for writing in binary mode...
   // Set Directory name
 //  strcpy (filename, "data/cstrike/maps/");
	strcpy(filename, mapname);
	strcat(filename, ".bmp");     // bitmap file
	fp = fopen(filename, "wb");
	if (fp == NULL)
	{
		fprintf(stderr, "RACC: WriteDebugBitmap(): unable to open BMP file!\n");
		if (bmp_buffer)
			free(bmp_buffer); // cannot open file, free DXF buffer
		bmp_buffer = NULL;
		return; // cancel if error creating file
	}

	// write the BMP header
	fwrite("BM", 2, 1, fp); // write the BMP header tag
	fseek(fp, sizeof(unsigned long), SEEK_CUR); // skip the file size field (will write it last)
	fwrite("\0\0", sizeof(short), 1, fp); // dump zeros in the first reserved field (unused)
	fwrite("\0\0", sizeof(short), 1, fp); // dump zeros in the second reserved field (unused)
	fseek(fp, sizeof(unsigned long), SEEK_CUR); // skip the data start field (will write it last)

	// write the info header
	dummy = 40;
	fwrite(&dummy, sizeof(unsigned long), 1, fp); // write the info header size (does 40 bytes)
	dummy = DEBUG_BMP_WIDTH;
	fwrite(&dummy, sizeof(long), 1, fp); // write the image width (2000 px)
	dummy = DEBUG_BMP_HEIGHT;
	fwrite(&dummy, sizeof(long), 1, fp); // write the image height (2000 px)
	dummy = 1;
	fwrite(&dummy, sizeof(short), 1, fp); // write the # of planes (1)
	dummy = 8;
	fwrite(&dummy, sizeof(short), 1, fp); // write the bit count (8)
	dummy = 0;
	fwrite(&dummy, sizeof(unsigned long), 1, fp); // write the compression id (no compression)
	dummy = DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT;
	fwrite(&dummy, sizeof(unsigned long), 1, fp); // write the image size (2000 * 2000)
	dummy = 0;
	fwrite(&dummy, sizeof(long), 1, fp); // write the X pixels per meter (not specified)
	fwrite(&dummy, sizeof(long), 1, fp); // write the Y pixels per meter (not specified)
	dummy = 256;
	fwrite(&dummy, sizeof(unsigned long), 1, fp); // write the # of colors used (all)
	fwrite(&dummy, sizeof(unsigned long), 1, fp); // write the # of important colors (wtf ?)

	// write the color palette (R, G, B, reserved byte)
	fputc(0x00, fp); fputc(0x00, fp); fputc(0x00, fp); fputc(0x00, fp); // 0=BLACK
	fputc(0xFF, fp); fputc(0xFF, fp); fputc(0xFF, fp); fputc(0x00, fp); // 1=WHITE
	fputc(0x80, fp); fputc(0x80, fp); fputc(0x80, fp); fputc(0x00, fp); // 2=GREY
	fputc(0xC0, fp); fputc(0xC0, fp); fputc(0xC0, fp); fputc(0x00, fp); // 3=SILVER
	fputc(0x80, fp); fputc(0x00, fp); fputc(0x00, fp); fputc(0x00, fp); // 4=DARK RED
	fputc(0xFF, fp); fputc(0x00, fp); fputc(0x00, fp); fputc(0x00, fp); // 5=RED
	fputc(0x80, fp); fputc(0x80, fp); fputc(0x00, fp); fputc(0x00, fp); // 6=DARK YELLOW
	fputc(0xFF, fp); fputc(0xFF, fp); fputc(0x00, fp); fputc(0x00, fp); // 7=YELLOW
	fputc(0x00, fp); fputc(0x80, fp); fputc(0x00, fp); fputc(0x00, fp); // 8=DARK GREEN
	fputc(0x00, fp); fputc(0xFF, fp); fputc(0x00, fp); fputc(0x00, fp); // 9=GREEN
	fputc(0x00, fp); fputc(0x00, fp); fputc(0x80, fp); fputc(0x00, fp); // 10=DARK BLUE
	fputc(0x00, fp); fputc(0x00, fp); fputc(0x80, fp); fputc(0x00, fp); // 11=BLUE
	fputc(0x80, fp); fputc(0x00, fp); fputc(0x80, fp); fputc(0x00, fp); // 12=DARK PURPLE
	fputc(0x80, fp); fputc(0x00, fp); fputc(0x80, fp); fputc(0x00, fp); // 13=PURPLE
	fputc(0xFF, fp); fputc(0xFF, fp); fputc(0xFF, fp); fputc(0x00, fp); // 14=WHITE
	fputc(0xEF, fp); fputc(0xEF, fp); fputc(0xEF, fp); fputc(0x00, fp); // 15=WHITE-GREY
	fputc(0xDF, fp); fputc(0xDF, fp); fputc(0xDF, fp); fputc(0x00, fp); // 16=GREY
	fputc(0xCF, fp); fputc(0xCF, fp); fputc(0xCF, fp); fputc(0x00, fp); // 17=DARKGREY
	fputc(0xBF, fp); fputc(0xBF, fp); fputc(0xBF, fp); fputc(0x00, fp); // 18=DARKGREY
	fputc(0xAF, fp); fputc(0xAF, fp); fputc(0xAF, fp); fputc(0x00, fp); // 19=DARKGREY

	for (dummy = 20; dummy < 256; dummy++)
	{
		// fill out the rest of the palette with zeros
		fputc(0x00, fp); fputc(0x00, fp); fputc(0x00, fp); fputc(0x00, fp);
	}

	// write the actual image data
	data_start = ftell(fp); // get the data start position (that's where we are now)
	fwrite(bmp_buffer, DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT, 1, fp); // write the image
	file_size = ftell(fp); // get the file size now that the image is dumped

	// now that we've dumped our data, we know the file size and the data start position

	fseek(fp, 0, SEEK_SET); // rewind
	fseek(fp, 2, SEEK_CUR); // skip the BMP header tag "BM"
	fwrite(&file_size, sizeof(unsigned long), 1, fp); // write the file size at its location
	fseek(fp, sizeof(short), SEEK_CUR); // skip the first reserved field
	fseek(fp, sizeof(short), SEEK_CUR); // skip the second reserved field
	fwrite(&data_start, sizeof(unsigned long), 1, fp); // write the data start at its location

	fclose(fp); // finished, close the BMP file

	if (bmp_buffer)
		free(bmp_buffer); // and free the BMP buffer
	bmp_buffer = NULL;

	return; // and return
}

void FindMinMax(void)
{
	int i;

	minx = miny = 9999.0;
	maxx = maxy = -9999.0;
	for (i = 0; i < iMaxUsedNodes; i++) {
		if (Nodes[i].origin.x > maxx) maxx = Nodes[i].origin.x;
		if (Nodes[i].origin.y > maxy) maxy = Nodes[i].origin.y;
		if (Nodes[i].origin.x < minx) minx = Nodes[i].origin.x;
		if (Nodes[i].origin.y < miny) miny = Nodes[i].origin.y;
	}

	// Add some margin
	minx -= 32;
	miny -= 32;
	maxx += 32;
	maxy += 32;
	// first compute the X and Y divider scale, and take the greatest of both
	scalex = (maxx - minx) / DEBUG_BMP_WIDTH;
	scaley = (maxy - miny) / DEBUG_BMP_HEIGHT;
	if (scalex > scaley)
		scale = scalex + scalex / 100; // add a little offset (margin) for safety
	else
		scale = scaley + scaley / 100; // add a little offset (margin) for safety
}

// Mark meridians as slighly darker in alternance

void MarkAxis(void)
{
	int x, y, x0, y0;

	x0 = (int)((0 - minx) / scale);
	y0 = (int)((0 - miny) / scale);

	// Mark X axis by keeping X to 0 and varying Y
	if ((minx < 0) && (0 < maxx))
		for (y = 0; y < DEBUG_BMP_HEIGHT; y++)
			bmp_buffer[y * DEBUG_BMP_WIDTH + x0] += 2;

	// Mar
	if ((miny < 0) && (0 < maxy))
		for (x = 0; x < DEBUG_BMP_WIDTH; x++)
			bmp_buffer[y0 * DEBUG_BMP_WIDTH + x] += 2;
}

void MarkMeredians(void)
{
	int x, y;
	int Meredian;

	// Mark some meredians
	for (x = 0; x < DEBUG_BMP_WIDTH; x++) {
		Meredian = abs(((float)x * scale + minx + 8192.0) / (float)SIZE_MEREDIAN);
		if (Meredian & 0x01) {
			for (y = 0; y < DEBUG_BMP_HEIGHT; y++)
				bmp_buffer[y * DEBUG_BMP_WIDTH + x]++;
		}
	}

	// Mark some meredians
	for (y = 0; y < DEBUG_BMP_HEIGHT; y++) {
		Meredian = abs(((float)y * scale + miny + 8192.0) / (float)SIZE_MEREDIAN);
		if (Meredian & 0x01) {
			for (x = 0; x < DEBUG_BMP_HEIGHT; x++)
				bmp_buffer[y * DEBUG_BMP_WIDTH + x]++;
		}
	}
}

void PlotNodes(void)
{
	int i, j;

	for (i = 0; i < iMaxUsedNodes; i++)
		for (j = 0; (j < MAX_NEIGHBOURS) && (Nodes[i].iNeighbour[j] >= 0); j++)
			DrawLineInDebugBitmap(Nodes[i].origin,
				Nodes[Nodes[i].iNeighbour[j]].origin, 0);
	for (i = 0; i < iMaxUsedNodes; i++)
		DrawPoint(Nodes[i].origin, 5);
}

int main(int argc, char* argv[])
{
	printf("DrawNodes Version %s\nBy eric@vyncke.org\n", Version);
	if (argc != 2) {
		fprintf(stderr, "Usage is %s mapname\n", argv[0]);
		exit;
	}
	load(argv[1]);
	FindMinMax();
	InitDebugBitmap();
	MarkMeredians();
	MarkAxis();
	PlotNodes();
	WriteDebugBitmap(argv[1]);
}