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

#include <cmath>
#include <cstring>
#include <extdll.h>
#include <dllapi.h>
#include <meta_api.h>
#include <entity_state.h>

#include "bot.h"
#include "game.h"
#include "bot_client.h"
#include "bot_weapons.h"
#include "bot_func.h"
#include "NodeMachine.h"
#include "ChatEngine.h"
// types of damage to ignore...

#define IGNORE_DAMAGE (DMG_CRUSH | DMG_FREEZE | DMG_FALL | DMG_SHOCK | DMG_DROWN | DMG_NERVEGAS | DMG_RADIATION | DMG_DROWNRECOVER | DMG_ACID | DMG_SLOWBURN | DMG_SLOWFREEZE | 0xFF000000)
extern int mod_id;
extern cBot bots[32];
extern cNodeMachine NodeMachine;
extern cGame Game;
extern cChatEngine ChatEngine;
extern int counterstrike;

bot_weapon_t weapon_defs[MAX_WEAPONS];  // array of weapon definitions

static FILE *fp;

// This message is sent when the Counter-Strike VGUI menu is displayed.
void BotClient_CS_VGUI(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_CS_VGUI()\n");
    if (*static_cast<int*>(p) == 2)       // is it a team select menu?
        bots[bot_index].start_action = MSG_CS_TEAM_SELECT;
    else if (*static_cast<int*>(p) == 26) // is is a terrorist model select menu?
        bots[bot_index].start_action = MSG_CS_T_SELECT;
    else if (*static_cast<int*>(p) == 27) // is is a counter-terrorist model select menu?
        bots[bot_index].start_action = MSG_CS_CT_SELECT;
}

// This message is sent when a menu is being displayed in Counter-Strike.
void BotClient_CS_ShowMenu(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_CS_ShowMenu()\n");
    static int state = 0;        // current state machine state

    if (state < 3) {
        state++;                  // ignore first 3 fields of message
        return;
    }

    if (std::strcmp(static_cast<char*>(p), "#Team_Select") == 0 ||
        std::strcmp(static_cast<char*>(p), "#Team_Select_Spect") == 0 ||
        std::strcmp(static_cast<char*>(p), "#IG_Team_Select_Spect") == 0 ||
        std::strcmp(static_cast<char*>(p), "#IG_Team_Select") == 0 ||
        std::strcmp(static_cast<char*>(p), "#IG_VIP_Team_Select") == 0 ||
        std::strcmp(static_cast<char*>(p), "#IG_VIP_Team_Select_Spect") == 0) {
        // team select menu?
        bots[bot_index].start_action = MSG_CS_TEAM_SELECT;
    } else if (std::strcmp(static_cast<char*>(p), "#Terrorist_Select") == 0) {
        // T model select?
        bots[bot_index].start_action = MSG_CS_T_SELECT;
    } else if (std::strcmp(static_cast<char*>(p), "#CT_Select") == 0) {
        // CT model select menu?
        bots[bot_index].start_action = MSG_CS_CT_SELECT;
    }

    state = 0;                   // reset state machine
}

// This message is sent when a client joins the game.  All of the weapons
// are sent with the weapon ID and information about what ammo is used.
void BotClient_Valve_WeaponList(void *p, int bot_index) {
    //DebugOut("bot_client (FINDME): BotClient_Valve_WeaponList()\n");
    static int state = 0;        // current state machine state
    static bot_weapon_t bot_weapon;

    if (state == 0) {
        state++;
        std::strcpy(bot_weapon.szClassname, static_cast<char*>(p));
    } else if (state == 1) {
        state++;
        bot_weapon.iAmmo1 = *static_cast<int*>(p);   // ammo index 1
    } else if (state == 2) {
        state++;
        bot_weapon.iAmmo1Max = *static_cast<int*>(p);        // max ammo1
    } else if (state == 3) {
        state++;
        bot_weapon.iAmmo2 = *static_cast<int*>(p);   // ammo index 2
    } else if (state == 4) {
        state++;
        bot_weapon.iAmmo2Max = *static_cast<int*>(p);        // max ammo2
    } else if (state == 5) {
        state++;
        bot_weapon.iSlot = *static_cast<int*>(p);    // slot for this weapon
    } else if (state == 6) {
        state++;
        bot_weapon.iPosition = *static_cast<int*>(p);        // position in slot
    } else if (state == 7) {
        state++;
        bot_weapon.iId = *static_cast<int*>(p);      // weapon ID
    } else if (state == 8) {
        state = 0;

        bot_weapon.iFlags = *static_cast<int*>(p);   // flags for weapon (WTF???)

        // store away this weapon with it's ammo information...
        weapon_defs[bot_weapon.iId] = bot_weapon;

        /*
           fp=fopen("!BotWeapon.txt","a");
           fprintf(fp,"[%s]\n", bot_weapon.szClassname);
           fprintf(fp, "; COUNTER-STRIKE SPECIFIC INFORMATION, DO NOT EDIT!\n");
           fprintf(fp,"Ammo1Index=%d\n", bot_weapon.iAmmo1);
           fprintf(fp,"Ammo1Max=%d\n", bot_weapon.iAmmo1Max);
           fprintf(fp,"Ammo2Index=%d\n", bot_weapon.iAmmo2);
           fprintf(fp,"Ammo2Max=%d\n", bot_weapon.iAmmo2Max);
           fprintf(fp,"Slot=%d\n", bot_weapon.iSlot);
           fprintf(fp,"iPosition=%d\n", bot_weapon.iPosition);
           fprintf(fp, "; --- END\n");
           fclose(fp);
         */
    }
}

