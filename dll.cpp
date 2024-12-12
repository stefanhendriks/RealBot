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

#include <algorithm>
#include <cstring>
#include <extdll.h>
#include <dllapi.h>
#include <meta_api.h>
#include <entity_state.h>

#include "bot.h"
#include "bot_func.h"
#include "IniParser.h"
#include "game.h"               // GAME CLASS
#include "NodeMachine.h"        // NodeMachine
#include "ChatEngine.h"

// this makes sure function `min` is available (instead of fmin).
#include <algorithm>

//using namespace std;

DLL_FUNCTIONS gFunctionTable;
DLL_FUNCTIONS gFunctionTable_post;

enginefuncs_t g_engfuncs;
globalvars_t *gpGlobals;
char g_argv[1024];
extern cBot bots[32];

extern bool radio_message;

extern const char *rb_version_nr;
extern char *message;

// DLL specific variables
DLL_GLOBAL const Vector g_vecZero = Vector(0, 0, 0);

// CLASS DEFINITIONS
cGame Game;
cNodeMachine NodeMachine;
cChatEngine ChatEngine;
FILE *fpRblog = nullptr;

float f_load_time = 0.0f;
float f_minplayers_think = 0.0f; // timer used to add realbots if internet play enabled
int mod_id = CSTRIKE_DLL;       // should be changed to 0 when we are going to do multi-mod stuff
int m_spriteTexture = 0;
bool isFakeClientCommand = false;
int fake_arg_count;
float bot_check_time = 30.0f;
int min_bots = -1;
int max_bots = -1;
int min_players = -1;           // minimum amount of players that should be in the server all the time
int num_bots = 0;
int prev_num_bots = 0;
bool g_GameRules = false;
edict_t *clients[32];
edict_t *pHostEdict = nullptr;
float welcome_time = 0.0f;
bool welcome_sent = false;

FILE *bot_cfg_fp = nullptr;
bool need_to_open_cfg = true;
float bot_cfg_pause_time = 0.0f;
float respawn_time = 0.0f;
bool spawn_time_reset = false;

// Interval between joining bots.
int internet_max_interval = 30;
int internet_min_interval = 10;
// End...

// Counter-Strike 1.6 or 1.5
int counterstrike = 0;          // Default 1.5

void UpdateClientData(const edict_s *ent, int sendweapons, clientdata_s *cd); //Not used? [APG]RoboCop[CL]

void ProcessBotCfgFile();

// External added variables
bool end_round = false;

bool autoskill = false;
bool draw_nodes = false;
int draw_nodepath = -1;
bool draw_connodes = false;
int kick_amount_bots = 0;
int kick_bots_team = 0;

// internet mode vars
bool internet_addbot = false;   // Add a bot?
float add_timer = -1;           // Timer for adding bots
bool internet_play = false;

void RealBot_ServerCommand();

// START of Metamod stuff
enginefuncs_t meta_engfuncs;
gamedll_funcs_t *gpGamedllFuncs;
mutil_funcs_t *gpMetaUtilFuncs;
meta_globals_t *gpMetaGlobals;

META_FUNCTIONS gMetaFunctionTable = {
	nullptr,                        // pfnGetEntityAPI()
	nullptr,                        // pfnGetEntityAPI_Post()
        GetEntityAPI2,               // pfnGetEntityAPI2()
        GetEntityAPI2_Post,          // pfnGetEntityAPI2_Post()
	nullptr,                        // pfnGetNewDLLFunctions()
	nullptr,                        // pfnGetNewDLLFunctions_Post()
        GetEngineFunctions,          // pfnGetEngineFunctions()
	nullptr,                        // pfnGetEngineFunctions_Post()
};

plugin_info_t Plugin_info = {
        META_INTERFACE_VERSION,      // interface version
        "RealBot",                   // plugin name
        rb_version_nr,                 // plugin version
        __DATE__,                    // date of creation
        "Stefan Hendriks",           // plugin author
        "http://realbot.bots-united.com/",   // plugin URL
        "REALBOT",                   // plugin logtag
        PT_CHANGELEVEL,              // when loadable <-- FIX
        PT_ANYTIME,                  // when unloadable
};


C_DLLEXPORT int Meta_Query(const char *ifvers, plugin_info_t **pPlugInfo,
                           mutil_funcs_t *pMetaUtilFuncs) {
    // this function is the first function ever called by metamod in the plugin DLL. Its purpose
    // is for metamod to retrieve basic information about the plugin, such as its meta-interface
    // version, for ensuring compatibility with the current version of the running metamod.

    // keep track of the pointers to metamod function tables metamod gives us
    gpMetaUtilFuncs = pMetaUtilFuncs;
    *pPlugInfo = &Plugin_info;

    // check for interface version compatibility
    if (std::strcmp(ifvers, Plugin_info.ifvers) != 0) {
        int mmajor = 0, mminor = 0, pmajor = 0, pminor = 0;
        LOG_CONSOLE(PLID,
                    "%s: meta-interface version mismatch (metamod: %s, %s: %s)",
                    Plugin_info.name, ifvers, Plugin_info.name,
                    Plugin_info.ifvers);
        LOG_MESSAGE(PLID,
                    "%s: meta-interface version mismatch (metamod: %s, %s: %s)",
                    Plugin_info.name, ifvers, Plugin_info.name,
                    Plugin_info.ifvers);

        // if plugin has later interface version, it's incompatible (update metamod)
        sscanf(ifvers, "%d:%d", &mmajor, &mminor);
        sscanf(META_INTERFACE_VERSION, "%d:%d", &pmajor, &pminor);
        if (pmajor > mmajor || (pmajor == mmajor && pminor > mminor)) {
            LOG_CONSOLE(PLID,
                        "metamod version is too old for this plugin; update metamod");
            LOG_ERROR(PLID,
                      "metamod version is too old for this plugin; update metamod");
            return false;
        }
            // if plugin has older major interface version, it's incompatible (update plugin)
        if (pmajor < mmajor) {
	        LOG_CONSOLE(PLID,
	                    "metamod version is incompatible with this plugin; please find a newer version of this plugin");
	        LOG_ERROR(PLID,
	                  "metamod version is incompatible with this plugin; please find a newer version of this plugin");
	        return false;
        }
    }
    return true;               // tell metamod this plugin looks safe
}

C_DLLEXPORT int
Meta_Attach(PLUG_LOADTIME now, META_FUNCTIONS *pFunctionTable,
            meta_globals_t *pMGlobals, gamedll_funcs_t *pGamedllFuncs) {
    // this function is called when metamod attempts to load the plugin. Since it's the place
    // where we can tell if the plugin will be allowed to run or not, we wait until here to make
    // our initialization stuff, like registering CVARs and dedicated server commands.

    // are we allowed to load this plugin now ?
    if (now > Plugin_info.loadable) {
        LOG_CONSOLE(PLID,
                    "%s: plugin NOT attaching (can't load plugin right now)",
                    Plugin_info.name);
        LOG_ERROR(PLID,
                  "%s: plugin NOT attaching (can't load plugin right now)",
                  Plugin_info.name);
        return false;           // returning FALSE prevents metamod from attaching this plugin
    }
    // keep track of the pointers to engine function tables metamod gives us
    gpMetaGlobals = pMGlobals;
    std::memcpy(pFunctionTable, &gMetaFunctionTable, sizeof(META_FUNCTIONS));
    gpGamedllFuncs = pGamedllFuncs;

    // print a message to notify about plugin attaching
    LOG_CONSOLE(PLID, "%s: plugin attaching", Plugin_info.name);
    LOG_MESSAGE(PLID, "%s: plugin attaching", Plugin_info.name);

    // ask the engine to register the server commands this plugin uses
    REG_SVR_COMMAND("realbot", RealBot_ServerCommand);

    // Notify user that 'realbot' command is regged
    LOG_CONSOLE(PLID, "realbot - command prefix is now reserved.");
    LOG_MESSAGE(PLID, "realbot - command prefix is now reserved.");
    return true;               // returning TRUE enables metamod to attach this plugin
}

C_DLLEXPORT int Meta_Detach(PLUG_LOADTIME now, PL_UNLOAD_REASON reason) {
    // this function is called when metamod unloads the plugin. A basic check is made in order
    // to prevent unloading the plugin if its processing should not be interrupted.

    // is metamod allowed to unload the plugin ?
    if ((now > Plugin_info.unloadable) && (reason != PNL_CMD_FORCED)) {
        LOG_CONSOLE(PLID,
                    "%s: plugin NOT detaching (can't unload plugin right now)",
                    Plugin_info.name);
        LOG_ERROR(PLID,
                  "%s: plugin NOT detaching (can't unload plugin right now)",
                  Plugin_info.name);
        return false;           // returning FALSE prevents metamod from unloading this plugin
    }

    NodeMachine.FreeVisibilityTable();
    free(message);
    return true;               // returning TRUE enables metamod to unload this plugin
}

// END of Metamod stuff

#ifdef _WIN32
// Required DLL entry point
int WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
   return true;
}

#endif                          /*  */

C_DLLEXPORT void WINAPI
GiveFnptrsToDll(enginefuncs_t *pengfuncsFromEngine,
                globalvars_t *pGlobals) {

    // get the engine functions from the engine...
    std::memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
    gpGlobals = pGlobals;
    mod_id = CSTRIKE_DLL;        // so far RealBot works only for CS, eh Stefan ? :)
    // @PMB -> Yes so far it does ;) working on this MOD-GAME independent structure... grmbl
}

