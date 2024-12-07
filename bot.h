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

#ifndef BOT_H
#define BOT_H

//#pragma warning( disable : 4005 )
#include "NodeDataTypes.h"

// Weapon price table (filled by the INI PARSER)
typedef struct {
    int iId;                     // the weapon ID value
    int price;                   // Price of weapon
    int priority;                // Priority to buy this weapon
    int iIdIndex;
} weapon_price_table;

// 30/07/04
// Declare the external here
extern weapon_price_table weapons_table[32];

// BOT SPECIFIC / AUDIOABLE / VISUAL PROPERTIES
#define BOT_HEARDISTANCE    1500
#define BOT_HEARFIREDISTANCE 2500

// define constants used to identify the MOD we are playing...
#define VALVE_DLL        1       // Added "Half-Life DeathMatch"
#define CSTRIKE_DLL        3       // Added "Counter-Strike"

// max names in rb_names.txt
#define MAX_BOT_NAMES 100

// Define Join states for other mods
#define JOIN_TEAM        1
#define JOIN_CLASS       2
#define JOIN_NONE        9999

// fix for steam (cs 1.6 bot will crash when FL_FAKECLIENT;
// this is fixed later again in STEAM, but leave it in anway).
#define FL_THIRDPARTYBOT (1 << 27)

// "Own predefined" item types (attached with buying code)
#define CS_WEAPON_ARMOR_LIGHT        199
#define CS_WEAPON_ARMOR_HEAVY        299

// damage tolerance
#define CSTRIKE_MIN_DAMAGE    7
#define CSTRIKE_MAX_DAMAGE    14

// The type of a weapon, not mod related. You can use these prefixed names
// for your functions
#define SNIPER        10      // Sniper gun
#define RIFLE        20      // Rifle
#define HANDGUN        30      // Handgun
#define ARMOR        40      // Armor
#define PRIMARY        50      // Its a primary weapon (for CS Mods)
#define SECONDARY    60      // Its a secondary weapon (for CS Mods)
#define GRENADE        70      // Grenade (aka, HE, Flashbang, C4, etc)
#define KNIFE        80      // Knife / Crowbar etc
#define SHIELD        90      // Shield (CS 1.6)
#define NONE        99      // No type

// Strafe
//#define STRAFE_LEFT           0
//#define STRAFE_RIGHT  1

// zooming
#define ZOOM_NONE          0
#define ZOOM_ONCE          1
#define ZOOM_TWICE          2

// instant damage (from cbase.h)
#define DMG_CRUSH            (1 << 0)        // crushed by falling or moving object
#define DMG_BURN            (1 << 3)        // heat burned
#define DMG_FREEZE            (1 << 4)        // frozen
#define DMG_FALL            (1 << 5)        // fell too far
#define DMG_SHOCK            (1 << 8)        // electric shock
#define DMG_DROWN            (1 << 14)       // Drowning
#define DMG_NERVEGAS        (1 << 16)       // nerve toxins, very bad
#define DMG_RADIATION        (1 << 18)       // radiation exposure
#define DMG_DROWNRECOVER    (1 << 19)       // drowning recovery
#define DMG_ACID            (1 << 20)       // toxic chemicals or acid burns
#define DMG_SLOWBURN        (1 << 21)       // in an oven
#define DMG_SLOWFREEZE        (1 << 22)       // in a subzero freezer

// define some function prototypes...
void FakeClientCommand(edict_t *pBot, char *arg1, char *arg2, char *arg3);

#define LADDER_UNKNOWN  0
#define LADDER_UP       1
#define LADDER_DOWN     2

#define WANDER_LEFT  1
#define WANDER_RIGHT 2

#define BOT_PITCH_SPEED 30
#define BOT_YAW_SPEED 30

#define RESPAWN_NONE             0      // Added by Stefan
#define RESPAWN_IDLE             1
#define RESPAWN_NEED_TO_RESPAWN  2
#define RESPAWN_IS_RESPAWNING    3

// game start messages for CS...
#define MSG_CS_IDLE         1
#define MSG_CS_TEAM_SELECT  2
#define MSG_CS_CT_SELECT    3
#define MSG_CS_T_SELECT     4

#define BOT_SKIN_LEN 32
#define BOT_NAME_LEN 32

#define MAX_BOT_WHINE 100

