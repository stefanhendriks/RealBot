/***
*
*  Copyright (c) 1999, Valve LLC. All rights reserved.
*
*  This product contains software technology licensed from Id
*  Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*  All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

/**
  * RealBot : Artificial Intelligence
  * Version : Work In Progress
  * Author  : Stefan Hendriks
  * Url     : http://realbot.bots-united.com
  **
  * DISCLAIMER
  *
  * History, Information & Credits:
  * RealBot is based partially upon the HPB-Bot Template #3 by Botman
  * Thanks to Ditlew (NNBot), Pierre Marie Baty (RACCBOT), Tub (RB AI PR1/2/3)
  * Greg Slocum & Shivan (RB V1.0), Botman (HPB-Bot) and Aspirin (JOEBOT). And
  * everybody else who helped me with this project.
  * Storage of Visibility Table using BITS by Cheesemonster.
  *
  * Some portions of code are from other bots, special thanks (and credits) go
  * to (in no specific order):
  *
  * Pierre Marie Baty
  * Count-Floyd
  *
  * !! BOTS-UNITED FOREVER !!
  *
  * This project is open-source, it is protected under the GPL license;
  * By using this source-code you agree that you will ALWAYS release the
  * source-code with your project.
  *
  **/

#ifndef _WIN32
#include <string.h>
#endif

#ifdef __linux__
#include <libgen.h>
#else
char* basename(char*);
char* dirname(char*);
#endif
#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>
#include <entity_state.h>

#include "../bot.h"
#include "../bot_weapons.h"
#include "../bot_func.h"
#include "../game.h"
#include "../ChatEngine.h"

// Below is tricky and dirty required because HL SDK redefine vec3_t as class Vector which brings a lot of trouble
#undef vec3_t
typedef vec_t vec3_t[3];

#include "bspfile.h"
#include "trace.h"

extern int mod_id;
extern cBot bots[32];
extern cGame Game;
extern cChatEngine ChatEngine;

int gmsgTextMsg = 0;
int gmsgSayText = 0;
int gmsgShowMenu = 0;

// Need to implement the basename & dirname functions under Windows...

#ifndef __linux__
// 21/07/04 Whistler
// Handle both \ and / as separator
char* basename(char* s)
{
	char* fs;

	if ((s == NULL) || (*s == 0)) return ".";
	if (strcmp(s, "\\") == 0) return s;
	if (strcmp(s, "/") == 0) return s;
	fs = strrchr(s, '\\');
	if (fs == NULL) fs = strrchr(s, '/');
	if (fs == NULL) return s;
	return fs + 1;
}

char* dirname(char* s)
{
	char* fs;

	if ((s == NULL) || (*s == 0)) return ".";
	if (strcmp(s, "\\") == 0) return s;
	if (strcmp(s, "/") == 0) return s;
	fs = strrchr(s, '\\');
	if (fs == NULL) fs = strrchr(s, '/');
	if (fs == NULL) return ".";
	*fs = 0;
	return s;
}
#endif

Vector UTIL_VecToAngles(const Vector& vec)
{
	float rgflVecOut[3];
	VEC_TO_ANGLES(vec, rgflVecOut);
	return Vector(rgflVecOut);
}

// Overloaded to add IGNORE_GLASS
void
UTIL_TraceLine(const Vector& vecStart, const Vector& vecEnd,
	IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass,
	edict_t* pentIgnore, TraceResult* ptr)
{
	TRACE_LINE(vecStart, vecEnd,
		(igmon ==
			ignore_monsters ? TRUE : FALSE) | (ignoreGlass ? 0x100 : 0),
		pentIgnore, ptr);
}

void
UTIL_TraceLine(const Vector& vecStart, const Vector& vecEnd,
	IGNORE_MONSTERS igmon, edict_t* pentIgnore,
	TraceResult* ptr)
{
	TRACE_LINE(vecStart, vecEnd, (igmon == ignore_monsters ? TRUE : FALSE),
		pentIgnore, ptr);
}

void
UTIL_MakeVectors(const Vector& vecAngles)
{
	MAKE_VECTORS(vecAngles);
}

