// Simple file to contain dummy version of functions required by NodeMachine.cpp
//
// evyncke@hec.be June 2004
//
//
#ifndef _WIN32
#include <string.h>
#endif

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>
#include <entity_state.h>

#include "../bot.h"
#include "../game.h"
#include "../bot_weapons.h"
#include "../bot_func.h"
#include "../NodeMachine.h"
#include "../IniParser.h"
#include "../ChatEngine.h"

// Most of below comes from dll.cpp

enginefuncs_t g_engfuncs;
globalvars_t pGlobals;
globalvars_t* gpGlobals = &pGlobals;
char g_argv[1024];
cBot bots[32];

int mod_id = CSTRIKE_DLL; // should be changed to 0 when we are going to do multi-mod stuff
int m_spriteTexture = 0;
int draw_nodepath = -1;

// CLASS DEFINITIONS
cGame Game;
cNodeMachine NodeMachine;
cChatEngine ChatEngine;

void FakeClientCommand(edict_t* pBot, char* arg1, char* arg2, char* arg3)
{
	fprintf(stderr, "FakeClientCommand is called!\n");
	exit(1);
}

// From game.cpp

// Debug message
void REALBOT_PRINT(cBot* pBot, const char* Function, const char* msg)
{
	// Message format:
	// Function name - [BOT NAME, BOT TEAM]: Message
	char team[9];
	char name[32];

	memset(team, 0, sizeof(team)); // clear
	memset(name, 0, sizeof(name)); // clear

	strcpy(team, "TERROR"); // t
	strcpy(name, "FUNCTION");

	if (pBot)
	{
		memset(name, 0, sizeof(name)); // clear
		strcpy(name, pBot->name); // copy name

		if (pBot->iTeam == 2)  strcpy(team, "COUNTER");
	}
	else
	{
		strcpy(team, "NONE");
	}

	printf("RBPRINT->[%s '%s']-[Team %s] : %s\n", name, Function, team, msg);

	char msgForFile[512];
	memset(msgForFile, 0, sizeof(msgForFile)); // clear
	sprintf(msgForFile, "RBPRINT->[%s '%s']-[Team %s] : %s\n", name, Function, team, msg);
	rblog(msgForFile);
}

// from ChatENgine.cpp

void cChatEngine::set_sentence(char csender[30], char csentence[128])
{
	fprintf(stderr, "cChatEngine::set_sentence is called!\n");
	exit(1);
}

// From bot.cpp

// Can see Edict?
bool cBot::canSeeEntity(edict_t* pEntity) const
{
	TraceResult tr;
	Vector start = pEdict->v.origin + pEdict->v.view_ofs;
	Vector vDest = pEntity->v.origin;

	// trace a line from bot's eyes to destination...
	UTIL_TraceLine(start, vDest, ignore_monsters, pEdict->v.pContainingEntity, &tr);

	if (tr.flFraction < 1.0)
	{
		// when the 'hit entity' is the same as pEntity, then its ok
		if (tr.pHit == pEntity)
			return true;          // it is visible

		return false;
	}

	return true;
}

bool cBot::Defuse()
{
	fprintf(stderr, "cBot::Defuse is called!\n");
	exit(1);
}

// From IniParser.cpp

// Parse IAD file:
// Important Area Definition file
void INI_PARSE_IAD()
{
	fprintf(stderr, "INI_PARSE_IAD is called!\n");
	exit(1);
}