#define REMEMBER_ENEMY_TIME   20        // remember for 20 seconds

typedef struct {
    int iId;                     // weapon ID
    int iClip;                   // amount of ammo in the clip
    int iAmmo1;                  // amount of ammo in primary reserve
    int iAmmo2;                  // amount of ammo in secondary reserve
}
        bot_current_weapon_t;

/*
 Bot is a class now 
 */
class cBot {

private:
    int iGoalNode;               // Goal Node #
    int goalIndex;               // is a goal node is set, it might be related to a goal, if so it is set here
    int pathIndex;               // Index on theWhich node we want to move to, derived from Path

    // Hostages edict
    edict_t *pBotHostage;        // the Hostage we will go after!
    edict_t *hostage1;           // we do not
    edict_t *hostage2;           // use
    edict_t *hostage3;           // any array
    edict_t *hostage4;           // here

    edict_t *pEnemyEdict;        // Enemy edict

    float fMoveToNodeTime;       // How long we should take to move to next node.
    int nodeTimeIncreasedAmount; // how many times did we increase the time to reach node?

    float freezeTime;         // How long it should 'freeze'.

    float distanceMoved;        // length of v_diff vector (distance bot moved) (timed calculation by distanceMovedTimer)
    float distanceMovedTimer;

    float f_defuse;              // Timer to press the use key for defusing

public:
    // Constructor
    cBot();

    // Hostage related
    bool isUsingHostage(edict_t *pHostage);
    void forgetHostage(edict_t *pHostage);
    void rememberHostageIsFollowingMe(edict_t *pHostage);
    void rememberWhichHostageToRescue(edict_t *pHostage);
    void clearHostageToRescueTarget();
    int getAmountOfHostagesBeingRescued() const;
    edict_t * findHostageToRescue();         // finds a hostage to rescue
    edict_t * getHostageToRescue() const;          // returns hostage state pointer
    bool isEscortingHostages();            // Does the bot has used any hostages yet?
    void checkOfHostagesStillFollowMe();

    bool hasTimeToMoveToNode() const;
    bool isDefusing() const;

    // ------------------------
    // TIMERS
    // ------------------------
    float fUpdateTime;
    float fLastRunPlayerMoveTime;

    float fCheckHostageStatusTimer;
    float fButtonTime;
    float fChatTime;
    float fMemoryTime;
    float fDoRadio;              // how much time we have before we can issue another radio message
    float fNotStuckTime;         // last time we got stuck
    float f_goback_time;         // time to go back (-movespeed)
    float f_allow_keypress;      // Timer to avoid 'too quick' using/defusing/etc

    float f_use_timer;           // time for use button (hostage rescue, preventing to fast using)
    float fBlindedTime;          // How long it is blinded ( when > globals->time)

    float f_max_speed;           // Max speed is the max speed it can get when holding that particular weapon
    float f_move_speed;          // The bots move speed
    float f_strafe_speed;        // Strafe speed (> 0 to the right < 0 to the left)
    float f_hold_duck;           // Hold duck
    float f_sec_weapon;          // secondary weapon time interval
    float f_prim_weapon;         // primary weapon time interval (for burst fire)
    float f_gren_time;           // time to hold grenade

    float f_shoot_wait_time;     // wait time, for bursting aswell (how long we may shoot)

    float f_may_jump_time;       // Timer when just off a ladder on top, we may not do jump tricks.

    float f_c4_time;             // C4 time (plant/defuse)
    float f_camp_time;           // Camp timer
    float f_wait_time;           // Wait timer
    float f_cover_time;          // How long to take cover

    float f_strafe_time;         // How long till we pick a new strafe direction (fighting only)

    float f_hear_time;           // Decision timer for hearing
    float f_walk_time;           // Timer to force bot to walk

    float f_follow_time;         // How long to follow our friend
    float f_update_weapon_time;  // Aargh

    float f_shoot_time;          // Shoot time.

    float f_light_time;          // light timer

    float fKickTime;
    float fCreateTime;

    // Personality related
    float fpXOffset;
    float fpYOffset;
    float fpZOffset;
    float fpMinReactTime;
    float fpMaxReactTime;

    // ------------------------
    // INTEGERS
    // ------------------------
    int iTeam;                   // team
    int iBotIndex;               // Index of the bot in the Bots array, this bot can use this to find its associated Path