void GameDLLInit() {
	FILE* fplog = std::fopen("reallog.txt", "wt");

    if (fplog) {
        std::fprintf(fplog, "Realbot Logbook\n");
        std::fprintf(fplog, "Version %s\n\n", rb_version_nr);
        std::fclose(fplog);
    }

    // and now open it for the entire bot-dll-lifetime
    fpRblog = std::fopen("reallog.txt", "at");

    rblog("Initializing clients..");
    for (edict_t*& client : clients)
	    client = nullptr;
    rblog("OK\n");

    // initialize the bots array of structures...
    rblog("Initializing memory for bots array..");
    std::memset(bots, 0, sizeof(bots));
    rblog("OK\n");

    rblog("Verifying realbot is installed correctly..");

	bool bInstalledCorrectly = false;
    FILE* fp = std::fopen("realbot/dll/realbot_mm.dll", "rb");
    if (fp != nullptr) {
        bInstalledCorrectly = true;
        std::fclose(fp);
    }

    fp = std::fopen("realbot/dll/realbot_mm.so", "rb");
    if (fp != nullptr) {
        bInstalledCorrectly = true;
        std::fclose(fp);
    }

    if (bInstalledCorrectly)
        rblog("OK\n");
    else
        rblog("NOT FOUND!\n");

    // When installed correctly let the user know
    if (bInstalledCorrectly)
        REALBOT_PRINT(nullptr, "GAMEDLLINIT",
                      "Notice: RealBot is installed in the correct directory.");
    else
        REALBOT_PRINT(nullptr, "GAMEDLLINIT",
                      "WARNING: RealBot is NOT installed in the correct directory.");

    Game.Init();
    Game.LoadNames();
	cGame::LoadBuyTable();

    // NodeMachine
    NodeMachine.init();
    NodeMachine.init_players();

    // Chat engine
    ChatEngine.initAndload();
    ChatEngine.fThinkTimer = gpGlobals->time;

    // Set 'installed correctly' flag.
    Game.bInstalledCorrectly = bInstalledCorrectly;

    // CHECK FOR STEAM;
    // As lazy as i am i use pierre's code which is in our modified metamod dll.
    // Omg, why use so many comments? Sory pierre for ripping some pieces out.

    // Pierre-Marie Baty -- STEAM/NOSTEAM auto-adaptative fix -- START

    // to check whether Steam is installed or not, I test Steam-specific files.
    // if you know a more orthodox way of doing this, please tell me.

    // test file, if found = STEAM Linux/Win32 dedicated server
    fp = std::fopen("valve/steam.inf", "rb");
    if (fp != nullptr) {
        std::fclose(fp);               // test was successful, close it
        counterstrike = 1;        // its cs 1.6
    }
    // test file, if found = STEAM Win32 listenserver
    fp = std::fopen("FileSystem_Steam.dll", "rb");
    if (fp != nullptr) {
        std::fclose(fp);               // test was successful, close it
        counterstrike = 1;        // its cs 1.6
    }

    if (counterstrike == 0)
        SERVER_PRINT("Notice: Assuming you run Counter-Strike 1.5\n");

    else
        SERVER_PRINT("Notice: Assuming you run Counter-Strike 1.6\n");
    RETURN_META(MRES_IGNORED);
}

// INITIALIZATION
int Spawn(edict_t *pent) {
    if (gpGlobals->deathmatch) {
	    const char *pClassname = const_cast<char*>(STRING(pent->v.classname));

        if (std::strcmp(pClassname, "worldspawn") == 0) {
            // do level initialization stuff here...
            draw_nodes = false;
            draw_nodepath = -1;
            draw_connodes = false;

            // FIX: Internet mode timing
            internet_addbot = false;       // Add a bot?
            add_timer = gpGlobals->time;   // Timer for adding bots

            m_spriteTexture = PRECACHE_MODEL("sprites/lgtning.spr");

            // sound
            PRECACHE_SOUND("misc/imgood12.wav");

            g_GameRules = true;
            //bot_cfg_pause_time = 0.0;
            respawn_time = 0.0f;
            spawn_time_reset = false;
            prev_num_bots = num_bots;
            num_bots = 0;
            bot_check_time = gpGlobals->time + 30.0f;
            rblog("SPAWN : f_load_time is set\n");
            f_load_time = gpGlobals->time + 6;

            // Node machine loads
            NodeMachine.init();
            NodeMachine.load();
            NodeMachine.experience_load();

            ChatEngine.fThinkTimer = gpGlobals->time;
        } else if (std::strcmp(pClassname, "trigger_multiple") == 0) {
            // make it a func_button?
            //sprintf(STRING(pent->v.classname), "func_button");

        }
    }

    RETURN_META_VALUE(MRES_IGNORED, 0);
}

BOOL
ClientConnect(edict_t *pEntity, const char *pszName,
              const char *pszAddress, char szRejectReason[128]) {
    if (gpGlobals->deathmatch) {
	    // check if this client is the listen server client
        if (std::strcmp(pszAddress, "loopback") == 0) {
            // save the edict of the listen server client...
            pHostEdict = pEntity;
        }
        // 20/06/04 - It does not matter who joins, but we should never
        // EVER try to look for players for some time now.
        f_minplayers_think = gpGlobals->time + 90;        // give 90 seconds instead of 60


        // check if this is NOT a bot joining the server...
        if (std::strcmp(pszAddress, "127.0.0.1") != 0) {

	        int count = 0;
	        int i;

	        // don't try to add bots for 60 seconds, give client time to get added
            bot_check_time = gpGlobals->time + 60.0f;

            for (i = 0; i < 32; i++) {
                if (bots[i].bIsUsed)        // count the number of bots in use
                    count++;
            }
            // if there are currently more than the minimum number of bots running
            // then kick one of the bots off the server...
            if (count > min_bots && min_bots != -1) {
                for (i = 0; i < 32; i++) {
                    if (bots[i].bIsUsed)     // is this slot used?
                    {
                        char cmd[80];

                        snprintf(cmd, sizeof(cmd), "kick \"%s\"\n", bots[i].name);
                        SERVER_COMMAND(cmd);  // kick the bot using (kick "name")

                        break;
                    }
                }
            }
        } else {}
    }

    RETURN_META_VALUE(MRES_IGNORED, 0);
}

void ClientDisconnect(edict_t *pEntity) {

    if (gpGlobals->deathmatch) {
	    int i = 0;
        while (i < 32 && clients[i] != pEntity)
            i++;

        if (i < 32)
            clients[i] = nullptr;

        // when a bot...
        for (i = 0; i < 32; i++) {
            if (bots[i].pEdict == pEntity) {

                // someone kicked this bot off of the server...
                bots[i].bIsUsed = false;    // this slot is now free to use
                bots[i].fKickTime = gpGlobals->time;        // save the kicked time
                bots[i].pEdict = nullptr;      // make NULL
                break;
            }
        }
    }

    RETURN_META(MRES_IGNORED);
}

void ClientPutInServer(edict_t *pEntity) {
    int i = 0;

    while (i < 32 && (clients[i] != nullptr))
        i++;

    if (i < 32)
        clients[i] = pEntity;     // store this clients edict in the clients array

    RETURN_META(MRES_IGNORED);
}

// CLIENT / CONSOLE / COMMANDS
void ClientCommand(edict_t *pEntity) {

    /*

       EMPTY - BANNED - UNUSED - MUHAHAHA
       SINCE METAMOD ;) BUILD 2053

       // only allow custom commands if deathmatch mode and NOT dedicated server and
       // client sending command is the listen server client...

       if ((gpGlobals->deathmatch) && !IS_DEDICATED_SERVER() &&
       (pEntity == pHostEdict))
       {
       const char *pcmd = CMD_ARGV(0);
       const char *arg1 = CMD_ARGV(1);
       const char *arg2 = CMD_ARGV(2);
       const char *arg3 = CMD_ARGV(3);
       const char *arg4 = CMD_ARGV(4);

       }
     */
    RETURN_META(MRES_IGNORED);
}