void BotClient_CS_WeaponList(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_CS_WeaponList()\n");
    // this is just like the Valve Weapon List message
    BotClient_Valve_WeaponList(p, bot_index);
}

void BotClient_Gearbox_WeaponList(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_Gearbox_WeaponList()\n");
    // this is just like the Valve Weapon List message
    BotClient_Valve_WeaponList(p, bot_index);
}

void BotClient_FLF_WeaponList(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_FLF_WeaponList()\n");
    // this is just like the Valve Weapon List message
    BotClient_Valve_WeaponList(p, bot_index);
}

// This message is sent when a weapon is selected (either by the bot chosing
// a weapon or by the server auto assigning the bot a weapon).
void BotClient_Valve_CurrentWeapon(void *p, int bot_index) {
    static int state = 0;        // current state machine state
    static int iState;
    static int iId;

    if (state == 0) {
        state++;
        iState = *static_cast<int*>(p);      // state of the current weapon
    } else if (state == 1) {
        state++;
        iId = *static_cast<int*>(p);         // weapon ID of current weapon
    } else if (state == 2) {
	    static int iClip;
	    state = 0;

        iClip = *static_cast<int*>(p);       // ammo currently in the clip for this weapon

        if (iId < static_cast<int>(std::size(weapon_defs))) {
            bots[bot_index].bot_weapons |= 1 << iId;     // set this weapon bit

            if (iState == 1) {
                bots[bot_index].current_weapon.iId = iId;   // weapon id
                bots[bot_index].current_weapon.iClip = iClip;       // currently in clp

                // update the ammo counts for this weapon...

                bots[bot_index].current_weapon.iAmmo1 =
                        bots[bot_index].m_rgAmmo[weapon_defs[iId].iAmmo1];

                bots[bot_index].current_weapon.iAmmo2 =
                        bots[bot_index].m_rgAmmo[weapon_defs[iId].iAmmo2];

            }
        }
    }
}

void BotClient_CS_CurrentWeapon(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_CS_CurrentWeapon()\n");
    // this is just like the Valve Current Weapon message
    BotClient_Valve_CurrentWeapon(p, bot_index);
}

void BotClient_Gearbox_CurrentWeapon(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_Gearbox_CurrentWeapon()\n");
    // this is just like the Valve Current Weapon message
    BotClient_Valve_CurrentWeapon(p, bot_index);
}

void BotClient_FLF_CurrentWeapon(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_FLF_CurrentWeapon()\n");
    // this is just like the Valve Current Weapon message
    BotClient_Valve_CurrentWeapon(p, bot_index);
}

// This message is sent whenever ammo ammounts are adjusted (up or down).
void BotClient_Valve_AmmoX(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_Valve_AmmoX()\n");
    static int state = 0;        // current state machine state
    static int index;

    if (state == 0) {
        state++;
        index = *static_cast<int*>(p);       // ammo index (for type of ammo)
    } else if (state == 1) {
	    static int ammount;
	    state = 0;

        ammount = *static_cast<int*>(p);     // the ammount of ammo currently available

        bots[bot_index].m_rgAmmo[index] = ammount;        // store it away

        const int ammo_index = bots[bot_index].current_weapon.iId;

        // update the ammo counts for this weapon...
        bots[bot_index].current_weapon.iAmmo1 =
                bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo1];

        bots[bot_index].current_weapon.iAmmo2 =
                bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo2];

    }
}

