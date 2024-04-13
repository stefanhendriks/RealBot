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
#include "bot_weapons.h"
#include "bot_func.h"
#include "NodeMachine.h"

extern cBot bots[32];
extern int mod_id;

extern int m_spriteTexture;

// defined in dll.cpp
extern FILE *fpRblog;

//
extern cNodeMachine NodeMachine;

// For taking cover decision
#define TOTAL_SCORE 16300       // 16000 money + 100 health + 100 fear + 100 camp desire

bool VectorIsVisibleWithEdict(edict_t *pEdict, const Vector& dest, const char *checkname) {
    TraceResult tr;

    const Vector start = pEdict->v.origin + pEdict->v.view_ofs;

    // trace a line from bot's eyes to destination...
    UTIL_TraceLine(start, dest, ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

    // When our check string is not "none" and the traceline has a hit...
    if (std::strcmp("none", checkname) != 0 && tr.flFraction < 1.0f) {
        // Check if the blocking entity is same as checkname..
        char entity_blocker[128];
        const edict_t *pent = tr.pHit;  // Ok now retrieve the entity
        std::strcpy(entity_blocker, STRING(pent->v.classname));        // the classname

        if (std::strcmp(entity_blocker, checkname) == 0)
            return true;           // We are blocked by our string, this means its ok.
        else {
            return false;          // We are blocked, but by something differernt then 'checkname' its not ok
        }

    } else {
        // check if line of sight to object is not blocked (i.e. visible)
        if (tr.flFraction >= 1.0f)
            return true;
        else
            return false;
    }

    //return false;

}

bool VectorIsVisible(const Vector& start, const Vector& dest, char *checkname) {
    TraceResult tr;

    // trace a line from bot's eyes to destination...
    UTIL_TraceLine(start, dest, dont_ignore_monsters, nullptr, &tr);

    // Als we geblokt worden EN we checken voor een naam
    if (std::strcmp("none", checkname) != 0 && tr.flFraction < 1.0f) {
        // Check if the blocking entity is same as checkname..
        char entity_blocker[128];
        const edict_t *pent = tr.pHit;  // Ok now retrieve the entity
        std::strcpy(entity_blocker, STRING(pent->v.classname));        // the classname

        if (std::strcmp(entity_blocker, checkname) == 0)
            return false;          // We worden geblokt door die naam..
        else
            return true;           // We worden NIET geblokt door die naam (dus we worden niet geblokt).

    } else {
        // check if line of sight to object is not blocked (i.e. visible)
        // Als er NONE wordt opgegeven dan checken we gewoon of we worden geblokt
        if (tr.flFraction >= 1.0f)
            return true;
        else
            return false;

    }

}

float func_distance(Vector v1, Vector v2) {
    // Returns distance between 2 vectors
    if (v1 && v2)
        return (v1 - v2).Length();
    else
        return 0.0f;
}

/**
 * return the absolute value of angle to destination entity (in degrees).
 * Zero degrees means straight ahead,  45 degrees to the left or
 * 45 degrees to the right is the limit of the normal view angle
 * @param pEntity
 * @param dest
 * @return
 */
int FUNC_InFieldOfView(edict_t *pEntity, const Vector& dest) {
    // NOTE: Copy from Botman's BotInFieldOfView() routine.
    // find angles from source to destination...
    Vector entity_angles = UTIL_VecToAngles(dest);

    // make yaw angle 0 to 360 degrees if negative...
    if (entity_angles.y < 0)
        entity_angles.y += 360;

    // get bot's current view angle...
    float view_angle = pEntity->v.v_angle.y;

    // make view angle 0 to 360 degrees if negative...
    if (view_angle < 0)
        view_angle += 360;

    // return the absolute value of angle to destination entity
    // zero degrees means straight ahead,  45 degrees to the left or
    // 45 degrees to the right is the limit of the normal view angle

    // rsm - START angle bug fix
    int angle = std::abs(static_cast<int>(view_angle) - static_cast<int>(entity_angles.y));

    if (angle > 180)
        angle = 360 - angle;

    return angle;
    // rsm - END
}

/**
 * Shorthand function
 * @param visibleForWho
 * @param start
 * @param end
 */
void DrawBeam(edict_t *visibleForWho, const Vector& start, const Vector& end) {
    DrawBeam(visibleForWho, start, end, 25, 1, 255, 255, 255, 255, 1);
}

/**
 * Shorthand function
 * @param visibleForWho
 * @param start
 * @param end
 * @param r
 * @param g
 * @param b
 */
void DrawBeam(edict_t *visibleForWho, const Vector& start, const Vector& end, int r, int g, int b) {
    DrawBeam(visibleForWho, start, end, 25, 1, r, g, b, 255, 1);
}

/**
 * This function draws a beam , used for debugging all kinds of vector related things.
 * @param visibleForWho
 * @param start
 * @param end
 * @param width
 * @param noise
 * @param red
 * @param green
 * @param blue
 * @param brightness
 * @param speed
 */
void DrawBeam(edict_t *visibleForWho, const Vector& start, const Vector& end, int width,
              int noise, int red, int green, int blue, int brightness,
              int speed) {
    MESSAGE_BEGIN(MSG_ONE, SVC_TEMPENTITY, nullptr, visibleForWho);
    WRITE_BYTE(TE_BEAMPOINTS);
    WRITE_COORD(start.x);
    WRITE_COORD(start.y);
    WRITE_COORD(start.z);
    WRITE_COORD(end.x);
    WRITE_COORD(end.y);
    WRITE_COORD(end.z);
    WRITE_SHORT(m_spriteTexture);
    WRITE_BYTE(1);               // framestart
    WRITE_BYTE(10);              // framerate
    WRITE_BYTE(10);              // life in 0.1's
    WRITE_BYTE(width);           // width
    WRITE_BYTE(noise);           // noise

    WRITE_BYTE(red);             // r, g, b
    WRITE_BYTE(green);           // r, g, b
    WRITE_BYTE(blue);            // r, g, b

    WRITE_BYTE(brightness);      // brightness
    WRITE_BYTE(speed);           // speed
    MESSAGE_END();
}

/**
 * Gets a bot close (NODE_ZONE distance)
 * @param pBot
 * @return
 */
cBot *getCloseFellowBot(cBot *pBot) {
	const edict_t *pEdict = pBot->pEdict;
    cBot *closestBot = nullptr;

    // Loop through all clients
    for (int i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t *pPlayer = INDEXENT(i);

        // skip invalid players
        if (pPlayer && !pPlayer->free && pPlayer != pEdict) {
            // skip this player if not alive (i.e. dead or dying)
            if (!IsAlive(pPlayer))
                continue;

            cBot *pBotPointer = UTIL_GetBotPointer(pPlayer);
            // skip anything that is not a RealBot
            if (pBotPointer == nullptr)       // not using FL_FAKECLIENT here so it is multi-bot compatible
                continue;

            if (func_distance(pBot->pEdict->v.origin, pPlayer->v.origin) < NODE_ZONE) {
                closestBot = pBotPointer;    // set pointer
            }
        }
    }

    return closestBot;               // return result (either NULL or bot pointer)
}

/**
 * Return TRUE of any players are near that could block him and which are within FOV
 * @param pBot
 * @return
 */
edict_t * getPlayerNearbyBotInFOV(cBot *pBot) {
	const edict_t *pEdict = pBot->pEdict;

    for (int i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t *pPlayer = INDEXENT(i);

        // skip invalid players and skip self (i.e. this bot)
        if (pPlayer && !pPlayer->free && pPlayer != pEdict) {
	        constexpr int fov = 90;// TODO: use server var "default_fov" ?
	        // skip this player if not alive (i.e. dead or dying)
            if (!IsAlive(pPlayer))
                continue;

            if (!(pPlayer->v.flags & FL_THIRDPARTYBOT
	            || pPlayer->v.flags & FL_FAKECLIENT
	            || pPlayer->v.flags & FL_CLIENT))
                continue;

            const int angleToPlayer = FUNC_InFieldOfView(pBot->pEdict, pPlayer->v.origin - pBot->pEdict->v.origin);

	        constexpr int distance = NODE_ZONE;
            if (func_distance(pBot->pEdict->v.origin, pPlayer->v.origin) < distance && angleToPlayer < fov) {
                return pPlayer;
            }
        }
    }
    return nullptr;
}

/**
 * Return TRUE of any players are near that could block him and which are within FOV
 * @param pBot
 * @return
 */
edict_t * getEntityNearbyBotInFOV(cBot *pBot) {
    edict_t *pEdict = pBot->pEdict;

    edict_t *pent = nullptr;
    while ((pent = UTIL_FindEntityInSphere(pent, pEdict->v.origin, 45)) != nullptr) {
        if (pent == pEdict) continue; // skip self

        if (FInViewCone(&pent->v.origin, pEdict)) {
            return pent; // yes it is the case
        }
    }
    return nullptr;
}

/**
 * Return TRUE of any players are near that could block him, regardless of FOV. Just checks distance
 * @param pBot
 * @return
 */ //TODO: FOV and angleToPlayer are unused variables [APG]RoboCop[CL]
bool isAnyPlayerNearbyBot(cBot *pBot) {
	const edict_t *pEdict = pBot->pEdict;
    //int fov = 105;

    for (int i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t *pPlayer = INDEXENT(i);

        // skip invalid players and skip self (i.e. this bot)
        if (pPlayer && !pPlayer->free && pPlayer != pEdict) {
            // skip this player if not alive (i.e. dead or dying)
            if (!IsAlive(pPlayer))
                continue;

            if (!(pPlayer->v.flags & FL_THIRDPARTYBOT
	            || pPlayer->v.flags & FL_FAKECLIENT
	            || pPlayer->v.flags & FL_CLIENT))
                continue;

            //int angleToPlayer = FUNC_InFieldOfView(pBot->pEdict, (pPlayer->v.origin - pBot->pEdict->v.origin));

            constexpr int distance = NODE_ZONE;
            if (func_distance(pBot->pEdict->v.origin, pPlayer->v.origin) < distance) {
                return true;
            }
        }
    }
    return false;
}

/**
 * Returns true if bot should jump, deals with func_illusionary
 * @param pBot
 * @return
 */
bool BotShouldJumpIfStuck(cBot *pBot) {
    if (pBot->isDefusing()) {
        pBot->rprint_trace("BotShouldJumpIfStuck", "Returning false because defusing.");
        return false;
    }

    if (pBot->isJumping()) return false; // already jumping

    if (pBot->iJumpTries > 5) {
        char msg[255];
        snprintf(msg, sizeof(msg), "Returning false because jumped too many times (%d)", pBot->iJumpTries);
        pBot->rprint_trace("BotShouldJumpIfStuck", msg);
        return false;
    }

    const bool result = BotShouldJump(pBot);
    if (result) {
        pBot->rprint_trace("BotShouldJumpIfStuck", "BotShouldJump returns true, so returning that");
        return true;
    }

    // should not jump, perhaps its a func_illusionary causing that we're stuck?
    const edict_t *entityInFov = getEntityNearbyBotInFOV(pBot);

    if (entityInFov && std::strcmp("func_illusionary", STRING(entityInFov->v.classname)) == 0) {
        return true; // yes it is the case
    }

    return false; // no need to jump
}

/**
 * Returns true if bot should jump. Does *not* deal with func_illusionary.
 * @param pBot
 * @return
 */
bool BotShouldJump(cBot *pBot) {
    // When a bot should jump, something is blocking his way.
    // Most of the time it is a fence, or a 'half wall' that reaches from body to feet
    // However, the body most of the time traces above this wall.
    // What i do:
    // Get position of bot
    // Get position of legs
    // Trace
    // If blocked, then we SHOULD jump

    if (pBot->isDefusing()) {
        pBot->rprint_trace("BotShouldJump", "Returning false because defusing.");
        return false;
    }

    if (pBot->isJumping()) 
        return false; // already jumping

    TraceResult tr;
    const edict_t *pEdict = pBot->pEdict;

    // convert current view angle to vectors for TraceLine math...

    Vector v_jump = FUNC_CalculateAngles(pBot);
    v_jump.x = 0;                // reset pitch to 0 (level horizontally)
    v_jump.z = 0;                // reset roll to 0 (straight up and down)

    UTIL_MakeVectors(v_jump);

    // Check if we can jump onto something with maximum jump height:
    // maximum jump height, so check one unit above that (MAX_JUMPHEIGHT)
    Vector v_source = pEdict->v.origin + Vector(0, 0, -CROUCHED_HEIGHT + (MAX_JUMPHEIGHT + 1));
    Vector v_dest = v_source + gpGlobals->v_forward * 90;

    // trace a line forward at maximum jump height...
    UTIL_TraceHull(v_source, v_dest, dont_ignore_monsters, point_hull, pEdict->v.pContainingEntity, &tr);

    // if trace hit something, return FALSE
    if (tr.flFraction < 1.0f) {
        pBot->rprint_trace("BotShouldJump", "I cannot jump because something is blocking the max jump height");
        return false;
    }
    pBot->rprint_trace("BotShouldJump", "I can make the jump, nothing blocking the jump height");

    // Ok the body is clear
    v_source = pEdict->v.origin + Vector(0, 0, ORIGIN_HEIGHT);
    v_dest = v_source + gpGlobals->v_forward * 90;

    // trace a line forward at maximum jump height...
    UTIL_TraceHull(v_source, v_dest, dont_ignore_monsters, point_hull, pEdict->v.pContainingEntity, &tr);

    if (tr.flFraction < 1.0f) {
        pBot->rprint_trace("BotShouldJump", "Cannot jump because body is blocked");
        return false;
    }
    pBot->rprint_trace("BotShouldJump", "Jump body is not blocked");

    // Ok the body is clear
    v_source = pEdict->v.origin + Vector(0, 0, -14); // 14 downwards (from center) ~ roughly the kneecaps
    v_dest = v_source + gpGlobals->v_forward * 40;

    //
	// int player_index = 0;
	//    for (player_index = 1; player_index <= gpGlobals->maxClients;
	//         player_index++) {
	//        edict_t *pPlayer = INDEXENT(player_index);
	//
	//        if (pPlayer && !pPlayer->free) {
	//            if (FBitSet(pPlayer->v.flags, FL_CLIENT)) { // do not draw for now
	//
	//                DrawBeam(
	//                        pPlayer, // player sees beam
	//                        v_source, // + Vector(0, 0, 32) (head?)
	//                        v_dest,
	//                        255, 255, 255
	//                );
	//            }
	//        }
	//    }

    UTIL_TraceHull(v_source, v_dest, dont_ignore_monsters, point_hull, pEdict->v.pContainingEntity, &tr);

    if (tr.flFraction < 1.0f) {
        pBot->rprint_trace("BotShouldJump", "Yes should jump, kneecaps hit something, so it is jumpable");
        return true;
    }

    // "func_illusionary" - although on cs_italy this is not detected, and probably in a lot of other cases as well
    if (tr.pHit) {
        pBot->rprint_trace("trace pHit", STRING(tr.pHit->v.classname));
        if (std::strcmp("func_illusionary", STRING(tr.pHit->v.classname)) == 0) {
        pBot->rprint_trace("BotShouldJump", "#1 Hit a func_illusionary, its a hit as well! (even though trace hit results no)");
        return true;
        }
    }

    pBot->rprint_trace("BotShouldJump", "No, should not jump");
    return false;
}

// FUNCTION: Calculates angles as pEdict->v.v_angle should be when checking for body
Vector FUNC_CalculateAngles(const cBot *pBot) {
    // aim for the head and/or body
    const Vector v_target = pBot->vBody - pBot->pEdict->v.origin;
    Vector v_body = UTIL_VecToAngles(v_target);

    if (v_body.y > 180)
        v_body.y -= 360;

    // Paulo-La-Frite - START bot aiming bug fix
    if (v_body.x > 180)
        v_body.x -= 360;

    // adjust the view angle pitch to aim correctly (MUST be after body v.angles stuff)
    v_body.x = -v_body.x;

    // Paulo-La-Frite - END
    return v_body;
}

bool BotShouldDuck(cBot *pBot) {
    // temp
    // TODO: Deal with this, is it good code? remove the other stuff below the return statement?

    if (pBot->iDuckTries > 3) {
        // tried to duck 3 times, so no longer!
        pBot->rprint_trace("BotShouldDuck", "Returning false because ducked too many times.");
        return false;
    }
	
    //return BotCanDuckUnder(pBot);  //Makes the code underneath unreachable? [APG]RoboCop[CL]

    // When a bot should jump, something is blocking his way.
    // Most of the time it is a fence, or a 'half wall' that reaches from body to feet
    // However, the body most of the time traces above this wall.
    // What i do:
    // Trace line from head
    // When head blocked and waist is free, then we should duck...

    TraceResult tr;
    const edict_t *pEdict = pBot->pEdict;

    // convert current view angle to vectors for TraceLine math...

    Vector v_duck = FUNC_CalculateAngles(pBot);
    v_duck.x = 0;                // reset pitch to 0 (level horizontally)
    v_duck.z = 0;                // reset roll to 0 (straight up and down)

    UTIL_MakeVectors(v_duck);

    // Check if head is blocked
    Vector v_source = pEdict->v.origin + Vector(0, 0, +37);
    Vector v_dest = v_source + gpGlobals->v_forward * 24;

    // trace a line forward at maximum jump height...
    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

    if (tr.flFraction >= 1.0f)
        return false;

    v_source = pEdict->v.origin;
    v_dest = v_source + gpGlobals->v_forward * 24;

    // trace a line forward at maximum jump height...
    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters,
                   pEdict->v.pContainingEntity, &tr);

    if (tr.flFraction < 1.0f)
        return false;

    return true;
}

