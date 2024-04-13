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

extern int mod_id;
extern int counterstrike;

extern cGame Game;

// Radio code
extern bool radio_message;

weapon_price_table weapons_table[32];

void LoadDefaultWeaponTable() {                               /* REMOVED */
}

// Returns price of a weapon, checking on WEAPON_ID (of CS)
int PriceWeapon(int weapon_id) {
    return weapons_table[weapons_table[weapon_id].iIdIndex].price;
}

// Returns the ID in the list, checking on WEAPON_ID (of CS)
int ListIdWeapon(int weapon_id) {
    return weapons_table[weapon_id].iIdIndex;
}

// The bot will be buying this weapon
void BotPrepareConsoleCommandsToBuyWeapon(cBot *pBot, const char *arg1, const char *arg2) {
    // To be sure the console will only change when we MAY change.
    // The values will only be changed when console_nr is 0
    if (Game.getRoundStartedTime() + 4 < gpGlobals->time)
        return;                   // Not valid to buy

    if (pBot->console_nr == 0) {
        // set up first command and argument
        std::strcpy(pBot->arg1, "buy");
        std::strcpy(pBot->arg2, arg1);

        // add argument
        if (arg2 != nullptr)
            std::strcpy(pBot->arg3, arg2);

        pBot->console_nr = 1;     // start console command sequence
    }
}

/**
 * Determines if the weapon that will be bought, is valid to be bought
 * by specific team, cs version, etc. Returns TRUE if valid.
 * @param weapon
 * @param team
 * @return
 */
bool GoodWeaponForTeam(int weapon, int team) {
    // Mod CS:
    if (mod_id == CSTRIKE_DLL) {

        // When not playing Counter-Strike 1.6, these weapons are automaticly wrong for all teams.
        if (counterstrike == 0)
            if (weapon == CS_WEAPON_GALIL || weapon == CS_WEAPON_FAMAS
                || weapon == CS_WEAPON_SHIELD)
                return false;

        // Check the real thing
        if (team == 2)            // Counter-Terrorist
        {
            // 30/07/04 by Josh
            // Use a switch instead of multiple if
            switch (weapon) {
                case CS_WEAPON_SG552:
                    return false;
                case CS_WEAPON_AK47:
                    return false;
                case CS_WEAPON_DEAGLE:
                    return false;
                case CS_WEAPON_MP5NAVY:
                    return false;
                case CS_WEAPON_GALIL:
                    return false;
                case CS_WEAPON_P90:
                    return false;
                // 30.8.04 added by frashman
                case CS_WEAPON_G3SG1:
                    return false;
            }
        } else {
            switch (weapon) {
                case CS_WEAPON_AUG:
                    return false;
                case CS_WEAPON_DEAGLE:
                    return false;
                case CS_WEAPON_M4A1:
                    return false;
                case CS_WEAPON_MP5NAVY:
                    return false;
                case CS_WEAPON_FAMAS:
                    return false;
                case CS_WEAPON_P90:
                    return false;
                //30.8.04 added by Frashman
                case CS_WEAPON_SG550:
                    return false;
                case CS_DEFUSEKIT:
                    return false;
            }
        }
    }
    // yes bot, you may buy this weapon.
    return true;
}

/*
 BotDecideWhatToBuy()
 
 In this function the bot will choose what weapon to buy from the table.
 */
