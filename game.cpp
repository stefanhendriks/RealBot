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
#include "game.h"
#include "NodeMachine.h"
#include "ChatEngine.h"
#include "IniParser.h"
#include "bot_func.h"

extern cNodeMachine NodeMachine;
extern cChatEngine ChatEngine;
extern cGame Game;
extern cBot bots[MAX_BOTS];

// GAME: Init
void cGame::Init() {

    // Default Bot Skill
    iDefaultBotSkill = -1;       // random
    iOverrideBotSkill = GAME_YES;

    // Randomized skill boundries
    iRandomMinSkill = 0;
    iRandomMaxSkill = 10;

    // Internet simulation
    iMinPlayRounds = 5;
    iMaxPlayRounds = 25;

    // New round stuff
    bNewRound = false;

    // Broadcasting
    iVersionBroadcasting = BROADCAST_ROUND;
    iKillsBroadcasting = BROADCAST_KILLS_FULL;
    iDeathsBroadcasting = BROADCAST_DEATHS_FULL;
    bSpeechBroadcasting = false; // @ default we do NOT broadcast this shit every round to the listen server

    // Installed into correct directory
    bInstalledCorrectly = true;

    // Round time
    fRoundTime = gpGlobals->time;

    // Dropped C4 location
    vDroppedC4 = Vector(9999, 9999, 9999);

    // May we walk with knife (when bots want to), default = yes (3600 seconds)
    fWalkWithKnife = 3600.0f;

    // Chat related
    iMaxSentences = 1;           // max sentences produced by chatengine per second (1=default)
    iProducedSentences = 0;      // currently produced sentences

    // DEBUG RELATED
    bDoNotShoot = false;         // ... guess
    bDebug = -2;               // ... prints debug messages (-2 is off, -1 == for everyone > -1 specific bot index)
    messageVerbosity = 0;         // normal verbosity
    bEngineDebug = false;        // ... prints engine debug messages (for figuring out engine interceptions)
    bPistols = false;            // pistols only mode

    // Speech sentences (from POD and a *few* own made)
    std::strcpy(cSpeechSentences[0], "hello user,communication is acquired");
    std::strcpy(cSpeechSentences[1], "your presence is acknowledged");
    std::strcpy(cSpeechSentences[2], "high man, your in command now");
    std::strcpy(cSpeechSentences[3], "blast your hostile for good");
    std::strcpy(cSpeechSentences[4], "high man, kill some idiot here");
    std::strcpy(cSpeechSentences[5], "is there a doctor in the area");
    std::strcpy(cSpeechSentences[6], "warning, experimental materials detected");
    std::strcpy(cSpeechSentences[7], "high amigo, shoot some but");
    std::strcpy(cSpeechSentences[8], "attention, hours of work software, detected");
    std::strcpy(cSpeechSentences[9], "time for some bad ass explosion");
    std::strcpy(cSpeechSentences[10], "bad ass son of a breach device activated");
    std::strcpy(cSpeechSentences[11], "high, do not question this great service");
    std::strcpy(cSpeechSentences[12], "engine is operative, hello and goodbye");
    std::strcpy(cSpeechSentences[13], "high amigo, your administration has been great last day");
    std::strcpy(cSpeechSentences[14], "attention, expect experimental armed hostile presence");
    std::strcpy(cSpeechSentences[15], "warning,medical attention required");

    InitNewRound();
}                               // Init()

void cGame::InitNewRound() {
    // Map goal flags
    bBombPlanted = false;
    vPlantedC4 = Vector(9999, 9999, 9999);
    bHostageRescueMap = false;

    // update map goal flags timer
    fUpdateGoalTimer = gpGlobals->time;

    resetRoundTime();
    DetermineMapGoal();

    // initialize bots for new round
    for (auto& bot : bots)
    {
	    if (bot.bIsUsed) {
            bot.NewRound();
        }
    }

    iProducedSentences = RANDOM_LONG(0, Game.iMaxSentences);
    ChatEngine.fThinkTimer = gpGlobals->time + RANDOM_FLOAT(0.0f, 0.5f);
}

