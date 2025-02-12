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

/**
  * INI PARSER
  * COPYRIGHTED BY STEFAN HENDRIKS (C) 
  **/

// Sections
#ifndef INIPARSER_H
#define INIPARSER_H

#define INI_NONE       (-1)
#define INI_SKILL       0       // Bot skill
#define INI_WEAPON      1       // Bot weapon preference
#define INI_GAME        2       // Bot general game behaviour
#define INI_RADIO       3       // Bot radio behaviour
#define INI_TEAM        4       // Bot team behaviour
#define INI_PERSON      5       // Bot person itself

#define INI_AREA		10
#define INI_BLOCK		11
#define INI_DEATHS		12
#define INI_WELCOME		13

// 'Weapon Sections' are the same as WEAPON ID in Counter-Strike.
// NOTE: For weapon_buy_table.iId!

// 'Words'
#define WORD_NONE       (-1)
#define WORD_WALK        0
#define WORD_RUN         1
#define WORD_SHOOT       2
#define WORD_WAIT        3
#define WORD_RADIO       4

// BOTPERSONALITY.INI words
#define WORD_PRIWEAPON      31
#define WORD_SECWEAPON      32
#define WORD_SAVEFORWEAP    33
#define WORD_GRENADE        34
#define WORD_FLASHBANG      35
#define WORD_SMOKEGREN      36
#define WORD_DEFUSEKIT      37
#define WORD_ARMOUR         54

#define WORD_XOFFSET        38
#define WORD_YOFFSET        39
#define WORD_ZOFFSET        40
#define WORD_BOTSKILL       41
#define WORD_MAXREACTTIME   42
#define WORD_MINREACTTIME   43
#define WORD_TURNSPEED      44

#define WORD_HOSTAGERATE    45
#define WORD_BOMBSPOTRATE   46
#define WORD_RANDOMRATE     47

#define WORD_REPLYRADIO     48
#define WORD_CREATERADIO    49

#define WORD_HELPTEAM       50

#define WORD_CAMPRATE       51
#define WORD_CHATRATE       52
#define WORD_WALKKNIFE      53

#define WORD_FEARRATE       55
#define WORD_HEARRATE       56

#define WORD_DROPPEDBOMB    57

// AREA SHIT
#define WORD_AREAX			60
#define WORD_AREAY			61
#define WORD_AREAZ			62

// CHAT
#define WORD_SENTENCE		67
#define WORD_WORD			68

// BUYTABLE.INI Words (arguments per weapon)
#define WORD_PRIORITY    5
#define WORD_PRICE       6
#define WORD_MAXAMMO1    88
#define WORD_MAXAMMO2    89
#define WORD_ISLOT       90
#define WORD_IPOSITION   91
#define WORD_IFLAGS      92
#define WORD_INDEX1      93
#define WORD_INDEX2      94


void INI_PARSE_BOTS(char cBotName[33], cBot * pBot);
void INI_PARSE_BUYTABLE();
void INI_PARSE_IAD();
void INI_PARSE_CHATFILE();

#endif // INIPARSER_H