void BotClient_CS_AmmoX(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_CS_AmmoX()\n");
    // this is just like the Valve AmmoX message
    BotClient_Valve_AmmoX(p, bot_index);
}

void BotClient_Gearbox_AmmoX(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_Gearbox_AmmoX()\n");
    // this is just like the Valve AmmoX message
    BotClient_Valve_AmmoX(p, bot_index);
}

void BotClient_FLF_AmmoX(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_FLF_AmmoX()\n");
    // this is just like the Valve AmmoX message
    BotClient_Valve_AmmoX(p, bot_index);
}

// This message is sent when the bot picks up some ammo (AmmoX messages are
// also sent so this message is probably not really necessary except it
// allows the HUD to draw pictures of ammo that have been picked up.  The
// bots don't really need pictures since they don't have any eyes anyway.
void BotClient_Valve_AmmoPickup(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_Valve_AmmoPickup()\n");
    static int state = 0;        // current state machine state
    static int index;

    if (state == 0) {
        state++;
        index = *static_cast<int*>(p);
    } else if (state == 1) {
	    static int ammount;
	    state = 0;

        ammount = *static_cast<int*>(p);

        bots[bot_index].m_rgAmmo[index] = ammount;

        const int ammo_index = bots[bot_index].current_weapon.iId;

        // update the ammo counts for this weapon...
        bots[bot_index].current_weapon.iAmmo1 =
                bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo1];
        bots[bot_index].current_weapon.iAmmo2 =
                bots[bot_index].m_rgAmmo[weapon_defs[ammo_index].iAmmo2];
    }
}

void BotClient_CS_AmmoPickup(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_CS_AmmoPickup()\n");
    // this is just like the Valve Ammo Pickup message
    BotClient_Valve_AmmoPickup(p, bot_index);
}

void BotClient_Gearbox_AmmoPickup(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_Gearbox_AmmoPickup()\n");
    // this is just like the Valve Ammo Pickup message
    BotClient_Valve_AmmoPickup(p, bot_index);
}

void BotClient_FLF_AmmoPickup(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_FLF_AmmoPickup()\n");
    // this is just like the Valve Ammo Pickup message
    BotClient_Valve_AmmoPickup(p, bot_index);
}

// This message gets sent when the bot picks up a weapon.
void BotClient_Valve_WeaponPickup(void *p, int bot_index) {
	const int index = *static_cast<int*>(p);

    // set this weapon bit to indicate that we are carrying this weapon
    bots[bot_index].bot_weapons |= (1 << index);
}

void BotClient_CS_WeaponPickup(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_CS_WeaponPickup()\n");
    // this is just like the Valve Weapon Pickup message
    BotClient_Valve_WeaponPickup(p, bot_index);
}

void BotClient_Gearbox_WeaponPickup(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_Gearbox_WeaponPickup()\n");
    // this is just like the Valve Weapon Pickup message
    BotClient_Valve_WeaponPickup(p, bot_index);
}

void BotClient_FLF_WeaponPickup(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_FLF_WeaponPickup()\n");
    // this is just like the Valve Weapon Pickup message
    BotClient_Valve_WeaponPickup(p, bot_index);
}

// This message gets sent when the bot picks up an item (like a battery
// or a healthkit)
void BotClient_Valve_ItemPickup(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_Valve_ItemPickup()\n");
}

void BotClient_CS_ItemPickup(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_CS_ItemPickup()\n");
    // this is just like the Valve Item Pickup message
    BotClient_Valve_ItemPickup(p, bot_index);
}

void BotClient_Gearbox_ItemPickup(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_Gearbox_ItemPickup()\n");
    // this is just like the Valve Item Pickup message
    BotClient_Valve_ItemPickup(p, bot_index);
}

void BotClient_FLF_ItemPickup(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_FLF_ItemPickup()\n");
    // this is just like the Valve Item Pickup message
    BotClient_Valve_ItemPickup(p, bot_index);
}

// This message gets sent when the bots health changes.
void BotClient_Valve_Health(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_Valve_Health()\n");
    bots[bot_index].bot_health = *static_cast<int*>(p);     // health ammount
}

void BotClient_CS_Health(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_CS_Health()\n");
    // this is just like the Valve Health message
    BotClient_Valve_Health(p, bot_index);
}