/**
 * If the c4 is dropped (not planted), then return true.
 * @return
 */
bool cGame::isC4Dropped() const
{
    return vDroppedC4 != Vector(9999, 9999, 9999);
}

/**
 * Is the C4 - when planted - discovered?
 * @return
 */
bool cGame::isPlantedC4Discovered() const
{
    return vPlantedC4 != Vector(9999, 9999, 9999);
}

// Returns random sentence for speech
char *cGame::RandomSentence() {
    return cSpeechSentences[RANDOM_LONG(0, 15)];
}

void cGame::DetermineMapGoal() const
{
    rblog("DetermineMapGoal called\n");
    edict_t *pEnt = nullptr;

    int hostagesFound = 0;
    while ((pEnt = UTIL_FindEntityByClassname(pEnt, "hostage_entity")) != nullptr) {
        hostagesFound++;
    }

    char msg[255] = {};
    snprintf(msg, sizeof(msg), "DetermineMapGoal: There are %d hostages found to rescue\n", hostagesFound);
    rblog(msg);
    Game.bHostageRescueMap = hostagesFound > 0;

    int rescueZonesFound = 0;
    // GOAL #3 - Hostage rescue zone
    while ((pEnt = UTIL_FindEntityByClassname(pEnt, "func_hostage_rescue")) != nullptr) {
        rescueZonesFound++;
    }

    // rescue zone can also be an entity of info_hostage_rescue
    while ((pEnt = UTIL_FindEntityByClassname(pEnt, "info_hostage_rescue")) != nullptr) {
        rescueZonesFound++;
    }

    std::memset(msg, 0, sizeof(msg));
    snprintf(msg, sizeof(msg), "DetermineMapGoal: There are %d rescue zones found\n", rescueZonesFound);
    rblog(msg);
    Game.bHostageRescueZoneFound = rescueZonesFound > 0;


    int bombSpots = 0;
    // GOAL #4 - Bombspot zone
    // Bomb spot
    while ((pEnt = UTIL_FindEntityByClassname(pEnt, "func_bomb_target")) != nullptr) {
        bombSpots++;
    }

    while ((pEnt = UTIL_FindEntityByClassname(pEnt, "info_bomb_target")) != nullptr) {
        bombSpots++;
    }
    std::memset(msg, 0, sizeof(msg));
    snprintf(msg, sizeof(msg), "DetermineMapGoal: There are %d bomb spots in this level\n", bombSpots);
    Game.bBombPlantMap = bombSpots > 0;
    rblog(msg);
}

void cGame::resetRoundTime() {
    // start counting down, but include freezetime to match up with the count-down
    SetRoundTime(gpGlobals->time + CVAR_GET_FLOAT("mp_freezetime"));
}

// GAME: Set round time
void cGame::SetRoundTime(float fTime) {
    fRoundTime = fTime;
}

// GAME: Get round time
// NOTE: This is the time when a round has just started!
float cGame::getRoundStartedTime() const
{
    return fRoundTime;
}

float cGame::getRoundTimeElapsed() const
{
    if (getRoundStartedTime() > -1) {
        return gpGlobals->time - getRoundStartedTime();
    }
    return -1;
}

// GAME: Set new round flag
void cGame::SetNewRound(bool bState) {
    bNewRound = bState;
}

// GAME: Get new round status
bool cGame::NewRound() const
{
    return bNewRound;
}


// GAME: Set min and max playing rounds
void cGame::SetPlayingRounds(int iMin, int iMax) {
    if (iMin > -1)
        iMinPlayRounds = iMin;
    if (iMax > -1)
        iMaxPlayRounds = iMax;

    // Boundries
    if (iMinPlayRounds < 1)
        iMinPlayRounds = 1;
    if (iMaxPlayRounds < 2)
        iMaxPlayRounds = 2;

    // Make sure MAX never is lower then MIN
    if (iMinPlayRounds > iMaxPlayRounds)
        iMinPlayRounds = iMaxPlayRounds - 1;

}                               // SetPlayingRounds()

// GAME: GET Max playing rounds
int cGame::GetMaxPlayRounds() const
{
    return iMaxPlayRounds;
}