edict_t*
UTIL_FindEntityInSphere(edict_t* pentStart, const Vector& vecCenter,
	float flRadius)
{
	edict_t* pentEntity;

	pentEntity = FIND_ENTITY_IN_SPHERE(pentStart, vecCenter, flRadius);

	if (!FNullEnt(pentEntity))
		return pentEntity;

	return NULL;
}

edict_t*
UTIL_FindEntityByString(edict_t* pentStart, const char* szKeyword,
	const char* szValue)
{
	edict_t* pentEntity;

	pentEntity = FIND_ENTITY_BY_STRING(pentStart, szKeyword, szValue);

	if (!FNullEnt(pentEntity))
		return pentEntity;
	return NULL;
}

edict_t*
UTIL_FindEntityByClassname(edict_t* pentStart, const char* szName)
{
	return UTIL_FindEntityByString(pentStart, "classname", szName);
}

edict_t*
UTIL_FindEntityByTargetname(edict_t* pentStart, const char* szName)
{
	return UTIL_FindEntityByString(pentStart, "targetname", szName);
}

void
UTIL_SetSize(entvars_t* pev, const Vector& vecMin, const Vector& vecMax)
{
	SET_SIZE(ENT(pev), vecMin, vecMax);
}

void
UTIL_SetOrigin(entvars_t* pev, const Vector& vecOrigin)
{
	SET_ORIGIN(ENT(pev), vecOrigin);
}

void
ClientPrint(edict_t* pEntity, int msg_dest, const char* msg_name)
{
}

void
UTIL_ClientPrintAll(int msg_dest, const char* msg_name, const char* param1,
	const char* param2, const char* param3,
	const char* param4)
{
}

void
UTIL_SayText(const char* pText, edict_t* pEdict)
{
	if (gmsgSayText == 0)
		gmsgSayText = REG_USER_MSG("SayText", -1);

	MESSAGE_BEGIN(MSG_ONE, gmsgSayText, NULL, pEdict);
	WRITE_BYTE(ENTINDEX(pEdict));
	//if (mod_id == FRONTLINE_DLL)
	//   WRITE_SHORT(0);
	WRITE_STRING(pText);
	MESSAGE_END();
}

void
UTIL_HostSay(edict_t* pEntity, int teamonly, char* message)
{
	int j;
	char text[128];
	char* pc;
	int sender_team, player_team;
	edict_t* client;

	// make sure the text has content
	for (pc = message; pc != NULL && *pc != 0; pc++)
	{
		if (isprint(*pc) && !isspace(*pc))
		{
			pc = NULL;		// we've found an alphanumeric character,  so text is valid
			break;
		}
	}

	if (pc != NULL)
		return;			// no character found, so say nothing

	  // turn on color set 2  (color on,  no sound)
	if (teamonly)
		sprintf(text, "%c(TEAM) %s: ", 2, STRING(pEntity->v.netname));
	else
		sprintf(text, "%c%s: ", 2, STRING(pEntity->v.netname));

	j = sizeof(text) - 2 - strlen(text);	// -2 for /n and null terminator
	if ((int)strlen(message) > j)
		message[j] = 0;

	strcat(text, message);
	strcat(text, "\n");

	// loop through all players
	// Start with the first player.
	// This may return the world in single player if the client types something between levels or during spawn
	// so check it, or it will infinite loop

	if (gmsgSayText == 0)
		gmsgSayText = REG_USER_MSG("SayText", -1);

	sender_team = UTIL_GetTeam(pEntity);

	client = NULL;
	while (((client = UTIL_FindEntityByClassname(client, "player")) != NULL) &&
		(!FNullEnt(client)))
	{
		if (client == pEntity)	// skip sender of message
			continue;

		player_team = UTIL_GetTeam(client);

		if (teamonly && (sender_team != player_team))
			continue;

		MESSAGE_BEGIN(MSG_ONE, gmsgSayText, NULL, client);
		WRITE_BYTE(ENTINDEX(pEntity));
		//if (mod_id == FRONTLINE_DLL)
		//   WRITE_SHORT(0);
		WRITE_STRING(text);
		MESSAGE_END();
	}

	// print to the sending client
	MESSAGE_BEGIN(MSG_ONE, gmsgSayText, NULL, pEntity);
	WRITE_BYTE(ENTINDEX(pEntity));
	//if (mod_id == FRONTLINE_DLL)
	//   WRITE_SHORT(0);
	WRITE_STRING(text);
	MESSAGE_END();

	// echo to server console
	g_engfuncs.pfnServerPrint(text);
}

