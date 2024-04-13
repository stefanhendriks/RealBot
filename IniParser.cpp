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
	* COPYRIGHTED BY STEFAN HENDRIKS (C) 2003-2004
	**/

#include <cstring>
#include <extdll.h>
#include <dllapi.h>
#include <meta_api.h>
#include <entity_state.h>

#include "bot.h"
#include "game.h"
#include "bot_weapons.h"
#include "bot_func.h"
#include "IniParser.h"
#include "NodeMachine.h"
#include "ChatEngine.h"

#include <cctype>

extern int mod_id;
extern edict_t* pHostEdict;
extern cNodeMachine NodeMachine;

extern int m_spriteTexture;
extern cGame Game;
extern cChatEngine ChatEngine;

// Reads out INPUT , will check for a [ at [0] and then checks till ], it will fill section[]
// with the chars in between. So : [MAP] -> section = 'MAP'. Use function INI_SectionType(..)
// to get the correct ID for that.
void INI_Section(char input[80], char section[30]) {
	int end_pos = -1;

	// clear out entire string
	for (int i = 0; i < 30; i++)
		section[i] = '\0';

	// check if the first character is a '['
	if (input[0] == '[') {
		int pos = 1;                  // Begin at character 1

		while (pos < 79) {
			if (input[pos] == ']') {
				end_pos = pos - 1;
				break;
			}
			pos++;
		}

		if (end_pos > 1 && end_pos < 29) {
			for (int wc = 0; wc < end_pos; wc++)
				section[wc] = input[wc + 1];

			section[end_pos] = '\0';       // terminate string
		}
	}
}

// Reads out INPUT and will check for an '=' Everything at the left of the
// '=' IS a word and will be put in 'word[]'. Use function INI_WordType(char word[25]) to get
// the correct ID tag.
void INI_Word(char input[80], char word[25]) {
	int pos = 0;
	int word_pos = -1;

	// clear out entire string
	for (int i = 0; i < 25; i++)
		word[i] = '\0';

	while (pos < 79) {
		if (input[pos] == '=') {
			word_pos = pos;
			break;
		}
		pos++;
	}

	if (word_pos > -1 && word_pos < 23) {
		for (int wc = 0; wc < word_pos; wc++)
			word[wc] = input[wc];

		word[word_pos] = '\0';    // terminate string
	}
}

// Reads out word[], does a string compare and returns type id
int INI_WordType(char word[25], int section) {
	if (word[0] != '\0') {
		if (std::strcmp(word, "Word") == 0)
			return WORD_WORD;
		if (std::strcmp(word, "Sentence") == 0)
			return WORD_SENTENCE;

		if (std::strcmp(word, "X") == 0)
			return WORD_AREAX;
		if (std::strcmp(word, "Y") == 0)
			return WORD_AREAY;
		if (std::strcmp(word, "Z") == 0)
			return WORD_AREAZ;

		// ------ personality stuff ------
		if (std::strcmp(word, "PrimaryWeapon") == 0)
			return WORD_PRIWEAPON;

		if (std::strcmp(word, "SecondaryWeapon") == 0)
			return WORD_SECWEAPON;

		if (std::strcmp(word, "SaveForWeapon") == 0)
			return WORD_SAVEFORWEAP;

		if (std::strcmp(word, "Grenade") == 0)
			return WORD_GRENADE;

		if (std::strcmp(word, "FlashBang") == 0)
			return WORD_FLASHBANG;

		if (std::strcmp(word, "SmokeGrenade") == 0)
			return WORD_SMOKEGREN;

		if (std::strcmp(word, "DefuseKit") == 0)
			return WORD_DEFUSEKIT;

		if (std::strcmp(word, "Armour") == 0)
			return WORD_ARMOUR;

		// ---- skill

		if (std::strcmp(word, "XOffset") == 0)
			return WORD_XOFFSET;

		if (std::strcmp(word, "YOffset") == 0)
			return WORD_YOFFSET;

		if (std::strcmp(word, "ZOffset") == 0)
			return WORD_ZOFFSET;

		if (std::strcmp(word, "BotSkill") == 0)
			return WORD_BOTSKILL;

		if (std::strcmp(word, "MaxReactionTime") == 0)
			return WORD_MAXREACTTIME;

		if (std::strcmp(word, "MinReactionTime") == 0)
			return WORD_MINREACTTIME;

		if (std::strcmp(word, "Turnspeed") == 0)
			return WORD_TURNSPEED;

		// ---- Game
		if (std::strcmp(word, "Hostage") == 0)
			return WORD_HOSTAGERATE;

		if (std::strcmp(word, "BompSpot") == 0)
			return WORD_BOMBSPOTRATE;

		if (std::strcmp(word, "Random") == 0)
			return WORD_RANDOMRATE;
		if (std::strcmp(word, "DroppedBomb") == 0)
			return WORD_DROPPEDBOMB;

		// ---- Radio
		if (std::strcmp(word, "Reply") == 0)
			return WORD_REPLYRADIO;

		if (std::strcmp(word, "Create") == 0)
			return WORD_CREATERADIO;

		// ---- Team
		if (std::strcmp(word, "HelpTeammate") == 0)
			return WORD_HELPTEAM;

		// ---- person
		if (std::strcmp(word, "WalkWithKnife") == 0)
			return WORD_WALKKNIFE;
		if (std::strcmp(word, "FearRate") == 0)
			return WORD_FEARRATE;
		if (std::strcmp(word, "HearRate") == 0)
			return WORD_HEARRATE;
		if (std::strcmp(word, "ChatRate") == 0)
			return WORD_CHATRATE;

		if (std::strcmp(word, "CampRate") == 0)
			return WORD_CAMPRATE;

		// ------ buy table stuff -------
		if (std::strcmp(word, "Price") == 0)
			return WORD_PRICE;

		if (std::strcmp(word, "Priority") == 0)
			return WORD_PRIORITY;

		if (std::strcmp(word, "Ammo1Index") == 0)
			return WORD_INDEX1;

		if (std::strcmp(word, "Ammo2Index") == 0)
			return WORD_INDEX2;

		if (std::strcmp(word, "Ammo1Max") == 0)
			return WORD_MAXAMMO1;

		if (std::strcmp(word, "Ammo2Max") == 0)
			return WORD_MAXAMMO2;
	}

	return WORD_NONE;
}

