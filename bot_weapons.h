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

#ifndef BOT_WEAPONS_H
#define BOT_WEAPONS_H

// weapon ID values for Valve's Team Fortress Classic & 1.5
#define TF_WEAPON_UNKNOWN1       1
#define TF_WEAPON_UNKNOWN2       2
#define TF_WEAPON_MEDIKIT        3
#define TF_WEAPON_SPANNER        4
#define TF_WEAPON_AXE            5
#define TF_WEAPON_SNIPERRIFLE    6
#define TF_WEAPON_AUTORIFLE      7
#define TF_WEAPON_SHOTGUN        8
#define TF_WEAPON_SUPERSHOTGUN   9
#define TF_WEAPON_NAILGUN       10
#define TF_WEAPON_SUPERNAILGUN  11
#define TF_WEAPON_GL            12
#define TF_WEAPON_FLAMETHROWER  13
#define TF_WEAPON_RPG           14
#define TF_WEAPON_IC            15
#define TF_WEAPON_UNKNOWN16     16
#define TF_WEAPON_AC            17
#define TF_WEAPON_UNKNOWN18     18
#define TF_WEAPON_UNKNOWN19     19
#define TF_WEAPON_TRANQ         20
#define TF_WEAPON_RAILGUN       21
#define TF_WEAPON_PL            22
#define TF_WEAPON_KNIFE         23

// weapon ID values for Counter-Strike
#define CS_WEAPON_P228           1
#define CS_WEAPON_SHIELD         2
#define CS_WEAPON_SCOUT          3
#define CS_WEAPON_HEGRENADE      4
#define CS_WEAPON_XM1014         5
#define CS_WEAPON_C4             6
#define CS_WEAPON_MAC10          7
#define CS_WEAPON_AUG            8
#define CS_WEAPON_SMOKEGRENADE   9
#define CS_WEAPON_ELITE         10
#define CS_WEAPON_FIVESEVEN     11
#define CS_WEAPON_UMP45         12
#define CS_WEAPON_SG550         13
#define CS_WEAPON_GALIL         14      // CS 1.6
#define CS_WEAPON_FAMAS         15      // CS 1.6
#define CS_WEAPON_USP           16
#define CS_WEAPON_GLOCK18       17
#define CS_WEAPON_AWP           18
#define CS_WEAPON_MP5NAVY       19
#define CS_WEAPON_M249          20
#define CS_WEAPON_M3            21
#define CS_WEAPON_M4A1          22
#define CS_WEAPON_TMP           23
#define CS_WEAPON_G3SG1         24
#define CS_WEAPON_FLASHBANG     25
#define CS_WEAPON_DEAGLE        26
#define CS_WEAPON_SG552         27
#define CS_WEAPON_AK47          28
#define CS_WEAPON_KNIFE         29
#define CS_WEAPON_P90           30

//30.8.04 redefined by frashman
#define CS_DEFUSEKIT		    98      // old value was 99, same as SHIELD -> Bug??

// NOT CONFIRMED
//#define CS_WEAPON_SHIELD        99      // Not used for detecting, only for
// bot.dll

// Woah, i rule! :D, figured out all Earth Special Forces Weapon ID's..
#define ESF_WEAPON_MELEE		1
#define ESF_KIBLAST				2
#define ESF_GALLITGUN			3
#define ESF_KAMEHAMEHA			4
#define ESF_DESTRUCTODISC		5
#define ESF_SOLARFLARE			6
#define ESF_EYELASER			7
#define ESF_FRIEZADISC			8
#define ESF_SPECIALBEAMCANNON	9
#define ESF_SPIRITBOMB			10
#define ESF_BIGBANG				11
#define ESF_FINGERLASER			12
#define ESF_FINALFLASH			13
#define ESF_MASENKO				14
#define ESF_DEATHBALL			15
#define ESF_BURNINGATTACK		16
#define ESF_SENSU				17

typedef struct {
   char szClassname[64];
   int iAmmo1;                  // ammo index for primary ammo
   int iAmmo1Max;               // max primary ammo
   int iAmmo2;                  // ammo index for secondary ammo
   int iAmmo2Max;               // max secondary ammo
   int iSlot;                   // HUD slot (0 based)
   int iPosition;               // slot position
   int iId;                     // weapon ID
   int iFlags;                  // flags???
}
bot_weapon_t;

// 30/07/04
extern bot_weapon_t weapon_defs[MAX_WEAPONS];   // array of weapon definitions
#endif                          // BOT_WEAPONS_H