#ifdef   DEBUG
edict_t*
DBG_EntOfVars(const entvars_t* pev)
{
	if (pev->pContainingEntity != NULL)
		return pev->pContainingEntity;
	ALERT(at_console,
		"entvars_t pContainingEntity is NULL, calling into engine");
	edict_t* pent = (*g_engfuncs.pfnFindEntityByVars) ((entvars_t*)pev);
	if (pent == NULL)
		ALERT(at_console, "DAMN!  Even the engine couldn't FindEntityByVars!");
	((entvars_t*)pev)->pContainingEntity = pent;
	return pent;
}
#endif //DEBUG

// Is the edict a vip or not?
bool
UTIL_IsVip(edict_t* pEntity)
{
	if (mod_id != CSTRIKE_DLL)
		return false;
	else
	{
		char* infobuffer;
		char model_name[32];

		infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer) (pEntity);
		strcpy(model_name, (g_engfuncs.pfnInfoKeyValue(infobuffer, "model")));

		if (strcmp(model_name, "vip") == 0)	// VIP
			return true;
	}

	return false;
}

// return team number 0 through 3 based what MOD uses for team numbers
int
UTIL_GetTeam(edict_t* pEntity)
{
	if (mod_id == CSTRIKE_DLL)
	{
		char* infobuffer;
		char model_name[32];

		infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer) (pEntity);
		strcpy(model_name, (g_engfuncs.pfnInfoKeyValue(infobuffer, "model")));

		if ((strcmp(model_name, "terror") == 0) ||	// Phoenix Connektion
			(strcmp(model_name, "arab") == 0) ||	// old L337 Krew
			(strcmp(model_name, "leet") == 0) ||	// L337 Krew
			(strcmp(model_name, "artic") == 0) ||	// Artic Avenger
			(strcmp(model_name, "arctic") == 0) ||	// Artic Avenger - fix for arctic? - seemed a typo?
			(strcmp(model_name, "guerilla") == 0))	// Gorilla Warfare
		{
			return 0;
		}
		else if ((strcmp(model_name, "urban") == 0) ||	// Seal Team 6
			(strcmp(model_name, "gsg9") == 0) ||	// German GSG-9
			(strcmp(model_name, "sas") == 0) ||	// UK SAS
			(strcmp(model_name, "gign") == 0) ||	// French GIGN
			(strcmp(model_name, "vip") == 0) ||	// VIP
			(strcmp(model_name, "spetsnatz") == 0))	// CZ Spetsnatz
		{
			return 1;
		}
		return -1;		// return zero if team is unknown
	}

	return 0;
}

// return class number 0 through N
int
UTIL_GetClass(edict_t* pEntity)
{
	char* infobuffer;
	char model_name[32];

	infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer) (pEntity);
	strcpy(model_name, (g_engfuncs.pfnInfoKeyValue(infobuffer, "model")));

	return 0;
}

int
UTIL_GetBotIndex(edict_t* pEdict)
{
	int index;

	for (index = 0; index < 32; index++)
	{
		if (bots[index].pEdict == pEdict)
		{
			return index;
		}
	}

	return -1;			// return -1 if edict is not a bot
}

cBot*
UTIL_GetBotPointer(edict_t* pEdict)
{
	int index;

	for (index = 0; index < 32; index++)
	{
		if (bots[index].pEdict == pEdict)
		{
			break;
		}
	}

	if (index < 32)
		return (&bots[index]);

	return NULL;			// return NULL if edict is not a bot
}

