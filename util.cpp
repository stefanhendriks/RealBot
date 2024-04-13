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

#include <cstring>
#include <extdll.h>
#include <dllapi.h>
#include <meta_api.h>
#include <entity_state.h>

#include "bot.h"
#include "bot_weapons.h"
#include "bot_func.h"
#include "game.h"
#include "ChatEngine.h"

#include <cctype>

#ifndef _WIN32
#define snprintf std::snprintf //-V1059
#endif

extern int mod_id;
extern cBot bots[32];
extern cGame Game;
extern cChatEngine ChatEngine;

int gmsgTextMsg = 0;
int gmsgSayText = 0;
int gmsgShowMenu = 0;

Vector UTIL_VecToAngles(const Vector& vec) {
	float rgflVecOut[3];
	VEC_TO_ANGLES(vec, rgflVecOut);
	return { rgflVecOut };
}

// Overloaded to add IGNORE_GLASS
void
UTIL_TraceLine(const Vector& vecStart, const Vector& vecEnd,
	IGNORE_MONSTERS igmon, IGNORE_GLASS ignoreGlass,
	edict_t* pentIgnore, TraceResult* ptr) {
	TRACE_LINE(vecStart, vecEnd,
		(igmon ==
		ignore_monsters) | (ignoreGlass ? 0x100 : 0),
		pentIgnore, ptr);
}

void
UTIL_TraceLine(const Vector& vecStart, const Vector& vecEnd,
	IGNORE_MONSTERS igmon, edict_t* pentIgnore,
	TraceResult* ptr) {
	TRACE_LINE(vecStart, vecEnd, (igmon == ignore_monsters),
		pentIgnore, ptr);
}

void
UTIL_TraceHull(const Vector& vecStart, const Vector& vecEnd,
	IGNORE_MONSTERS igmon, int hullNumber, edict_t* pentIgnore,
	TraceResult* ptr) {
	TRACE_HULL(vecStart, vecEnd, (igmon == ignore_monsters), hullNumber, pentIgnore, ptr);
}

void UTIL_MakeVectors(const Vector& vecAngles) {
	MAKE_VECTORS(vecAngles);
}

edict_t* UTIL_FindEntityInSphere(edict_t* pentStart,
	const Vector& vecCenter, float flRadius) {
	edict_t* pentEntity = FIND_ENTITY_IN_SPHERE(pentStart, vecCenter, flRadius);

	if (!FNullEnt(pentEntity))
		return pentEntity;

	return nullptr;
}

edict_t* UTIL_FindEntityByString(edict_t* pentStart,
	const char* szKeyword,
	const char* szValue) {
	edict_t* pentEntity = FIND_ENTITY_BY_STRING(pentStart, szKeyword, szValue);

	if (!FNullEnt(pentEntity))
		return pentEntity;
	return nullptr;
}

edict_t* UTIL_FindEntityByClassname(edict_t* pentStart,
	const char* szName) {
	return UTIL_FindEntityByString(pentStart, "classname", szName);
}

edict_t* UTIL_FindEntityByTargetname(edict_t* pentStart,
	const char* szName) {
	return UTIL_FindEntityByString(pentStart, "targetname", szName);
}

int UTIL_PointContents(const Vector& vec) {
	return POINT_CONTENTS(vec);
}

void
UTIL_SetSize(entvars_t* pev, const Vector& vecMin, const Vector& vecMax) {
	SET_SIZE(ENT(pev), vecMin, vecMax);
}

void UTIL_SetOrigin(entvars_t* pev, const Vector& vecOrigin) {
	SET_ORIGIN(ENT(pev), vecOrigin);
}

void ClientPrint(edict_t* pEntity, int msg_dest, const char* msg_name) {

	if (gmsgTextMsg == 0)
		gmsgTextMsg = REG_USER_MSG("TextMsg", -1);

	MESSAGE_BEGIN(MSG_ONE, gmsgTextMsg, nullptr, pEntity);

	WRITE_BYTE(msg_dest);
	WRITE_STRING(msg_name);
	MESSAGE_END();
}

void UTIL_ClientPrintAll(int msg_dest, const char* msg_name, const char* param1,
	const char* param2, const char* param3,
	const char* param4) {
	if (gmsgTextMsg == 0)
		gmsgTextMsg = REG_USER_MSG("TextMsg", -1);

	MESSAGE_BEGIN(MSG_ALL, gmsgTextMsg);
	WRITE_BYTE(msg_dest);
	WRITE_STRING(msg_name);

	if (param1)
		WRITE_STRING(param1);
	if (param2)
		WRITE_STRING(param2);
	if (param3)
		WRITE_STRING(param3);
	if (param4)
		WRITE_STRING(param4);

	MESSAGE_END();
}