// TODO: Revise this method
void StartFrame() {
    if (!gpGlobals->deathmatch) return; // bots only work in 'deathmatch mode'
	//REALBOT_PRINT("StartFrame", "BEGIN");

    edict_t *pPlayer;
    static int i, player_index, bot_index;
    static float previous_time = -1.0f;
    static float client_update_time = 0.0f;
    clientdata_s cd;
    int count = 0;
    int waits = 0;            // How many bots had to wait.

    // TODO: Add Autovacate regulation with bot count loop
    // Experimental autovacate script from POD-Bot Incomplete [APG]RoboCop[CL]
    /*max_bots = max_bots > gpGlobals->maxClients ? gpGlobals->maxClients : max_bots < 0 ? 0 : max_bots;
    min_bots = min_bots > gpGlobals->maxClients ? gpGlobals->maxClients : min_bots < 0 ? 0 : min_bots;
    if (max_bots < min_bots)
        min_bots = max_bots;*/

    // When a user - or anything else - specified a higher number of 0 to
    // kick bots, then we will do as told.
    if (kick_amount_bots > 0) {
        int kicking_team = 0;  // What team should we kick?

        if (kick_bots_team == 0 || kick_bots_team == 6)        // No kick_bots_team so we get
            kick_bots_team = 6 + RANDOM_LONG(0, 1);             // a default one! (random)

        if (kick_bots_team == 6)       // Its random.. (5 means CT)
        {
            kicking_team = 1;
            kick_bots_team++;
        } else if (kick_bots_team == 7) {
            kicking_team = 2;
            kick_bots_team = 6;
        } else
            kicking_team = kick_bots_team;

        if (kick_bots_team > 7)
            kick_bots_team = 6;

        if (kick_bots_team == 1)
            kicking_team = 1;

        if (kick_bots_team == 2)
            kicking_team = 2;

        for (i = 0; i < 32; i++) {
            if (bots[i].bIsUsed)        // is this slot used?
            {
	            if (bots[i].iTeam == kicking_team) {
		            char cmd[80];
		            snprintf(cmd, sizeof(cmd), "kick \"%s\"\n", bots[i].name);
                    SERVER_COMMAND(cmd);  // kick the bot using (kick "name")
                    break;
                }

            }                   // Used?
        }                      // I

        kick_amount_bots--;    // next frame we kick another bot
    } else {
        kick_bots_team = 0;    // its always 0 when we have no one to kick
    }

    // if a new map has started then (MUST BE FIRST IN StartFrame)...
    // which is determined by comparing the previously recorded time (at the end of this function)
    // with the current time. If the current time somehow was less (before) the previous time, then we
    // assume a reset/restart/reload of a map.
    if (gpGlobals->time + 0.1f < previous_time) {
	    static int index;
	    static float check_server_cmd = 0.0f;
	    rblog("NEW MAP because time is reset #1\n");
        check_server_cmd = 0.0f;        // reset at start of map

        count = 0;

        // mark the bots as needing to be respawned...
        for (index = 0; index < 32; index++) {
            cBot *pBot = &bots[index];
            if (count >= prev_num_bots) {
                pBot->bIsUsed = false;
                pBot->respawn_state = RESPAWN_NONE;
                pBot->fKickTime = 0.0f;
            }

            if (pBot->bIsUsed)    // is this slot used?
            {
                pBot->respawn_state = RESPAWN_NEED_TO_RESPAWN;
                count++;
            }

            // check for any bots that were very recently kicked...
            if (pBot->fKickTime + 5.0f > previous_time) {
                pBot->respawn_state = RESPAWN_NEED_TO_RESPAWN;
                count++;
            } else {
                pBot->fKickTime = 0.0f;     // reset to prevent false spawns later
            }

            // set the respawn time
            if (IS_DEDICATED_SERVER()) {
                respawn_time = gpGlobals->time + 5.0f;
            } else {
                respawn_time = gpGlobals->time + 20.0f;
            }

            // Send welcome message
            welcome_sent = false;
            welcome_time = 0.0f;
        }

        NodeMachine.init_players();
        NodeMachine.init_round();
        NodeMachine.setUpInitialGoals();
        NodeMachine.resetCheckedValuesForGoals();

        //ChatEngine.fThinkTimer = gpGlobals->time;
        client_update_time = gpGlobals->time + 10.0f;   // start updating client data again

        bot_check_time = gpGlobals->time + 30.0f;
    } // New map/reload/restart

    //
    // SEND WELCOME MESSAGE
    //
    if (!welcome_sent) {
        int iIndex = 0;
        if (welcome_time == 0.0f) {

            // go through all clients (except bots)
            for (iIndex = 1; iIndex <= gpGlobals->maxClients; iIndex++) {
                edict_t *pPlayer = INDEXENT(iIndex);
                // skip invalid players
                if (pPlayer && !pPlayer->free) {
                    // we found a player which is alive. w00t
                    welcome_time = gpGlobals->time + 10.0f;
                    break;
                }
            }
        }

        if (welcome_time > 0.0f && welcome_time < gpGlobals->time) {
            // let's send a welcome message to this client...
            char total_welcome[256];
            snprintf(total_welcome, sizeof(total_welcome), "RealBot - Version %s\nBy Stefan Hendriks\n", rb_version_nr);
            int r, g, b;
            /*
               r = RANDOM_LONG(30, 255);
               g = RANDOM_LONG(30, 255);
               b = RANDOM_LONG(30, 255);

               // Send to listen server
               if (pHostEdict)
               HUD_DrawString(r,g,b, total_welcome, pHostEdict);

             */
            for (iIndex = 1; iIndex <= gpGlobals->maxClients; iIndex++) {
                edict_t *pPlayer = INDEXENT(iIndex);
                // skip invalid players and skip self (i.e. this bot)
                if (pPlayer && !pPlayer->free) {
                    // skip bots!
                    if (UTIL_GetBotPointer(pPlayer))
                        continue;

                    // skip fake clients
                    if (pPlayer->v.flags & FL_THIRDPARTYBOT
	                    || pPlayer->v.flags & FL_FAKECLIENT)
                        continue;

                    // random color
                    r = RANDOM_LONG(30, 255);
                    g = RANDOM_LONG(30, 255);
                    b = RANDOM_LONG(30, 255);
                    HUD_DrawString(r, g, b, total_welcome, pPlayer);

                    // use speak command
                    if (pPlayer == pHostEdict)
                        UTIL_SpeechSynth(pHostEdict, Game.RandomSentence());
                }
            }

            welcome_sent = true;        // clear this so we only do it once
            welcome_time = 0.0f;
        }
    }

    if (client_update_time <= gpGlobals->time) {
        client_update_time = gpGlobals->time + 1.0f;

        for (i = 0; i < 32; i++) {
            if (bots[i].bIsUsed) {
                std::memset(&cd, 0, sizeof(cd));

                MDLL_UpdateClientData(bots[i].pEdict, 1, &cd);

                // see if a weapon was dropped...
                if (bots[i].bot_weapons != cd.weapons) {
                    bots[i].bot_weapons = cd.weapons;
                }
            }
        }
    }

    // a few seconds after map load we assign goals
    if (f_load_time < gpGlobals->time && f_load_time != 0.0f) {
        f_load_time = 0.0f;     // do not load again
        rblog("NEW MAP because time is reset #2\n");
        Game.DetermineMapGoal();
        Game.resetRoundTime();
        NodeMachine.setUpInitialGoals();
    }

    // Fix kill all with new round
    if (Game.NewRound()) {
        rblog("dll.cpp:740, Game.NewRound\n");
        // Send a message to clients about RealBot every round
        if (Game.iVersionBroadcasting == BROADCAST_ROUND) {
            welcome_sent = false;
            welcome_time = gpGlobals->time + 2.0f;
        }

        NodeMachine.init_round();
        NodeMachine.save();    // save information
        NodeMachine.experience_save();
        NodeMachine.setUpInitialGoals();
        end_round = false;
    } // new round - before any bots realized yet

    // When min players is set
    if (min_players > 0 && f_minplayers_think < gpGlobals->time) {
        internet_play = false; // when there is min_players set there is NO simulated internet_play!
        int iHumans = 0, iBots = 0;

        // Search for human players, simple method...
        for (int i = 1; i <= gpGlobals->maxClients; i++) {
            edict_t *pPlayer = INDEXENT(i);
            // skip invalid players and skip self (i.e. this bot)
            if (pPlayer && !pPlayer->free) {
                // a bot
                if (UTIL_GetBotPointer(pPlayer) != nullptr)
                    iBots++;
                else {
                    if (pPlayer->v.flags & FL_CLIENT)
                        iHumans++; // it is 'human' (well unless some idiot uses another bot, i cannot detect that!)
                }
            }
        }

        // There are not enough humans, so add/remove bots
        if (iHumans < min_players) {
            int iTotal = iHumans + iBots;

            //      char msg[80];

            // total players is higher then min_players due BOTS, remove a bot
            if (iTotal > min_players) {
                // find one bot and remove it (one by one)
                SERVER_PRINT("RBSERVER: Too many players, kicking one bot.\n");
                kick_amount_bots = 1;
            }
                // total players is lower then min_players due BOTS, add a bot
            else if (iTotal < min_players) {
                // add a bot
                SERVER_PRINT("RBSERVER: Too few player slots filled, adding one bot.\n");
                Game.createBot(nullptr, nullptr, nullptr, nullptr, nullptr);
            }
        } else {
            if (num_bots > 0) {
                // there are enough human players, remove any bot
                kick_amount_bots = 32;
            }
        }
        // 2 seconds thinking
        f_minplayers_think = gpGlobals->time + 2.0f;
    }
    // INTERNET MODE:
    if (internet_play == false)
        add_timer = gpGlobals->time + 2.0f;
    else                      // ------------ INTERNET MODE IS ON ------------
    {
        bool max_serverbots = true;    // Reached MAX bots?

        // When MAX_BOTS disabled...
        if (max_bots < 0)
            max_serverbots = false;

        // When MAX_BOTS enabled...
        if (max_bots > -1)
            if (num_bots < max_bots)
                max_serverbots = false;  // we may add bots

        if (max_serverbots == false)   // we may add bots
            if (num_bots + 1 >= gpGlobals->maxClients)
                max_serverbots = true;   // we are actually full. Give one player the chance to fill in

        // Keep adding bots until 'full'
        if (max_serverbots == false) {
            if (add_timer < gpGlobals->time) {
                add_timer =
                        gpGlobals->time + RANDOM_LONG(internet_min_interval,
                                                      internet_max_interval);
                internet_addbot = true;  // add a bot! w00t
            }
        }
    }

    if (internet_addbot) {
        // When timer is set, create a new bot.
        if (add_timer > gpGlobals->time) {
            internet_addbot = false;
            Game.createBot(nullptr, nullptr, nullptr, nullptr, nullptr);
            bot_check_time = gpGlobals->time + 8.0f;
        }

    }

    NodeMachine.addNodesForPlayers();

    count = 0;
    waits = 0;

    // -------------------------------------------------------------
    // GAME : Update game status first, before we go think about it
    // -------------------------------------------------------------
    Game.UpdateGameStatus();

    for (bot_index = 0; bot_index < gpGlobals->maxClients; bot_index++) {
        cBot &bot = bots[bot_index];
        if (bot.bIsUsed &&                        // is this slot used AND
            bot.respawn_state == RESPAWN_IDLE)    // not respawning
        {
            if (bot.fUpdateTime < gpGlobals->time) {
                BotThink(&bot);
            }
            count++;
        }
    }                         // FOR


    // -------------------------------------------------------------
    // CHATENGINE : Think now
    // -------------------------------------------------------------
    ChatEngine.think();

    // Keep number of bots up-to-date.
    num_bots = std::max(count, num_bots);

    // handle radio commands
    if (radio_message) {
        BotRadioAction();
        radio_message = false;
    }
    // FIX: Suggested by Greg, unnescesary loops.
    if (draw_nodes) {
        for (player_index = 1; player_index <= gpGlobals->maxClients;
             player_index++) {
            pPlayer = INDEXENT(player_index);

            if (pPlayer && !pPlayer->free) {
                if (FBitSet(pPlayer->v.flags, FL_CLIENT)) {
                    NodeMachine.draw(pPlayer);
                    break;
                }
            }
        }
    }

    if (draw_connodes) {
        for (player_index = 1; player_index <= gpGlobals->maxClients;
             player_index++) {
            pPlayer = INDEXENT(player_index);

            if (pPlayer && !pPlayer->free) {
                if (FBitSet(pPlayer->v.flags, FL_CLIENT)) {
                    NodeMachine.connections(pPlayer);
                    break;
                }
            }
        }
    }

    // Draw node path
    if (draw_nodepath > -1) {
        NodeMachine.path_draw(pHostEdict);
    }

    // Counter-Strike - A new round has started
    if (Game.NewRound()) {
        rblog("dll.cpp:917, Game.NewRound\n");
        NodeMachine.scale_danger();    // Scale danger
        NodeMachine.scale_contact();   // same for contact
        Game.InitNewRound();
        Game.SetNewRound(false);
        rblog("dll.cpp:917, Game.NewRound - finished\n");
    }

    // are we currently respawning bots and is it time to spawn one yet?
    if (respawn_time > 1.0f && respawn_time <= gpGlobals->time) {
        int index = 0;
        // find bot needing to be respawned...
        while (index < 32
               && bots[index].respawn_state != RESPAWN_NEED_TO_RESPAWN)
            index++;

        if (index < 32) {
            bots[index].respawn_state = RESPAWN_IS_RESPAWNING;
            bots[index].bIsUsed = false;        // free up this slot

            // respawn 1 bot then wait a while (otherwise engine crashes)
            // 01/07/04 - Stefan - Thanks Evy for pointing out this one: On skill 10
            // the c_skill (was [2]) would be messed up (perhaps this is some memory related bug later on?)
            char c_skill[3];
            char c_team[2];
            char c_class[3];

			snprintf(c_skill, sizeof(c_skill), "%d", bots[index].bot_skill);
            snprintf(c_team, sizeof(c_team), "%d", bots[index].iTeam);
            snprintf(c_class, sizeof(c_class), "%d", bots[index].bot_class);

            Game.createBot(nullptr, c_team, c_skill, c_class,
                           bots[index].name);

            // 01/07/04 - Stefan - make 100% sure we do not crash on this part with the auto-add function
            f_minplayers_think = gpGlobals->time + 15;  // do not check this for 15 seconds from now

            // Added more delay to prevent possible "Tried to write to uninitialized sizebuf_t" crashes - [APG]RoboCop[CL]
            respawn_time = gpGlobals->time + 7.0f;       // set next respawn time
            bot_check_time = gpGlobals->time + 8.0f;
        } else {
            respawn_time = 0.0f;
        }
    }

    if (g_GameRules) {
        if (need_to_open_cfg)  // have we open bot.cfg file yet?
        {
	        char msg[256];
	        char filename[256];

            need_to_open_cfg = false;   // only do this once!!!

            UTIL_BuildFileNameRB("bot.cfg", filename);

            snprintf(msg, sizeof(msg), "Executing %s\n", filename);
            ALERT(at_console, msg);

            bot_cfg_fp = std::fopen(filename, "r");

            if (bot_cfg_fp == nullptr)
                ALERT(at_console, "bot.cfg file not found\n");

            if (IS_DEDICATED_SERVER())
                bot_cfg_pause_time = gpGlobals->time + 5.0f;
            else
                bot_cfg_pause_time = gpGlobals->time + 20.0f;
        }

        if (!IS_DEDICATED_SERVER() && !spawn_time_reset) {
            if (pHostEdict != nullptr) {
                if (IsAlive(pHostEdict)) {
                    spawn_time_reset = true;

                    if (respawn_time >= 1.0f)
                        respawn_time = std::min(respawn_time, gpGlobals->time + 1.0f);

                    if (bot_cfg_pause_time >= 1.0f)
                        bot_cfg_pause_time =
                                std::min(bot_cfg_pause_time,
                                    gpGlobals->time + 1.0f);
                }
            }
        }
        // DO BOT.CFG crap here
        if (bot_cfg_fp && bot_cfg_pause_time >= 1.0f
            && bot_cfg_pause_time <= gpGlobals->time) {
            // process bot.cfg file options...
            ProcessBotCfgFile();
        }

    }

    // remember the time
    previous_time = gpGlobals->time;

	// REALBOT_PRINT("StartFrame", "END");
    RETURN_META(MRES_IGNORED);
}