void BotClient_Gearbox_Health(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_Gearbox_Health()\n");
    // this is just like the Valve Health message
    BotClient_Valve_Health(p, bot_index);
}

void BotClient_FLF_Health(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_FLF_Health()\n");
    // this is just like the Valve Health message
    BotClient_Valve_Health(p, bot_index);
}

// This message gets sent when the bots armor changes.
void BotClient_Valve_Battery(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_Valve_Battery()\n");
    bots[bot_index].bot_armor = *static_cast<int*>(p);      // armor ammount
}

void BotClient_CS_Battery(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_CS_Battery()\n");
    // this is just like the Valve Battery message
    BotClient_Valve_Battery(p, bot_index);
}

void BotClient_Gearbox_Battery(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_Gearbox_Battery()\n");
    // this is just like the Valve Battery message
    BotClient_Valve_Battery(p, bot_index);
}

void BotClient_FLF_Battery(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_FLF_Battery()\n");
    // this is just like the Valve Battery message
    BotClient_Valve_Battery(p, bot_index);
}

// This message gets sent when the bots are getting damaged.
void BotClient_Valve_Damage(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_Valve_Damage()\n");
    static int state = 0;        // current state machine state
    static int damage_armor;
    static int damage_taken;
    static int damage_bits;      // type of damage being done
    static Vector damage_origin;

    if (state == 0) {
        state++;
        damage_armor = *static_cast<int*>(p);
    } else if (state == 1) {
        state++;
        damage_taken = *static_cast<int*>(p);
    } else if (state == 2) {
        state++;
        damage_bits = *static_cast<int*>(p);
    } else if (state == 3) {
        state++;
        damage_origin.x = *static_cast<float*>(p);
    } else if (state == 4) {
        state++;
        damage_origin.y = *static_cast<float*>(p);
    } else if (state == 5) {
        state = 0;

        damage_origin.z = *static_cast<float*>(p);
        if (damage_armor > 0 || damage_taken > 0) {

            // Damage recieved:
            // - when the prev node was higher (so we are sure we do FIX the correct nodes!)
            //   we fix it (only when dist > 90)
            cBot *pBot = &bots[bot_index];

            if (damage_bits & (DMG_FALL | DMG_CRUSH)) {
                pBot->rprint_trace("BotClient_Valve_Damage", "Taken fall damage!");
                if (pBot->getPathIndex() > 0) { // was one node further, so we can use previous node!
	                const int iNode = NodeMachine.getNodeIndexFromBotForPath(pBot->iBotIndex, pBot->getPreviousPathIndex());

	                const float fDist = std::fabs(damage_origin.z - NodeMachine.node_vector(iNode).z);
                    if (fDist > 90) {
                        // we know where we came from, and we know where we went to
                        const int iNodeTo = NodeMachine.getNodeIndexFromBotForPath(pBot->iBotIndex,
                                                                                   pBot->getPathIndex());

                        // remove connection?

                        pBot->rprint_trace("BotClient_Valve_Damage", "Removing connection!!");
                        NodeMachine.removeConnection(iNode, iNodeTo);
                        pBot->rprint_trace("BotClient_Valve_Damage", "Removing connection - finished!!");
                    }
                }
            }

            // ignore certain types of damage...
            if (damage_bits & IGNORE_DAMAGE)
                return;

            // depending on bot skill slow this bot down a bit
			// pBot->f_move_speed *= (((10 - pBot->bot_skill) + 1) / 10);

            // if the bot doesn't have an enemy and someone is shooting at it then
            // turn in the attacker's direction...
            if (!pBot->hasEnemy()) {
                // face the attacker...
                edict_t *damageInflictor = pBot->pEdict->v.dmg_inflictor;
                if (damageInflictor) {
                    if (std::strcmp(STRING(damageInflictor->v.classname), "player") == 0) {
                        // Face danger vector
                        pBot->vHead = damage_origin;
                        pBot->vBody = damage_origin;

                        // move to damage vector
                        pBot->f_camp_time = gpGlobals->time;
                        pBot->rprint("BotClient_Valve_Damage", "Damage taken, by player, change goal to damage origin.");

                        char msg[255];
                        snprintf(msg, sizeof(msg), "damage_origin (x,y,z) => (%f,%f,%f) | damageInflictor->v.origin (x,y,z) => (%f,%f,%f)",
                                damage_origin.x,
                                damage_origin.y,
                                damage_origin.z,
                                damageInflictor->v.origin.x,
                                damageInflictor->v.origin.y,
                                damageInflictor->v.origin.z
                        );
                        pBot->rprint("BotClient_Valve_Damage", msg);
                        // with the above log message I confirmed that damageInflictor->v.origin is the same
                        // as damage_origin

                        pBot->setGoalNode(NodeMachine.getClosestNode(damage_origin, NODE_ZONE*2, damageInflictor));
                        pBot->forgetPath();
                    } else {
                        char msg[255];
                        snprintf(msg, sizeof(msg), "I have a damage inflictor! -> %s", STRING(damageInflictor->v.classname));
                        pBot->rprint("BotClient_Valve_Damage", msg);
                    }
                } else {
                    // probably never happens, if I see correctly the inflictor is 'worldspawn' if no entity
                    // is responsible
                    pBot->rprint("BotClient_Valve_Damage", "I have taken damage, without a damage conflictor");
                }
            }
        }
    }
}