void UTIL_SayText(const char* pText, edict_t* pEdict) {
	if (gmsgSayText == 0)
		gmsgSayText = REG_USER_MSG("SayText", -1);

	MESSAGE_BEGIN(MSG_ONE, gmsgSayText, nullptr, pEdict);
	WRITE_BYTE(ENTINDEX(pEdict));
	//if (mod_id == FRONTLINE_DLL)
	//   WRITE_SHORT(0);
	WRITE_STRING(pText);
	MESSAGE_END();
}

void UTIL_HostSay(edict_t* pEntity, int teamonly, char* message) {
	char text[128];
	char* pc;

	// make sure the text has content
	for (pc = message; pc != nullptr && *pc != 0; pc++) {
		if (isprint(*pc) && !isspace(*pc)) {
			pc = nullptr;             // we've found an alphanumeric character,  so text is valid
			break;
		}
	}

	if (pc != nullptr)
		return;                   // no character found, so say nothing

	// turn on color set 2  (color on,  no sound)
	if (teamonly)
		std::sprintf(text, "%c(TEAM) %s: ", 2, STRING(pEntity->v.netname));
	else
		std::sprintf(text, "%c%s: ", 2, STRING(pEntity->v.netname));

	const int j = sizeof(text) - 2 - std::strlen(text); // -2 for /n and null terminator
	if (static_cast<int>(std::strlen(message)) > j)
		message[j] = 0;

	std::strcat(text, message);
	std::strcat(text, "\n");

	// loop through all players
	// Start with the first player.
	// This may return the world in single player if the client types something between levels or during spawn
	// so check it, or it will infinite loop

	if (gmsgSayText == 0)
		gmsgSayText = REG_USER_MSG("SayText", -1);

	const int sender_team = UTIL_GetTeam(pEntity);

	edict_t* client = nullptr;
	while ((client = UTIL_FindEntityByClassname(client, "player")) != nullptr
		&& !FNullEnt(client)) {
		if (client == pEntity)    // skip sender of message
			continue;

		const int player_team = UTIL_GetTeam(client);

		if (teamonly && sender_team != player_team)
			continue;

		MESSAGE_BEGIN(MSG_ONE, gmsgSayText, nullptr, client);
		WRITE_BYTE(ENTINDEX(pEntity));
		//if (mod_id == FRONTLINE_DLL)
		//   WRITE_SHORT(0);
		WRITE_STRING(text);
		MESSAGE_END();
	}

	// print to the sending client
	MESSAGE_BEGIN(MSG_ONE, gmsgSayText, nullptr, pEntity);
	WRITE_BYTE(ENTINDEX(pEntity));
	//if (mod_id == FRONTLINE_DLL)
	//   WRITE_SHORT(0);
	WRITE_STRING(text);
	MESSAGE_END();

	// echo to server console
	g_engfuncs.pfnServerPrint(text);
}

#ifdef   DEBUG
edict_t* DBG_EntOfVars(const entvars_t* pev) {
	if (pev->pContainingEntity != NULL)
		return pev->pContainingEntity;
	ALERT(at_console,
		"entvars_t pContainingEntity is NULL, calling into engine");
	edict_t* pent = (*g_engfuncs.pfnFindEntityByVars) ((entvars_t*)pev);
	if (pent == NULL)
		ALERT(at_console,
			"DAMN!  Even the engine couldn't FindEntityByVars!");
	((entvars_t*)pev)->pContainingEntity = pent;
	return pent;
}
#endif                          //DEBUG

// Is the edict a vip or not?
bool UTIL_IsVip(edict_t* pEntity) {
	if (mod_id != CSTRIKE_DLL)
		return false;
	char model_name[32];

	char* infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(pEntity);
	std::strcpy(model_name, g_engfuncs.pfnInfoKeyValue(infobuffer, "model"));

	if (std::strcmp(model_name, "vip") == 0)       // VIP
		return true;

	return false;
}