    int bot_class;               // class
    int bot_health;              // Amount of health
    int bot_armor;               // Amount of armor
    int bot_weapons;             // bit map of weapons the bot is carrying
    int bot_money;               // for Counter-Strike

    int prev_health;
    int play_rounds;             // How many rounds a bot will play in round-based games.
    int played_rounds;           // how many rounds it has played already
    int bot_use_special;         // Use silencer etc.
    int buy_flashbang;
    int zoomed;
    int iPrimaryWeapon;
    int iSecondaryWeapon;
    int i_camp_style;            // 0 - see yourself, 1 stand, 2 duck
    int respawn_state;
    int bot_skill;
    int iPathFlags;              // With what thought to create a path?
    int iDiedNode;               // On which node did we die?

    // Personality related
    int ipHostage;               // Hostage rescue frequency
    int ipBombspot;              // Bomb spot frequency
    int ipRandom;                // Random goal (wandering) frequency
    int ipTurnSpeed;             // Turn speed
    int ipReplyToRadio;          // Reply to radio frequency
    int ipCreateRadio;           // Creating new radio command frequency
    int ipHelpTeammate;          // Help teammate frequency
    int ipWalkWithKnife;         // Walk with knife frequency
    int ipCampRate;              // Camp frequency
    int ipChatRate;              // Chat frequency
    int ipFearRate;              // How much 'fear' has a bot
    int ipHearRate;              // How good can a bot hear?

    // Buy-Personality related
    int ipFavoPriWeapon;
    int ipFavoSecWeapon;
    int ipBuyFlashBang;
    int ipBuyGrenade;
    int ipBuySmokeGren;
    int ipBuyDefuseKit;
    int ipSaveForWeapon;         // Frequency for NOT buying a weapon to buy the favo weapon
    int ipBuyArmour;
    int ipDroppedBomb;           // how glad do we got to the dropped bomb?

    // Remember stuck stuff
    int iJumpTries;
    int iDuckTries;
    int iDuckJumpTries;  // Experimental DuckJump added for this new node [APG]RoboCop[CL]

    // ------------------------
    // BOOLEANS
    // ------------------------
    bool buy_primary;
    bool buy_secondary;
    bool buy_ammo_primary;
    bool buy_ammo_secondary;
    bool buy_armor;
    bool buy_grenade;
    bool buy_smokegrenade;       //31.08.04 Frashman added support for Smoke Grenade
    bool bIsUsed;                // Bot is 'used'/'playing' (if set to true, the bot is active)
    bool bInitialize;
    bool buy_defusekit;
    bool bWalkKnife;             // likes to walk around with knife
    bool vip;

    bool bFirstOutOfSight;

    // ------------------------
    // HUD
    // ------------------------
    bool bHUD_C4_plantable;      // HUD says it is ok?  -- BERKED

    // chat
    char chChatSentence[160];

    // ------------------------
    //
    // ------------------------

    edict_t *pEdict;             // Edict_t of the bot

    char name[BOT_NAME_LEN + 1];
    char skin[BOT_SKIN_LEN + 1];

    bool hasJoinedTeam;
    int start_action;

    int iPreviousGoalNode;        // Previous goal
    int iCloseNode;                // The closest node to this bot.
    float fWanderTime;           // Wandering around time

    // Nav
    Vector vHead;                // Vector where bot should look at
    Vector vBody;                // Vector where bot should move to
    Vector vEar;                 // Vector where the bot hears

    // Console
    int console_nr;              // when != 0 it means it performs a 'console command' action (state machine)
    float f_console_timer;
    char arg1[25];
    char arg2[25];
    char arg3[25];

    Vector prevOrigin;
    Vector lastSeenEnemyVector;              // Vector where we last saw our enemy (when not 0,0,0)

    float f_jump_time;
    float f_bot_see_enemy_time;
    float f_bot_find_enemy_time;

    edict_t *pButtonEdict;       // button edict

    edict_t *killer_edict;       // Killer edict

    Vector vecMoveAngles;        // Vector we move to


    bot_current_weapon_t current_weapon; // one current weapon for each bot
    int m_rgAmmo[MAX_AMMO_SLOTS];        // total ammo amounts (1 array for each bot)