bool BotShouldDuckJump(cBot* pBot) //Experimental DuckJump Incomplete [APG]RoboCop[CL]
{
	// This is crucial for bots to sneak inside vents and tight areas in order
	// to inflitrate and prooceed on ahead. DuckJump is required for vaulting
	// on top of crates, window ledges and edges as an important method.

    if (pBot->isDefusing()) {
        pBot->rprint_trace("BotShouldDuckJump", "Returning false because defusing.");
        return false;
    }

    if (pBot->iDuckJumpTries > 5) {
	    // tried to duck 5 times, so no longer!
    	pBot->rprint_trace("BotShouldDuck", "Returning false because ducked too many times.");
    	return false;
    }
	
    if (pBot->isDuckJumping()) 
        return false; // already duckjumping
	
    return false;
}

/**
 * Returns if a bot can and wants to do radio. Wanting is based on personality flag.
 * @param pBot
 * @return
 */
bool FUNC_DoRadio(const cBot *pBot) {

    if (pBot->fDoRadio > gpGlobals->time) // allowed?
        return false;

    const int iRadio = pBot->ipCreateRadio;

    return RANDOM_LONG(0, 100) < iRadio; // want?
}

// DECIDE: Take cover or not
bool FUNC_ShouldTakeCover(cBot *pBot) {
    // Do not allow taking cover within 3 seconds again.
    if (pBot->f_cover_time + 3 > gpGlobals->time)
        return false;

    if (!pBot->hasEnemy())
        return false;

    // wait 3 seconds before deciding again
    pBot->f_cover_time = gpGlobals->time;

    // MONEY: The less we have, the more we want to take cover
    const int vMoney = 16000 - pBot->bot_money;

    // HEALTH: The less we have, the more we want to take cover
    const int vHealth = 100 - pBot->bot_health;

    // CAMP: The more we want, the more we want to take cover
    const int vCamp = pBot->ipCampRate;

    return RANDOM_LONG(0, TOTAL_SCORE) < vMoney + vHealth + vCamp;
}