void BotClient_CS_Damage(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_CS_Damage()\n");
    // this is just like the Valve Battery message
    BotClient_Valve_Damage(p, bot_index);
}

// Not required? [APG]RoboCop[CL]
/*void BotClient_Gearbox_Damage(void* p, int bot_index) {
    //DebugOut("bot_client: BotClient_Gearbox_Damage()\n");
    // this is just like the Valve Battery message
    BotClient_Valve_Damage(p, bot_index);
}

void BotClient_FLF_Damage(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_FLF_Damage()\n");
    // this is just like the Valve Battery message
    BotClient_Valve_Damage(p, bot_index);
}*/


void BotClient_CS_SayText(void *p, int bot_index) {
    static unsigned char ucEntIndex;

    /**
     * Since it has been a while I had to re-figure out how this engine calling/messaging works.
     * In a nutshell: the function BotClient_CS_SayText is determined to call at the function pfnMessageBegin,
     * then data is written to this function via pfnWriteByte, pfnWriteChar, pfnWriteShort, etc. These are separate calls
     * hence the function itself should have a 'state' so it knows how many times it has been called already and which data
     * it is receiving through *p. Hence the so called 'state machine'.
     */
    static int state = 0;        // current state machine state,

    // Different Counter-Strike versions mean different
    // handling of this "SayText" thingy.
    if (counterstrike == 0) {
        if (state == 0) {
            ucEntIndex = *static_cast<unsigned char*>(p);
        } else if (state == 1) {
	        const cBot *pBot = &bots[bot_index];

            if (ENTINDEX(pBot->pEdict) != ucEntIndex) {
                char sentence[MAX_SENTENCE_LENGTH];
                char chSentence[MAX_SENTENCE_LENGTH];
                char netname[30];

                std::memset(sentence, 0, sizeof(sentence));
                std::memset(chSentence, 0, sizeof(chSentence));
                std::memset(netname, 0, sizeof(netname));

                std::strcpy(sentence, static_cast<char*>(p)); // the actual sentence

                unsigned int length = 0;

                // FIXED: In any case that this might return NULL, do not crash the server
                const char* found = std::strstr(sentence, " : ");
                if (found != nullptr)
                    length = std::strlen(sentence) - std::strlen(found);

                int tc = 0;

                for (unsigned int c = length; c < MAX_SENTENCE_LENGTH; c++) {
                    chSentence[tc] = sentence[c];
                    tc++;
                }

                const edict_t *pPlayer = INDEXENT(ucEntIndex);
                std::strcpy(netname, STRING(pPlayer->v.netname));

                ChatEngine.set_sentence(netname, chSentence);
                state = -1;
            }
        }
    } else if (counterstrike == 1) {
        // CS 1.6
        if (state == 0) {
            // who sent this message?
            ucEntIndex = *static_cast<unsigned char*>(p);
        }
            // to who?
        else if (state == 1) {
            // here must be some team check so we do not let bots
            // of the other team react to this somehow..
            // - after playing with the dll i did not notice any weird stuff yet...
            // - so only when people discover some bugs with this we are going to fix this
            // - thing... ie "Only fix it when its broken".
        }
            // don't know?
        else if (state == 2) {
            // do nothing here
        }
            // the actual sentence
        else if (state == 3) {
            // sentence
            char sentence[MAX_SENTENCE_LENGTH];
            char netname[30];

            // init
            std::memset(sentence, 0, sizeof(sentence));
            std::memset(netname, 0, sizeof(netname));

            // copy in memory
            std::strcpy(sentence, static_cast<char*>(p));

            // copy netname
            const edict_t *pPlayer = INDEXENT(ucEntIndex);
            std::strcpy(netname, STRING(pPlayer->v.netname));

            // and give chatengine something to do
            ChatEngine.set_sentence(netname, sentence);

            state = -1;
        }
    }
    state++;
}