    // ------------------------ FUNCTIONS
    void UpdateStatus();         // Update status
    void Think();                // Think functions
    void Combat();               // Combat
    void Memory();               // Memory
    void Walk();                 // Walk
    void InteractWithPlayers();  // Interaction
    void Act();                  // Do as been told
    bool Defuse();               // Defuse (ACT) sub-function
    void FightEnemy();           // Fight
    void JoinTeam();

    void CheckGear();

    void ThinkAboutGoals();      // Think about goals
    bool TakeCover() const;

    bool CarryWeapon(int iType) const;

    int CarryWeaponType() const;

    void setHeadAiming(const Vector& vTarget);    // Aim at vector
    void AimAtEnemy();

    void PickBestWeapon();

    void FireWeapon();

    static int ReturnTurnedAngle(float speed, float current, float ideal);

    int FindEnemy();

    float ReactionTime(int iSkill);      // Reaction time based upon skill
    void FindCover();

    bool canSeeVector(const Vector& vDest) const;

    bool canSeeEntity(edict_t *pEntity) const;

    void InteractWithFriends();

    void PrepareChat(char sentence[128]);        // prepares bot to alk in act()

    void clearHostages();

    // Set methods
    int determineCurrentNode();
    int determineCurrentNodeWithTwoAttempts();
    int determineCurrentNode(float range) const;

    // Get methods
    float getDistanceTo(int nodeIndex);
    float getDistanceTo(const Vector& vDest) const;

    // "is" Methods (booleans, statements, etc)
    bool isDead() const;
    bool isHeadingForGoalNode() const;
    bool isEnemyAlive() const;         // If enemy set, is it alive? (else returns false)
    bool isOnLadder() const;             // Bot on ladder or not?
    bool isSeeingEnemy();          // If enemy set, can we see it? (takes blinded by flashbang into account)
    bool isCounterTerrorist() const;
    bool isTerrorist() const;

    // "is" methods, related to weapons
    bool wantsToBuyStuff() const;
    bool isUsingConsole() const;
    bool isOwningWeapon(int weaponId) const;
    bool isHoldingWeapon(int weaponId) const;
    bool ownsFavoritePrimaryWeapon() const;
    bool ownsFavoriteSecondaryWeapon() const;
    bool hasFavoritePrimaryWeaponPreference() const;
    bool hasFavoriteSecondaryWeaponPreference() const;
    bool hasPrimaryWeaponEquiped() const;
    bool hasSecondaryWeaponEquiped() const;
    bool hasPrimaryWeapon(int weaponId) const;
    bool hasSecondaryWeapon(int weaponId) const;
    bool canAfford(int price) const; //price muddled with weaponId? [APG]RoboCop[CL]

    // -------------------
    // 20/06/04 - CS 1.6 shield functions
    bool hasShield() const;
    bool hasShieldDrawn() const;

    bool isHoldingGrenadeOrFlashbang() const;
    bool isBlindedByFlashbang() const;

    // void / action methods
    void pickWeapon(int weaponId);
    void performBuyActions(int weaponIdToBuy);
    void performBuyWeapon(const char *menuItem, const char *subMenuItem);

    // -------------------
    void CheckAround();          // Check around body
    // -------------------
	
    bool hasEnemy() const;
    bool hasEnemy(const edict_t * pEdict) const;
    edict_t * getEnemyEdict() const;

    bool hasGoal() const;
    bool hasGoalIndex() const;
    tGoal *getGoalData() const;
    bool shouldBeWandering();
    bool hasBomb() const;

    // ------------
    bool isWalkingPath() const;

    void beginWalkingPath();
    void nextPathIndex();       // increases index so we know which node to walk on path
    void prevPathIndex();       // decreases index so we know which node to walk on path
    void stopMoving();
    void forgetGoal();
    void forgetPath();
    void forgetEnemy();
    void setGoalNode(tGoal *goal);
    void setGoalNode(int nodeIndex);
    void setGoalNode(int nodeIndex, int iGoalIndex);

    int getCurrentNode();                      // the current (closest) node we are at
    int getCurrentPathNodeToHeadFor() const;         // get Node from path
    int getPreviousPathNodeToHeadFor() const;        // get previous Node from path
    int getNextPathNode() const;                     // get next Node from path
    int getPathIndex() const;
    int getPreviousPathIndex() const;