bool IsAlive(edict_t* pEdict) {
	// FIX: Make sure the edict is valid and such, else return false:
	return ((pEdict != NULL) &&	// VALID
		(pEdict->v.deadflag == DEAD_NO) &&	// NOT DEAD
		(pEdict->v.health > 0) &&	// ENOUGHT HEALTH
		!(pEdict->v.flags & FL_NOTARGET) &&	// ?
		(pEdict->v.takedamage != 0));	// CAN TAKE DAMAGE
}

bool
FInViewCone(Vector* pOrigin, edict_t* pEdict)
{
#ifdef EVYISWRONG
	return TRUE;
#endif
	Vector2D vec2LOS;
	float flDot;

	UTIL_MakeVectors(pEdict->v.angles);

	vec2LOS = (*pOrigin - pEdict->v.origin).Make2D();
	vec2LOS = vec2LOS.Normalize();

	flDot = DotProduct(vec2LOS, gpGlobals->v_forward.Make2D());

	if (flDot > 0.50)		// 60 degree field of view
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

// FVisible()
bool
FVisible(const Vector& vecOrigin, edict_t* pEdict)
{
#ifdef EVYISWRONG
	return TRUE;
#endif
	TraceResult tr;
	Vector vecLookerOrigin;

	// look through caller's eyes
	vecLookerOrigin = pEdict->v.origin + pEdict->v.view_ofs;

	int bInWater = (UTIL_PointContents(vecOrigin) == CONTENTS_WATER);
	int bLookerInWater =
		(UTIL_PointContents(vecLookerOrigin) == CONTENTS_WATER);

	// don't look through water
	if (bInWater != bLookerInWater)
		return FALSE;

	UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters, ignore_glass,
		pEdict, &tr);

	if (tr.flFraction != 1.0)
	{
		return FALSE;		// Line of sight is not established
	}
	else
	{
		return TRUE;		// line of sight is valid.
	}
}

Vector
GetGunPosition(edict_t* pEdict)
{
	return (pEdict->v.origin + pEdict->v.view_ofs);
}

void
UTIL_SelectItem(edict_t* pEdict, char* item_name)
{
	/*BotDebug( item_name); */
	FakeClientCommand(pEdict, item_name, NULL, NULL);
}

Vector
VecBModelOrigin(edict_t* pEdict)
{
	return pEdict->v.absmin + (pEdict->v.size * 0.5);
}

void
UTIL_ShowMenu(edict_t* pEdict, int slots, int displaytime, bool needmore,
	char* pText)
{
	if (gmsgShowMenu == 0)
		gmsgShowMenu = REG_USER_MSG("ShowMenu", -1);

	MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, NULL, pEdict);

	WRITE_SHORT(slots);
	WRITE_CHAR(displaytime);
	WRITE_BYTE(needmore);
	WRITE_STRING(pText);

	MESSAGE_END();
}

void
UTIL_BuildFileName(char* filename, char* arg1, char* arg2)
{
	if (mod_id == VALVE_DLL)
		strcpy(filename, "valve/");

	else if (mod_id == CSTRIKE_DLL)
		strcpy(filename, "cstrike/");

	else
	{
		filename[0] = 0;
		return;
	}

	if ((arg1) && (*arg1) && (arg2) && (*arg2))
	{
		strcat(filename, arg1);
		strcat(filename, "/");
		strcat(filename, arg2);
	}
	else if ((arg1) && (*arg1))
	{
		strcat(filename, arg1);
	}
}

// added by Tub
// copies subdir/file to filename

// Heavily modified in the standalone version
void
UTIL_BuildFileNameRB(char* subdir, char* filename)
{
	char* temp, * temp2;

	temp = strdup(subdir);
	temp2 = basename(temp);
	strcpy(filename, temp2);
	free(temp);
}

//=========================================================
// UTIL_LogPrintf - Prints a logged message to console.
// Preceded by LOG: ( timestamp ) < message >
//=========================================================
void
UTIL_LogPrintf(char* fmt, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, fmt);
	vsprintf(string, fmt, argptr);
	va_end(argptr);

	// Print to server console
	ALERT(at_logged, "%s", string);
}