// -------------------------------------------------------------
// BERKED -- call with nullptr to set state to zero.
// Save HUD display of bomb plantability.
// Would like to get rid of state.
// Converted to use switches.

// This message gets sent when the bot enters a buyzone
void BotClient_CS_StatusIcon(void *p, int bot_index) {
    /*
       FROM SDK 2.3
       // Message handler for StatusIcon message
       // accepts five values:
       //         byte   : TRUE = ENABLE icon, FALSE = DISABLE icon
       //         string : the sprite name to display
       //         byte   : red
       //         byte   : green
       //         byte   : blue
     */
    static int state = 0;        // current state machine state

    if (p == nullptr)                  // A bandaid.  Make this whole thing more robust!
    {
        state = 0;
        return;
    }

    switch (state++) {
	    static int EnableIcon;
    case 0:                     // Enable or not?
            EnableIcon = *static_cast<int*>(p);  // check the byte
            break;
        case 1:                     // Which icon
            if (std::strcmp(static_cast<char*>(p), "buyzone") == 0) {
                switch (EnableIcon) {
                    case 0:               // Not in buy zone
                        state = 0;
                        break;
                    case 1:               // In buy zone
                        break;
                    default:
                        break;
                }
            } else if (std::strcmp(static_cast<char*>(p), "c4") == 0) {
                switch (EnableIcon) {
                    case 0:               // No C4
                        bots[bot_index].bHUD_C4_plantable = false;
                        state = 0;
                        break;
                    case 1:               // C4
                        bots[bot_index].bHUD_C4_plantable = false;
                        break;
                    case 2:               // C4 plantable
                        bots[bot_index].bHUD_C4_plantable = true;   // BINGO!!! -- BERKED
                        break;
                    default:
                        break;
                }
            } else if (std::strcmp(static_cast<char*>(p), "defuser") == 0) {
                switch (EnableIcon) {
                    case 0:               // No defuser
                        state = 0;
                        break;
                    case 1:               // defuser
                        break;
                    default:
                        break;
                }
            } else if (std::strcmp(static_cast<char*>(p), "rescue") == 0) {
                switch (EnableIcon) {
                    case 0:               // Not in rescue zone
                        state = 0;
                        break;
                    case 1:               // In rescue zone
                        break;
                    default:
                        break;
                }
            } else                    // something we don't know about yet.
            {}
            break;
        case 2:                     // colors
            // RED
            break;
        case 3:
            // GREEN
            break;
        case 4:
            // BLUE
            state = 0;
            break;
        default:
            state = 0;
            break;
    }
}

// This message gets sent when the bots money ammount changes (for CS)
void BotClient_CS_Money(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_CS_Money()\n");
    static int state = 0;        // current state machine state

    if (state == 0) {
        state++;

        bots[bot_index].bot_money = *static_cast<int*>(p);   // amount of money
    } else {
        state = 0;                // ingore this field
    }
}

// This message gets sent when the bots get killed
void BotClient_Valve_DeathMsg(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_Valve_DeathMsg()\n");
    static int state = 0;        // current state machine state
    static int killer_index;
    static int victim_index;

    if (state == 0) {
        state++;
        killer_index = *static_cast<int*>(p);        // ENTINDEX() of killer
    } else if (state == 1) {
        state++;
        victim_index = *static_cast<int*>(p);        // ENTINDEX() of victim
    } else if (state == 2) {
	    static int index;
	    static edict_t *victim_edict;
	    state = 0;

        victim_edict = INDEXENT(victim_index);

        index = UTIL_GetBotIndex(victim_edict);

        // is this message about a bot being killed?
        if (index != -1) {
            if ((killer_index == 0) || (killer_index == victim_index)) {
                // bot killed by world (worldspawn) or bot killed self...
                bots[index].killer_edict = nullptr;
            } else {
                // store edict of player that killed this bot...
                bots[index].killer_edict = INDEXENT(killer_index);
            }
        }
    }
}