bool FUNC_TakeCover(cBot* pBot) //Experimental [APG]RoboCop[CL]
{
    // If we are not allowed to take cover, return false
    if (!FUNC_ShouldTakeCover(pBot))
        return false;

    return true;
}

int FUNC_BotEstimateHearVector(cBot *pBot, const Vector& v_sound) {
    // here we normally figure out where to look at when we hear an enemy, RealBot AI PR 2 lagged a lot on this so we need another approach

    return -1;
}

// Added Stefan
// 7 November 2001
int FUNC_PlayerSpeed(const edict_t *edict) {
    if (edict != nullptr)
        return static_cast<int>(edict->v.velocity.Length2D());      // Return speed of any edict given

    return 0;
}

bool FUNC_PlayerRuns(int speed) {
    if (speed < 200)
        return false;               // We make no sound
	
    return true;                    // We make sound
}

// return weapon type of edict.
// only when 'important enough'.
int FUNC_EdictHoldsWeapon(const edict_t *pEdict) {
    // sniper guns
    //if (strcmp("models/p_awp.mdl", STRING(pEdict->v.weaponmodel)) == 0) //Excluded for high prices and accuracy [APG]RoboCop[CL]
    //    return CS_WEAPON_AWP;
    if (std::strcmp("models/p_scout.mdl", STRING(pEdict->v.weaponmodel)) == 0)
        return CS_WEAPON_SCOUT;

    // good weapons (ak, m4a1, mp5)
    if (std::strcmp("models/p_ak47.mdl", STRING(pEdict->v.weaponmodel)) == 0)
        return CS_WEAPON_AK47;
    if (std::strcmp("models/p_m4a1.mdl", STRING(pEdict->v.weaponmodel)) == 0)
        return CS_WEAPON_M4A1;
    if (std::strcmp("models/p_mp5navy.mdl", STRING(pEdict->v.weaponmodel)) == 0)
        return CS_WEAPON_MP5NAVY;

    // grenade types
    if (std::strcmp("models/p_smokegrenade.mdl", STRING(pEdict->v.weaponmodel)) == 0)
        return CS_WEAPON_SMOKEGRENADE;
    if (std::strcmp("models/p_hegrenade.mdl", STRING(pEdict->v.weaponmodel)) == 0)
        return CS_WEAPON_HEGRENADE;
    if (std::strcmp("models/p_flashbang.mdl", STRING(pEdict->v.weaponmodel)) == 0)
        return CS_WEAPON_FLASHBANG;

    // shield types //Most CS Veterans dislikes the shield [APG]RoboCop[CL]
    //if (strcmp("models/p_shield.mdl", STRING(pEdict->v.weaponmodel)) == 0)
    //    return CS_WEAPON_SHIELD;

    // unknown
    return -1;
}