// return team number 0 through 3 based what MOD uses for team numbers
int UTIL_GetTeam(edict_t* pEntity) {
	if (mod_id == CSTRIKE_DLL) {
		char model_name[32];

		char* infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(pEntity);
		std::strcpy(model_name,
			g_engfuncs.pfnInfoKeyValue(infobuffer, "model"));

		if (std::strcmp(model_name, "terror") == 0 ||        // Phoenix Connektion
			std::strcmp(model_name, "arab") == 0 ||  // old L337 Krew
			std::strcmp(model_name, "leet") == 0 ||  // L337 Krew
			std::strcmp(model_name, "artic") == 0 || // Artic Avenger
			std::strcmp(model_name, "arctic") == 0 ||        // Artic Avenger - fix for arctic? - seemed a typo?
			std::strcmp(model_name, "guerilla") == 0)      // Gorilla Warfare
			//(strcmp(model_name, "militia") == 0))       // CZ Militia
		{
			return 0; // team Terrorists
		}
		if (std::strcmp(model_name, "urban") == 0 ||  // Seal Team 6
			std::strcmp(model_name, "gsg9") == 0 ||   // German GSG-9
			std::strcmp(model_name, "sas") == 0 ||    // UK SAS
			std::strcmp(model_name, "gign") == 0 ||   // French GIGN
			std::strcmp(model_name, "vip") == 0 ||    // VIP
			std::strcmp(model_name, "spetsnatz") == 0)        // CZ Spetsnatz
		{
			return 1; // team Counter-Terrorists
		}

		return -1; // unknown team
	}

	return -2; // unknown mod
}

// return class number 0 through N
int UTIL_GetClass(edict_t* pEntity) {
	char model_name[32];

	char* infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(pEntity);
	std::strcpy(model_name, (g_engfuncs.pfnInfoKeyValue(infobuffer, "model")));

	return 0;
}

int UTIL_GetBotIndex(edict_t* pEdict) {
	for (int index = 0; index < 32; index++) {
		if (bots[index].pEdict == pEdict) {
			return index;
		}
	}

	return -1;                   // return -1 if edict is not a bot
}

/**
 * Returns bot pointer for possible pEdict (player)
 * @param pEdict
 * @return
 */
cBot* UTIL_GetBotPointer(edict_t* pEdict) {
	for (cBot& bot : bots)
	{
		if (bot.pEdict == pEdict) {
			return (&bot);
		}
	}

	return nullptr;                 // return NULL if edict is not a bot
}

bool IsAlive(edict_t* pEdict) {
	// FIX: Make sure the edict is valid and such, else return false:
	return pEdict != nullptr &&  // VALID
		pEdict->v.deadflag == DEAD_NO &&   // NOT DEAD
		pEdict->v.health > 0 &&    // ENOUGHT HEALTH
		!(pEdict->v.flags & FL_NOTARGET) &&  // ?
		pEdict->v.takedamage != 0.0f;        // CAN TAKE DAMAGE
}

bool FInViewCone(Vector* pOrigin, edict_t* pEdict) {
	UTIL_MakeVectors(pEdict->v.angles);

	Vector2D vec2LOS = (*pOrigin - pEdict->v.origin).Make2D();
	vec2LOS = vec2LOS.Normalize();

	const float flDot = DotProduct(vec2LOS, gpGlobals->v_forward.Make2D());

	if (flDot > 0.50f)            // 60 degree field of view
	{
		return true;
	}
	return false;
}

// FVisible()
bool FVisible(const Vector& vecOrigin, edict_t* pEdict) {
	TraceResult tr;

	// look through caller's eyes
	const Vector vecLookerOrigin = pEdict->v.origin + pEdict->v.view_ofs;

	const int bInWater = UTIL_PointContents(vecOrigin) == CONTENTS_WATER;
	const int bLookerInWater =
		UTIL_PointContents(vecLookerOrigin) == CONTENTS_WATER;

	// don't look through water
	if (bInWater != bLookerInWater)
		return false;

	UTIL_TraceLine(vecLookerOrigin, vecOrigin, ignore_monsters,
		ignore_glass, pEdict, &tr);

	if (tr.flFraction != 1.0f) {
		return false;             // Line of sight is not established
	}
	return true;              // line of sight is valid.
}

Vector GetGunPosition(edict_t* pEdict) {
	return pEdict->v.origin + pEdict->v.view_ofs;
}

void UTIL_SelectItem(edict_t* pEdict, char* item_name) {
	/*BotDebug( item_name); */
	FakeClientCommand(pEdict, item_name, nullptr, nullptr);
}

Vector VecBModelOrigin(edict_t* pEdict) {
	return pEdict->v.absmin + (pEdict->v.size * 0.5);
}