// GAME: GET Min playing rounds
int cGame::GetMinPlayRounds() const
{
    return iMinPlayRounds;
}

// GAME: Load bot.cfg
void cGame::LoadCFG() {
    // loads bot.cfg?

}                               // LoadCFG()

// GAME: Load names file
// NOTE: This function is just a copy/paste stuff from Botmans template, nothing more, nothing less
// TODO: Rewrite this, can be done much cleaner.
void cGame::LoadNames() {
	char filename[256];
    UTIL_BuildFileNameRB("rb_names.txt", filename);
    FILE* bot_name_fp = std::fopen(filename, "r");
    if (bot_name_fp != nullptr) {
	    char name_buffer[80];
	    while (iAmountNames < MAX_BOT_NAMES &&
               std::fgets(name_buffer, 80, bot_name_fp) != nullptr) {
            int length = static_cast<int>(std::strlen(name_buffer));
            if (length > 0 && name_buffer[length - 1] == '\n') {
                name_buffer[length - 1] = 0;        // remove '\n'
                length--;
            }
            int str_index = 0;
            while (str_index < length) {
                if (name_buffer[str_index] < ' '
                    || name_buffer[str_index] > '~'
                    || name_buffer[str_index] == '"')
                    for (int index = str_index; index < length; index++)
                        name_buffer[index] = name_buffer[index + 1];
                str_index++;
            }

            if (name_buffer[0] != 0) {
                std::strncpy(cBotNames[iAmountNames], name_buffer, BOT_NAME_LEN);
                iAmountNames++;
            }
        }
        std::fclose(bot_name_fp);
    }
}                               // LoadNames()

// Any names available?
bool cGame::NamesAvailable() const
{
    if (iAmountNames > 0)
        return true;

    return false;
}                               // NamesAvailable()

// Picks a random name
// rewritten on april 10th 2004
void cGame::SelectName(char *name) const
{
	int iNameIndex = 0;              // zero based (RANDOM_LONG (0, iAmountNames-1))

    bool iNameUsed[MAX_BOT_NAMES];
    for (bool& i : iNameUsed)
    {
	    i = false;
    }

    // check make sure this name isn't used
    bool bUsed = true;
    while (bUsed) {
        iNameIndex = RANDOM_LONG(0, iAmountNames - 1);    // pick random one
        const int iLimit = iNameIndex;  // remember this.

        // make sure it is not checked yet
        while (iNameUsed[iNameIndex]) {
            // check again
            if (iNameUsed[iNameIndex] == false)
                break;

            // add up
            iNameIndex++;

            // make sure that it does not check out of range
            if (iNameIndex == iAmountNames)
                iNameIndex = 0;     // go to 0 (will be set to 0 next 'name')

            // when we are back to where we came from, get the fuck outta here
            if (iNameIndex == iLimit) {
                std::strcpy(name, "RealBot");
                return;
            }
        }

        // so far we did not find evidence that this name has been used already
        bUsed = false;

        // check if this name is used
        for (int iIndex = 1; iIndex <= gpGlobals->maxClients; iIndex++) {
	        const edict_t* pPlayer = INDEXENT(iIndex);
            if (pPlayer && !pPlayer->free) {
                if (std::strcmp(cBotNames[iNameIndex], STRING(pPlayer->v.netname))
                    == 0) {
                    // atten tion, this namehas been used.
                    bUsed = true;
                    break;
                }
            }
        }

        if (bUsed)
            iNameUsed[iNameIndex] = true;  // set on true

    }

    // copy name into the name_buffer
    std::strcpy(name, cBotNames[iNameIndex]);
}                               // SelectName()

// GAME: Load BUYTABLE.INI file
void cGame::LoadBuyTable() {
    INI_PARSE_BUYTABLE();
}