int FUNC_FindFarWaypoint(cBot* pBot, const Vector& avoid, bool safest) //Experimental [APG]RoboCop[CL]
{
	// Find a waypoint that is far away from the enemy.
	// If safest is true, then we want the safest waypoint.
	// If safest is false, then we want the farthest waypoint.

	// Find the farthest waypoint
	int farthest = -1;
	float farthest_distance = 0.0f;

	for (int i = 0; i < gpGlobals->maxEntities; i++) {
		const edict_t *pEdict = INDEXENT(i);

		if (pEdict == nullptr)
			continue;

		if (pEdict->v.flags & FL_DORMANT)
			continue;

        if (pEdict->v.classname != 0 && std::strcmp(STRING(pEdict->v.classname), "info_waypoint") == 0) {
            if (farthest == -1) {
                farthest = i;
                farthest_distance = (pEdict->v.origin - pBot->pEdict->v.origin).Length();
            } else {
                const float distance = (pEdict->v.origin - pBot->pEdict->v.origin).Length();

                if (safest) {
                    if (distance < farthest_distance) {
                        farthest = i;
                        farthest_distance = distance;
                    }
                } else {
                    if (distance > farthest_distance) {
                        farthest = i;
                        farthest_distance = distance;
                    }
                }
            }
        }
	}

	return farthest;
}