void
UTIL_ShowMenu(edict_t* pEdict, int slots, int displaytime, bool needmore,
	char* pText) {
	if (gmsgShowMenu == 0)
		gmsgShowMenu = REG_USER_MSG("ShowMenu", -1);

	MESSAGE_BEGIN(MSG_ONE, gmsgShowMenu, nullptr, pEdict);

	WRITE_SHORT(slots);
	WRITE_CHAR(displaytime);
	WRITE_BYTE(needmore);
	WRITE_STRING(pText);

	MESSAGE_END();
}

void UTIL_BuildFileName(char* filename, char* arg1, char* arg2) {

	if (mod_id == VALVE_DLL)
		std::strcpy(filename, "valve/");

	else if (mod_id == CSTRIKE_DLL)
		std::strcpy(filename, "cstrike/");

	else {
		filename[0] = 0;
		return;
	}

	if (arg1 && *arg1 && arg2 && *arg2) {
		std::strcat(filename, arg1);
		std::strcat(filename, "/");
		std::strcat(filename, arg2);
	}
	else if (arg1 && *arg1) {
		std::strcat(filename, arg1);
	}
}

// added by Tub
// copies subdir/file to filename
void UTIL_BuildFileNameRB(const char* subdir, char* filename) {
#if DO_DEBUG != 0
	char* filenamedebug = filename;
#endif
#ifdef _WIN32
#define S '\\'
#else
#define S '/'
#endif

	std::strcpy(filename, "realbot/");
	std::strcat(filename, subdir);
	while (*filename) {
		if ((*filename == '/') || (*filename == '\\'))
			*filename = S;
		filename++;
	}
#if DO_DEBUG != 0
	DebugOut(filenamedebug);
#endif
#undef S
}

//=========================================================
// UTIL_LogPrintf - Prints a logged message to console.
// Preceded by LOG: ( timestamp ) < message >
//=========================================================
void UTIL_LogPrintf(char* fmt, ...) {
	va_list argptr;
	static char string[1024];

	va_start(argptr, fmt);
	vsprintf(string, fmt, argptr);
	va_end(argptr);

	// Print to server console
	ALERT(at_logged, "%s", string);
}

void UTIL_BotPressKey(cBot* pBot, int type) {
	if (type == IN_JUMP || type == IN_DUCK) {
		if (pBot->isFreezeTime()) {
			pBot->rprint_trace("UTIL_BotPressKey", "IN_JUMP or IN_DUCK press requested, but in freezetime - IGNORING");
			return;                // do nothing when in freezetime
		}
	}

	if (type == IN_JUMP) {
		if (pBot->f_may_jump_time > gpGlobals->time || // do nothing when we may not jump
			pBot->f_camp_time > gpGlobals->time || // when camping do not jump
			FUNC_IsOnLadder(pBot->pEdict)    // don't jump from ladder
			) {
			pBot->rprint_trace("UTIL_BotPressKey",
				"IN_JUMP requested, but either camping, still jumping or on ladder - IGNORING");
			return;
		}
	}

	// KEY: Reload
	if (type == IN_RELOAD)       // when reloading, there is NO zooming (when holding a zoomable weapon or a sniper gun)
	{
		pBot->rprint_trace("UTIL_BotPressKey", "IN_RELOAD");
		if (FUNC_BotHoldsZoomWeapon(pBot) || UTIL_GiveWeaponType(pBot->current_weapon.iId) == SNIPER)
			//pBot->zoomed = ZOOM_NONE; // not zoomed anymore

			// FIX: Do not let bots do anything with this weapon for 0.7 second. So the engine can
			// update the information.
			pBot->f_update_weapon_time = gpGlobals->time + 0.7f;
	}
	// KEY: End

	switch (type) {
	case IN_DUCK:
		pBot->rprint_trace("UTIL_BotPressKey", "IN_DUCK");
		break;
	case IN_JUMP:
		pBot->rprint_trace("UTIL_BotPressKey", "IN_JUMP");
		break;
	case IN_USE:
		pBot->rprint_trace("UTIL_BotPressKey", "IN_USE");
		break;
	case IN_RELOAD:
		pBot->rprint_trace("UTIL_BotPressKey", "IN_RELOAD");
		break;
	case IN_ATTACK:
		pBot->rprint_trace("UTIL_BotPressKey", "IN_ATTACK");
		break;
	case IN_RUN:
		pBot->rprint_trace("UTIL_BotPressKey", "IN_RUN");
		break;
	case IN_MOVELEFT:
		pBot->rprint_trace("UTIL_BotPressKey", "IN_MOVELEFT");
		break;
	case IN_MOVERIGHT:
		pBot->rprint_trace("UTIL_BotPressKey", "IN_MOVERIGHT");
		break;
	default:
		char msg[255];
		snprintf(msg, sizeof(msg), "unknown key to print [%d]", type);
		pBot->rprint_trace("UTIL_BotPressKey", msg);
		break;
	}

	pBot->pEdict->v.button |= type;
}