// Reads out an entire sentence and returns it
void INI_Sentence(FILE* f, char result[80]) {
	char ch;
	int pos = 0;

	// clear out entire string
	for (int i = 0; i < 80; i++)
		result[i] = '\0';

	while (feof(f) == 0 && (ch = fgetc(f)) != '\n') {
		result[pos] = ch;
		pos++;

		// do not allow strings greater then 80 characters. This check prevents a crash for
		// users who do exceed the limit.
		if (pos > 79)
			break;

		// remove dedicated server garbage
		//putchar (ch);
	}
}

// Reads out section[], does a string compare and returns type id
int INI_SectionType(char section[30], int last) {
	if (std::strcmp(section, "BLOCK") == 0)
		return INI_BLOCK;

	if (std::strcmp(section, "DEATH") == 0)
		return INI_DEATHS;

	if (std::strcmp(section, "WELCOME") == 0)
		return INI_WELCOME;

	if (std::strcmp(section, "AREA") == 0)
		return INI_AREA;

	if (std::strcmp(section, "WEAPON") == 0)
		return INI_WEAPON;

	if (std::strcmp(section, "SKILL") == 0)
		return INI_SKILL;

	if (std::strcmp(section, "GAME") == 0)
		return INI_GAME;

	if (std::strcmp(section, "RADIO") == 0)
		return INI_RADIO;

	if (std::strcmp(section, "TEAM") == 0)
		return INI_TEAM;

	if (std::strcmp(section, "PERSON") == 0)
		return INI_PERSON;

	// When nothing found; we assume its just a new ID tag for some unit or structure
	// Therefor we return the last known SECTION ID so we can assign the proper WORD ID's
	return last;
}

// Reads out section[], does a string compare and returns type id
// BUYTABLE.INI SPECIFIC!
int INI_SectionType_BUYTABLE(char section[30], int last) {
	if (std::strcmp(section, "P228") == 0)
		return CS_WEAPON_P228;
	if (std::strcmp(section, "HEGRENADE") == 0)
		return CS_WEAPON_HEGRENADE;
	if (std::strcmp(section, "AK47") == 0)
		return CS_WEAPON_AK47;
	if (std::strcmp(section, "DEAGLE") == 0)
		return CS_WEAPON_DEAGLE;
	if (std::strcmp(section, "MAC10") == 0)
		return CS_WEAPON_MAC10;
	if (std::strcmp(section, "AUG") == 0)
		return CS_WEAPON_AUG;
	if (std::strcmp(section, "SG552") == 0)
		return CS_WEAPON_SG552;
	if (std::strcmp(section, "ELITE") == 0)
		return CS_WEAPON_ELITE;
	if (std::strcmp(section, "FIVESEVEN") == 0)
		return CS_WEAPON_FIVESEVEN;
	if (std::strcmp(section, "UMP45") == 0)
		return CS_WEAPON_UMP45;
	if (std::strcmp(section, "SG550") == 0)
		return CS_WEAPON_SG550;
	if (std::strcmp(section, "USP") == 0)
		return CS_WEAPON_USP;
	if (std::strcmp(section, "GLOCK18") == 0)
		return CS_WEAPON_GLOCK18;
	if (std::strcmp(section, "AWP") == 0)
		return CS_WEAPON_AWP;
	if (std::strcmp(section, "MP5") == 0)
		return CS_WEAPON_MP5NAVY;
	if (std::strcmp(section, "M249") == 0)
		return CS_WEAPON_M249;
	if (std::strcmp(section, "M3") == 0)
		return CS_WEAPON_M3;
	if (std::strcmp(section, "M4A1") == 0)
		return CS_WEAPON_M4A1;
	if (std::strcmp(section, "TMP") == 0)
		return CS_WEAPON_TMP;
	if (std::strcmp(section, "G3SG1") == 0)
		return CS_WEAPON_G3SG1;
	if (std::strcmp(section, "SCOUT") == 0)
		return CS_WEAPON_SCOUT;
	if (std::strcmp(section, "FLASHBANG") == 0)
		return CS_WEAPON_FLASHBANG;
	if (std::strcmp(section, "C4") == 0)
		return CS_WEAPON_C4;
	if (std::strcmp(section, "SMOKEGRENADE") == 0)
		return CS_WEAPON_SMOKEGRENADE;
	if (std::strcmp(section, "XM1014") == 0)
		return CS_WEAPON_XM1014;
	if (std::strcmp(section, "KNIFE") == 0)
		return CS_WEAPON_KNIFE;
	if (std::strcmp(section, "P90") == 0)
		return CS_WEAPON_P90;

	// Counter-Strike 1.6
	if (std::strcmp(section, "FAMAS") == 0)
		return CS_WEAPON_FAMAS;
	if (std::strcmp(section, "GALIL") == 0)
		return CS_WEAPON_GALIL;

	if (std::strcmp(section, "SHIELD") == 0)
		return CS_WEAPON_SHIELD;

	// When nothing found; we assume its just a new ID tag for some unit or structure
	// Therefor we return the last known SECTION ID so we can assign the proper WORD ID's
	return last;
}