void BotClient_CS_DeathMsg(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_CS_DeathMsg()\n");
    // this is just like the Valve DeathMsg message
    BotClient_Valve_DeathMsg(p, bot_index);
}

void BotClient_Gearbox_DeathMsg(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_Gearbox_DeathMsg()\n");
    // this is just like the Valve DeathMsg message
    BotClient_Valve_DeathMsg(p, bot_index);
}

void BotClient_FLF_DeathMsg(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_FLF_DeathMsg()\n");
    // this is just like the Valve DeathMsg message
    BotClient_Valve_DeathMsg(p, bot_index);
}

void BotClient_Valve_ScreenFade(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_Valve_ScreenFade()\n");
    static int state = 0;        // current state machine state
    static int duration;
    static int hold_time;

    if (state == 0) {
        state++;
        duration = *static_cast<int*>(p);
    } else if (state == 1) {
        state++;
        hold_time = *static_cast<int*>(p);
    } else if (state == 2) {
	    static int fade_flags;
	    state++;
        fade_flags = *static_cast<int*>(p);
    } else if (state == 6) {
        state = 0;

        float length = (static_cast<float>(duration + hold_time)) / 4096.0f;
        int iDevide = bots[bot_index].bot_skill;
        if (iDevide < 1)
            iDevide = 1;

        length -= (static_cast<float>(10) / static_cast<float>(iDevide) * 0.5f);

        bots[bot_index].fBlindedTime = gpGlobals->time + length;

        cBot* pBot = &bots[bot_index];

        const int iCurrentNode = NodeMachine.getClosestNode(pBot->pEdict->v.origin, NODE_ZONE, pBot->pEdict);
        const int iCoverNode = NodeMachine.node_cover(iCurrentNode, iCurrentNode, pBot->pEdict);

        if (iCoverNode > -1) {
		// pBot->forgetPath();
		// pBot->rprint("Setting goal from screenfade");
		// pBot->setGoalNode(iCoverNode);
            pBot->rprint("TODO: Make bot react upon screenfade/flashbang\n");
        }

    } else {
        state++;
    }
}

void BotClient_CS_ScreenFade(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_CS_ScreenFade()\n");
    // this is just like the Valve ScreenFade message
    BotClient_Valve_ScreenFade(p, bot_index);
}

// Not required? [APG]RoboCop[CL]
/*void BotClient_Gearbox_ScreenFade(void* p, int bot_index) {
    //DebugOut("bot_client: BotClient_Gearbox_ScreenFade()\n");
    // this is just like the Valve ScreenFade message
    BotClient_Valve_ScreenFade(p, bot_index);
}

void BotClient_FLF_ScreenFade(void *p, int bot_index) {
    //DebugOut("bot_client: BotClient_FLF_ScreenFade()\n");
    // this is just like the Valve ScreenFade message
    BotClient_Valve_ScreenFade(p, bot_index);
}*/

// STEFAN
// This message gets sent for HLTV, according to Alfred (Valve) also
// unique for each round-start.
void BotClient_CS_HLTV(void *p, int bot_index) {
    static int state = 0;        // current state machine state
    static int players = 0;

    /*
       From e-mail Alfred:

       // reset all players health for HLTV
       MESSAGE_BEGIN( MSG_SPEC, gmsgHLTV );
       WRITE_BYTE( 0 );   // 0 = all players
       WRITE_BYTE( 100 | 128 );
       MESSAGE_END();

       // reset all players FOV for HLTV
       MESSAGE_BEGIN( MSG_SPEC, gmsgHLTV );
       WRITE_BYTE( 0 );   // all players
       WRITE_BYTE( 0 );
       MESSAGE_END();

     */

    if (state == 0) {
        players = *static_cast<int*>(p);     // check the byte
        state++;
    } else if (state == 1) {
        // I check here on purpose. Multiple HLTV messages get sent within the game,
        // by checking for the second state i AND the 'all players' flag as above in Alfreds
        // code i hopefully eliminate all faulty 'new round' detections. Testing
        // has shown me that the state machine MUST be in, else you will get "New Round"
        // detections on strange occasions!

        // We could do some cool stuff here, but all i want to know is if the
        // round has (re)started, so i just set that flag and i am done.
        if (players == 0) {
            rblog("CS HLTV: Game new round\n");
            // New round started.
            Game.SetNewRound(true);
        }
        state = 0;
    }
}

// STEFAN - END