int FUNC_FindCover(const cBot* pBot) //Experimental [APG]RoboCop[CL]
{
	// Find a waypoint that is far away from the enemy.
	// If safest is true, then we want the safest waypoint.
	// If safest is false, then we want the farthest waypoint.

	// Find the farthest waypoint
	int farthest = -1;
	float farthest_distance = 0.0f;

	for (int i = 0; i < gpGlobals->maxEntities; i++) {
		const edict_t *pEdict = INDEXENT(i);

		if (pEdict == nullptr)
			continue;

		if (pEdict->v.flags & FL_DORMANT)
			continue;

        if (pEdict->v.classname != 0 && std::strcmp(STRING(pEdict->v.classname), "info_waypoint") == 0) {
            if (farthest == -1) {
                farthest = i;
                farthest_distance = (pEdict->v.origin - pBot->pEdict->v.origin).Length();
            } else {
                const float distance = (pEdict->v.origin - pBot->pEdict->v.origin).Length();

                if (distance > farthest_distance) {
                    farthest = i;
                    farthest_distance = distance;
                }
            }
        }
	}

	return farthest;
}

// Function to let a bot react on some sound which he cannot see
void FUNC_HearingTodo(cBot *pBot) {
    // This is called every frame.
    if (pBot->f_hear_time > gpGlobals->time)
        return;                   // Do nothing, we need more time to think

    if (pBot->f_camp_time > gpGlobals->time)
        return;

    if (pBot->f_wait_time > gpGlobals->time)
        return;

    // I HEAR SOMETHING
    // More chance on getting to true
    const int health = pBot->bot_health;

    int action;
    float etime;

    if (health < 25)
        action = 2;
    else if (health >= 25 && health < 75)
        action = 1;
    else
        action = -1;

    if (action == 0) {
        etime = RANDOM_LONG(2, 6);
        pBot->f_camp_time = gpGlobals->time + etime;
        pBot->forgetGoal();
    } else if (action == 1) {
        etime = RANDOM_LONG(1, 7);
        pBot->f_walk_time = gpGlobals->time + etime;
    } else if (action == 2) {
        etime = RANDOM_LONG(1, 5);
        pBot->f_hold_duck = gpGlobals->time + etime;
    }

    pBot->f_hear_time = gpGlobals->time + 6;     // Always keep a 6 seconds
    // think time.
}