void
UTIL_BotPressKey(cBot* pBot, int type) {
	if (type == IN_JUMP || type == IN_DUCK)
		if (pBot->f_freeze_time > gpGlobals->time)
			return;			// do nothing when in freezetime

	if (type == IN_JUMP)
		if (pBot->f_may_jump_time > gpGlobals->time)
			return;			// do nothing when we may not jump

	if (type == IN_JUMP && pBot->f_camp_time > gpGlobals->time)
		return;			// Do not jump when camping.

		// don't jump from ladder
	if (FUNC_IsOnLadder(pBot->pEdict) && type == IN_JUMP)
		return;

	// KEY: Reload
	if (type == IN_RELOAD)	// when reloading, there is NO zooming (when holding a zoomable weapon or a sniper gun)
	{
		if (FUNC_BotHoldsZoomWeapon(pBot)
			|| UTIL_GiveWeaponType(pBot->current_weapon.iId) == SNIPER)

			//pBot->zoomed = ZOOM_NONE; // not zoomed anymore

			// FIX: Do not let bots do anything with this weapon for 0.7 second. So the engine can
			// update the information.
			pBot->f_update_weapon_time = gpGlobals->time + 0.7;
	}

	// KEY: End
	pBot->pEdict->v.button |= type;

	if (type == IN_JUMP)
	{
		if (pBot->f_hold_duck < gpGlobals->time)
			pBot->f_hold_duck = gpGlobals->time + 0.35;

		pBot->f_may_jump_time = gpGlobals->time + 0.3;
	}
}

int
UTIL_GiveWeaponType(int weapon_id)
{
	int kind = NONE;

	// Check 1. Is it a knife?
	if (weapon_id == CS_WEAPON_KNIFE)
		kind = KNIFE;

	// Check 2, is it a 'tool'?
	if (weapon_id == CS_WEAPON_FLASHBANG ||
		weapon_id == CS_WEAPON_HEGRENADE || weapon_id == CS_WEAPON_SMOKEGRENADE)
		kind = GRENADE;

	// Check 3, is it a secondary gun?
	if (weapon_id == CS_WEAPON_P228 ||
		weapon_id == CS_WEAPON_ELITE ||
		weapon_id == CS_WEAPON_UMP45 ||
		weapon_id == CS_WEAPON_USP ||
		weapon_id == CS_WEAPON_GLOCK18 ||
		weapon_id == CS_WEAPON_DEAGLE || weapon_id == CS_WEAPON_FIVESEVEN)
		kind = SECONDARY;

	// Check 4, is it a sniper gun?
	if (weapon_id == CS_WEAPON_SCOUT ||
		weapon_id == CS_WEAPON_SG550 ||
		weapon_id == CS_WEAPON_AWP || weapon_id == CS_WEAPON_G3SG1)
		kind = SNIPER;

	// When the kind of weapon is still not found, its a primary (in CS)
	if (kind == NONE)
		kind = PRIMARY;

	if (weapon_id < 1)
		kind = NONE;

	return kind;
}