void FakeClientCommand(edict_t* pBot, char* arg1, char* arg2, char* arg3) {
    int length;

    std::memset(g_argv, 0, sizeof(g_argv));

    isFakeClientCommand = true;

    if (arg1 == nullptr || *arg1 == 0)
        return;

    if (arg2 == nullptr || *arg2 == 0) {
        length = snprintf(&g_argv[0], sizeof(g_argv), "%s", arg1);
        fake_arg_count = 1;
    }
    else if (arg3 == nullptr || *arg3 == 0) {
        length = snprintf(&g_argv[0], sizeof(g_argv), "%s %s", arg1, arg2);
        fake_arg_count = 2;
    }
    else {
        length = snprintf(&g_argv[0], sizeof(g_argv), "%s %s %s", arg1, arg2, arg3);
        fake_arg_count = 3;
    }

    g_argv[length] = 0;          // null terminate just in case

    std::strcpy(&g_argv[64], arg1);

    if (arg2)
        std::strcpy(&g_argv[128], arg2);

    if (arg3)
        std::strcpy(&g_argv[192], arg3);

    // allow the MOD DLL to execute the ClientCommand...
    MDLL_ClientCommand(pBot);

    isFakeClientCommand = false;
}

/*void UpdateClientData(const edict_s* ent, int sendweapons, clientdata_s* cd) // KWo 02.03.2010 - thanks to MeRcyLeZZ
{
    static int sending;
    static int bot_index;
    static edict_t* pPlayer;

    if (g_i_cv_latencybot != 2)
        RETURN_META(MRES_IGNORED);

    sending = 0;

    // Scoreboard key being pressed?
    if (!FNullEnt(ent) && ent->v.flags & FL_CLIENT
        && (ent->v.button & IN_SCORE || ent->v.oldbuttons & IN_SCORE))
    {
        pPlayer = INDEXENT(ENTINDEX(ent));

        for (bot_index = 0; bot_index < gpGlobals->maxClients; bot_index++)
        {
            if (bots[bot_index].is_used && !FNullEnt(bots[bot_index].pEdict))
            {
                // Send message with the weird arguments
                switch (sending)
                {
                case 0:
                {
                    // Start a new message
                    MESSAGE_BEGIN(MSG_ONE_UNRELIABLE, SVC_PINGS, NULL, pPlayer);
                    WRITE_BYTE(bots[bot_index].iOffsetPing[0] * 64 + (1 + 2 * bot_index));
                    WRITE_SHORT(bots[bot_index].iArgPing[0]);
                    sending++;
                }
                case 1:
                {
                    // Append additional data
                    WRITE_BYTE(bots[bot_index].iOffsetPing[1] * 128 + (2 + 4 * bot_index));
                    WRITE_SHORT(bots[bot_index].iArgPing[1]);
                    sending++;
                }
                case 2:
                {
                    // Append additional data and end message
                    WRITE_BYTE(4 + 8 * bot_index);
                    WRITE_SHORT(bots[bot_index].iArgPing[2]);
                    WRITE_BYTE(0);
                    MESSAGE_END();
                    sending = 0;
                }
                }
            }
        }
        // End message if not yet sent
        if (sending)
        {
            WRITE_BYTE(0);
            MESSAGE_END();
        }
    }
    RETURN_META(MRES_IGNORED);
}*/

void UpdateClientData(const edict_s* ent, int sendweapons, clientdata_s* cd) //TODO: Needed implemented
{
}

void ProcessBotCfgFile() {
	char cmd_line[256];
	static char server_cmd[80];
    char *arg2, *arg3, *arg4;
    char msg[80];

    if (bot_cfg_pause_time > gpGlobals->time)
        return;

    if (bot_cfg_fp == nullptr)
        return;

    int cmd_index = 0;
    cmd_line[cmd_index] = 0;

    int ch = fgetc(bot_cfg_fp);

    // skip any leading blanks
    while (ch == ' ')
        ch = fgetc(bot_cfg_fp);

    while (ch != EOF && ch != '\r' && ch != '\n') {
        if (ch == '\t')           // convert tabs to spaces
            ch = ' ';

        cmd_line[cmd_index] = ch;

        ch = fgetc(bot_cfg_fp);

        // skip multiple spaces in input file
        while (cmd_line[cmd_index] == ' ' && ch == ' ')
            ch = fgetc(bot_cfg_fp);

        cmd_index++;
    }

    if (ch == '\r')              // is it a carriage return?
    {
        ch = fgetc(bot_cfg_fp);   // skip the linefeed
    }
    // if reached end of file, then close it
    if (ch == EOF) {
        std::fclose(bot_cfg_fp);

        bot_cfg_fp = nullptr;

		bot_cfg_pause_time = 8.0f;  // wait 8 seconds before starting
    }

    cmd_line[cmd_index] = 0;     // terminate the command line

    // copy the command line to a server command buffer...
    //TODO: To use std:string for this [APG]RoboCop[CL]
    std::strncpy(server_cmd, cmd_line, sizeof(server_cmd));
    server_cmd[sizeof(server_cmd) - 1] = '\0';

    std::strcat(server_cmd, "\n");

    cmd_index = 0;
	const char* cmd = cmd_line;
	const char* arg1 = arg2 = arg3 = arg4 = nullptr;

    // skip to blank or end of string...
    while (cmd_line[cmd_index] != ' ' && cmd_line[cmd_index] != 0)
        cmd_index++;

    if (cmd_line[cmd_index] == ' ') {
        cmd_line[cmd_index++] = 0;
        arg1 = &cmd_line[cmd_index];

        // skip to blank or end of string...
        while (cmd_line[cmd_index] != ' ' && cmd_line[cmd_index] != 0)
            cmd_index++;

        if (cmd_line[cmd_index] == ' ') {
            cmd_line[cmd_index++] = 0;
            arg2 = &cmd_line[cmd_index];


            // skip to blank or end of string...
            while (cmd_line[cmd_index] != ' ' && cmd_line[cmd_index] != 0)
                cmd_index++;

            if (cmd_line[cmd_index] == ' ') {
                cmd_line[cmd_index++] = 0;
                arg3 = &cmd_line[cmd_index];

                // skip to blank or end of string...
                while (cmd_line[cmd_index] != ' '
                       && cmd_line[cmd_index] != 0)
                    cmd_index++;

                if (cmd_line[cmd_index] == ' ') {
                    cmd_line[cmd_index++] = 0;
                    arg4 = &cmd_line[cmd_index];
                }
            }
        }
    }

    if (cmd_line[0] == '#' || cmd_line[0] == 0)
        return;                   // return if comment or blank line

    if (std::strcmp(cmd, "pause") == 0) {
        bot_cfg_pause_time = gpGlobals->time + std::atoi(arg1);
        return;
    }
    // 07/02/04 - This gives a user theoreticly the power as in the console
    // use 'realbot addbot' to add a bot, etc. I dont think we need more
    // it would double the work.

    snprintf(msg, sizeof(msg), "BOT.CFG >> Executing command: %s", server_cmd);        // removed \n
    ALERT(at_console, msg);

    if (IS_DEDICATED_SERVER())
        printf("%s", msg);

    SERVER_COMMAND(server_cmd);
}