void BotDecideWhatToBuy(cBot *pBot) {
    /**
     * Stefan 02/09/2018
     *
     * This function is called multiple frames. And it basically checks boolean flags (buy_*) method, if true
     * then a weapon to buy is choosen. Once choosen it is bought. The flag which was 'true' now became 'false'.
     *
     * Then the next frame, the boolean flag before will become 'false' and thus another if block is executed.
     *
     * Effectively doing:
     * - buy primary weapon first
     * - then secondary
     * - ammo for primary
     * - ammo for secondary
     * - armor
     * - defuse kit
     * - etc.
     *
     * the order of if statements is leading
     *
     * In other words, it needs refactoring...
     *
     */
    const int money = pBot->bot_money; // Money
    const int team = pBot->iTeam;      // Team

    int buy_weapon = -1;

    // Buy a primary weapon, think of the best choice
    if (pBot->buy_primary) {
        pBot->rprint("BotDecideWhatToBuy()", "buy_primary");
        // Buy primary

        // Personality related:
        // Check if we can buy our favorite weapon
        if (pBot->hasFavoritePrimaryWeaponPreference()) {
            pBot->rprint("BotDecideWhatToBuy()", "I have a primary weapon preference");

            if (!pBot->ownsFavoritePrimaryWeapon()) {
                pBot->rprint("BotDecideWhatToBuy()", "I do not own my primary weapon preference");

                if (GoodWeaponForTeam(pBot->ipFavoPriWeapon, pBot->iTeam)) { // can we buy it for this team?

                    if (pBot->canAfford(PriceWeapon(pBot->ipFavoPriWeapon))) { // can we afford it?
                        // Buy favorite weapon!
                        pBot->rprint("BotDecideWhatToBuy()", "Can buy my favorite primary weapon, doing it");
                        buy_weapon = pBot->ipFavoPriWeapon;
                    } else {
                        pBot->rprint("BotDecideWhatToBuy()", "Can't afford my favorite primary weapon.");

                        // bot personality: if we want to save money for our favorite weapon, then set other values to false
                        if (RANDOM_LONG(0, 100) < pBot->ipSaveForWeapon) {    // 31.08.04 Frashman forgotten brace
                            pBot->rprint("Decided to save extra money");
                            pBot->buy_primary = false;
                            pBot->buy_secondary = false;       // don't buy a secondary
                            return;    // get out of function, don't buy anything
                        }
                    }
                }
            } else {
                pBot->rprint("BotDecideWhatToBuy()", "I already have my favorite primary weapon");
                // already have my favorite weapon
                pBot->buy_primary = false; // do not buy a primary weapon
                return;
            }
        }

        // not decided what to buy yet
        if (buy_weapon < 0) {
            pBot->rprint("BotDecideWhatToBuy()", "I have no primary weapon preference, deciding what to buy.");

            // Find weapon we can buy in the list of weapons
            for (const weapon_price_table& i : weapons_table)
            {

                // 31.08.04 Frashman Filter Out all except PRIMARY and SHIELD
                // SHIELD is used as primary weapon

                if (UTIL_GiveWeaponType(i.iId) != PRIMARY
	                && UTIL_GiveWeaponType(i.iId) != SHIELD)
                    continue;

                // must be a weapon that the team can buy (CT/T weapon)
                if (!GoodWeaponForTeam(i.iId, team))
                    continue;

                // can afford it
                if (i.price <= money) {

                    // owns a primary weapon
                    if (pBot->iPrimaryWeapon > -1) {
                        // and the primary weapon has a higher priority than the other primary weapon
                        if (weapons_table[ListIdWeapon(pBot->iPrimaryWeapon)].priority >= i.priority)
                            continue;
                    }

                    // nothing to buy yet, so chose this one
                    if (buy_weapon == -1) {
                        buy_weapon = i.iId;
                    } else {
                        // randomly overrule it based on priority. The higher priority the more chance
                        // it will be bought.
                        if (RANDOM_LONG(0, 100) < i.priority) {
                            buy_weapon = i.iId; // randomly buy a different weapon
                        }
                    }
                }
            }
        }

        pBot->buy_primary = false;

        // has decided which weapon to buy
        if (buy_weapon != -1) {
            pBot->rprint("BotDecideWhatToBuy()", "Found a primary weapon to buy");

            // depending on amount of money we have left buy *also* secondary weapon
            const int iMoneyLeft = money - PriceWeapon(buy_weapon);

            // TODO: this should be dependant on something else... not only money
            // 01.09.04 Frashman if buyed a Shield, try to buy a good Pistol
            if (iMoneyLeft >= 600)
                if (RANDOM_LONG(0, 100) < 15
                    || pBot->iPrimaryWeapon == CS_WEAPON_SHIELD)
                    pBot->buy_secondary = true;
        }
    } else if (pBot->buy_secondary) {
        pBot->rprint("BotDecideWhatToBuy()", "buy_secondary");
        // Buy secondary

        // Personality related:
        // Check if we can buy our favorite weapon
        if (pBot->hasFavoriteSecondaryWeaponPreference()) {
            pBot->rprint("BotDecideWhatToBuy()", "I have a secondary weapon preference");

            if (!pBot->ownsFavoriteSecondaryWeapon()) {
                pBot->rprint("BotDecideWhatToBuy()", "I do not own my secondary weapon preference");

                if (GoodWeaponForTeam(pBot->ipFavoSecWeapon, pBot->iTeam)) {
                    if (pBot->canAfford(pBot->ipFavoSecWeapon)) {
                        pBot->rprint("BotDecideWhatToBuy()", "I can afford my favorite secondary weapon, buying it now");
                        // Buy favorite weapon
                        buy_weapon = pBot->ipFavoPriWeapon;
                    } else {
                        pBot->rprint("BotDecideWhatToBuy()", "I cannot afford my favorite secondary weapon");

                        // do not buy a random secondary weapon - rather save money for it.
                        // We do here something to 'save' for our favorite weapon
                        if (RANDOM_LONG(0, 100) < pBot->ipSaveForWeapon) {    // 31.08.04 Frashman forgotten brace
                            pBot->rprint("I have decided to save money for my favorite secondary weapon");
                            pBot->buy_secondary = false;       // don't buy a secondary
                            return;    // get out of function, don't buy anything
                        }
                    }
                }
            } else {
                pBot->rprint("BotDecideWhatToBuy()", "I already own my favorite secondary weapon");
                // we already own it, do nothing
                return;
            }
        }

        // no weapon choosen to buy yet - and no preference
        if (buy_weapon < 0) {
            pBot->rprint("BotDecideWhatToBuy()", "Deciding which secondary weapon to buy");
            // Buy secondary
            // Find weapon we can buy in the list of weapons
            for (const weapon_price_table& i : weapons_table)
            {

                // When enough money and the priority is high enough..
                // Filter out Secondary and Grenades
                if (UTIL_GiveWeaponType(i.iId) != SECONDARY)
                    continue;

                if (GoodWeaponForTeam(i.iId, team) == false)
                    continue;

                if (i.price <= money) {
                    if (pBot->iSecondaryWeapon > -1) {
	                    const int index =
                                weapons_table[pBot->iSecondaryWeapon].iIdIndex;
                        // 31.08.04 Frashman > corrected to >= ,
                        // else the bot will buy another weapon with the same priority
                        if (weapons_table[index].priority >=
	                        i.priority)
                            continue;
                    }

                    if (buy_weapon == -1)
                        buy_weapon = i.iId;
                    else {
                        if (RANDOM_LONG(0, 100) < i.priority)
                            buy_weapon = i.iId;
                    }
                    if (RANDOM_LONG(0, 100) < i.priority)
                        break;
                }
            }
        }

        pBot->buy_secondary = false;

        // found a secondary weapon to buy
        if (buy_weapon != -1) {
            pBot->rprint("Found a secondary weapon to buy");
        }
    } else if (pBot->buy_ammo_primary == true) {
        pBot->rprint("BotDecideWhatToBuy()", "buy_ammo_primary");
        // Buy primary ammo
        BotPrepareConsoleCommandsToBuyWeapon(pBot, "6", nullptr);
        pBot->buy_ammo_primary = false;
        return;

    } else if (pBot->buy_ammo_secondary == true) {
        pBot->rprint("BotDecideWhatToBuy()", "buy_ammo_secondary");
        // Buy secondary ammo
        BotPrepareConsoleCommandsToBuyWeapon(pBot, "7", nullptr);
        pBot->buy_ammo_secondary = false;
        return;
    } else if (pBot->buy_defusekit) {
        pBot->rprint("BotDecideWhatToBuy()", "buy_defusekit");
        pBot->buy_defusekit = false;
        if (money >= 200) {
            buy_weapon = CS_DEFUSEKIT;
        }
    } else if (pBot->buy_armor) {
        pBot->rprint("BotDecideWhatToBuy()", "buy_armor");
        if (money < 1000 && money >= 650) {
            // Buy light armor
            buy_weapon = CS_WEAPON_ARMOR_LIGHT;
        } else if (money >= 1000) {
            // Buy heavy armor
            buy_weapon = CS_WEAPON_ARMOR_HEAVY;
        }
        pBot->buy_armor = false;
    } else if (pBot->buy_grenade) {
        pBot->rprint("BotDecideWhatToBuy()", "buy_grenade");
        // Buy grenade
        if (money >= weapons_table[ListIdWeapon(CS_WEAPON_HEGRENADE)].price) {
            buy_weapon = CS_WEAPON_HEGRENADE;
        }

        pBot->buy_grenade = false;
    } else if (pBot->buy_flashbang > 0) {
        pBot->rprint("BotDecideWhatToBuy()", "buy_flashbang");
        // Buy flashbang
        if (money >= weapons_table[ListIdWeapon(CS_WEAPON_FLASHBANG)].price) {
            buy_weapon = CS_WEAPON_FLASHBANG;
            pBot->buy_flashbang--;
        } else {
            pBot->buy_flashbang = 0;       // do not buy
        }
    } else if (pBot->buy_smokegrenade)   //31.08.04 Frashman added Smoke Grenade support
    {
        pBot->rprint("BotDecideWhatToBuy()", "buy_smokegrenade");
        // Buy SmokeGrenade
        if (money >= weapons_table[ListIdWeapon(CS_WEAPON_SMOKEGRENADE)].price) {
            buy_weapon = CS_WEAPON_SMOKEGRENADE;
        }

        pBot->buy_smokegrenade = false;
    }

    // Perform the actual buy commands to acquire weapon
    pBot->performBuyActions(buy_weapon);
}