/*
 *	FUNC    : FUNC_ClearEnemyPointer(edict)
 *	Author  : Stefan Hendriks
 *  Function: Removes all pointers to a certain edict.
 *  Created : 16/11/2001
 *	Changed : 16/11/2001
 */
void FUNC_ClearEnemyPointer(edict_t *pPtr) { //pPtr muddled with c_pointer? [APG]RoboCop[CL]
    // Go through all bots and remove their enemy pointer that matches the given
    // pointer pPtr
    for (int i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t *pPlayer = INDEXENT(i);

        // Skip invalid players.
        if (pPlayer && !pPlayer->free) {

            // skip this player if not alive (i.e. dead or dying)
            if (!IsAlive(pPlayer))
                continue;

            // skip human players
            if (!(pPlayer->v.flags & FL_THIRDPARTYBOT))
                continue;

            // check if it is a bot known to us (ie, not another metamod supported bot)
            cBot *botpointer = UTIL_GetBotPointer(pPlayer);

            if (botpointer &&                   // Is a bot managed by us
                botpointer->hasEnemy(pPtr) // and has the pointer we want to get rid of
                    ) {
                botpointer->forgetEnemy();    // Clear its pointer
            }
        }
    }
}

// Returns true/false if an entity is on a ladder
bool FUNC_IsOnLadder(const edict_t *pEntity) {
    if (pEntity == nullptr)
        return false;

    if (pEntity->v.movetype == MOVETYPE_FLY)
        return true;

    return false;
}

void FUNC_FindBreakable(edict_t* pEntity) //TODO: not functioning, bots won't shoot windows nor vent doors to proceed [APG]RoboCop[CL]
{
	// The "func_breakable" entity required for glass breaking and weak doors for bots to recognise,
	// in order to attack breakable objects that would block their way.
	
	for (int i = 0; i < gpGlobals->maxEntities; i++) {
		edict_t* pEdict = INDEXENT(i);

		if (pEdict == nullptr)
			continue;

		if (pEdict->v.flags & FL_DORMANT)
			continue;

        if (pEdict->v.classname != 0 && std::strcmp(STRING(pEdict->v.classname), "func_breakable") == 0) {
            if (pEdict->v.origin == pEntity->v.origin) {
                pEntity->v.enemy = pEdict;
                return;
            }
        }
	}
}

/*bool IsShootableBreakable(edict_t* pent)  // KWo - 08.02.2006
{
    if (pent == nullptr)
        return false;

    return (FStrEq("func_breakable", STRING(pent->v.classname))
        && (pent->v.playerclass == 1 || pent->v.health == 0
            || pent->v.health > 1 && pent->v.health < 1000
            || pent->v.rendermode == 4) // KWo - 21.02.2006 - br. crates has rendermode 4
        || FStrEq("func_pushable", STRING(pent->v.classname))
        && pent->v.health < 1000 && pent->v.spawnflags & SF_PUSH_BREAKABLE)  // KWo - 03.02.2007
        && pent->v.impulse == 0
        && pent->v.takedamage > 0
        && !(pent->v.spawnflags & SF_BREAK_TRIGGER_ONLY);
}*/

void FUNC_CheckForBombPlanted(edict_t* pEntity) //Experimental [APG]RoboCop[CL]
{
	// Check if the bot has a bomb planted.
	// If so, then we need to go to the bomb site.
	// If not, then we need to go to the waypoint.
	
    // "models/w_c4.mdl" needed for CTs to see the bomb? [APG]RoboCop[CL]
	if (pEntity->v.model != 0 && std::strcmp(STRING(pEntity->v.model), "models/w_c4.mdl") == 0) {
		// Bot has a bomb planted.
		// Go to the bomb site.
		pEntity->v.button |= IN_USE;
		pEntity->v.button |= IN_ATTACK;
	} else {
		// Bot does not have a bomb planted.
		// Go to the waypoint.
		pEntity->v.button |= IN_USE;
	}
}

