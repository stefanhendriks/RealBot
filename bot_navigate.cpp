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
#include "NodeMachine.h"

extern int mod_id;
extern edict_t* pHostEdict;

constexpr float TURN_ANGLE = 75.0f; // Degrees to turn when avoiding obstacles
constexpr float MOVE_DISTANCE = 24.0f; // Distance to move forward
constexpr std::uint8_t SCAN_RADIUS = 60; // Radius to scan to prevent blocking with players

/**
 * Given an angle, makes sure it wraps around properly
 * @param angle
 * @return
 */
float fixAngle(const float angle) {
    if (angle > 180.0f) return angle - 360.0f;
    if (angle < -180.0f) return angle + 360.0f;
    return angle;
}

void botFixIdealPitch(edict_t* pEdict) {
    pEdict->v.idealpitch = fixAngle(pEdict->v.idealpitch);
}

void botFixIdealYaw(edict_t* pEdict) {
    pEdict->v.ideal_yaw = fixAngle(pEdict->v.ideal_yaw);
}

bool traceLine(const Vector& v_source, const Vector& v_dest, const edict_t* pEdict, TraceResult& tr) {
    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, pEdict->v.pContainingEntity, &tr);
    return tr.flFraction >= 1.0f;
}

bool BotCanJumpUp(const cBot* pBot) {
    // What I do here is trace 3 lines straight out, one unit higher than
    // the highest normal jumping distance.  I trace once at the center of
    // the body, once at the right side, and once at the left side.  If all
    // three of these TraceLines don't hit an obstruction then I know the
    // area to jump to is clear.  I then need to trace from head level,
    // above where the bot will jump to, downward to see if there is anything
    // blocking the jump.  There could be a narrow opening that the body
    // will not fit into.  These horizontal and vertical TraceLines seem
    // to catch most of the problems with falsely trying to jump on something
    // that the bot can not get onto.

    TraceResult tr;
    const edict_t* pEdict = pBot->pEdict;

    // convert current view angle to vectors for TraceLine math...

    Vector v_jump = pEdict->v.v_angle;
    v_jump.x = 0;                // reset pitch to 0 (level horizontally)
    v_jump.z = 0;                // reset roll to 0 (straight up and down)

    UTIL_MakeVectors(v_jump);

    // use center of the body first...

    // maximum jump height is 45, so check one unit above that (46)
    Vector v_source = pEdict->v.origin + Vector(0, 0, -36 + MAX_JUMPHEIGHT);
    Vector v_dest = v_source + gpGlobals->v_forward * 24;

    // trace a line forward at maximum jump height...
    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters,
        pEdict->v.pContainingEntity, &tr);

    // if trace hit something, return FALSE
    if (tr.flFraction < 1.0f)
        return false;

    // now check same height to one side of the bot...
    v_source =
        pEdict->v.origin + gpGlobals->v_right * 16 + Vector(0, 0,
            -36 + MAX_JUMPHEIGHT);
    v_dest = v_source + gpGlobals->v_forward * 24;

    // trace a line forward at maximum jump height...
    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters,
        pEdict->v.pContainingEntity, &tr);

    // if trace hit something, return FALSE
    if (tr.flFraction < 1.0f)
        return false;

    // now check same height on the other side of the bot...
    v_source =
        pEdict->v.origin + gpGlobals->v_right * -16 + Vector(0, 0,
            -36 + MAX_JUMPHEIGHT);
    v_dest = v_source + gpGlobals->v_forward * 24;

    // trace a line forward at maximum jump height...
    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters,
        pEdict->v.pContainingEntity, &tr);

    // if trace hit something, return FALSE
    if (tr.flFraction < 1.0f)
        return false;

    // now trace from head level downward to check for obstructions...

    // start of trace is 24 units in front of bot, 72 units above head...
    v_source = pEdict->v.origin + gpGlobals->v_forward * 24;

    // offset 72 units from top of head (72 + 36)
    v_source.z = v_source.z + 108;

    // end point of trace is 99 units straight down from start...
    // (99 is 108 minus the jump limit height which is 45 - 36 = 9)
    // fix by stefan, max jump height is 63 , not 45! (using duckjump)
    // 108 - (63-36) = 81
    v_dest = v_source + Vector(0, 0, -81);

    // trace a line straight down toward the ground...
    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters,
        pEdict->v.pContainingEntity, &tr);

    // if trace hit something, return FALSE
    if (tr.flFraction < 1.0f)
        return false;

    // now check same height to one side of the bot...
    v_source =
        pEdict->v.origin + gpGlobals->v_right * 16 +
        gpGlobals->v_forward * 24;
    v_source.z = v_source.z + 108;
    v_dest = v_source + Vector(0, 0, -81);

    // trace a line straight down toward the ground...
    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters,
        pEdict->v.pContainingEntity, &tr);

    // if trace hit something, return FALSE
    if (tr.flFraction < 1.0f)
        return false;

    // now check same height on the other side of the bot...
    v_source =
        pEdict->v.origin + gpGlobals->v_right * -16 +
        gpGlobals->v_forward * 24;
    v_source.z = v_source.z + 108;
    v_dest = v_source + Vector(0, 0, -81);

    // trace a line straight down toward the ground...
    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters,
        pEdict->v.pContainingEntity, &tr);

    // if trace hit something, return FALSE
    if (tr.flFraction < 1.0f)
        return false;

    return true;
}