// GAME: Update global vars (called by StartFrame)
void cGame::UpdateGameStatus() {
	edict_t* pEnt = nullptr;

    // ------------------
    // Update: Dropped C4
    // ------------------
    // Its not dropped unless stated otherwise.
    vDroppedC4 = Vector(9999, 9999, 9999);

    // Find the dropped bomb
    while ((pEnt = UTIL_FindEntityByClassname(pEnt, "weaponbox")) != nullptr) {
        // when DROPPED C4
        if ((FStrEq(STRING(pEnt->v.model), "models/w_backpack.mdl"))) {
            vDroppedC4 = pEnt->v.origin;   // this is the origin.
            break;
        }
    }

    // ------------------
    // Update: Is the bomb planted?
    // ------------------
    // Same as dropped c4, its NOT, unless stated otherwise.
    pEnt = nullptr;
    bool bPlanted = bBombPlanted;

    while ((pEnt = UTIL_FindEntityByClassname(pEnt, "grenade")) != nullptr) {
        if (UTIL_GetGrenadeType(pEnt) == 4) {
            bPlanted = true;         // Found planted bomb!
            break;
        }
    }

    // Discovered the bomb is planted and it was a different value than before (not planted).
    // all counter-terrorists should know this, and they should head for the bomb
    if (bPlanted && // found a planted bomb
        bPlanted != bBombPlanted // and previously we didn't know that
            ) {
	    for (int i = 1; i <= gpGlobals->maxClients; i++) {
            edict_t *pPlayer = INDEXENT(i);
            cBot *bot = UTIL_GetBotPointer(pPlayer);

            // valid bot
            if (bot) {
                if (bot->isCounterTerrorist()) {
                    bot->forgetPath();
                    bot->rprint("Setting goal for bombspot");
                    tGoal *bombSpotGoal = NodeMachine.getRandomGoalByType(GOAL_BOMBSPOT);
                    if (bombSpotGoal) {
                        bot->setGoalNode(bombSpotGoal); // picks a random bomb spot
                    }
                }             // ct
            }                // bot
        }                   // through all clients

        // Now update bBombPlanted
        bBombPlanted = bPlanted;
    } // planted, and not planted before

    // Every 3 seconds update the goals
    if (gpGlobals->time > fUpdateGoalTimer + 3) {
	//  rblog("cGame::UpdateGameStatus - updateGoals\n");
        NodeMachine.updateGoals();
        fUpdateGoalTimer = gpGlobals->time;
    }

} // UpdateGameStatus()

/**
 * Add bot -> ARG1(team), ARG2(skill), ARG3(model), ARG4(name)
 * pPlayer == player who wanted to create bot (if not null)
 * @param pPlayer
 * @param teamArg
 * @param skillArg
 * @param modelArg
 * @param nameArg
 * @return
 */