/*
 ConsoleThink()
 
 This function is important to bots. The bot will actually execute any console command
 given right here. This also activates the buy behaviour at the start of a round.
*/
void ConsoleThink(cBot *pBot) {
    // RealBot only supports Counter-Strike
    if (mod_id != CSTRIKE_DLL) return;
    if (pBot->isUsingConsole()) return; // busy executing console commands, so do not decide anything else

    // buy time is in minutes, we need
    // gpGlobals->time is in seconds, so we need to translate the minutes into seconds
    const float buyTime = CVAR_GET_FLOAT("mp_buytime") * 60;
    if (Game.getRoundStartedTime() + buyTime > gpGlobals->time &&
        pBot->wantsToBuyStuff()) {
        BotDecideWhatToBuy(pBot);
    }
    // Buying code in CS
}

////////////////////////////////////////////////////////////////////////////////
/// Console Handling by Bots
////////////////////////////////////////////////////////////////////////////////
void BotConsole(cBot *pBot) {
    // Nothing to execute and alive, think about any console action to be taken.
    if (pBot->console_nr == 0 && pBot->pEdict->v.health > 0) {
        if (pBot->f_console_timer <= gpGlobals->time) {
            pBot->arg1[0] = 0;     // clear
            pBot->arg2[0] = 0;     // the
            pBot->arg3[0] = 0;     // variables
        }
        ConsoleThink(pBot);       // Here it will use the console, think about what to do with it
    }

    // Here the bot will excecute the console commands if the console counter has been set/changed
    if (pBot->console_nr != 0 && pBot->f_console_timer < gpGlobals->time) {
        // safety net
        if (pBot->console_nr < 0)
            pBot->console_nr = 0;  // Set it to 0

        // issue command (buy/radio)
        if (pBot->console_nr == 1)
            FakeClientCommand(pBot->pEdict, pBot->arg1, nullptr, nullptr);

        // do menuselect
        if (pBot->console_nr == 2)
            FakeClientCommand(pBot->pEdict, "menuselect", pBot->arg2, nullptr);

        // do menuselect
        if (pBot->console_nr == 3) {
            // When the last parameter is not null, we will perform that action.
            if (pBot->arg3[0] != 0)
                FakeClientCommand(pBot->pEdict, "menuselect", pBot->arg3,
                nullptr);

            // reset
            pBot->console_nr = -1;
            pBot->f_console_timer = gpGlobals->time + RANDOM_FLOAT(0.2f, 0.5f);
        }

        if (pBot->console_nr > 0)
            pBot->console_nr++;    // Increase command

        //return;
    }

}                               // BotConsole()