bool BotCanDuckUnder(const cBot* pBot) {
    // What I do here is trace 3 lines straight out, one unit higher than
    // the ducking height.  I trace once at the center of the body, once
    // at the right side, and once at the left side.  If all three of these
    // TraceLines don't hit an obstruction then I know the area to duck to
    // is clear.  I then need to trace from the ground up, 72 units, to make
    // sure that there is something blocking the TraceLine.  Then we know
    // we can duck under it.

    TraceResult tr;
    const edict_t* pEdict = pBot->pEdict;

    // convert current view angle to vectors for TraceLine math...

    Vector v_duck = pEdict->v.v_angle;
    v_duck.x = 0;                // reset pitch to 0 (level horizontally)
    v_duck.z = 0;                // reset roll to 0 (straight up and down)

    UTIL_MakeVectors(v_duck);

    // use center of the body first...

    // duck height is 36, so check one unit above that (37)
    Vector v_source = pEdict->v.origin + Vector(0, 0, -36 + 37);
    Vector v_dest = v_source + gpGlobals->v_forward * 24;

    // trace a line forward at duck height...
    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters,
        pEdict->v.pContainingEntity, &tr);

    // if trace hit something, return FALSE
    if (tr.flFraction < 1.0f)
        return false;

    // now check same height to one side of the bot...
    v_source =
        pEdict->v.origin + gpGlobals->v_right * 16 + Vector(0, 0, -36 + 37);
    v_dest = v_source + gpGlobals->v_forward * 24;

    // trace a line forward at duck height...
    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters,
        pEdict->v.pContainingEntity, &tr);

    // if trace hit something, return FALSE
    if (tr.flFraction < 1.0f)
        return false;

    // now check same height on the other side of the bot...
    v_source =
        pEdict->v.origin + gpGlobals->v_right * -16 + Vector(0, 0,
            -36 + 37);
    v_dest = v_source + gpGlobals->v_forward * 24;

    // trace a line forward at duck height...
    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters,
        pEdict->v.pContainingEntity, &tr);

    // if trace hit something, return FALSE
    if (tr.flFraction < 1.0f)
        return false;

    // now trace from the ground up to check for object to duck under...

    // start of trace is 24 units in front of bot near ground...
    v_source = pEdict->v.origin + gpGlobals->v_forward * 24;
    v_source.z = v_source.z - 35;        // offset to feet + 1 unit up

    // end point of trace is 72 units straight up from start...
    v_dest = v_source + Vector(0, 0, 72);

    // trace a line straight up in the air...
    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters,
        pEdict->v.pContainingEntity, &tr);

    // if trace didn't hit something, return FALSE
    if (tr.flFraction >= 1.0f)
        return false;

    // now check same height to one side of the bot...
    v_source =
        pEdict->v.origin + gpGlobals->v_right * 16 +
        gpGlobals->v_forward * 24;
    v_source.z = v_source.z - 35;        // offset to feet + 1 unit up
    v_dest = v_source + Vector(0, 0, 72);

    // trace a line straight up in the air...
    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters,
        pEdict->v.pContainingEntity, &tr);

    // if trace didn't hit something, return FALSE
    if (tr.flFraction >= 1.0f)
        return false;

    // now check same height on the other side of the bot...
    v_source =
        pEdict->v.origin + gpGlobals->v_right * -16 +
        gpGlobals->v_forward * 24;
    v_source.z = v_source.z - 35;        // offset to feet + 1 unit up
    v_dest = v_source + Vector(0, 0, 72);

    // trace a line straight up in the air...
    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters,
        pEdict->v.pContainingEntity, &tr);

    // if trace didn't hit something, return FALSE
    if (tr.flFraction >= 1.0f)
        return false;

    return true;
}

