/***
*
*  Copyright (c) 1998, Valve LLC. All rights reserved.
*
*  This product contains software technology licensed from Id
*  Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*  All Rights Reserved.
*
****/

// scriplib.h

#ifndef __CMDLIB__
#include "cmdlib.h"
#endif

#ifndef SCRIPLIB_H
#define SCRIPLIB_H

#define MAXTOKEN  4096

extern char token[MAXTOKEN];
extern char* scriptbuffer, * script_p, * scriptend_p;
extern int  grabbed;
extern int  scriptline;
extern qboolean   endofscript;

void LoadScriptFile(char* filename);
void ParseFromMemory(char* buffer, int size);

qboolean GetToken(qboolean crossline);
void UnGetToken(void);
qboolean TokenAvailable(void);

#endif  // SCRIPLIB_H