int cGame::createBot(edict_t *pPlayer, const char *teamArg, const char *skillArg, const char *modelArg, const char *nameArg) const
{

    // NAME
    char botName[BOT_NAME_LEN + 1] = {};
    // if name given, use that
    if (nameArg != nullptr && *nameArg != 0) {
        std::strncpy(botName, nameArg, BOT_NAME_LEN - 1);
        botName[BOT_NAME_LEN] = 0; // make sure botName is null terminated
    } else { // else pick random one or fallback to default "RealBot"
        if (NamesAvailable()) {
            SelectName(botName);
        } else {
            std::strcpy(botName, "RealBot");
        }
    }

    // length of name
    int lengthOfBotName = static_cast<int>(std::strlen(botName));

    for (int i = 0; i < lengthOfBotName; i++) {
        if ((botName[i] <= ' ') || (botName[i] > '~') || (botName[i] == '"')) {
            // move chars to the left (and null)
            for (int j = i; j < lengthOfBotName; j++) {
                botName[j] = botName[j + 1];
            }
            lengthOfBotName--;
        }
    }

    int botSkill = -2; // -2, not valid

    // Skill argument provided
    if ((skillArg != nullptr) && (*skillArg != 0)) {
        botSkill = std::atoi(skillArg);       // set to given skill
    }

    // when not valid (-2), it has default skill
    if (botSkill < -1 || botSkill > 10) {
        botSkill = iDefaultBotSkill;
    }

    // When skill is -1, random, we set it by boundries given
    if (botSkill == -1) {
        botSkill = RANDOM_LONG(iRandomMinSkill, iRandomMaxSkill);
    }

    // CREATE fake client!
    edict_t *pBotEdict = (*g_engfuncs.pfnCreateFakeClient)(botName);
    if (FNullEnt(pBotEdict)) {
        REALBOT_PRINT(nullptr, "cGame::CreateBot", "Cannot create bot, server is full");
        return GAME_MSG_FAIL_SERVERFULL;  // failed
    }

    char ptr[128];            // allocate space for message from ClientConnect

    int freeBotIndex = 0;
    while (freeBotIndex < MAX_BOTS && bots[freeBotIndex].bIsUsed)
        freeBotIndex++;

    if (freeBotIndex == MAX_BOTS) { // failure
        return GAME_MSG_FAILURE;
    }
	
    // create the player entity by calling MOD's player function
    // (from LINK_ENTITY_TO_CLASS for player object)

    // FIX: Free data for bot, so we can fill in new
    if (pBotEdict->pvPrivateData != nullptr)
        FREE_PRIVATE(pBotEdict);

    pBotEdict->pvPrivateData = nullptr;
    pBotEdict->v.frags = 0;

    // END OF FIX: --- score resetted
    CALL_GAME_ENTITY(PLID, "player", VARS(pBotEdict));
    char* infobuffer = (*g_engfuncs.pfnGetInfoKeyBuffer)(pBotEdict);
    const int clientIndex = ENTINDEX(pBotEdict);

    (*g_engfuncs.pfnSetClientKeyValue)(clientIndex, infobuffer, "model", "");
    (*g_engfuncs.pfnSetClientKeyValue)(clientIndex, infobuffer, "rate", "3500.000000");
    (*g_engfuncs.pfnSetClientKeyValue)(clientIndex, infobuffer, "cl_updaterate", "20");
    (*g_engfuncs.pfnSetClientKeyValue)(clientIndex, infobuffer, "tracker", "0");
    (*g_engfuncs.pfnSetClientKeyValue)(clientIndex, infobuffer, "cl_dlmax", "128");

    if (RANDOM_LONG(0, 100) < 50) {
        (*g_engfuncs.pfnSetClientKeyValue)(clientIndex, infobuffer, "lefthand", "1");
    } else {
        (*g_engfuncs.pfnSetClientKeyValue)(clientIndex, infobuffer, "lefthand", "0");
    }

    (*g_engfuncs.pfnSetClientKeyValue)(clientIndex, infobuffer, "friends", "0");
    (*g_engfuncs.pfnSetClientKeyValue)(clientIndex, infobuffer, "dm", "0");
    (*g_engfuncs.pfnSetClientKeyValue)(clientIndex, infobuffer, "ah", "1");
    (*g_engfuncs.pfnSetClientKeyValue)(clientIndex, infobuffer, "_vgui_menus", "0");

    MDLL_ClientConnect(pBotEdict, botName, "127.0.0.1", ptr);

    // Pieter van Dijk - use instead of DispatchSpawn() - Hip Hip Hurray!
    MDLL_ClientPutInServer(pBotEdict);
    pBotEdict->v.flags |= FL_THIRDPARTYBOT;

    // initialize all the variables for this bot...

    // Retrieve Pointer
    cBot *pBot = &bots[freeBotIndex];

    // TODO: Stefan 05/09/2019 - init function? (re-use, so much duplication here)
    // Set variables
    pBot->iBotIndex = freeBotIndex;
    pBot->bIsUsed = true;
    pBot->respawn_state = RESPAWN_IDLE;
    pBot->fCreateTime = gpGlobals->time;
    pBot->fKickTime = 0.0f;
    pBot->name[0] = 0;        // name not set by server yet
    pBot->bot_money = 0;

    // clear
    constexpr char c_skin[BOT_SKIN_LEN + 1] = {};
    std::strcpy(pBot->skin, c_skin);

    pBot->pEdict = pBotEdict;
    pBot->hasJoinedTeam = false;   // hasn't joined game yet

    // CS Message IDLE..
    pBot->start_action = MSG_CS_IDLE;
    pBot->SpawnInit();
    pBot->bInitialize = false;        // don't need to initialize yet
    pBotEdict->v.idealpitch = pBotEdict->v.v_angle.x;
    pBotEdict->v.ideal_yaw = pBotEdict->v.v_angle.y;
    pBotEdict->v.pitch_speed = BOT_PITCH_SPEED;
    pBotEdict->v.yaw_speed = BOT_YAW_SPEED;
    pBot->bot_skill = botSkill;

    // Personality related
    pBot->ipHostage = 0;
    pBot->ipBombspot = 0;
    pBot->ipRandom = 0;
    pBot->ipTurnSpeed = 20;
    pBot->ipReplyToRadio = 0;
    pBot->ipCreateRadio = 0;
    pBot->ipHelpTeammate = 0;
    pBot->ipWalkWithKnife = 0;
    pBot->ipDroppedBomb = 0;
    pBot->ipCampRate = 0;
    pBot->ipChatRate = 0;
    pBot->ipFearRate = 0;
    pBot->ipHearRate = 0;

    pBot->played_rounds = 0;

    // Buy-personality related
    pBot->ipFavoPriWeapon = -1;
    pBot->ipFavoSecWeapon = -1;
    pBot->ipBuyFlashBang = 0;
    pBot->ipBuyGrenade = 0;
    pBot->ipBuySmokeGren = 0;
    pBot->ipBuyDefuseKit = 0;
    pBot->ipSaveForWeapon = 0;
    pBot->ipBuyArmour = 0;

    // here we set team
    if ((teamArg != nullptr) && (teamArg[0] != 0)) {
        pBot->iTeam = std::atoi(teamArg);

        // and class
        if ((modelArg != nullptr) && (modelArg[0] != 0)) {
            pBot->bot_class = std::atoi(modelArg);
        }
    }

    // Parsing name into bot identity
    INI_PARSE_BOTS(botName, pBot);

    pBot->clearHostages();
    pBot->clearHostageToRescueTarget();

    // return success
    return GAME_MSG_SUCCESS;
}                               // CreateBot()