/**
 * Returns true when hostage is not marked as being rescued by any other alive bot.
 *
 * @param pBotWhoIsAsking
 * @param pHostage
 * @return
 */
bool isHostageFree(cBot* pBotWhoIsAsking, edict_t* pHostage) {
    if (pHostage == nullptr) return false;
    if (pBotWhoIsAsking == nullptr) return false;
	
    for (int i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t* pPlayer = INDEXENT(i);
        if (!pPlayer || pPlayer->free) // free - ie no client
            continue; // next
    	
        // skip this player if not alive (i.e. dead or dying)
        if (!IsAlive(pPlayer))
            continue;
    	
        // not a bot
        if (!(pPlayer->v.flags & FL_THIRDPARTYBOT))
            continue;
    	
        // Only check other bots (do not check self)
        cBot* botpointer = UTIL_GetBotPointer(pPlayer);
        if (botpointer && // a bot
            botpointer != pBotWhoIsAsking && // not self
            !botpointer->isDead()) { // not dead
        	
            // other bot uses hostage, so hostage is not 'free'
            if (botpointer->isUsingHostage(pHostage)) {
                pBotWhoIsAsking->rprint("Looks like the hostage is used by another one");
                botpointer->rprint("I am using the hostage!");
                return false;
            }
        }
    }
	
    return true;
}

void TryToGetHostageTargetToFollowMe(cBot *pBot) {
    if (pBot->hasEnemy()) {
        return;                   // enemy, do not check
    }

    if (pBot->isTerrorist()) {
        // terrorists do not rescue hostages
        return;
    }

    edict_t *pHostage = pBot->getHostageToRescue();

    if (pHostage == nullptr) {
        pHostage = pBot->findHostageToRescue();
    }

    // still NULL
    if (pHostage == nullptr) {
        // Note: this means a hostage that is near and visible and rescueable etc.
        return; // nothing to do yet
    }

    // Whenever we have a hostage to go after, verify it is still rescueable
    const bool isRescueable = isHostageRescueable(pBot, pHostage);

    if (!isRescueable) {
        pBot->rprint_trace("TryToGetHostageTargetToFollowMe", "Hostage found, but not rescueable, forgetting...");
        pBot->forgetHostage(pHostage);
        return;
    }
    pBot->rprint_trace("TryToGetHostageTargetToFollowMe", "Remembering hostage (target) to rescue");
    pBot->rememberWhichHostageToRescue(pHostage);

    // Prevent bots getting to close here
    const float distanceToHostage = func_distance(pBot->pEdict->v.origin, pHostage->v.origin);

    // From here, we should get the hostage when still visible
    if (pBot->canSeeEntity(pHostage)) {
        pBot->rprint_trace("TryToGetHostageTargetToFollowMe", "I can see the hostage to rescue!");
        // set body to hostage!
        pBot->vBody = pBot->vHead = pHostage->v.origin + Vector(0, 0, 36);
        // by default run
        pBot->setMoveSpeed(pBot->f_max_speed);

        if (distanceToHostage <= 80) {
            pBot->rprint_trace("TryToGetHostageTargetToFollowMe", "I can see hostage AND really close!");
            pBot->setMoveSpeed(0.0f); // too close, do not move

            // only use hostage when facing
            const int angle_to_hostage = FUNC_InFieldOfView(pBot->pEdict, (pBot->vBody - pBot->pEdict->v.origin));

            if (angle_to_hostage <= 30
                && (pBot->f_use_timer < gpGlobals->time)) {
                pBot->rprint_trace("TryToGetHostageTargetToFollowMe", "I can see hostage AND really REALLY close AND facing!");
                // within FOV and we assume we can now press the USE key. In order to make sure we press it once
                // and not multiple times we set the timer.
                pBot->f_use_timer = gpGlobals->time + 0.7f;

                UTIL_BotPressKey(pBot, IN_USE);

                // assuming it worked, remember bot is rescueing this hostage
                pBot->rememberHostageIsFollowingMe(pHostage);
                pBot->clearHostageToRescueTarget();
                pBot->rprint_trace("TryToGetHostageTargetToFollowMe", "I pressed USE and assume i have used the hostage");
                pBot->f_wait_time = gpGlobals->time + 0.5f;
            }
        }

        pBot->forgetGoal();
    }
}

bool isHostageRescued(cBot *pBot, const edict_t *pHostage) //pBot not used [APG]RoboCop[CL]
{
    if (pHostage == nullptr) return false;

    if (FBitSet(pHostage->v.effects, EF_NODRAW)) {
//        pBot->rprint("isHostageRescued()", "Hostage is rescued");
        return true;
    }
//    pBot->rprint("isHostageRescued()", "Hostage is NOT rescued");
    return false;
}