bool isBotNearby(const cBot* pBot, const float radius) {
    if (!pBot || !pBot->pEdict) {
        return false; // Validate input
    }

    for (int i = 0; i < gpGlobals->maxClients; ++i) {
        edict_t* pPlayer = INDEXENT(i + 1);

        if (pPlayer && !pPlayer->free && pPlayer != pBot->pEdict) {
            float distance = (pPlayer->v.origin - pBot->pEdict->v.origin).Length();
            if (distance < radius) {
                return true;
            }
        }
    }

    return false;
}

void adjustBotAngle(const cBot* pBot, const float angle) {
    if (!pBot || !pBot->pEdict) {
        return;
    }

    pBot->pEdict->v.v_angle.y += angle;
    UTIL_MakeVectors(pBot->pEdict->v.v_angle);
}

void avoidClustering(const cBot* pBot) {
    if (!pBot) {
        return;
    }

    if (isBotNearby(pBot, SCAN_RADIUS)) {
        adjustBotAngle(pBot, TURN_ANGLE);
    }
}

bool isPathBlocked(const cBot* pBot, const Vector& v_dest) {
    if (!pBot || !pBot->pEdict) {
        return true; // Assume blocked if input is invalid
    }

    TraceResult tr;
    const Vector v_source = pBot->pEdict->v.origin;

    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, pBot->pEdict->v.pContainingEntity, &tr);

	return tr.flFraction < 1.0f;
}

void adjustPathIfBlocked(const cBot* pBot) {
    if (!pBot) {
        return;
    }

    Vector v_dest = pBot->pEdict->v.origin + gpGlobals->v_forward * MOVE_DISTANCE;

    if (isPathBlocked(pBot, v_dest)) {
        adjustBotAngle(pBot, TURN_ANGLE);
    }
}

bool performTrace(const Vector& v_source, const Vector& v_dest, edict_t* pEntity, TraceResult& tr) {
    UTIL_TraceLine(v_source, v_dest, dont_ignore_monsters, pEntity, &tr);

    return tr.flFraction >= 1.0f;
}

bool isPathClear(const cBot* pBot, const Vector& v_dest) {
    if (!pBot || !pBot->pEdict) {
        return false; // Invalid input, assume path is not clear
    }
    TraceResult tr;

    return performTrace(pBot->pEdict->v.origin, v_dest, pBot->pEdict->v.pContainingEntity, tr);
}

void BotNavigate(const cBot* pBot) {
    if (!pBot) {
        return;
    }

    // Avoid clustering
    avoidClustering(pBot);

    // Adjust path if blocked
    adjustPathIfBlocked(pBot);

    // Check if the path is clear before moving
    Vector v_dest = pBot->pEdict->v.origin + gpGlobals->v_forward * MOVE_DISTANCE;

    if (!isPathBlocked(pBot, v_dest)) {
        // Move the bot
        pBot->pEdict->v.origin = v_dest;
    }
}