int UTIL_GiveWeaponType(int weapon_id) {
	int kind = NONE;
	// 30/07/04 by Josh
	// Use switch instead of multiple if
	switch (weapon_id) {
	case CS_WEAPON_KNIFE:
		kind = KNIFE;
		break;                    // Check1. Is it a knife?
	case CS_WEAPON_FLASHBANG:
	case CS_WEAPON_HEGRENADE:
	case CS_WEAPON_SMOKEGRENADE:
		kind = GRENADE;
		break;                    // Check 2, is it a 'tool'?
	case CS_WEAPON_P228:
	case CS_WEAPON_ELITE:
	case CS_WEAPON_USP:
	case CS_WEAPON_GLOCK18:
	case CS_WEAPON_DEAGLE:
	case CS_WEAPON_FIVESEVEN:
		kind = SECONDARY;
		break;                    // Check 3, is it a secondary gun?
	case CS_WEAPON_SCOUT:
	case CS_WEAPON_SG550:
	case CS_WEAPON_AWP:
	case CS_WEAPON_G3SG1:
		kind = SNIPER;
		break;                    // Check 4, is it a sniper gun?
		//31.08.04 Frashman: added to switch construct
	case CS_WEAPON_SHIELD:
		kind = SHIELD;            // 08/07/04 - Also take shield into account
	}

	// When the kind of weapon is still not found, its a primary (in CS)
	if (kind == NONE)
		kind = PRIMARY;

	if (weapon_id < 1)
		kind = NONE;

	return kind;
}

// Return weapon ID (depended on mod)
int UTIL_GiveWeaponId(const char* name) {
	if (mod_id == CSTRIKE_DLL) {
		if (name != nullptr && std::strcmp(name, "weapon_knife") == 0)
			return CS_WEAPON_KNIFE;
		if (name != nullptr && std::strcmp(name, "weapon_c4") == 0)
			return CS_WEAPON_C4;
		if (name != nullptr && std::strcmp(name, "weapon_mp5navy") == 0)
			return CS_WEAPON_MP5NAVY;
		if (name != nullptr && std::strcmp(name, "weapon_ak47") == 0)
			return CS_WEAPON_AK47;
		if (name != nullptr && std::strcmp(name, "weapon_m3") == 0)
			return CS_WEAPON_M3;
		if (name != nullptr && std::strcmp(name, "weapon_aug") == 0)
			return CS_WEAPON_AUG;
		if (name != nullptr && std::strcmp(name, "weapon_sg552") == 0)
			return CS_WEAPON_SG552;
		if (name != nullptr && std::strcmp(name, "weapon_m249") == 0)
			return CS_WEAPON_M249;
		if (name != nullptr && std::strcmp(name, "weapon_xm1014") == 0)
			return CS_WEAPON_XM1014;
		if (name != nullptr && std::strcmp(name, "weapon_p90") == 0)
			return CS_WEAPON_P90;
		if (name != nullptr && std::strcmp(name, "weapon_tmp") == 0)
			return CS_WEAPON_TMP;
		if (name != nullptr && std::strcmp(name, "weapon_m4a1") == 0)
			return CS_WEAPON_M4A1;
		if (name != nullptr && std::strcmp(name, "weapon_awp") == 0)
			return CS_WEAPON_AWP;
		if (name != nullptr && std::strcmp(name, "weapon_fiveseven") == 0)
			return CS_WEAPON_FIVESEVEN;
		if (name != nullptr && std::strcmp(name, "weapon_ump45") == 0)
			return CS_WEAPON_UMP45;
		if (name != nullptr && std::strcmp(name, "weapon_sg550") == 0)
			return CS_WEAPON_SG550;
		if (name != nullptr && std::strcmp(name, "weapon_scout") == 0)
			return CS_WEAPON_SCOUT;
		if (name != nullptr && std::strcmp(name, "weapon_mac10") == 0)
			return CS_WEAPON_MAC10;
		if (name != nullptr && std::strcmp(name, "weapon_g3sg1") == 0)
			return CS_WEAPON_G3SG1;
		if (name != nullptr && std::strcmp(name, "weapon_elite") == 0)
			return CS_WEAPON_ELITE;
		if (name != nullptr && std::strcmp(name, "weapon_p228") == 0)
			return CS_WEAPON_P228;
		if (name != nullptr && std::strcmp(name, "weapon_deagle") == 0)
			return CS_WEAPON_DEAGLE;
		if (name != nullptr && std::strcmp(name, "weapon_usp") == 0)
			return CS_WEAPON_USP;
		if (name != nullptr && std::strcmp(name, "weapon_glock18") == 0)
			return CS_WEAPON_GLOCK18;
		// Counter-Strike 1.6
		if (name != nullptr && std::strcmp(name, "weapon_famas") == 0)
			return CS_WEAPON_FAMAS;
		if (name != nullptr && std::strcmp(name, "weapon_galil") == 0)
			return CS_WEAPON_GALIL;
		
		// 06/07/04
		// Unconfirmed for handling shield
		// 31.08.04 Frashman: moved shield string before unknown weapon, not after it
		if (name != nullptr && std::strcmp(name, "weapon_shield") == 0)
			return CS_WEAPON_SHIELD;
		
		char buffer[80];
		if (name != nullptr) {
			snprintf(buffer, 79, "UTIL_GiveWeaponId: Unknown weapon name %s\n", name);
		}
		else {
			snprintf(buffer, 79, "UTIL_GiveWeaponId: Unknown weapon name (null)\n");
		}
		rblog(buffer);
	}

	return -1;
}