int FUNC_GiveHostage(cBot* pBot) //Experimental [APG]RoboCop[CL]
{
	if (pBot->isTerrorist()) {
		return 0;
	}

	// find a hostage to rescue
	edict_t *pHostage = pBot->getHostageToRescue();

	if (pHostage == nullptr) {
		pHostage = pBot->findHostageToRescue();
	}

	// still NULL
	if (pHostage == nullptr) {
		// Note: this means a hostage that is near and visible and rescueable etc.
		return 0; // nothing to do yet
	}

	// Whenever we have a hostage to go after, verify it is still rescueable
	const bool isRescueable = isHostageRescueable(pBot, pHostage);

	if (!isRescueable) {
		pBot->rprint_trace("GiveHostage", "Hostage found, but not rescueable, forgetting...");
		pBot->forgetHostage(pHostage);
		return 0;
	}
	pBot->rprint_trace("GiveHostage", "Remembering hostage (target) to rescue");
	pBot->rememberWhichHostageToRescue(pHostage);

	// Prevent bots getting to close here
	const float distanceToHostage = func_distance(pBot->pEdict->v.origin, pHostage->v.origin);

	// From here, we should get the hostage when still visible
    if (pBot->canSeeEntity(pHostage))
    {
	    pBot->rprint_trace("GiveHostage", "I can see the hostage to rescue!");
    	// set body to hostage!
    	pBot->vBody = pBot->vHead = pHostage->v.origin + Vector(0, 0, 36);
    	// by default run
    	pBot->setMoveSpeed(pBot->f_max_speed);

    	if (distanceToHostage <= 80)
    	{
    		pBot->rprint_trace("GiveHostage", "I can see hostage AND really close!");
    		pBot->setMoveSpeed(0.0f); // too close, do not move
    	}
    }
	return true; //gives any hostage we still have to go for
}

bool isHostageRescueable(cBot *pBot, edict_t *pHostage) {
    if (pHostage == nullptr) return false;
//    pBot->rprint("isHostageRescueable");

    // Already rescued?
    if (isHostageRescued(pBot, pHostage)) {
        return false;
    }

    // dead
    if (!FUNC_EdictIsAlive(pHostage)) {
        return false;
    }

    // Already moving? (used by human player?)
    if (FUNC_PlayerSpeed(pHostage) > 2) {
        return false;
    }
    // Already used by bot?

    if (pBot != nullptr) {
    	// rblog("isHostageRescueable - pBot is != NULL\n");
        if (pBot->isUsingHostage(pHostage)) return false;
        // Is the hostage not used by *any other* bot?
        if (!isHostageFree(pBot, pHostage)) {
            rblog("isHostageRescueable - Hostage is not free");
            return false;
        }
    }

    // yes we can rescue this hostage
    return true;
}

bool FUNC_EdictIsAlive(edict_t *pEdict) {
    if (pEdict == nullptr) return false;
    return pEdict->v.health > 0;
}

// HostageNear()

bool FUNC_BotHoldsZoomWeapon(cBot *pBot) {
    // Check if the bot holds a weapon that can zoom, but is not a sniper gun.
    return pBot->isHoldingWeapon(CS_WEAPON_AUG) || pBot->isHoldingWeapon(CS_WEAPON_SG552);
}

void FUNC_BotChecksFalling(cBot *pBot) {
    // This routine should never be filled with code.
    // - Bots should simply never fall
    // - If bots do fall, check precalculation routine.
}

// New function to display a message on the center of the screen
void CenterMessage(char *buffer) {
    //DebugOut("waypoint: CenterMessage():\n");
    //DebugOut(buffer);
    //DebugOut("\n");
    UTIL_ClientPrintAll(HUD_PRINTCENTER, buffer);
}

// Bot Takes Cover
bool BOT_DecideTakeCover(cBot *pBot) {
    /*
           UTIL_ClientPrintAll( HUD_PRINTCENTER, "DECISION TO TAKE COVER\n" );

        int iNodeEnemy = NodeMachine.getCloseNode(pBot->pBotEnemy->v.origin, NODE_ZONE);
        int iNodeHere  = NodeMachine.getCloseNode(pBot->pEdict->v.origin, NODE_ZONE);
        int iCoverNode = NodeMachine.node_cover(iNodeHere, iNodeEnemy, pBot->pEdict);

        if (iCoverNode > -1)
        {
            // TODO TODO TODO make sure the cover code works via iGoalNode
    //		pBot->v_cover = NodeMachine.node_vector(iCoverNode);
            pBot->f_cover_time = gpGlobals->time + RANDOM_LONG(3,5);
            pBot->f_camp_time = gpGlobals->time;
            return true;
        }
        */

    return false;
}

// logs into a file
void rblog(const char* txt) {
    // output to stdout
    printf("%s", txt);

    // and to reallog file
    if (fpRblog) {
        std::fprintf(fpRblog, "%s", txt);        // print the text into the file

        // this way we make sure we have all latest info - even with crashes
        fflush(fpRblog);
    }
}