// REALBOT COMMAND
// SERVER Command: RealBot
void RealBot_ServerCommand() {
    const char *pcmd = CMD_ARGV(1);
    const char *arg1 = CMD_ARGV(2);
    const char *arg2 = CMD_ARGV(3);
    const char *arg3 = CMD_ARGV(4);
    const char *arg4 = CMD_ARGV(5);

    char cMessage[256];
    bool bSendMessage = true;

    // When we need an edict to send messages to, we do that to the pHostEdict
    edict_t *pEntity = pHostEdict;

    // Handle command here
    if (FStrEq(pcmd, "help")) {
        // Give information
        bSendMessage = false;     // do not use 'standard' stuff
        SERVER_PRINT("=====================================\n");
        SERVER_PRINT("Syntax: realbot [command] [arg1/subcommand] [arg2] [arg3] [arg4]\n\n");
        SERVER_PRINT("List of most-used commands; for full command list read the readme.\n\n");
        SERVER_PRINT("realbot add (team) (skill) (model) (name)\n");
        SERVER_PRINT("realbot max (max amount bots)\n");
        SERVER_PRINT("realbot killall\n");
        SERVER_PRINT("realbot internet (1=ON, 0=OFF(default))\n");
        SERVER_PRINT("realbot remove (amount) ((optional) of team 1(T)/2(CT))\n");
        SERVER_PRINT("realbot skill (-1(random), 0(godlike)-10(newbie)\n");
        SERVER_PRINT("realbot server [subcommand]\n");
        SERVER_PRINT("=====================================\n");
    } else if (FStrEq(pcmd, "chatrate")) {
        if (arg1 != nullptr && *arg1 != 0) {
            Game.iMaxSentences = std::atoi(arg1);
            Game.iMaxSentences = std::max(Game.iMaxSentences, -1);

            Game.iMaxSentences = std::min(Game.iMaxSentences, 10);

            snprintf(cMessage, sizeof(cMessage), "REALBOT: Chat-rate set to %d",
                     Game.iMaxSentences);
        } else {
            snprintf(cMessage, sizeof(cMessage),
                    "REALBOT: No argument given, current chat-rate is %d",
                    Game.iMaxSentences);

        }
    } else if (FStrEq(pcmd, "sound")) {
        EMIT_SOUND_DYN2(pEntity, CHAN_VOICE, "misc/imgood12.wav", 1.0f,
                        ATTN_NORM, 0, 100);
    } else if (FStrEq(pcmd, "add")) {
	    const int iStatus = Game.createBot(pEntity, arg1, arg2, arg3, arg4);
        bot_check_time = gpGlobals->time + 8.0f;
        if (iStatus == GAME_MSG_SUCCESS)
            snprintf(cMessage, sizeof(cMessage), "REALBOT: Successfully created bot.");

        else if (iStatus == GAME_MSG_FAILURE)
            snprintf(cMessage, sizeof(cMessage), "REALBOT: Failed creating bot.");

        else if (iStatus == GAME_MSG_FAIL_SERVERFULL)
            snprintf(cMessage, sizeof(cMessage),
                    "REALBOT: Failed creating bot, server is full.");
    } else if (FStrEq(pcmd, "walkwithknife")) {
        if (arg1 != nullptr && *arg1 != 0) {
	        const float fVar = std::atof(arg1);

            // Only set when valid
            if (fVar < 0)
                Game.fWalkWithKnife = 0;

            else
                Game.fWalkWithKnife = fVar;

            // Show amount set
            if (Game.fWalkWithKnife > 0)
                snprintf(cMessage, sizeof(cMessage),
                        "REALBOT: Bots may walk with knife for %f seconds.",
                        Game.fWalkWithKnife);

            else
                snprintf(cMessage, sizeof(cMessage),
                        "REALBOT: Bots may not walk with knife (value=0)");
        } else
            snprintf(cMessage, sizeof(cMessage), "REALBOT: No valid argument given.");
    } else if (FStrEq(pcmd, "max")) {
        if (arg1 != nullptr && *arg1 != 0) {
            max_bots = std::atoi(arg1);
            if (max_bots < 0 || max_bots > 31)
                max_bots = -1;

            // Show amount set
            snprintf(cMessage, sizeof(cMessage), "REALBOT: Max amount of bots is set to %d.",
                    max_bots);
        } else {
            // sprintf (cMessage, "REALBOT: No valid argument given.");
            snprintf(cMessage, sizeof(cMessage),
                    "REALBOT: Max amount of bots is %d -- no valid argument given.",
                    max_bots);
        }
    } else if (FStrEq(pcmd, "important")) {
        // Broadcast
        if (FStrEq(arg1, "add")) {
            if (pHostEdict != nullptr) {
                NodeMachine.addGoal(nullptr, GOAL_IMPORTANT, pHostEdict->v.origin);
                snprintf(cMessage, sizeof(cMessage), "REALBOT: Added important area/goal.");
            } else
                snprintf(cMessage, sizeof(cMessage), "REALBOT: Only a listen server can execute this command!");
        } else if (FStrEq(arg1, "save")) {
            NodeMachine.save_important();
            snprintf(cMessage, sizeof(cMessage),
                    "REALBOT: Important Area Definitions written to INI file");
        } else if (FStrEq(arg1, "init")) {
            // clear all goals that are 'goal_important'
            NodeMachine.ClearImportantGoals();
            snprintf(cMessage, sizeof(cMessage),
                    "REALBOT: All important goals have been removed.");
        } else
            snprintf(cMessage, sizeof(cMessage),
                    "REALBOT: 'important' sub-commands are: add, save, init");


    } else if (FStrEq(pcmd, "killall")) {
        snprintf(cMessage, sizeof(cMessage), "REALBOT: Killing all bots.");
        end_round = true;
    } else if (FStrEq(pcmd, "csversion")) {
        if (arg1 != nullptr && *arg1 != 0) {
	        const int temp = std::atoi(arg1);
            if (temp <= 0)
                counterstrike = 0;  // cs 1.5
            else
                counterstrike = 1;  // cs 1.6
            if (counterstrike == 0)
                snprintf(cMessage, sizeof(cMessage),
                        "REALBOT: Set bot-rules for Counter-Strike 1.5.");

            else
                snprintf(cMessage, sizeof(cMessage),
                        "REALBOT: Set bot-rules for Counter-Strike 1.6.");
        } else {
            if (counterstrike == 0)
                snprintf(cMessage, sizeof(cMessage),
                        "REALBOT: bot-rules are set for Counter-Strike 1.5.");

            else
                snprintf(cMessage, sizeof(cMessage),
                        "REALBOT: bot-rules are set for Counter-Strike 1.6.");
        }
    } else if (FStrEq(pcmd, "internet")) {
        if (arg1 != nullptr && *arg1 != 0) {

            // switch on/off internet mode
            const int temp = std::atoi(arg1);
            if (temp == 0)
                internet_play = false;

            else
                internet_play = true;
            if (internet_play)
                snprintf(cMessage, sizeof(cMessage), "REALBOT: Internet simulation - enabled.");

            else
                snprintf(cMessage, sizeof(cMessage), "REALBOT: Internet simulation - disabled.");
        } else
            snprintf(cMessage, sizeof(cMessage), "REALBOT: No valid argument given.");
    } else if (FStrEq(pcmd, "internet_interval")) {

        // 1st argument
        if (arg1 != nullptr && *arg1 != 0) {

            // switch on/off internet mode
            int temp = std::atoi(arg1);
            if (temp > -1) {
	            temp = std::max(temp, 1);
	            internet_min_interval = temp;
            }
        }
        // 2nd argument
        if (arg2 != nullptr && *arg2 != 0) {

            // switch on/off internet mode
            int temp = std::atoi(arg2);
            if (temp > -1) {
	            temp = std::max(temp, internet_min_interval);
	            internet_max_interval = temp;
            }
        }
        // Create message
        snprintf(cMessage, sizeof(cMessage),
                "REALBOT: Internet simulation - Interval set to, MIN %d - MAX %d",
                internet_min_interval, internet_max_interval);
    } else if (FStrEq(pcmd, "remove") && kick_amount_bots == 0) {

        // ARG1 - Amount
        if (arg1 != nullptr && *arg1 != 0) {

            // get amount
            const int temp = std::atoi(arg1);
            kick_amount_bots = temp;
            kick_amount_bots = std::min(kick_amount_bots, 31);
            kick_amount_bots = std::max(kick_amount_bots, 0);
        }
        // ARG 2 - Team
        if (arg2 != nullptr && *arg2 != 0) {

            // get team
            int team = 0;
            const int temp = std::atoi(arg2);
            if (temp == 1)
                team = 1;
            if (temp == 2)
                team = 2;
            kick_bots_team = team;
        }
        if (kick_bots_team < 1)
            snprintf(cMessage, sizeof(cMessage), "REALBOT: Removing randomly %d bots.",
                    kick_amount_bots);

        else {
            if (kick_bots_team == 1)
                snprintf(cMessage, sizeof(cMessage), "REALBOT: Removing %d terrorist bots.",
                        kick_amount_bots);

            else
                snprintf(cMessage, sizeof(cMessage),
                        "REALBOT: Removing %d counter-terrorist bots.",
                        kick_amount_bots);
        }
    } else if (FStrEq(pcmd, "roundlimit")) {

        // Check if Arguments are valid
        if (arg2 != nullptr && *arg2 != 0
            && (arg1 != nullptr && *arg1 != 0)) {
            int s1 = -1, s2 = -1;

            // argument 1
            if (arg1 != nullptr && *arg1 != 0)
                s1 = std::atoi(arg1);

            // argument 2
            if (arg2 != nullptr && *arg2 != 0)
                s2 = std::atoi(arg2);
            Game.SetPlayingRounds(s1, s2);
            snprintf(cMessage, sizeof(cMessage),
                    "REALBOT: Bots play at minimum %d and at maximum %d rounds.\n",
                    Game.GetMinPlayRounds(), Game.GetMaxPlayRounds());
        } else
            snprintf(cMessage, sizeof(cMessage),
                    "REALBOT: No(t) (enough) valid arguments given.");
    } else if (FStrEq(pcmd, "setrandom")) {
        int s1 = -2, s2 = -2;

        // argument 1
        if (arg1 != nullptr && *arg1 != 0)
            s1 = std::atoi(arg1);

        // argument 2
        if (arg2 != nullptr && *arg2 != 0)
            s2 = std::atoi(arg2);

        // When first argument is invalid
        if (s1 < -1) {
            snprintf(cMessage, sizeof(cMessage),
                    "REALBOT: No valid argument(s) given. (minimum random skill=%d, maximum random skill=%d).",
                    Game.iRandomMinSkill, Game.iRandomMaxSkill);
        } else {
            if (s2 < -1)
                s2 = Game.iRandomMaxSkill;
            s1 = std::min(s1, 10);
            s1 = std::max(s1, 0);
            s2 = std::min(s2, 10);
            s2 = std::max(s2, 0);
            s2 = std::max(s2, s1);
            Game.iRandomMinSkill = s1;
            Game.iRandomMaxSkill = s2;
            snprintf(cMessage, sizeof(cMessage),
                    "REALBOT: minimum random skill=%d, maximum random skill=%d.",
                    Game.iRandomMinSkill, Game.iRandomMaxSkill);
        }
    } else if (FStrEq(pcmd, "autoskill")) {
        if (arg1 != nullptr && *arg1 != 0) {
	        const int temp = std::atoi(arg1);
            if (temp == 1)
                autoskill = true;

            else if (temp == 0)
                autoskill = false;
        }
        if (autoskill)
            snprintf(cMessage, sizeof(cMessage), "REALBOT: Auto adjust skill - enabled.");

        else
            snprintf(cMessage, sizeof(cMessage), "REALBOT: Auto adjust skill - disabled.");
    } else if (FStrEq(pcmd, "override_skill")) {
        if (arg1 != nullptr && *arg1 != 0) {
	        const int temp = std::atoi(arg1);
            if (temp == 1)
                Game.iOverrideBotSkill = GAME_YES;

            else if (temp == 0)
                Game.iOverrideBotSkill = GAME_NO;
        }
        if (Game.iOverrideBotSkill == GAME_YES)
            snprintf(cMessage, sizeof(cMessage),
                    "REALBOT: Using personality skill (if present) instead of default bot skill.");

        else
            snprintf(cMessage, sizeof(cMessage),
                    "REALBOT: Using default bot skill at all times.");
    } else if (FStrEq(pcmd, "skill")) {
        if (arg1 != nullptr && *arg1 != 0) {
	        const int temp = std::atoi(arg1);
            if (temp < -1 || temp > 10) {
                snprintf(cMessage, sizeof(cMessage),
                        "REALBOT: Invalid argument given - default skill = %d.",
                        Game.iDefaultBotSkill);
            } else {
                Game.iDefaultBotSkill = temp;
                snprintf(cMessage, sizeof(cMessage), "REALBOT: Default skill = %d",
                        Game.iDefaultBotSkill);
            }
        } else {
            snprintf(cMessage, sizeof(cMessage), "REALBOT: Default skill = %d",
                    Game.iDefaultBotSkill);
        }
    }
        // -----------------------------------------
        // SERVER SUPPORT
        // -----------------------------------------
    else if (FStrEq(pcmd, "server")) {
        // Minimum amount of playing players... bots or not
        if (FStrEq(arg1, "players")) {
            if (arg2 != nullptr && *arg2 != 0) {
                int temp = std::atoi(arg2);      // argument
                temp = std::min(temp, 31);
                temp = std::max(temp, -1);

                min_players = temp;
                f_minplayers_think = gpGlobals->time;
            }

            snprintf(cMessage, sizeof(cMessage), "RBSERVER: Minimum playing forced to %d.",
                    min_players);
        }
            // Broadcast
        else if (FStrEq(arg1, "broadcast")) {
            // Broadcast what?
            if (FStrEq(arg2, "version")) {
                // How do we broadcast version message?
                if (arg3 != nullptr && *arg3 != 0) {
                    int temp = std::atoi(arg3);   // argument
                    temp = std::max(temp, 0);
                    temp = std::min(temp, 1);
                    Game.iVersionBroadcasting = temp;
                    if (Game.iVersionBroadcasting == BROADCAST_ROUND)
                        snprintf(cMessage, sizeof(cMessage),
                                "RBSERVER: Broadcasting RealBot version every round and map change.\n");

                    else
                        snprintf(cMessage, sizeof(cMessage),
                                "RBSERVER: Broadcasting RealBot version every map change only.\n");
                }
            } else if (FStrEq(arg2, "speech")) {
                if (arg2 != nullptr) {
                    if (std::atoi(arg2) == 1)
                        Game.bSpeechBroadcasting = true;
                    else
                        Game.bSpeechBroadcasting = false;
                }

                if (Game.bSpeechBroadcasting)
                    snprintf(cMessage, sizeof(cMessage), "RBSERVER: Broadcasting speech is ON");
                else
                    snprintf(cMessage, sizeof(cMessage), "RBSERVER: Broadcasting speech is OFF");

            } else if (FStrEq(arg2, "kills")) {
                // How do we broadcast kills by bots?
                if (arg3 != nullptr && *arg3 != 0) {
                    int temp = std::atoi(arg3);   // argument
                    temp = std::max(temp, 0);
                    temp = std::min(temp, 2);
                    if (temp == 0)
                        Game.iKillsBroadcasting = BROADCAST_KILLS_FULL;
                    if (temp == 1)
                        Game.iKillsBroadcasting = BROADCAST_KILLS_MIN;
                    if (temp == 2)
                        Game.iKillsBroadcasting = BROADCAST_KILLS_NONE;
                    if (Game.iKillsBroadcasting == BROADCAST_KILLS_FULL)
                        snprintf(cMessage, sizeof(cMessage),
                                "RBSERVER: Broadcasting name and skill of bot who killed human player.\n");

                    else if (Game.iKillsBroadcasting == BROADCAST_KILLS_MIN)
                        snprintf(cMessage, sizeof(cMessage),
                                "RBSERVER: Broadcasting name of bot who killed human player.\n");
                    else
                        snprintf(cMessage, sizeof(cMessage),
                                "RBSERVER: Nothing will be sent to player.\n");
                } else {
                    if (Game.iKillsBroadcasting == BROADCAST_KILLS_FULL)
                        snprintf(cMessage, sizeof(cMessage),
                                "RBSERVER: Broadcasting name and skill of bot who killed human player.\n");
                    else if (Game.iKillsBroadcasting == BROADCAST_KILLS_MIN)
                        snprintf(cMessage, sizeof(cMessage),
                                "RBSERVER: Broadcasting name of bot who killed human player.\n");
                    else
                        snprintf(cMessage, sizeof(cMessage),
                                "RBSERVER: Nothing will be sent to player.\n");
                }
            } else if (FStrEq(arg2, "deaths")) {
                // How do we broadcast deaths by bots.
                if (arg3 != nullptr && *arg3 != 0) {
                    int temp = std::atoi(arg3);   // argument
                    temp = std::max(temp, 0);
                    temp = std::min(temp, 2);
                    if (temp == 0)
                        Game.iDeathsBroadcasting = BROADCAST_DEATHS_FULL;
                    if (temp == 1)
                        Game.iDeathsBroadcasting = BROADCAST_DEATHS_MIN;
                    if (temp == 2)
                        Game.iDeathsBroadcasting = BROADCAST_DEATHS_NONE;
                    if (Game.iDeathsBroadcasting == BROADCAST_DEATHS_FULL)
                        snprintf(cMessage, sizeof(cMessage),
                                "RBSERVER: Broadcasting name and skill of bot who killed human player.\n");
                    else if (Game.iDeathsBroadcasting == BROADCAST_DEATHS_MIN)
                        snprintf(cMessage, sizeof(cMessage),
                                "RBSERVER: Broadcasting name of bot who killed human player.\n");
                    else
                        snprintf(cMessage, sizeof(cMessage),
                                "RBSERVER: Nothing will be sent to player.\n");
                } else {
                    if (Game.iDeathsBroadcasting == BROADCAST_DEATHS_FULL)
                        snprintf(cMessage, sizeof(cMessage),
                                "RBSERVER: Broadcasting name and skill of bot who killed human player.\n");
                    else if (Game.iDeathsBroadcasting == BROADCAST_DEATHS_MIN)
                        snprintf(cMessage, sizeof(cMessage),
                                "RBSERVER: Broadcasting name of bot who killed human player.\n");
                    else
                        snprintf(cMessage, sizeof(cMessage),
                                "RBSERVER: Nothing will be sent to player.\n");
                }
            } else {
                snprintf(cMessage, sizeof(cMessage),
                        "RBSERVER: Broadcast what?\nversion\nspeech\nkills\ndeaths");
            }
        } else
            snprintf(cMessage, sizeof(cMessage),
                    "RBSERVER: Invalid sub-command.\nValid commands are:\nbroadcast (version/kill)\nplayers (keep ## player slots full)");
    }
        // -----------------------------------------
        // NODE EDITOR COMMANDS
        // -----------------------------------------
    else if (FStrEq(pcmd, "nodes")) {
        int iOnNode = -1;

        if (pHostEdict != nullptr) {
            iOnNode = NodeMachine.getClosestNode(pHostEdict->v.origin, 35, pHostEdict);
        }

        if (iOnNode > -1) {
            // Arg 1 is command:
            if (FStrEq(arg1, "addto")) {
                bool bValidArg = false;

                // check for valid argument
                if (arg2 != nullptr && *arg2 != 0) {
	                const int iTo = std::atoi(arg2);    // add connection TO

                    // Add this connection
                    if (iTo > -1) {
	                    const bool bSuccess =
                                NodeMachine.add_neighbour_node(iOnNode, iTo);

                        if (bSuccess)
                            snprintf(cMessage, sizeof(cMessage),
                                    "NODES EDITOR: Added connection from node %d to node %d.",
                                    iOnNode, iTo);
                        else
                            snprintf(cMessage, sizeof(cMessage),
                                    "NODES EDITOR: Connection could not be added, max amount of connections reached or connection already exists.");

                        bValidArg = true;
                    }
                }

                if (bValidArg == false)
                    snprintf(cMessage, sizeof(cMessage),
                            "NODES EDITOR: Give argument to which node this connection is valid!");
            } else if (FStrEq(arg1, "removeto")) {
                // removes connection TO
                bool bValidArg = false;

                // check for valid argument
                if (arg2 != nullptr && *arg2 != 0) {
	                const int iTo = std::atoi(arg2);    // remove connection TO

                    // remove this connection
                    if (iTo > -1) {
	                    const bool bSuccess =
                                NodeMachine.removeConnection(iOnNode, iTo);

                        if (bSuccess)
                            snprintf(cMessage, sizeof(cMessage),
                                    "NODES EDITOR: Removed connection from node %d to node %d.",
                                    iOnNode, iTo);
                        else
                            snprintf(cMessage, sizeof(cMessage),
                                    "NODES EDITOR: Could not remove connection, connection does not exist.");

                        bValidArg = true;
                    }
                }

                if (bValidArg == false)
                    snprintf(cMessage, sizeof(cMessage),
                            "NODES EDITOR: Give argument to which node this connection is valid!");

            } else if (FStrEq(arg1, "removeall")) {
	            const bool bSuccess = NodeMachine.remove_neighbour_nodes(iOnNode);

                if (bSuccess)
                    snprintf(cMessage, sizeof(cMessage),
                            "NODES EDITOR: Removed all connections from node %d.",
                            iOnNode);
            } else if (FStrEq(arg1, "draw")) {
                if (draw_nodes == false)
                    draw_nodes = true;
                else
                    draw_nodes = false;
                if (draw_nodes)
                    snprintf(cMessage, sizeof(cMessage), "NODES EDITOR: Drawing nodes - enabled.");
                else
                    snprintf(cMessage, sizeof(cMessage),
                            "NODES EDITOR: Drawing nodes - disabled.");
            } else if (FStrEq(arg1, "connections")) {
                if (draw_connodes == false)
                    draw_connodes = true;
                else
                    draw_connodes = false;

                if (draw_connodes)
                    snprintf(cMessage, sizeof(cMessage),
                            "NODES EDITOR: Drawing nodes connections - enabled.");
                else
                    snprintf(cMessage, sizeof(cMessage),
                            "NODES EDITOR: Drawing nodes connections - disabled.");
            } else if (FStrEq(arg1, "init")) {
                NodeMachine.init();
                snprintf(cMessage, sizeof(cMessage), "NODES EDITOR: Nodes initialized.");
            } else if (FStrEq(arg1, "save")) {
                NodeMachine.save();
                snprintf(cMessage, sizeof(cMessage), "NODES EDITOR: Nodes saved.");
            } else if (FStrEq(arg1, "load")) {
                NodeMachine.load();
                snprintf(cMessage, sizeof(cMessage), "NODES EDITOR: Nodes loaded.");
            } else {
                snprintf(cMessage, sizeof(cMessage),
                        "NODES EDITOR: Unknown command\n Valid commands are:\naddto,removeto,removeall,draw,connections,init,save,load.");
            }
        } else {

            // commands not needed for a node to be close
            if (FStrEq(arg1, "draw")) {
                if (draw_nodes == false)
                    draw_nodes = true;
                else
                    draw_nodes = false;
                if (draw_nodes)
                    snprintf(cMessage, sizeof(cMessage), "NODES EDITOR: Drawing nodes - enabled.");
                else
                    snprintf(cMessage, sizeof(cMessage),
                            "NODES EDITOR: Drawing nodes - disabled.");
            } else if (FStrEq(arg1, "connections")) {
                if (draw_connodes == false)
                    draw_connodes = true;
                else
                    draw_connodes = false;

                if (draw_connodes)
                    snprintf(cMessage, sizeof(cMessage),
                            "NODES EDITOR: Drawing nodes connections - enabled.");
                else
                    snprintf(cMessage, sizeof(cMessage),
                            "NODES EDITOR: Drawing nodes connections - disabled.");
            } else if (FStrEq(arg1, "init")) {
                NodeMachine.init();
                snprintf(cMessage, sizeof(cMessage), "NODES EDITOR: Nodes initialized.");
            } else if (FStrEq(arg1, "save")) {
                NodeMachine.save();
                snprintf(cMessage, sizeof(cMessage), "NODES EDITOR: Nodes saved.");
            } else if (FStrEq(arg1, "load")) {
                NodeMachine.load();
                snprintf(cMessage, sizeof(cMessage), "NODES EDITOR: Nodes loaded.");
            } else
                snprintf(cMessage, sizeof(cMessage),
                        "NODES EDITOR: Not close enough to a node to edit.");
        }                         // commands not needed for a node to be close

        // 17/07/04
        // Allows adding a connections between two specific nodes even on dedicated server
        // Don't care whether we are close to a node...
        if (FStrEq(arg1, "connect")) {
            // check for valid argument
            if (arg2 != nullptr && *arg2 != 0 && arg3 != nullptr
                && *arg3 != 0) {
	            const int Node1 = std::atoi(arg2);     // add connection TO
	            const int Node2 = std::atoi(arg3);     // add connection TO
                if (Node1 >= 0 && Node2 >= 0
                    && NodeMachine.add_neighbour_node(Node1, Node2))
                    snprintf(cMessage, sizeof(cMessage),
                            "NODES EDITOR: Added connection from node %d to node %d.",
                            Node1, Node2);
                else
                    snprintf(cMessage, sizeof(cMessage),
                            "NODES EDITOR: Connection could not be added, max amount of connections reached or connection already exists.");
            } else
                snprintf(cMessage, sizeof(cMessage),
                        "NODES EDITOR: this command requires TWO numeric arguments!");
        } else if (FStrEq(arg1, "disconnect")) {
            // check for valid argument
            if (arg2 != nullptr && *arg2 != 0 && arg3 != nullptr
                && *arg3 != 0) {
	            const int Node1 = std::atoi(arg2);
	            const int Node2 = std::atoi(arg3);
                if (Node1 >= 0 && Node2 >= 0
                    && NodeMachine.removeConnection(Node1, Node2))
                    snprintf(cMessage, sizeof(cMessage),
                            "NODES EDITOR: Removed connection from node %d to node %d.",
                            Node1, Node2);
                else
                    snprintf(cMessage, sizeof(cMessage),
                            "NODES EDITOR: Connection could not be removed...");
            } else
                snprintf(cMessage, sizeof(cMessage),
                        "NODES EDITOR: this command requires TWO numeric arguments!");
        }
    }
        // -----------------------------------------
        // DEBUG COMMANDS
        // -----------------------------------------
    else if (FStrEq(pcmd, "debug")) {

        // Arg 1 is command:
        if (FStrEq(arg1, "dontshoot")) { // realbot debug dontshoot [1/0]
            // check for valid argument
            if (arg2 != nullptr && *arg2 != 0) {
	            const int temp = std::atoi(arg2);
                if (temp)
                    Game.bDoNotShoot = true;
                else
                    Game.bDoNotShoot = false;
            }

            if (Game.bDoNotShoot)
                snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Bots will not shoot.");
            else
                snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Bots will shoot.");
        }
            // 17/07/04
        else if (FStrEq(arg1, "pistols")) { // realbot debug pistols [1/0]
            if (arg2 != nullptr && *arg2 != 0) {
	            const int temp = std::atoi(arg2);
                if (temp)
                    Game.bPistols = true;
                else
                    Game.bPistols = false;
            }

            if (Game.bPistols)
                snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Bots will only use pistols.");
            else
                snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Bots will use all weapons.");
        } else if (FStrEq(arg1, "goals")) // Print all current goals
        {
            NodeMachine.dump_goals();
            std::strcpy(cMessage, "RBDEBUG: Dumping goals.");
        } else if (FStrEq(arg1, "bots")) // realbot debug bots
            // Print information about all current bots
        {
	        rblog("Dumping information about all bots:\n");
            for (int iBot = 0; iBot < 32 && bots[iBot].bIsUsed == true; iBot++) {
                bots[iBot].Dump();
            }

            NodeMachine.dump_goals();
            std::strcpy(cMessage, "RBDEBUG: Dumping bots' information.");
        } else if (FStrEq(arg1, "print")) { // realbot debug print (toggles, and last argument is bot index)
            if (Game.bDebug > -2) {
                Game.bDebug = -2;
                snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Debug messages off.");
            } else {
                if (arg2 != nullptr && *arg2 != 0) {
	                const int temp = std::atoi(arg2);
                    Game.bDebug = temp;
                    snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Debug messages on for bot [%d].", Game.bDebug);
                } else {
                    Game.bDebug = -1;
                    snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Debug messages on for all bots"/*, Game.bDebug*/);
                }
            }
        } else if (FStrEq(arg1, "verbosity")) { // realbot verbosity
            if (FStrEq(arg2, "low")) {
                snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Message verbosity low.");
                Game.messageVerbosity = 0;
            } else if (FStrEq(arg2, "normal")) {
                snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Message verbosity normal.");
                Game.messageVerbosity = 1;
            } else if (FStrEq(arg2, "trace")) {
                snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Message verbosity trace.");
                Game.messageVerbosity = 2; // extreme verbose
            } else {
                snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Usage: realbot debug verbosity [low][normal][trace]");
            }
        } else if (FStrEq(arg1, "nodes")) { // realbot debug nodes
            if (FStrEq(arg2, "dumpbmp")) { // realbot debug nodes dumpbmp
                snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Dumping Nodes information into bitmap file");
                NodeMachine.Draw();
            } else if (FStrEq(arg2, "draw")) {
                if (draw_nodes == false)
                    draw_nodes = true;
                else
                    draw_nodes = false;
                if (draw_nodes)
                    snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Drawing nodes - enabled.");
                else
                    snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Drawing nodes - disabled.");
            } else if (FStrEq(arg2, "path")) {
                int iFrom = -1, iTo = -1;
                if (arg3 != nullptr && *arg3 != 0)
                    iFrom = std::atoi(arg3);
                if (arg4 != nullptr && *arg4 != 0)
                    iTo = std::atoi(arg4);

                if (iFrom > -1) {
                    if (iTo < 0) {
	                    const tGoal *ptr = NodeMachine.getRandomGoalByType(GOAL_SPAWNT);
                        if (ptr) iTo = ptr->iNode;
                    }

                    snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Creating path from [%d] to [%d].", iFrom, iTo);
                    NodeMachine.createPath(iFrom, iTo, 0, nullptr, PATH_DANGER);
                } else {
                    snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Usage: realbot debug nodes path <fromNode> <toNode>");
                }
            } else if (FStrEq(arg2, "drawpath")) {
                if (arg3 != nullptr && *arg3 != 0)
                    draw_nodepath = std::atoi(arg3);
                if (draw_nodepath > -1) {
                    snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Drawing path of bot id [%d].", draw_nodepath);
                } else
                    snprintf(cMessage, sizeof(cMessage),
                            "RBDEBUG: Drawing path of bot id [%d] - no better or valid argument given.",
                            draw_nodepath);
            } else if (FStrEq(arg2, "connections")) {
                if (draw_connodes == false)
                    draw_connodes = true;
                else
                    draw_connodes = false;

                if (draw_connodes)
                    snprintf(cMessage, sizeof(cMessage),
                            "RBDEBUG: Drawing nodes connections - enabled.");
                else
                    snprintf(cMessage, sizeof(cMessage),
                            "RBDEBUG: Drawing nodes connections - disabled.");
            } else if (FStrEq(arg2, "init")) {
                NodeMachine.init();
                snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Nodes initialized.");
            } else if (FStrEq(arg2, "save")) {
                NodeMachine.save();
                snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Nodes saved.");
            } else if (FStrEq(arg2, "load")) {
                NodeMachine.load();
                snprintf(cMessage, sizeof(cMessage), "RBDEBUG: Nodes loaded.");
            } else
                snprintf(cMessage, sizeof(cMessage), "RBDEBUG: No argument given.");
        } else {
            snprintf(cMessage, sizeof(cMessage),
                    "RBDEBUG: Unknown debug command.\n\nKnown commands are:\ndontshoot, pistols, nodes, print");
        }
    } else {

        // Not a valid command
        snprintf(cMessage, sizeof(cMessage),
                "REALBOT: Unknown command.\nValid commands are:\nhelp, add, remove, skill, max, debug, server");
        bool bValidCommand = false; //Unused variable boolean [APG]RoboCop[CL]
    }

    // Send message
    if (bSendMessage) {
        // Adding return carriage
        std::strcat(cMessage, "\n");

        // Put a carriage return when using dedicated server.
        if (IS_DEDICATED_SERVER()) {
            SERVER_PRINT("\n");
        }

        SERVER_PRINT("=====================================\n");
        SERVER_PRINT(cMessage);
        SERVER_PRINT("=====================================\n");

        // Put an extra carriage return when using dedicated server.
        if (IS_DEDICATED_SERVER()) {
            SERVER_PRINT("\n");
        }

        if (!Game.bInstalledCorrectly) {
            SERVER_PRINT("WARNING: RealBot is NOT installed into the correct directory!\nWARNING: rbn\rbx files will not be created nor personality files!\n");
        }
    }
}