    int getGoalNode() const;

    void setMoveSpeed(float value);
    void setStrafeSpeed(float value, float time);
    void strafeLeft(float time);
    void strafeRight(float time);
    void startWandering(float time);

    // -------------------
    void SpawnInit();            // When bot spawns again (added/revived)
    void NewRound();             // When bot enters new round

    // Debug -------
    void rprint(const char *Function, const char *msg); // low
    void rprint_normal(const char *Function, const char *msg); // normal
    void rprint_trace(const char *Function, const char *msg); // trace

    void rprint(const char *msg);
    void rprint_normal(const char *msg);
    void rprint_trace(const char *msg);
    void Dump();

    bool hasHostageToRescue() const;

    bool canSeeHostageToRescue() const;

    void checkIfHostagesAreRescued();

    bool isOnSameTeamAs(const cBot *pBot) const;

    bool shouldBeAbleToMove() const;

    edict_t *getEntityBetweenMeAndCurrentPathNodeToHeadFor() const;

    float getDistanceToNextNode() const;

    void setBodyToNode(int nodeIndex);

    void lookAtNode(int nodeIndex);

    void setTimeToMoveToNode(float timeInSeconds);
    void increaseTimeToMoveToNode(float timeInSeconds);

    float getMoveToNodeTimeRemaining() const;

    bool shouldActWithC4() const;

    int keyPressed(int key) const;

    bool shouldCamp() const;
    bool shouldWait() const;
    void setTimeToWait(float timeInSeconds);

    bool shouldBeAbleToInteractWithButton() const;

    bool hasButtonToInteractWith() const;
    bool hasCurrentNode() const;

    bool createPath(int destinationNode, int flags);
    bool createPath(int destinationNode);

    void doJump(const Vector &vector);
    void doJump();
    bool isJumping();

    // Experimental DuckJump added for the NodeMachine [APG]RoboCop[CL]
    void doDuckJump(); 
    bool isDuckJumping();

    void doDuck();
    bool isDucking();
    bool isWalking();

    bool isFreezeTime() const;

    void rememberEnemyFound();
};

// new UTIL.CPP functions...
edict_t *UTIL_FindEntityInSphere(edict_t *pentStart,
                                 const Vector &vecCenter, float flRadius);

edict_t *UTIL_FindEntityByString(edict_t *pentStart,
                                 const char *szKeyword,
                                 const char *szValue);

edict_t *UTIL_FindEntityByClassname(edict_t *pentStart,
                                    const char *szName);

edict_t *UTIL_FindEntityByTargetname(edict_t *pentStart,
                                     const char *szName);

void ClientPrint(edict_t *pEdict, int msg_dest, const char *msg_name);

void UTIL_SayText(const char *pText, edict_t *pEdict);

void UTIL_HostSay(edict_t *pEntity, int teamonly, char *message);

int UTIL_GetTeam(edict_t *pEntity);

int UTIL_GetClass(edict_t *pEntity);

bool IsAlive(edict_t *pEdict);

bool FInViewCone(Vector *pOrigin, edict_t *pEdict);

bool FVisible(const Vector &vecOrigin, edict_t *pEdict);

Vector Center(edict_t *pEdict);

Vector GetGunPosition(edict_t *pEdict);

Vector VecBModelOrigin(edict_t *pEdict);

void UTIL_ShowMenu(edict_t *pEdict, int slots, int displaytime, bool needmore, char *pText);

void UTIL_SelectItem(edict_t *pEdict, char *item_name);

void UTIL_BuildFileName(char *filename, char *arg1, char *arg2);

void UTIL_BuildFileNameRB(const char *subdir, char *filename);

unsigned short fixed_unsigned16(float value, float scale); //redundant declaration? [APG]RoboCop[CL]

short fixed_signed16(float value, float scale); //redundant declaration? [APG]RoboCop[CL]

void HUD_DrawString(int r, int g, int b, char *msg, edict_t *edict);

void UTIL_FixAngles(Vector *Angles);

void UTIL_SayTextBot(const char *pText, cBot *pBot);

// combat.h

// Bot specific
int UTIL_GetBotIndex(edict_t *pEdict);

cBot *UTIL_GetBotPointer(edict_t *pEdict);


// dll.cpp
bool BotRadioAction();

#endif                          // BOT_H