// Debug message (without BOT)
void REALBOT_PRINT(const char *Function, const char *msg) {
    REALBOT_PRINT(nullptr, Function, msg);
}

// Debug message
void REALBOT_PRINT(cBot *pBot, const char *Function, const char *msg) {
    char cMessage[512];
    char team[9];
    char name[MAX_NAME_LENGTH];
    char mapName[32];
    int botIndex = -1;

    std::memset(team, 0, sizeof(team));       // clear
    std::memset(name, 0, sizeof(name));       // clear
    std::memset(mapName, 0, sizeof(mapName));       // clear

    std::strcpy(team, "NONE");
    std::strcpy(name, "FUNCTION");

    if (gpGlobals->mapname) {
        std::strcpy(mapName, STRING(gpGlobals->mapname));
    } else {
        std::strcpy(mapName, "NA");
    }

    if (pBot) {
        botIndex = pBot->iBotIndex;
        std::memset(name, 0, sizeof(name));    // clear

        //TODO: To use std:string for this [APG]RoboCop[CL]
        std::strncpy(name, pBot->name, sizeof(name));
        name[sizeof(name) - 1] = '\0';

        if (pBot->isCounterTerrorist()) {
            std::strcpy(team, "COUNTER");
        } else if (pBot->isTerrorist()) {
            std::strcpy(team, "TERROR");
        }
    }

    float roundElapsedTimeInSeconds = Game.getRoundTimeElapsed();
    if (roundElapsedTimeInSeconds < 0) roundElapsedTimeInSeconds = 1; // sensible default to prevent division by 0
    // Counter-Strike uses a timer that counts down, so we need to reverse it as well
    float roundTimeInMinutes = CVAR_GET_FLOAT("mp_roundtime");
    if (roundTimeInMinutes < 0) roundTimeInMinutes = 4; // sensible default

    const float roundTimeInSeconds = roundTimeInMinutes * 60;
    const float roundTimeRemaining = roundTimeInSeconds - roundElapsedTimeInSeconds;
    const int minutesLeft = static_cast<int>(roundTimeRemaining) / 60;
    const int secondsLeft = static_cast<int>(roundTimeRemaining) % 60;

    snprintf(cMessage, sizeof(cMessage), "[rb] [%s] [%0d:%02d] - [%s|%d] [%s] [%s] : %s\n", mapName, minutesLeft, secondsLeft, name, botIndex, team, Function, msg);

    // print in console only when on debug print
    if (Game.bDebug > -2) {
        if (Game.bDebug == -1) {
            rblog(cMessage);
        } else if (Game.bDebug == botIndex && botIndex > -1) {
            rblog(cMessage);
        }
    }
}  // REALBOT_PRINT()