int Spawn_Post(edict_t *pent) {
    // solves the bots unable to see through certain types of glass bug.
    // MAPPERS: NEVER ALLOW A TRANSPARENT ENTITY TO WEAR THE FL_WORLDBRUSH FLAG !!!

    // is this a transparent entity ?
    if (pent->v.rendermode == kRenderTransTexture //||
	// (pent->v.rendermode == kRenderTransAlpha && strcmp("func_illusionary", STRING(pent->v.classname)) == 0)
        ) { // func_illusionary on cs_italy uses this rendermode


        pent->v.flags |= FL_WORLDBRUSH; // set WORLD BRUSH

	// this seems to enforce solidness, and hence the tracehull/line will detect it now.
	// pent->v.solid = SOLID_BSP;
	// pent->v.movetype = MOVETYPE_PUSHSTEP; // this seemed to be the best choice, does not do much
	// pent->v.rendermode == kRenderNormal;
    } else {
    }

    char msg[255];
    snprintf(msg, sizeof(msg), "Found an entity %s - %s - %s with rendermode %d!\n", STRING(pent->v.classname), STRING(pent->v.netname), STRING(pent->v.model), pent->v.rendermode);
    rblog(msg);

    // is this a trigger_multiple ?
    if (std::strcmp(STRING(pent->v.classname), "trigger_multiple") == 0) {
        //pent->v.flags &= ~FL_WORLDBRUSH; // then clear the FL_WORLDBRUSH flag
        pent->v.flags |= FL_WORLDBRUSH;   // make it detectable!
        pent->v.rendermode = kRenderNormal;

        // Perhaps this fixes also the as_oilrig problem where a traceline goes through the button without
        // detecting??
        // this seems to enforce solidness, and hence the tracehull/line will detect it now.
		// pent->v.solid = SOLID_BSP;
		// pent->v.movetype = MOVETYPE_PUSHSTEP; // this seemed to be the best choice, does not do much

		// pent->v.solid = SOLID_TRIGGER;
		// pent->v.solid = SOLID_BSP;
		// SERVER_PRINT("Adjusted a trigger_multiple!!\n");
    }

    RETURN_META_VALUE(MRES_IGNORED, 0);
}