// Reads out 'result' and will return the value after the '='. Returns integer.
// For CHAR returns see "INI_WordValueCHAR(char result[80]);
int INI_WordValueINT(char result[80]) {
	int pos = 0;
	int is_pos = -1;

	while (pos < 79) {
		if (result[pos] == '=') {
			is_pos = pos;
			break;
		}
		pos++;
	}

	if (is_pos > -1) {
		// Whenever the IS (=) position is known, we make a number out of 'IS_POS' till the next empty
		// space.
		int end_pos = -1;

		while (pos < 79) {
			if (result[pos] == '\0') {
				end_pos = pos;
				break;
			}
			pos++;
		}

		// End position found!
		if (end_pos > -1) {
			// We know the END position. We will use that piece of string to read out a number.
			char number[10];

			// clear out entire string
			for (char& i : number)
				i = '\0';

			// Copy the part to 'number', Make sure we won't get outside the array of the character.
			int cp = is_pos + 1;
			int c = 0;
			while (cp < end_pos) {
				number[c] = result[cp];
				c++;
				cp++;
				if (c > 9)
					break;
			}

			/*
			   char aa[80];
			   sprintf(aa, "Original %s, atoi %d\n", number, atoi(number));
			   DebugOut(aa);
			 */

			return std::atoi(number);
		}
		// nothing here, so we return NULL at the end
	}

	return 0;                    // No value, return 0 (was NULL)
}

// Reads out 'result' and will return the value after the '='. Returns integer.
// For CHAR returns see "INI_WordValueCHAR(char result[80]);
float INI_WordValueFLOAT(char result[80]) {
	int pos = 0;
	int is_pos = -1;

	while (pos < 79) {
		if (result[pos] == '=') {
			is_pos = pos;
			break;
		}
		pos++;
	}

	if (is_pos > -1) {
		// Whenever the IS (=) position is known, we make a number out of 'IS_POS' till the next empty
		// space.
		int end_pos = -1;

		while (pos < 79) {
			if (result[pos] == '\0') {
				end_pos = pos;
				break;
			}
			pos++;
		}

		// End position found!
		if (end_pos > -1) {
			// We know the END position. We will use that piece of string to read out a number.
			char number[10];

			// clear out entire string
			for (char& i : number)
				i = '\0';

			// Copy the part to 'number', Make sure we won't get outside the array of the character.
			int cp = is_pos + 1;
			int c = 0;
			while (cp < end_pos) {
				number[c] = result[cp];
				c++;
				cp++;
				if (c > 9)
					break;
			}
			return static_cast<float>(std::atof(number));
		}
		// nothing here, so we return NULL at the end
	}

	return 0.0;                  // No value, return 0.0 was NULL
}

// Reads out 'result' and will return the value after the '='. Returns nothing but will put
// the result in 'value[25]'. Max argument may be 25 characters!
void INI_WordValueCHAR(char result[80], char value[80]) {
	int pos = 0;
	int is_pos = -1;

	// clear out entire string
	for (int i = 0; i < 25; i++)
		value[i] = '\0';

	while (pos < 79) {
		if (result[pos] == '=') {
			is_pos = pos;
			break;
		}
		pos++;
	}

	if (is_pos > -1) {
		// Whenever the IS (=) position is known, we make a number out of 'IS_POS' till the next empty
		// space.
		int end_pos = -1;

		while (pos < 79) {
			if (result[pos] == '\0') {
				end_pos = pos;
				break;
			}
			pos++;
		}

		// End position found!
		if (end_pos > -1) {
			// We know the END position. We will use that piece of string to read out a number.

			// Copy the part to 'value', Make sure we won't get outside the array of the character.
			int cp = is_pos + 1;
			int c = 0;
			while (cp < end_pos) {
				value[c] = result[cp];
				c++;
				cp++;
				if (c > 79)
					break;
			}
		}
	}
}