// Return weapon ID (depended on mod)
int
UTIL_GiveWeaponId(char* name)
{
	if (mod_id == CSTRIKE_DLL)
	{
		if (strcmp(name, "weapon_knife") == 0)
			return CS_WEAPON_KNIFE;

		if (strcmp(name, "weapon_c4") == 0)
			return CS_WEAPON_C4;
		if (strcmp(name, "weapon_mp5navy") == 0)
			return CS_WEAPON_MP5NAVY;
		if (strcmp(name, "weapon_ak47") == 0)
			return CS_WEAPON_AK47;
		if (strcmp(name, "weapon_m3") == 0)
			return CS_WEAPON_M3;
		if (strcmp(name, "weapon_aug") == 0)
			return CS_WEAPON_AUG;
		if (strcmp(name, "weapon_sg552") == 0)
			return CS_WEAPON_SG552;
		if (strcmp(name, "weapon_m249") == 0)
			return CS_WEAPON_M249;
		if (strcmp(name, "weapon_xm1014") == 0)
			return CS_WEAPON_XM1014;
		if (strcmp(name, "weapon_p90") == 0)
			return CS_WEAPON_P90;
		if (strcmp(name, "weapon_tmp") == 0)
			return CS_WEAPON_TMP;
		if (strcmp(name, "weapon_m4a1") == 0)
			return CS_WEAPON_M4A1;
		if (strcmp(name, "weapon_awp") == 0)
			return CS_WEAPON_AWP;
		if (strcmp(name, "weapon_fiveseven") == 0)
			return CS_WEAPON_FIVESEVEN;
		if (strcmp(name, "weapon_ump45") == 0)
			return CS_WEAPON_UMP45;
		if (strcmp(name, "weapon_sg550") == 0)
			return CS_WEAPON_SG550;
		if (strcmp(name, "weapon_scout") == 0)
			return CS_WEAPON_SCOUT;
		if (strcmp(name, "weapon_mac10") == 0)
			return CS_WEAPON_MAC10;
		if (strcmp(name, "weapon_g3sg1") == 0)
			return CS_WEAPON_G3SG1;
		if (strcmp(name, "weapon_elite") == 0)
			return CS_WEAPON_ELITE;
		if (strcmp(name, "weapon_p228") == 0)
			return CS_WEAPON_P228;
		if (strcmp(name, "weapon_deagle") == 0)
			return CS_WEAPON_DEAGLE;
		if (strcmp(name, "weapon_usp") == 0)
			return CS_WEAPON_USP;
		if (strcmp(name, "weapon_glock18") == 0)
			return CS_WEAPON_GLOCK18;
		// Counter-Strike 1.6
		if (strcmp(name, "weapon_famas") == 0)
			return CS_WEAPON_FAMAS;
		if (strcmp(name, "weapon_galil") == 0)
			return CS_WEAPON_GALIL;

		// TODO: Detect shield carrying.
	}

	return -1;
}

// Return weapon ID (depended on mod)
char*
UTIL_GiveWeaponName(int id)
{
	if (mod_id == CSTRIKE_DLL)
	{
		if (id == CS_WEAPON_C4)
			return "weapon_c4";
		if (id == CS_WEAPON_MP5NAVY)
			return "weapon_mp5navy";
		if (id == CS_WEAPON_AK47)
			return "weapon_ak47";
		if (id == CS_WEAPON_M3)
			return "weapon_m3";
		if (id == CS_WEAPON_AUG)
			return "weapon_aug";
		if (id == CS_WEAPON_SG552)
			return "weapon_sg552";
		if (id == CS_WEAPON_M249)
			return "weapon_m249";
		if (id == CS_WEAPON_XM1014)
			return "weapon_xm1014";
		if (id == CS_WEAPON_P90)
			return "weapon_p90";
		if (id == CS_WEAPON_TMP)
			return "weapon_tmp";
		if (id == CS_WEAPON_M4A1)
			return "weapon_m4a1";
		if (id == CS_WEAPON_AWP)
			return "weapon_awp";
		if (id == CS_WEAPON_FIVESEVEN)
			return "weapon_fiveseven";
		if (id == CS_WEAPON_UMP45)
			return "weapon_ump45";
		if (id == CS_WEAPON_SG550)
			return "weapon_ag550";
		if (id == CS_WEAPON_SCOUT)
			return "weapon_scout";
		if (id == CS_WEAPON_MAC10)
			return "weapon_mac10";
		if (id == CS_WEAPON_G3SG1)
			return "weapon_g3sg1";
		if (id == CS_WEAPON_ELITE)
			return "weapon_elite";
		if (id == CS_WEAPON_P228)
			return "weapon_p228";
		if (id == CS_WEAPON_DEAGLE)
			return "weapon_deagle";
		if (id == CS_WEAPON_USP)
			return "weapon_usp";
		if (id == CS_WEAPON_GLOCK18)
			return "weapon_glock18";

		// Counter-Strike 1.6
		if (id == CS_WEAPON_FAMAS)
			return "weapon_famas";
		if (id == CS_WEAPON_GALIL)
			return "weapon_galil";

		// Unconfirmed shield
		if (id == CS_WEAPON_SHIELD)
			return "weapon_shield";
	}

	return "weapon_knife";	// return knife, always good ;)
}