// $Log: game.cpp,v $
// Revision 1.18  2004/09/07 15:44:34  eric
// - bumped build nr to 3060
// - minor changes in add2 (to add nodes for Bsp2Rbn utilities)
// - if compiled with USE_EVY_ADD, then the add2() function is used when adding
//   nodes based on human players instead of add()
// - else, it now compiles mostly without warnings :-)
//
// Revision 1.17  2004/07/30 15:02:30  eric
// - jumped to version 3057
// - improved readibility (wapen_tabel -> weapons_table) :-P
// - all Josh Borke modifications to the buying stuff:
//     * using a switch() instead of several if
//     * better buying code for shield and primary weapons
//     * new command 'debug pistols 0/1'
//
// Revision 1.16  2004/07/02 16:43:35  stefan
// - upped to build 3051
// - changed log() into rblog()
// - removed BOT.CFG code that interpets old RB V1.0 commands
// - neater respons of the RealBot console
// - more help from RealBot console (ie, type realbot server broadcast ... with no arguments it will tell you what you can do with this, etc)
// - removed message "bot personality loaded from file"
// - in overal; some cleaning done, no extra features added
//
// Revision 1.15  2004/07/01 18:09:46  stefan
// - fixed skill 10 bots not causing memory bugger on re-adding (respawning)
// - added extra check for respawning bots so auto-add function cannot crash
// - fixed 2 nitpicks pointed out on the forums
//
// Revision 1.14  2004/06/23 08:24:14  stefan
// - upped to build 3049
// - added swat behaviour (team leader assignment, radio response change and leaders command team-mates) - THIS IS EXPERIMENTAL AND DOES NOT ALWAYS WORK AS I WANT IT TO.
// - changed some conditions in nodemachine
// - sorry evy, still not added your new goals ;) will do next time, i promise
//
// Revision 1.13  2004/06/18 12:20:07  stefan
// - fixed another bug in chatting, CS 1.5 won't crash now
// - added some limit to bots searching for goals
//
// Revision 1.12  2004/06/17 21:23:23  stefan
// - fixes several connection problems with nodes. Going down from steep + crates (de_dust) PLUS going up/down from very steep slopes on as_oilrig.. 0wnage and thx to PMB and Evy
// - fixed chat bug in CS 1.6, its still CS 1.5 & CS 1.6 compatible though
//
// Revision 1.11  2004/06/13 20:08:21  stefan
// - 'bad score for goals' added
// - bmp dump info (Thanks Evy)
// - added 'realbot server players', so you can keep a server full at NR players at all times
// - slightly adjusted goal selection code
// - wander code disabled
// - lots of debug info introduced, do not use this source for REAL USAGE!
//
// Revision 1.10  2004/05/29 19:05:47  stefan
// - upped to BUILD 3044
// - removed several debug messages on screen
// - changed default 'chatrate (max sentences)' to 3
// - removed copyright notice, which is not valid due GPL license
//
// i know, nothing special :)
//
// Revision 1.9  2004/04/26 16:36:48  stefan
// changed:
// #elseif into #else to keep MSVC satisfied
//
// added:
// comment lines in game.cpp so you know what vars are what.
//
// Revision 1.8  2004/04/18 19:33:57  stefan
// - Restructured code a bit
//
// Revision 1.7  2004/04/18 17:39:19  stefan
// - Upped to build 2043
// - REALBOT_PRINT() works properly now
// - Log() works properly now
// - Clearing in dll.cpp of reallog.txt at dll init
// - Logging works now, add REALBOT_PRINT() at every point you want to log something.
//