// parses the chat file (loads in blocks)
void INI_PARSE_CHATFILE() {
	char dirname[256];
	char filename[256];

	FILE* stream;
	int section = INI_NONE;

	// Set Directory name + file
	std::strcpy(dirname, "data/cstrike/chat.ini");

	// writes whole path into "filename", Linux compatible
	UTIL_BuildFileNameRB(dirname, filename);

	// make sure the engine knows...
	REALBOT_PRINT(nullptr, "INI_PARSE_CHATFILE", "Loading CHAT.INI\n");

	int iBlockId = -1;
	int iBlockWord = -1;
	int iBlockSentence = -1;

	// load it
	if ((stream = std::fopen(filename, "r+t")) != nullptr) {
		// infinite loop baby
		while (!feof(stream)) {
			char linesection[30];
			char linefeed[80];
			INI_Sentence(stream, linefeed);

			// Linefeed contains a string of 1 sentence. Whenever the first character is a commentary
			// character (which is "//", ";" or "#"), or an empty line, then skip it
			if (linefeed[0] == ';' ||
				linefeed[0] == '#' ||
				(linefeed[0] == '/' && linefeed[1] == '/') ||
				linefeed[0] == '\n' || linefeed[0] == '\0')
				continue;           // Skip

			// Every line is checked for a new section.
			INI_Section(linefeed, linesection);

			if (linesection[0] != '\0' && std::strlen(linesection) > 1) {
				section = INI_SectionType(linesection, section);

				if (section == INI_BLOCK) {
					iBlockId++;
					if (iBlockId > 97)
						iBlockId = 97;
					section = INI_NONE;
					iBlockWord = -1;
					iBlockSentence = -1;
				}

				if (section == INI_DEATHS) {
					iBlockId = 99;   // 99 = death
					iBlockWord = -1;
					iBlockSentence = -1;
					section = INI_NONE;
				}

				if (section == INI_WELCOME) {
					iBlockId = 98;   // 98 = welcome
					iBlockWord = -1;
					iBlockSentence = -1;
					section = INI_NONE;
				}

				continue;           // next line
			}

			if (iBlockId > -1) {
				char lineword[25];
				INI_Word(linefeed, lineword);
				const int wordtype = INI_WordType(lineword, section);

				// We load in words
				if (wordtype == WORD_WORD) {
					iBlockWord++;
					if (iBlockWord > 9)
						iBlockWord = 9;

					// write the word in the block
					char chWord[25] = {};
					INI_WordValueCHAR(linefeed, chWord);
					// lower case
					//chWord = _strlwr( _strdup( chWord ) );
#ifdef _WIN32

					_strupr(chWord);
					// #elseif did not work for MSVC
#else
			   //for linux by ok:
			   // transform removed by evyncke since it works only on strings and not char array
			   //further changed back by evyncke as these are normal string not CString
			   //Hence, hardcoding the strupr inline...
					char* pString;
					pString = chWord;
					while (*pString) {
						*pString = toupper(*pString);
						pString++;
					}
#endif
					std::strcpy(ChatEngine.ReplyBlock[iBlockId].word[iBlockWord],
						chWord);
				}

				if (wordtype == WORD_SENTENCE) {
					iBlockSentence++;
					if (iBlockSentence > 49)
						iBlockSentence = 49;

					// write the word in the block
					char chSentence[80] = {};
					INI_WordValueCHAR(linefeed, chSentence);
					std::strcpy(ChatEngine.ReplyBlock[iBlockId].
						sentence[iBlockSentence], chSentence);

					// here we say it is used
					ChatEngine.ReplyBlock[iBlockId].bUsed = true;
				}
			}
		}                         // while

		std::fclose(stream);
	}
}

// Parse IAD file:
// Important Area Definition file
void INI_PARSE_IAD() {
	char dirname[256];
	char filename[256];

	FILE* stream;
	int section = INI_NONE;

	// Set Directory name
	std::strcpy(dirname, "data/cstrike/ini/");

	std::strcat(dirname, STRING(gpGlobals->mapname));
	std::strcat(dirname, ".ini");     // nodes file

	// writes whole path into "filename", Linux compatible
	UTIL_BuildFileNameRB(dirname, filename);

	SERVER_PRINT(filename);
	SERVER_PRINT("\n");

	float AreaY, AreaZ;
	float AreaX = AreaY = AreaZ = 9999;

	if ((stream = std::fopen(filename, "r+t")) != nullptr) {
		while (!feof(stream)) {
			char linesection[30];
			char linefeed[80];
			INI_Sentence(stream, linefeed);

			// Linefeed contains a string of 1 sentence. Whenever the first character is a commentary
			// character (which is "//", ";" or "#"), or an empty line, then skip it
			if (linefeed[0] == ';' ||
				linefeed[0] == '#' ||
				(linefeed[0] == '/' && linefeed[1] == '/') ||
				linefeed[0] == '\n' || linefeed[0] == '\0')
				continue;           // Skip

			// Every line is checked for a new section.
			INI_Section(linefeed, linesection);

			if (linesection[0] != '\0' && std::strlen(linesection) > 1) {
				section = INI_SectionType(linesection, section);
				continue;           // next line
			}

			// Check word only when in a section
			if (section != INI_NONE) {
				char lineword[25];
				INI_Word(linefeed, lineword);
				const int wordtype = INI_WordType(lineword, section);

				if (section == INI_AREA) {
					if (wordtype == WORD_AREAX)
						AreaX = static_cast<float>(INI_WordValueINT(linefeed));
					if (wordtype == WORD_AREAY)
						AreaY = static_cast<float>(INI_WordValueINT(linefeed));
					if (wordtype == WORD_AREAZ)
						AreaZ = static_cast<float>(INI_WordValueINT(linefeed));

					if (AreaX != 9999 && AreaY != 9999 && AreaZ != 9999) {
						// add this to goal
						rblog("IAD: Adding an important area/goal\n");
						NodeMachine.addGoal(nullptr, GOAL_IMPORTANT, Vector(AreaX, AreaY, AreaZ));

						AreaX = AreaY = AreaZ = 9999;
					}
				}
			}
		}                         // while

		std::fclose(stream);
	}
}