// Thanks Botman for this code (from forum).
void
UTIL_BotSprayLogo(edict_t* pEntity, char* logo_name)
{
	int index;
	TraceResult pTrace;
	Vector v_src, v_dest;
	UTIL_MakeVectors(pEntity->v.v_angle);
	v_src = pEntity->v.origin + pEntity->v.view_ofs;
	v_dest = v_src + gpGlobals->v_forward * 80;
	UTIL_TraceLine(v_src, v_dest, ignore_monsters,
		pEntity->v.pContainingEntity, &pTrace);

	index = DECAL_INDEX(logo_name);

	if (index < 0)
		return;

	if ((pTrace.pHit) && (pTrace.flFraction < 1.0))
	{
		if (pTrace.pHit->v.solid != SOLID_BSP)
			return;

		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);

		if (index > 255)
		{
			WRITE_BYTE(TE_WORLDDECALHIGH);
			index -= 256;
		}
		else
			WRITE_BYTE(TE_WORLDDECAL);

		WRITE_COORD(pTrace.vecEndPos.x);
		WRITE_COORD(pTrace.vecEndPos.y);
		WRITE_COORD(pTrace.vecEndPos.z);
		WRITE_BYTE(index);

		MESSAGE_END();

		EMIT_SOUND_DYN2(pEntity, CHAN_VOICE, "player/sprayer.wav", 1.0,
			ATTN_NORM, 0, 100);
	}
}

// Give a radio message botty boy!
void
UTIL_BotRadioMessage(cBot* pBot, int radio, char* arg1, char* arg2)
{
	// To be sure the console will only change when we MAY change.
	// The values will only be changed when console_nr is 0
	if (pBot->console_nr == 0)
	{
		switch (radio)
		{
		case 1:
			strcpy(pBot->arg1, "radio1");
			break;
		case 2:
			strcpy(pBot->arg1, "radio2");
			break;
		case 3:
			strcpy(pBot->arg1, "radio3");
			break;
		}

		strcpy(pBot->arg2, arg1);
		strcpy(pBot->arg3, arg2);
		pBot->console_nr = 1;	// Begin message
		int iExtra = (100 / pBot->ipCreateRadio);
		if (iExtra > 30) iExtra = 30;
		pBot->fDoRadio = gpGlobals->time + iExtra;
	}
}

//////////////////////////////////
// UTIL_getGrenadeType function // - Stefan
//////////////////////////////////
int
UTIL_GetGrenadeType(edict_t* pEntity)
{
	char model_name[32];

	strcpy(model_name, STRING(pEntity->v.model));

	if (strcmp(model_name, "models/w_hegrenade.mdl") == 0)
		return 1;			// He grenade
	if (strcmp(model_name, "models/w_flashbang.mdl") == 0)
		return 2;			// FlashBang
	if (strcmp(model_name, "models/w_smokegrenade.mdl") == 0)
		return 3;			// SmokeGrenade
	if (strcmp(model_name, "models/w_c4.mdl") == 0)
		return 4;			// C4 Explosive

	return 0;
}

// 2 functions from podbot source
unsigned short
FixedUnsigned16(float value, float scale)
{
	int output;

	output = value * scale;
	if (output < 0)
		output = 0;
	if (output > 0xFFFF)
		output = 0xFFFF;

	return (unsigned short)output;
}

short
FixedSigned16(float value, float scale)
{
	int output;

	output = value * scale;

	if (output > 32767)
		output = 32767;

	if (output < -32768)
		output = -32768;

	return (short)output;
}