// Return weapon ID (depended on mod)
char* UTIL_GiveWeaponName(int id) {
	if (mod_id == CSTRIKE_DLL) {
		switch (id) {
		case CS_WEAPON_C4:
			return "weapon_c4";
		case CS_WEAPON_MP5NAVY:
			return "weapon_mp5navy";
		case CS_WEAPON_AK47:
			return "weapon_ak47";
		case CS_WEAPON_M3:
			return "weapon_m3";
		case CS_WEAPON_AUG:
			return "weapon_aug";
		case CS_WEAPON_SG552:
			return "weapon_sg552";
		case CS_WEAPON_M249:
			return "weapon_m249";
		case CS_WEAPON_XM1014:
			return "weapon_xm1014";
		case CS_WEAPON_P90:
			return "weapon_p90";
		case CS_WEAPON_TMP:
			return "weapon_tmp";
		case CS_WEAPON_M4A1:
			return "weapon_m4a1";
		case CS_WEAPON_AWP:
			return "weapon_awp";
		case CS_WEAPON_FIVESEVEN:
			return "weapon_fiveseven";
		case CS_WEAPON_UMP45:
			return "weapon_ump45";
		case CS_WEAPON_SG550:
			return "weapon_ag550";
		case CS_WEAPON_SCOUT:
			return "weapon_scout";
		case CS_WEAPON_MAC10:
			return "weapon_mac10";
		case CS_WEAPON_G3SG1:
			return "weapon_g3sg1";
		case CS_WEAPON_ELITE:
			return "weapon_elite";
		case CS_WEAPON_P228:
			return "weapon_p228";
		case CS_WEAPON_DEAGLE:
			return "weapon_deagle";
		case CS_WEAPON_USP:
			return "weapon_usp";
		case CS_WEAPON_GLOCK18:
			return "weapon_glock18";

		// Counter-Strike 1.6
		case CS_WEAPON_FAMAS:
			return "weapon_famas";
		case CS_WEAPON_GALIL:
			return "weapon_galil";

		// Unconfirmed shield
		case CS_WEAPON_SHIELD:
			return "weapon_shield";
		}
	}

	return "weapon_knife";       // return knife, always good ;)
}