// Parse personality file
void INI_PARSE_BOTS(char cBotName[33], cBot* pBot) {
	/*
	   Revisited: 02/07/05 - Stefan
	   Last bug/issue report:
	   - seems to overwrite personality file ? (loading does not work?)
	   RESULT: There is no bug.
	   - removed any messages sent by this function.
	 */

	FILE* stream;
	int section = INI_NONE;

	char dirname[256];
	char filename[256];

	// Set Directory name
	if (mod_id == CSTRIKE_DLL)
		std::strcpy(dirname, "data/cstrike/bots/");

	//strcat(dirname, STRING(gpGlobals->mapname));
	std::strcat(dirname, cBotName);
	std::strcat(dirname, ".ini");

	// writes whole path into "filename", Linux compatible
	UTIL_BuildFileNameRB(dirname, filename);

	// we open the file here!
	if ((stream = std::fopen(filename, "r+t")) != nullptr) {
		// infinite loop baby
		while (!feof(stream)) {
			char linesection[30];
			char linefeed[80];
			INI_Sentence(stream, linefeed);

			// Linefeed contains a string of 1 sentence. Whenever the first character is a commentary
			// character (which is "//", ";" or "#"), or an empty line, then skip it
			if (linefeed[0] == ';' ||
				linefeed[0] == '#' ||
				(linefeed[0] == '/' && linefeed[1] == '/') ||
				linefeed[0] == '\n' || linefeed[0] == '\0')
				continue;           // Skip

			// Every line is checked for a new section.
			INI_Section(linefeed, linesection);

			if (linesection[0] != '\0' && std::strlen(linesection) > 1) {
				section = INI_SectionType(linesection, section);
				continue;           // next line
			}
			// Check word only when in a section
			if (section != INI_NONE) {
				char lineword[25];
				INI_Word(linefeed, lineword);
				const int wordtype = INI_WordType(lineword, section);

				// WEAPON
				if (section == INI_WEAPON) {
					// 30/07/04 Josh
					// Code optimization using a case instead of series of IF THEN
					switch (wordtype) {
					case WORD_PRIWEAPON:
						pBot->ipFavoPriWeapon = INI_WordValueINT(linefeed);
						break;
					case WORD_SECWEAPON:
						pBot->ipFavoSecWeapon = INI_WordValueINT(linefeed);
						break;
					case WORD_GRENADE:
						pBot->ipBuyGrenade = INI_WordValueINT(linefeed);
						break;
					case WORD_SAVEFORWEAP:
						pBot->ipSaveForWeapon = INI_WordValueINT(linefeed);
						break;
					case WORD_FLASHBANG:
						pBot->ipBuyFlashBang = INI_WordValueINT(linefeed);
						break;
					case WORD_SMOKEGREN:
						pBot->ipBuySmokeGren = INI_WordValueINT(linefeed);
						break;
					case WORD_DEFUSEKIT:
						pBot->ipBuyDefuseKit = INI_WordValueINT(linefeed);
						break;
					case WORD_ARMOUR:
						pBot->ipBuyArmour = INI_WordValueINT(linefeed);
						break;
					}
				}
				// SKILL
				if (section == INI_SKILL) {
					switch (wordtype) {
					case WORD_MINREACTTIME:
						pBot->fpMinReactTime = INI_WordValueFLOAT(linefeed);
						break;
					case WORD_MAXREACTTIME:
						pBot->fpMaxReactTime = INI_WordValueFLOAT(linefeed);
						break;
					case WORD_XOFFSET:
						pBot->fpXOffset = INI_WordValueFLOAT(linefeed);
						break;
					case WORD_YOFFSET:
						pBot->fpYOffset = INI_WordValueFLOAT(linefeed);
						break;
					case WORD_ZOFFSET:
						pBot->fpZOffset = INI_WordValueFLOAT(linefeed);
						break;
					}
					// default we override bot skill with personality skill
					if (Game.iOverrideBotSkill == GAME_YES)
						if (wordtype == WORD_BOTSKILL)
							pBot->bot_skill = INI_WordValueINT(linefeed);
				}
				// GAME
				if (section == INI_GAME) {
					if (wordtype == WORD_HOSTAGERATE)
						pBot->ipHostage = INI_WordValueINT(linefeed);
					if (wordtype == WORD_BOMBSPOTRATE)
						pBot->ipBombspot = INI_WordValueINT(linefeed);
					if (wordtype == WORD_RANDOMRATE)
						pBot->ipRandom = INI_WordValueINT(linefeed);
					if (wordtype == WORD_DROPPEDBOMB)
						pBot->ipDroppedBomb = INI_WordValueINT(linefeed);
				}
				// RADIO
				if (section == INI_RADIO) {
					if (wordtype == WORD_REPLYRADIO)
						pBot->ipReplyToRadio = INI_WordValueINT(linefeed);
					if (wordtype == WORD_CREATERADIO)
						pBot->ipCreateRadio = INI_WordValueINT(linefeed);
				}
				// TEAM
				if (section == INI_TEAM) {
					if (wordtype == WORD_HELPTEAM)
						pBot->ipHelpTeammate = INI_WordValueINT(linefeed);
				}
				// PERSON
				if (section == INI_PERSON) {
					if (wordtype == WORD_TURNSPEED)
						pBot->ipTurnSpeed = INI_WordValueINT(linefeed);
					if (wordtype == WORD_WALKKNIFE)
						pBot->ipWalkWithKnife = INI_WordValueINT(linefeed);
					if (wordtype == WORD_FEARRATE)
						pBot->ipFearRate = INI_WordValueINT(linefeed);
					if (wordtype == WORD_HEARRATE)
						pBot->ipHearRate = INI_WordValueINT(linefeed);
					if (wordtype == WORD_CHATRATE)
						pBot->ipChatRate = INI_WordValueINT(linefeed);
					if (wordtype == WORD_CAMPRATE)
						pBot->ipCampRate = INI_WordValueINT(linefeed);
				}
			}
		}
		std::fclose(stream);
	}
	// When we end up here, there is NO file?
	else {
		// Create new variables and save them into file.

		// Buy preferences
		pBot->ipFavoPriWeapon = -1;
		pBot->ipFavoSecWeapon = -1;
		pBot->ipBuyFlashBang = RANDOM_LONG(20, 80);
		pBot->ipBuyGrenade = RANDOM_LONG(20, 80);
		pBot->ipBuySmokeGren = RANDOM_LONG(20, 80);
		pBot->ipBuyDefuseKit = RANDOM_LONG(20, 80);
		pBot->ipDroppedBomb = RANDOM_LONG(20, 80);
		pBot->ipSaveForWeapon = RANDOM_LONG(0, 20);
		pBot->ipBuyArmour = RANDOM_LONG(30, 100);
		pBot->ipFearRate = RANDOM_LONG(20, 60);

		// Skill, everything but botskill can change.

		// Determine reaction time based upon botskill here
		float fMinReact;
		if (pBot->bot_skill == 0)
			fMinReact = 0.0f;
		else
			//30.8.04 redefined by frashman
			// fMinReact = RANDOM_FLOAT (0.05, (pBot->bot_skill / 10));
			// Reaction Time delay added for realistic gameplay [APG]RoboCop[CL]
			fMinReact =
			RANDOM_FLOAT(pBot->bot_skill / 20.0f + 0.3f,
				pBot->bot_skill / 5.0f + 0.3f);

		const float fMaxReact = fMinReact + RANDOM_FLOAT(0.2f, 0.4f);

		// SET them
		pBot->fpMinReactTime = fMinReact;
		pBot->fpMaxReactTime = fMaxReact;

		// Set Offsets (note, they are extra upon current aiming code)
		// 30.8.04 redefined by frashman
		// float fOffset = RANDOM_FLOAT ((pBot->bot_skill / 5), (pBot->bot_skill / 2));
		const float fOffset = RANDOM_FLOAT(pBot->bot_skill / 5.0f + 0.05f,
			pBot->bot_skill / 2.0f + 0.05f);

		// SET
		pBot->fpXOffset = pBot->fpYOffset = pBot->fpZOffset = fOffset;

		// Team
		pBot->ipHelpTeammate = RANDOM_LONG(40, 80);

		// Game
		pBot->ipHostage = RANDOM_LONG(25, 70);
		pBot->ipBombspot = RANDOM_LONG(25, 70);
		pBot->ipRandom = RANDOM_LONG(40, 80);

		// Radio
		pBot->ipReplyToRadio = RANDOM_LONG(10, 20);
		pBot->ipCreateRadio = RANDOM_LONG(10, 20);
		pBot->ipHearRate = RANDOM_LONG(20, 60);

		// Person
		pBot->ipTurnSpeed = RANDOM_LONG(20, 40);
		pBot->ipCampRate = RANDOM_LONG(0, 40);
		pBot->ipChatRate = RANDOM_LONG(10, 20);
		pBot->ipWalkWithKnife = RANDOM_LONG(0, 30);

		// SAVE TO DISK:
		//char dirname[256];
		//char filename[256];

		// Set Directory name
		if (mod_id == CSTRIKE_DLL)
			std::strcpy(dirname, "data/cstrike/bots/");

		std::strcat(dirname, cBotName);
		std::strcat(dirname, ".ini");

		// writes whole path into "filename", Linux compatible
		UTIL_BuildFileNameRB(dirname, filename);

		// Only save if lock type is < 1
		FILE* rbl = std::fopen(filename, "w+t");

		// Created file
		if (rbl != nullptr) {
			std::fprintf(rbl, "; RealBot\n");
			std::fprintf(rbl, "; \n");
			std::fprintf(rbl,
				"; This personality is created with random values. You may\n; change this file to create your own personality.\n\n");

			// WEAPON
			std::fprintf(rbl, "[WEAPON]\n");
			std::fprintf(rbl, "PrimaryWeapon=%d\n", pBot->ipFavoPriWeapon);
			std::fprintf(rbl, "SecondaryWeapon=%d\n", pBot->ipFavoSecWeapon);
			std::fprintf(rbl, "SaveForWeapon=%d\n", pBot->ipSaveForWeapon);
			std::fprintf(rbl, "Grenade=%d\n", pBot->ipBuyGrenade);
			std::fprintf(rbl, "Flashbang=%d\n", pBot->ipBuyFlashBang);
			std::fprintf(rbl, "SmokeGrenade=%d\n", pBot->ipBuySmokeGren);
			std::fprintf(rbl, "DefuseKit=%d\n", pBot->ipBuyDefuseKit);
			std::fprintf(rbl, "Armour=%d\n", pBot->ipBuyArmour);
			std::fprintf(rbl, "\n");

			// SKILL
			std::fprintf(rbl, "[SKILL]\n");
			std::fprintf(rbl, "XOffset=%f\n", pBot->fpXOffset);
			std::fprintf(rbl, "YOffset=%f\n", pBot->fpYOffset);
			std::fprintf(rbl, "ZOffset=%f\n", pBot->fpZOffset);
			std::fprintf(rbl, "BotSkill=%d\n", pBot->bot_skill);
			std::fprintf(rbl, "MaxReactionTime=%f\n", pBot->fpMaxReactTime);
			std::fprintf(rbl, "MinReactionTime=%f\n", pBot->fpMinReactTime);
			std::fprintf(rbl, "\n");

			// GAME
			std::fprintf(rbl, "[GAME]\n");
			std::fprintf(rbl, "Hostage=%d\n", pBot->ipHostage);
			std::fprintf(rbl, "BombSpot=%d\n", pBot->ipBombspot);
			std::fprintf(rbl, "DroppedBomb=%d\n", pBot->ipDroppedBomb);
			std::fprintf(rbl, "Random=%d\n", pBot->ipRandom);
			std::fprintf(rbl, "\n");

			// RADIO
			std::fprintf(rbl, "[RADIO]\n");
			std::fprintf(rbl, "Reply=%d\n", pBot->ipReplyToRadio);
			std::fprintf(rbl, "Create=%d\n", pBot->ipCreateRadio);
			std::fprintf(rbl, "\n");

			// TEAM
			std::fprintf(rbl, "[TEAM]\n");
			std::fprintf(rbl, "HelpTeammate=%d\n", pBot->ipHelpTeammate);
			std::fprintf(rbl, "\n");

			// PERSON
			std::fprintf(rbl, "[PERSON]\n");
			std::fprintf(rbl, "Turnspeed=%d\n", pBot->ipTurnSpeed);
			std::fprintf(rbl, "WalkWithKnife=%d\n", pBot->ipWalkWithKnife);
			std::fprintf(rbl, "HearRate=%d\n", pBot->ipHearRate);
			std::fprintf(rbl, "FearRate=%d\n", pBot->ipFearRate);
			std::fprintf(rbl, "ChatRate=%d\n", pBot->ipChatRate);
			std::fprintf(rbl, "CampRate=%d\n", pBot->ipCampRate);
			std::fprintf(rbl, "\n");

			// Close file
			std::fclose(rbl);
		}
	}
}                               // INI parsing