C_DLLEXPORT int
GetEntityAPI2(DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion) {
    gFunctionTable.pfnGameInit = GameDLLInit;
    gFunctionTable.pfnSpawn = Spawn;
    gFunctionTable.pfnClientConnect = ClientConnect;
    gFunctionTable.pfnClientDisconnect = ClientDisconnect;
    gFunctionTable.pfnClientPutInServer = ClientPutInServer;
    gFunctionTable.pfnClientCommand = ClientCommand;
    gFunctionTable.pfnStartFrame = StartFrame;
    std::memcpy(pFunctionTable, &gFunctionTable, sizeof(DLL_FUNCTIONS));
    return true;
}

// Whistler:
C_DLLEXPORT int
GetEntityAPI2_Post(DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion) {
    gFunctionTable_post.pfnSpawn = Spawn_Post;   // need to declare another gFunctionTable_post in the top of the dll.cpp file
    std::memcpy(pFunctionTable, &gFunctionTable_post, sizeof(DLL_FUNCTIONS));
    return true;
}

// Stefan says: You where right, i did not understand it properly. But now i see the little
// difference "gFunctionTable_post" is used here, it makes sense now.

// $Log: dll.cpp,v $
// Revision 1.19  2004/09/07 15:44:34  eric
// - bumped build nr to 3060
// - minor changes in add2 (to add nodes for Bsp2Rbn utilities)
// - if compiled with USE_EVY_ADD, then the add2() function is used when adding
//   nodes based on human players instead of add()
// - else, it now compiles mostly without warnings :-)
//
// Revision 1.18  2004/07/30 15:02:30  eric
// - jumped to version 3057
// - improved readibility (wapen_tabel -> weapons_table) :-P
// - all Josh Borke modifications to the buying stuff:
//     * using a switch() instead of several if
//     * better buying code for shield and primary weapons
//     * new command 'debug pistols 0/1'
//
// Revision 1.17  2004/07/17 21:32:01  eric
// - bumped version to 3055
// - handling of es_ and as_ maps with new goals
// - added two debug commands:
//    realbot debug goals
//    realbot debug bots
// - added two nodes commands (for dedicated servers mainly)
//    realbot nodes connect n1 n2
//    realbot nodes disconnect n1 n2
// - slight modification in goal scoring (only reduced score when two bots of
//   the SAME team select the same goal)
//
// Revision 1.16  2004/07/16 12:41:34  stefan
// - fixed memory leaks mentioned on the forum (thx Josh & Whistler)
//
// Revision 1.15  2004/07/05 19:15:54  eric
// - bumped to build 3052
// - modified the build_nr system to allow for easier build increment
// - a build.cpp file has been added and need to be incremented on each commit
// - some more flexibility for ChatEngine: ignore case and try to cope
//   with accute letters of French and Spanish and .. languages
//
// Revision 1.14  2004/07/02 16:43:35  stefan
// - upped to build 3051
// - changed log() into rblog()
// - removed BOT.CFG code that interpets old RB V1.0 commands
// - neater respons of the RealBot console
// - more help from RealBot console (ie, type realbot server broadcast ... with no arguments it will tell you what you can do with this, etc)
// - removed message "bot personality loaded from file"
// - in overal; some cleaning done, no extra features added
//
// Revision 1.13  2004/07/01 18:09:46  stefan
// - fixed skill 10 bots not causing memory bugger on re-adding (respawning)
// - added extra check for respawning bots so auto-add function cannot crash
// - fixed 2 nitpicks pointed out on the forums
//
// Revision 1.12  2004/06/25 07:39:00  stefan
// - upped to build 3050
// - fixed reaction time (instant reaction time) bug
// - added evy's goals, but they are not used yet
// - fixed some radio responses here and there for swat behaviour.
// - swat leader automaticly assigned again when one dies
// - HINT: you can see any changes made by me, by looking at DD/MM/YY - Stefan (ie, 22/06/04 - Stefan, will let you find all changes i made that day)
//
// Revision 1.11  2004/06/23 08:24:14  stefan
// - upped to build 3049
// - added swat behaviour (team leader assignment, radio response change and leaders command team-mates) - THIS IS EXPERIMENTAL AND DOES NOT ALWAYS WORK AS I WANT IT TO.
// - changed some conditions in nodemachine
// - sorry evy, still not added your new goals ;) will do next time, i promise
//
// Revision 1.10  2004/06/20 10:24:14  stefan
// - fixed another steep/stair thingy
// - changed a bit of the aiming code
//
// Revision 1.9  2004/06/19 21:06:14  stefan
// - changed distance check in nodemachine
// - fixed some 'steep' bug in nodemachine
//
// Revision 1.8  2004/06/17 21:23:23  stefan
// - fixes several connection problems with nodes. Going down from steep + crates (de_dust) PLUS going up/down from very steep slopes on as_oilrig.. 0wnage and thx to PMB and Evy
// - fixed chat bug in CS 1.6, its still CS 1.5 & CS 1.6 compatible though
//
// Revision 1.7  2004/06/13 20:08:21  stefan
// - 'bad score for goals' added
// - bmp dump info (Thanks Evy)
// - added 'realbot server players', so you can keep a server full at NR players at all times
// - slightly adjusted goal selection code
// - wander code disabled
// - lots of debug info introduced, do not use this source for REAL USAGE!
//
// Revision 1.6  2004/06/06 21:16:29  stefan
// - evy fixed compile issues for Linux + making sure that it works (no crashes in linux anymore yey)
// - fixed some 'double calling of functions' , pointed out by Whistler (still needs some checking, just applied very fast fix).
//
// Revision 1.5  2004/05/29 19:05:47  stefan
// - upped to BUILD 3044
// - removed several debug messages on screen
// - changed default 'chatrate (max sentences)' to 3
// - removed copyright notice, which is not valid due GPL license
//
// i know, nothing special :)
//
// Revision 1.4  2004/05/07 13:33:49  stefan
// added more comments, more neat code now
//
// Revision 1.3  2004/04/18 17:39:19  stefan
// - Upped to build 2043
// - REALBOT_PRINT() works properly now
// - Log() works properly now
// - Clearing in dll.cpp of reallog.txt at dll init
// - Logging works now, add REALBOT_PRINT() at every point you want to log something.
//