// Thanks Botman for this code (from forum).
void UTIL_BotSprayLogo(edict_t* pEntity, char* logo_name) {
	TraceResult pTrace;
	UTIL_MakeVectors(pEntity->v.v_angle);
	const Vector v_src = pEntity->v.origin + pEntity->v.view_ofs;
	const Vector v_dest = v_src + gpGlobals->v_forward * 80;
	UTIL_TraceLine(v_src, v_dest, ignore_monsters,
		pEntity->v.pContainingEntity, &pTrace);

	int index = DECAL_INDEX(logo_name);

	if (index < 0)
		return;

	if ((pTrace.pHit) && (pTrace.flFraction < 1.0f)) {
		if (pTrace.pHit->v.solid != SOLID_BSP)
			return;

		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);

		if (index > 255) {
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
void UTIL_BotRadioMessage(cBot* pBot, int radio, const char* arg1, const char* arg2) {
	// To be sure the console will only change when we MAY change.
	// The values will only be changed when console_nr is 0
	if (pBot->console_nr == 0) {
		switch (radio) {
		case 1:
			std::strcpy(pBot->arg1, "radio1");
			break;
		case 2:
			std::strcpy(pBot->arg1, "radio2");
			break;
		case 3:
			std::strcpy(pBot->arg1, "radio3");
			break;
		}

		std::strcpy(pBot->arg2, arg1);
		std::strcpy(pBot->arg3, arg2);
		pBot->console_nr = 1;     // Begin message

		// 02/07/04 - pointed out, eliminate any possible 'devide by zero'
		int iExtra = 100 / (pBot->ipCreateRadio + 1);
		if (iExtra > 30)
			iExtra = 30;
		pBot->fDoRadio = gpGlobals->time + static_cast<float>(iExtra);
	}
}

//////////////////////////////////
// UTIL_getGrenadeType function // - Stefan
//////////////////////////////////
int UTIL_GetGrenadeType(edict_t* pEntity) {
	constexpr int length = 32;

	char model_name[length] = {};

	std::strncpy(model_name, STRING(pEntity->v.model), length - 1);
	model_name[length - 1] = 0;      // Make sure it is NULL terminated

	if (std::strcmp(model_name, "models/w_hegrenade.mdl") == 0) return 1;          // He grenade
	if (std::strcmp(model_name, "models/w_flashbang.mdl") == 0) return 2;          // FlashBang
	if (std::strcmp(model_name, "models/w_smokegrenade.mdl") == 0) return 3;       // SmokeGrenade
	if (std::strcmp(model_name, "models/w_c4.mdl") == 0) return 4;                 // C4 Explosive

	// when an empty string, let us know we missed something
	if (model_name[0] == '\0') {

		char msg[512] = {};

		snprintf(msg, sizeof(msg), "UTIL_GetGrenadeType unknown grenade model: %s\n", model_name);
	}

	return 0;
}

// 2 functions from podbot source
unsigned short fixed_unsigned16(float value, float scale) {
	int output = static_cast<int>(value * scale);
	if (output < 0)
		output = 0;
	if (output > 0xFFFF)
		output = 0xFFFF;

	return static_cast<unsigned short>(output);
}

short fixed_signed16(float value, float scale) {
	int output = static_cast<int>(value * scale);

	if (output > 32767)
		output = 32767;

	if (output < -32768)
		output = -32768;

	return static_cast<short>(output);
}

// Using POD/SDK source to print nice messages on the client machine
void HUD_DrawString(int r, int g, int b, char* msg, edict_t* edict) {
	// FROM PODBOT SOURCE
	// Hacked together Version of HUD_DrawString
	MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, nullptr, edict);
	WRITE_BYTE(TE_TEXTMESSAGE);
	WRITE_BYTE(1);
	WRITE_SHORT(fixed_signed16(-1, 1 << 13));
	WRITE_SHORT(fixed_signed16(0, 1 << 13));
	WRITE_BYTE(2);
	WRITE_BYTE(r);               //r
	WRITE_BYTE(g);               //g
	WRITE_BYTE(b);               //b
	WRITE_BYTE(0);
	WRITE_BYTE(255);
	WRITE_BYTE(255);
	WRITE_BYTE(255);
	WRITE_BYTE(200);
	WRITE_SHORT(fixed_unsigned16(0.0078125, 1 << 8));
	WRITE_SHORT(fixed_unsigned16(2, 1 << 8));
	WRITE_SHORT(fixed_unsigned16(6, 1 << 8));
	WRITE_SHORT(fixed_unsigned16(0.1f, 1 << 8));
	WRITE_STRING(static_cast<const char*>(&msg[0]));
	MESSAGE_END();
}


void UTIL_FixAngles(Vector* Angles) {
	if (Angles->x > 180.0f)
		Angles->x -= 360.0f;
	if (Angles->x < -180.0f)
		Angles->x += 360.0f;
	if (Angles->y > 180.0f)
		Angles->y -= 360.0f;
	if (Angles->y < -180.0f)
		Angles->y += 360.0f;

	Angles->z = 0.0f;
}

void UTIL_SayTextBot(const char* pText, cBot* pBot) {
	if (gmsgSayText == 0)
		gmsgSayText = REG_USER_MSG("SayText", -1);

	char szTemp[160];
	char szName[BOT_NAME_LEN + 1];
	int i;

	// clear out
	std::memset(szTemp, 0, sizeof(szTemp));
	std::memset(szName, 0, sizeof(szName));

	// init
	szTemp[0] = 2;

	const int entind = ENTINDEX(pBot->pEdict);

	if (IsAlive(pBot->pEdict)) {
		std::strcpy(szName, pBot->name);
		for (i = 1; i <= gpGlobals->maxClients; i++) {
			edict_t* pPlayer = INDEXENT(i);

			// valid
			if (pPlayer)
				if (IsAlive(pPlayer))       // alive
				{
					MESSAGE_BEGIN(MSG_ONE, gmsgSayText, nullptr, pPlayer);
					WRITE_BYTE(entind);
					std::sprintf(&szTemp[1], "%s :   %s", szName, pText);
					WRITE_STRING(&szTemp[0]);
					MESSAGE_END();
				}
		}
	}
	else {
		std::strcpy(szName, pBot->name);
		for (i = 1; i <= gpGlobals->maxClients; i++) {
			edict_t* pPlayer = INDEXENT(i);
			if (pPlayer)
				if (!IsAlive(pPlayer)) {
					MESSAGE_BEGIN(MSG_ONE, gmsgSayText, nullptr, pPlayer);
					WRITE_BYTE(entind);
					std::sprintf(&szTemp[1], "*DEAD*%s :   %s", szName, pText);
					WRITE_STRING(&szTemp[0]);
					MESSAGE_END();
				}
		}
	}


	// pass through on ChatEngine (not always)
	if (RANDOM_LONG(0, 100) < 90) {
		char chSentence[80] = {};

		// copy pText to chSentence
		std::strcpy(chSentence, pText);

		// pass through
		ChatEngine.set_sentence(pBot->name, chSentence);
	}
}

// This UTIL_SpeechSynth() is taken from the POD source.
void UTIL_SpeechSynth(edict_t* pEdict, char* szMessage) {
	if (Game.bSpeechBroadcasting == false)
		return;

	char szSpeak[128];

	std::sprintf(szSpeak, "speak \"%s\"\n", szMessage);
	CLIENT_COMMAND(pEdict, szSpeak);
}

// $Log: util.cpp,v $
// Revision 1.11  2004/09/07 18:23:02  eric
// - bumped version to 3061
// - adding Frashman code to buy the weapon as selected by Josh's code
// - Realbot is really the result of multiple people :-)
//
// Revision 1.10  2004/09/07 15:44:34  eric
// - bumped build nr to 3060
// - minor changes in add2 (to add nodes for Bsp2Rbn utilities)
// - if compiled with USE_EVY_ADD, then the add2() function is used when adding
//   nodes based on human players instead of add()
// - else, it now compiles mostly without warnings :-)
//
// Revision 1.9  2004/07/30 15:02:30  eric
// - jumped to version 3057
// - improved readibility (wapen_tabel -> weapons_table) :-P
// - all Josh Borke modifications to the buying stuff:
//     * using a switch() instead of several if
//     * better buying code for shield and primary weapons
//     * new command 'debug pistols 0/1'
//
// Revision 1.8  2004/07/18 07:33:27  eric
// - bumped version to 3056
// - rblog the weapon_name when unknown weapon is found
//
// Revision 1.7  2004/07/08 09:04:57  stefan
// - fixed some shield detection bug... hopefully fixes improper buy behaviour
//
// Revision 1.6  2004/07/06 06:47:02  eric
// - built 3054
// - attempt to rblog() when unknown weapon (potentially "weapon_shield") is passed
//   to UTIL_GiveWeaponId
//
// Revision 1.5  2004/07/02 14:53:49  stefan
// - fixed 'devide by zero' bug pointed out on the forums
//
// Revision 1.4  2004/06/13 20:08:21  stefan
// - 'bad score for goals' added
// - bmp dump info (Thanks Evy)
// - added 'realbot server players', so you can keep a server full at NR players at all times
// - slightly adjusted goal selection code
// - wander code disabled
// - lots of debug info introduced, do not use this source for REAL USAGE!
//
// Revision 1.3  2004/05/29 19:05:47  stefan
// - upped to BUILD 3044
// - removed several debug messages on screen
// - changed default 'chatrate (max sentences)' to 3
// - removed copyright notice, which is not valid due GPL license
//
// i know, nothing special :)
//
// Revision 1.2  2004/04/18 17:39:19  stefan
// - Upped to build 2043
// - REALBOT_PRINT() works properly now
// - Log() works properly now
// - Clearing in dll.cpp of reallog.txt at dll init
// - Logging works now, add REALBOT_PRINT() at every point you want to log something.
//