/**
 * The buytable.ini file is located at REALBOT_HOME/data/cstrike
 *
 * It contains weapon information such as which slot (in buy menu) and pricing. The buy logic uses the price configured
 * here to choose which weapon to buy.
 *
 * The buytable.ini is not in the source file, but in the binary downloads of REALBOT.
 */
void INI_PARSE_BUYTABLE() {
	FILE* stream;
	int section = INI_NONE;
	int prev_section = section;
	int weapon_id = -1;

	char dirname[256];
	char filename[256];

	// Set Directory name
	if (mod_id == CSTRIKE_DLL)
		std::strcpy(dirname, "data/cstrike/");

	//strcat(dirname, STRING(gpGlobals->mapname));
	std::strcat(dirname, "buytable.ini");

	// writes whole path into "filename", Linux compatible
	UTIL_BuildFileNameRB(dirname, filename);

	// clear out weapon table completely
	for (int cl = 0; cl < 32; cl++) {
		weapons_table[cl].iId = -1;
		weapons_table[cl].price = -1;
		weapons_table[cl].priority = -1;
		weapons_table[cl].iIdIndex = -1;
	}

	if ((stream = std::fopen(filename, "r+t")) != nullptr) {
		while (!feof(stream)) {
			char linesection[30];
			char linefeed[80];
			INI_Sentence(stream, linefeed);

			// Linefeed contains a string of 1 sentence. Whenever the first character is a commentary
			// character (which is "//", ";" or "#"), or an empty line, then skip it
			if (linefeed[0] == ';' ||
				linefeed[0] == '#' ||
				(linefeed[0] == '/' && linefeed[1] == '/') ||
				linefeed[0] == '\n' || linefeed[0] == '\0')
				continue;           // Skip

			// Every line is checked for a new section.
			INI_Section(linefeed, linesection);

			// Found a new section
			if (linesection[0] != '\0' && std::strlen(linesection) > 1) {
				section = INI_SectionType_BUYTABLE(linesection, section);
				// Check if its the same as the previous section
				if (section != prev_section) {
					weapon_id++;     // new weapon
					if (weapon_id > MAX_WEAPONS - 1) {
						// done
						//std::fclose(stream);
						break;        // out of the loop
					}

					weapons_table[weapon_id].iId = section;
					weapons_table[section].iIdIndex = weapon_id;
				}

				prev_section = section;     // Equal the sections now
				continue;           // next line
			}

			// Check word only when in a section
			if (section != INI_NONE) {
				char lineword[25];
				INI_Word(linefeed, lineword);
				const int wordtype = INI_WordType(lineword, section);
				if (wordtype != WORD_NONE) {
					if (wordtype == WORD_PRICE) {
						//BotDebug("Loading price\n");
						weapons_table[weapon_id].price =
							INI_WordValueINT(linefeed);
					} else if (wordtype == WORD_PRIORITY) {
						//BotDebug("Loading priority\n");
						weapons_table[weapons_table[weapon_id].iId].priority =
							INI_WordValueINT(linefeed);
					}
				}
			}
		}
		std::fclose(stream);
	}
}                               // INI parsing