// Using POD/SDK source to print nice messages on the client machine
void
HUD_DrawString(int r, int g, int b, char* msg, edict_t* edict)
{
	// FROM PODBOT SOURCE
	// Hacked together Version of HUD_DrawString
	MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, NULL, edict);
	WRITE_BYTE(TE_TEXTMESSAGE);
	WRITE_BYTE(1);
	WRITE_SHORT(FixedSigned16(-1, 1 << 13));
	WRITE_SHORT(FixedSigned16(0, 1 << 13));
	WRITE_BYTE(2);
	WRITE_BYTE(r);		//r
	WRITE_BYTE(g);		//g
	WRITE_BYTE(b);		//b
	WRITE_BYTE(0);
	WRITE_BYTE(255);
	WRITE_BYTE(255);
	WRITE_BYTE(255);
	WRITE_BYTE(200);
	WRITE_SHORT(FixedUnsigned16(0.0078125, 1 << 8));
	WRITE_SHORT(FixedUnsigned16(2, 1 << 8));
	WRITE_SHORT(FixedUnsigned16(6, 1 << 8));
	WRITE_SHORT(FixedUnsigned16(0.1, 1 << 8));
	WRITE_STRING((const char*)&msg[0]);
	MESSAGE_END();
}

void
UTIL_FixAngles(Vector* Angles)
{
	if (Angles->x > 180.0)
		Angles->x -= 360.0;
	if (Angles->x < -180.0)
		Angles->x += 360.0;
	if (Angles->y > 180.0)
		Angles->y -= 360.0;
	if (Angles->y < -180.0)
		Angles->y += 360.0;

	Angles->z = 0.0;
}

// POD SAYING:
void UTIL_SayTextBot(const char* pText, cBot* pBot)
{
	if (gmsgSayText == 0)
		gmsgSayText = REG_USER_MSG("SayText", -1);

	char szTemp[160];
	char szName[BOT_NAME_LEN + 1];
	int i = 0;

	// clear out
	memset(szTemp, 0, sizeof(szTemp));
	memset(szName, 0, sizeof(szName));

	// init
	szTemp[0] = 2;

	int entind = ENTINDEX(pBot->pEdict);

	if (IsAlive(pBot->pEdict))
	{
		strcpy(szName, pBot->name);
		for (i = 1; i <= gpGlobals->maxClients; i++)
		{
			edict_t* pPlayer = INDEXENT(i);

			// valid
			if (pPlayer)
				if (IsAlive(pPlayer)) // alive
				{
					MESSAGE_BEGIN(MSG_ONE, gmsgSayText, NULL, pPlayer);
					WRITE_BYTE(entind);
					sprintf(&szTemp[1], "%s :   %s", szName, pText);
					WRITE_STRING(&szTemp[0]);
					MESSAGE_END();
				}
		}
	}
	else
	{
		strcpy(szName, pBot->name);
		for (i = 1; i <= gpGlobals->maxClients; i++)
		{
			edict_t* pPlayer = INDEXENT(i);
			if (pPlayer)
				if (!IsAlive(pPlayer))
				{
					MESSAGE_BEGIN(MSG_ONE, gmsgSayText, NULL, pPlayer);
					WRITE_BYTE(entind);
					sprintf(&szTemp[1], "*DEAD*%s :   %s", szName, pText);
					WRITE_STRING(&szTemp[0]);
					MESSAGE_END();
				}
		}
	}

	// pass through on ChatEngine (not always)
	if (RANDOM_LONG(0, 100) < 90)
	{
		char chSentence[80];
		memset(chSentence, 0, sizeof(chSentence));

		// copy pText to chSentence
		strcpy(chSentence, pText);

		// pass through
		ChatEngine.set_sentence(pBot->name, chSentence);
	}
}

// $Log: util.cpp,v $
// Revision 1.4  2004/07/21 06:32:36  eric
// - version is now 0.9.4
// - better handling of dirname/basename in Windows (thanks to Whistler)
//
// Revision 1.3  2004/07/18 18:52:29  eric
// - added version number, currently 0.9.2
// - added Windows version of basename & dirname functions
// - added two other utilities DrawNodes & DumpNodes
// - updated README file
// - fixed compilation warnings (thanks dstruct2k)
//