// $Log: IniParser.cpp,v $
// Revision 1.15  2004/09/07 18:23:02  eric
// - bumped version to 3061
// - adding Frashman code to buy the weapon as selected by Josh's code
// - Realbot is really the result of multiple people :-)
//
// Revision 1.13  2004/07/30 15:02:29  eric
// - jumped to version 3057
// - improved readibility (wapen_tabel -> weapons_table) :-P
// - all Josh Borke modifications to the buying stuff:
//     * using a switch() instead of several if
//     * better buying code for shield and primary weapons
//     * new command 'debug pistols 0/1'
//
// Revision 1.12  2004/07/02 16:43:34  stefan
// - upped to build 3051
// - changed log() into rblog()
// - removed BOT.CFG code that interpets old RB V1.0 commands
// - neater respons of the RealBot console
// - more help from RealBot console (ie, type realbot server broadcast ... with no arguments it will tell you what you can do with this, etc)
// - removed message "bot personality loaded from file"
// - in overal; some cleaning done, no extra features added
//
// Revision 1.11  2004/07/01 18:22:40  stefan
// - forgot to fix an issue on engine.cpp (WeaponList retrieval for CS 1.6) , its fixed now.. thanks Whistler.
//
// Revision 1.10  2004/06/23 08:24:13  stefan
// - upped to build 3049
// - added swat behaviour (team leader assignment, radio response change and leaders command team-mates) - THIS IS EXPERIMENTAL AND DOES NOT ALWAYS WORK AS I WANT IT TO.
// - changed some conditions in nodemachine
// - sorry evy, still not added your new goals ;) will do next time, i promise
//
// Revision 1.9  2004/06/19 21:06:14  stefan
// - changed distance check in nodemachine
// - fixed some 'steep' bug in nodemachine
//
// Revision 1.8  2004/06/18 12:20:05  stefan
// - fixed another bug in chatting, CS 1.5 won't crash now
// - added some limit to bots searching for goals
//
// Revision 1.7  2004/06/17 21:23:22  stefan
// - fixes several connection problems with nodes. Going down from steep + crates (de_dust) PLUS going up/down from very steep slopes on as_oilrig.. 0wnage and thx to PMB and Evy
// - fixed chat bug in CS 1.6, its still CS 1.5 & CS 1.6 compatible though
//
// Revision 1.6  2004/06/06 21:16:29  stefan
// - evy fixed compile issues for Linux + making sure that it works (no crashes in linux anymore yey)
// - fixed some 'double calling of functions' , pointed out by Whistler (still needs some checking, just applied very fast fix).
//
// Revision 1.5  2004/05/29 19:05:46  stefan
// - upped to BUILD 3044
// - removed several debug messages on screen
// - changed default 'chatrate (max sentences)' to 3
// - removed copyright notice, which is not valid due GPL license
//
// i know, nothing special :)
//
// Revision 1.4  2004/04/26 16:36:48  stefan
// changed:
// #elseif into #else to keep MSVC satisfied
//
// added:
// comment lines in game.cpp so you know what vars are what.
//
// Revision 1.3  2004/04/23 10:35:33  ok
// changes for linux, using tranform to get cstings to uppercase
//
// Revision 1.2  2004/04/18 17:39:19  stefan
// - Upped to build 2043
// - REALBOT_PRINT() works properly now
// - Log() works properly now
// - Clearing in dll.cpp of reallog.txt at dll init
// - Logging works now, add REALBOT_PRINT() at every point you want to log something.
//