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
  *	NODE MACHINE
  * COPYRIGHTED BY STEFAN HENDRIKS (C) 2003-2004
  **/

#include <algorithm>
#include <cmath>
#include <cstring>
#include <extdll.h>
#include <dllapi.h>
#include <meta_api.h>
#include <entity_state.h>

// malloc stuff?
#include <cstdlib>
// ---

#include "bot.h"
#include "IniParser.h"
#include "bot_weapons.h"
#include "game.h"
#include "bot_func.h"

#include "NodeMachine.h"

#include <string>

tNodestar astar_list[MAX_NODES];

const Vector &INVALID_VECTOR = Vector(9999, 9999, 9999);

extern edict_t *pHostEdict;
extern cGame Game;
extern cBot bots[32];
extern int draw_nodepath;

//---------------------------------------------------------
//CODE: CHEESEMONSTER

int cNodeMachine::GetVisibilityFromTo(int iFrom, int iTo) const // BERKED
{
    // prevent negative indexes on iVisChecked below -- BERKED
    if (iFrom >= 0 && iFrom < 4096 && iTo >= 0 && iTo < 4096) {
        if (iVisChecked[iFrom] == 0)
            return VIS_UNKNOWN;       // we have no clue
    }
    else {
        rblog("ERROR: Negative or out of bound index on iFrom! Returning Visibility NULL\n");
        return 0;
    }
	
    // was int
	
    // work out the position
    const long iPosition = iFrom * MAX_NODES + iTo;
	
    const long iByte = static_cast<int>(iPosition / 8);
    const unsigned int iBit = iPosition % 8;
	
    if (iByte < static_cast<long>(g_iMaxVisibilityByte)) {
        // Get the Byte that this is in
        const unsigned char* ToReturn = cVisTable + iByte;
        // get the bit in the byte
        return (*ToReturn & 1 << iBit) > 0 ? VIS_VISIBLE : VIS_BLOCKED;       // BERKED
    }
	
    return VIS_BLOCKED;          // BERKED
}

void cNodeMachine::SetVisibilityFromTo(int iFrom, int iTo, bool bVisible) {
    // prevent negative indexes on iVisChecked below, fixing SEGV -- BERKED
    if (iFrom < 0 || iFrom > MAX_NODES || iTo < 0 || iTo > MAX_NODES) {
        return;
    }

    if (iFrom >= 0 && iFrom < 4096) {
        iVisChecked[iFrom] = 1;      // WE HAVE CHECKED THIS ONE
    }
    else {
        rblog("ERROR: Negative or out of bound index on iFrom! Returning Visibility NULL\n");
        return;
    }

    // was int
    const long iPosition = iFrom * MAX_NODES + iTo;

    const long iByte = static_cast<int>(iPosition / 8);
    const unsigned int iBit = iPosition % 8;

    if (iByte < static_cast<long>(g_iMaxVisibilityByte)) {
        unsigned char *ToChange = cVisTable + iByte;

        if (bVisible)
            *ToChange |= 1 << iBit;
        else
            *ToChange &= ~(1 << iBit);
    }
}

void cNodeMachine::ClearVisibilityTable() const
{
    if (cVisTable) {
        std::memset(cVisTable, 0, g_iMaxVisibilityByte);
    }
}

void cNodeMachine::FreeVisibilityTable() const
{
    free(cVisTable);
}

//---------------------------------------------------------

// Initialization of node machine
void cNodeMachine::init() {
    rblog("cNodeMachine::init() - START\n");
    iMaxUsedNodes = 0;

    for (int i = 0; i < MAX_NODES; i++) {
        // --- nodes
        Nodes[i].origin = Vector(9999, 9999, 9999);
        for (int& n : Nodes[i].iNeighbour)
	        n = -1;

        // No bits set
        Nodes[i].iNodeBits = 0;

        // --- info nodes
        for (float& d : InfoNodes[i].fDanger)
        {
	        d = 0.0f;
        }
    }

    // Init trouble
    for (tTrouble& Trouble : Troubles)
    {
	    Trouble.iFrom = -1;
	    Trouble.iTo = -1;
	    Trouble.iTries = -1;
    }

    initGoals();

    // Init paths
    for (int p = 0; p < MAX_BOTS; p++)
        path_clear(p);

    // Init VisTable
    for (unsigned char& iVx : iVisChecked)
    {
	    iVx = 0;     // not checked yet
    }

    // CODE: From cheesemonster
    constexpr unsigned long iSize = g_iMaxVisibilityByte;

    //create a heap type thing...
    FreeVisibilityTable();       // 16/07/04 - free it first
    cVisTable = static_cast<unsigned char*>(malloc(iSize));
    if (cVisTable != nullptr) {
        std::memset(cVisTable, 0, iSize);
    }
    else {
    	rblog("cNodeMachine::init() - ERROR: Could not allocate memory for visibility table\n");
    }

    ClearVisibilityTable();
    // END:

    // Init Meredians
    for (tMeredian (&Meredian)[64] : Meredians)
	    for (tMeredian& iMy : Meredian)
		    for (int& iNode : iMy.iNodes)
			    iNode = -1;

    rblog("cNodeMachine::init() - END\n");
}

void cNodeMachine::initGoals() {
    // Init goals
    for (int g = 0; g < MAX_GOALS; g++) {
        initGoal(g);
    }
}

void cNodeMachine::initGoal(int g) {
    Goals[g].iNode = -1;
    Goals[g].pGoalEdict = nullptr;
    Goals[g].iType = GOAL_NONE;
    Goals[g].index = g;
    Goals[g].iChecked = 0;
    Goals[g].iBadScore = 0;   // no bad score at init
    std::memset(Goals[g].name, 0, sizeof(Goals[g].name));
}

int cNodeMachine::GetTroubleIndexForConnection(int iFrom, int iTo) const
{
	//    sprintf(msg, "GetTroubleIndexForConnection | from %d to %d\n", iFrom, iTo);
	//    rblog(msg);
    // in case of invalid values, return -1 - no need to loop
    if (iFrom < -1 || iFrom >= MAX_NODES) {
        rblog("GetTroubleIndexForConnection | invalid iFrom\n");
        return -1;
    }
    if (iTo < -1 || iTo >= MAX_NODES) {
        rblog("GetTroubleIndexForConnection | invalid iTo\n");
        return -1;
    }

    for (int index = 0; index < MAX_TROUBLE; index++) {
        if (Troubles[index].iFrom == iFrom &&
            Troubles[index].iTo == iTo) {
	        char msg[255] = {};
	        snprintf(msg, sizeof(msg), "GetTroubleIndexForConnection | Found index [%d] for from %d to %d\n", index, iFrom, iTo);
            rblog(msg);
            // found troubled connection, return its index
            return index;
        }
    }

//    rblog("GetTroubleIndexForConnection | found no index matching from/to. Returning -1\n");
    return -1;
}

/**
 * Adds a 'troubled connection' to the list of troubled connections.
 *
 * @param iFrom
 * @param iTo
 * @return index of newly created index
 */
int cNodeMachine::AddTroubledConnection(int iFrom, int iTo)
{
	const int existingIndex = GetTroubleIndexForConnection(iFrom, iTo);
    if (existingIndex > -1)
        return existingIndex; // already exists

    int iNew = -1;

	for (int t = 0; t < MAX_TROUBLE; t++)
        if (Troubles[t].iFrom < 0 ||
            Troubles[t].iTo < 0) {
            iNew = t;
            break;
        }

    if (iNew < 0) {
        return -1;
    }

    Troubles[iNew].iFrom = iFrom;
    Troubles[iNew].iTo = iTo;
    Troubles[iNew].iTries = 0;

    return iNew;
}

bool cNodeMachine::hasAttemptedConnectionTooManyTimes(int index) const
{
    if (index < 0) {
        rblog("(trouble) hasAttemptedConnectionTooManyTimes | invalid index for hasAttemptedConnectionTooManyTimes()\n");
        // deal with invalid connection
        return false;
    }

    const tTrouble &trouble = Troubles[index];
    char msg[255];
    snprintf(msg, sizeof(msg), "(trouble) hasAttemptedConnectionTooManyTimes | Connection %d (%d->%d) has %d tries.\n", index, trouble.iFrom, trouble.iTo, trouble.iTries);
    rblog(msg);

    if (trouble.iTries > 2) {
        rblog("(trouble) hasAttemptedConnectionTooManyTimes | Connection has been tried too many times!\n");
        // after 3 times we quit
        return true;
    }
    return false;
}

/**
 * Remembers that this is a bad connection, increases a counter. When counter exceeds max attempts, it will
 * remove the connection. When it removes the connection, this method will return "false". If a bot
 * may have another go with this connection, this method will return "true".
 * @param iFrom
 * @param iTo
 * @return
 */
bool cNodeMachine::IncreaseAttemptsForTroubledConnectionOrRemoveIfExceeded(int iFrom, int iTo)
{
	const int index = AddTroubledConnection(iFrom, iTo);
    IncreaseAttemptsForTroubledConnection(index);
    if (hasAttemptedConnectionTooManyTimes(index)) {
        rblog("(trouble) IncreaseAttemptsForTroubledConnectionOrRemoveIfExceeded | a troubled connection - tried too many times!\n");

        // remove connection
        if (!removeConnection(iFrom, iTo)) {
            rblog("(trouble) IncreaseAttemptsForTroubledConnectionOrRemoveIfExceeded | for some reason the connection was not removed!?\n");
        } else {
            rblog("(trouble) IncreaseAttemptsForTroubledConnectionOrRemoveIfExceeded | the connection is removed!\n");
        }

        return false;
    }
	rblog("(trouble) IncreaseAttemptsForTroubledConnectionOrRemoveIfExceeded | may attempt another time\n");
	return true;
}

void cNodeMachine::IncreaseAttemptsForTroubledConnection(int index)
{
    if (index < 0 || index >= MAX_TROUBLE) return;

    char msg[255] = {};
    snprintf(msg, sizeof(msg), "(trouble) IncreaseAttemptsForTroubledConnection | Increasing trouble for connection [%d]\n", index);
    rblog(msg);

    Troubles[index].iTries++;

    const tTrouble &trouble = Troubles[index];
    std::memset(msg, 0, sizeof(msg));
    snprintf(msg, sizeof(msg), "(trouble) IncreaseAttemptsForTroubledConnection | Connection %d (%d->%d) has %d tries.\n", index, trouble.iFrom, trouble.iTo, trouble.iTries);
    rblog(msg);
}

bool cNodeMachine::ClearTroubledConnection(int iFrom, int iTo)
{
    char msg[255];
    snprintf(msg, sizeof(msg), "(trouble) NodeMachine::ClearTroubledConnection | %d -> %d - START\n", iFrom, iTo);
    rblog(msg);

    const int index = GetTroubleIndexForConnection(iFrom, iTo);

    std::memset(msg, 0, sizeof(msg));
    snprintf(msg, sizeof(msg), "(trouble) NodeMachine::ClearTroubledConnection | %d -> %d has index %d\n", iFrom, iTo, index);
    rblog(msg);

    if (index < 0) {
        // deal with scenario that this is a non-existing connection
        return false;
    }

    // clear values
    Troubles[index].iFrom = -1;
    Troubles[index].iTo = -1;
    Troubles[index].iTries = -1;

    return true;
}

void cNodeMachine::path_clear(int botIndex)
{
    for (int nodeIndex = 0; nodeIndex < MAX_NODES; nodeIndex++) {
        iPath[botIndex][nodeIndex] = -1;
    }
}

// Return
Vector cNodeMachine::node_vector(int iNode) const
{
    if (iNode > -1) {
        return Nodes[iNode].origin;
    }

    return {9999, 9999, 9999};
}

// Input: Vector, Output X and Y Meredians
void cNodeMachine::VectorToMeredian(const Vector& vOrigin, int *iX, int *iY)
{
    // Called for lookupt and for storing
    float iCoordX = vOrigin.x + 8192.0f;  // map height (converts from - to +)
    float iCoordY = vOrigin.y + 8192.0f;  // map width (converts from - to +)

    // Meredian:
    iCoordX = iCoordX / SIZE_MEREDIAN;
    iCoordY = iCoordY / SIZE_MEREDIAN;

    *iX = static_cast<int>(iCoordX);
    *iY = static_cast<int>(iCoordY);
}

void cNodeMachine::AddToMeredian(int iX, int iY, int iNode)
{
    int index = -1;
    for (int i = 0; i < MAX_NODES_IN_MEREDIANS; i++)
        if (Meredians[iX][iY].iNodes[i] < 0) {
            index = i;
            break;
        }

    if (index < 0)
        return;

    Meredians[iX][iY].iNodes[index] = iNode;
}

// Does the node float?
bool cNodeMachine::node_float(const Vector& vOrigin, edict_t *pEdict)
{
    TraceResult tr;
    const Vector tr_end = vOrigin - Vector(0, 0, ORIGIN_HEIGHT * 1.2f);

    //Using TraceHull to detect de_aztec bridge and other entities. (skill self)
    //UTIL_TraceHull(vOrigin, tr_end, ignore_monsters, point_hull, pEdict->v.pContainingEntity, &tr);
    if (pEdict)
        UTIL_TraceHull(vOrigin, tr_end, ignore_monsters, human_hull,
                       pEdict->v.pContainingEntity, &tr);
    else
        UTIL_TraceHull(vOrigin, tr_end, ignore_monsters, human_hull, nullptr,
                       &tr);

    // if nothing hit: floating too high, return false
    if (tr.flFraction >= 1.0f)
        return true;              // floating

    // *NOTE*: Actually this check should not be nescesary!
    if (tr.flFraction < 1.0f)
        if (tr.pHit == pEdict)
            return true;

    // if inside wall: return false
    if (tr.fStartSolid == 1) {
		rblog("(node) node_float | node is inside wall!\n");
		return false;
    }

    return false;                // not floating
}

// Does the node stand on a crate? or a steep slope?
bool cNodeMachine::node_on_crate(const Vector& vOrigin, edict_t *pEdict)
{
    TraceResult tr;
    const Vector tr_end = vOrigin - Vector(0, 0, ORIGIN_HEIGHT * 1.2f);

    //Using TraceHull to detect de_aztec bridge and other entities. (skill self)
    if (pEdict)
        UTIL_TraceHull(vOrigin, tr_end, ignore_monsters, human_hull,
                       pEdict->v.pContainingEntity, &tr);
    else
        UTIL_TraceHull(vOrigin, tr_end, ignore_monsters, human_hull, nullptr,
                       &tr);

    // if nothing hit: floating too high, return false
    if (tr.flFraction >= 1.0f)
        return false;

    // hit something
    if (tr.flFraction < 1.0f) {
        // thanks a million to PMB , so i know what the difference
        // is between something straight (crate) and steep... although i have
        // no clue yet how to compute these values myself.
        if ( /*tr.vecPlaneNormal.z >= 0.7 && */ tr.vecPlaneNormal.z == 1.0f) {
            return true;
        }
    }

    // impossible to reach
    return false;
}

int cNodeMachine::node_dangerous(int iTeam, const Vector& vOrigin, float fMaxDistance) //TODO: Experimental & Incomplete [APG]RoboCop[CL]
{
	// check if node is dangerous
	//int iDangerous = 0;
	//int iDangerousCount = 0;
	//int iDangerousIndex = 0;
	
    return 0;
}

/**
 * Find a node close to vOrigin within distance fDist. Ignoring any pEdict it hits.
 * @param vOrigin
 * @param fDist
 * @param pEdict
 * @return
 */
int cNodeMachine::getClosestNode(const Vector& vOrigin, float fDist, edict_t *pEdict) const
{
    // REDO: Need faster method to find a node
    // TOADD: For secure results all nodes should be checked to figure out the real
    //        'closest' node.

    // Use Meredians to search for nodes
    // TODO: we should take care in the situation where we're at the 'edge' of such a meridian (subspace). So we should
    // basicly take edging meridians as well when too close to the edge.
    int iX, iY;
    VectorToMeredian(vOrigin, &iX, &iY);

    // no Meridian coordinates found, bail
    if (iX < 0 || iY < 0) {
        return -1;
    }

    float dist = fDist;
    int iCloseNode = -1;

    // Search in this meredian
    for (const int i : Meredians[iX][iY].iNodes)
    {
        if (i < 0) continue; // skip invalid node indexes

        const int iNode = i;

		//if (Nodes[iNode].origin.z > (vOrigin.z + 32)) continue; // do not pick nodes higher than us

        const float distanceFromTo = func_distance(vOrigin, Nodes[iNode].origin);
        if (distanceFromTo < dist) {
            dist = distanceFromTo;

            TraceResult tr;
            Vector nodeVector = Nodes[iNode].origin;

            //Using TraceHull to detect de_aztec bridge and other entities.
            //DONT_IGNORE_MONSTERS, we reached it only when there are no other bots standing in our way!
            //UTIL_TraceHull(vOrigin, nodeVector, dont_ignore_monsters, point_hull, pEdict, &tr);
            //UTIL_TraceHull(vOrigin, nodeVector, dont_ignore_monsters, human_hull, pEdict, &tr);
            if (pEdict) {
                UTIL_TraceHull(vOrigin, nodeVector, dont_ignore_monsters,
                               head_hull, pEdict->v.pContainingEntity,
                               &tr);
            } else {
                UTIL_TraceHull(vOrigin, nodeVector, dont_ignore_monsters,
                               head_hull, nullptr, &tr);
            }

            // if nothing hit:
            if (tr.flFraction >= 1.0f && pEdict != nullptr) {
                // Regardless of conditions, set iFarNode to iNode
                iCloseNode = iNode;
            }
        }
    }
    return iCloseNode;
}

/**
 * Find a node as far away from vOrigin but still within distance fDist. Ignoring any pEdict it hits.
 * @param vOrigin
 * @param fDist
 * @param pEdict
 * @return
 */
int cNodeMachine::getFurthestNode(const Vector& vOrigin, float fDist, const edict_t *pEdict) const
{
    // Use Meredians to search for nodes
    // TODO: we should take care in the situation where we're at the 'edge' of such a meridian (subspace). So we should
    // basicly take edging meridians as well when too close to the edge.
    int iX, iY;
    VectorToMeredian(vOrigin, &iX, &iY);

    // no Meridian coordinates found, bail
    if (iX < 0 || iY < 0) {
        return -1;
    }

    float dist = 0;
    int iFarNode = -1;

    // Search in this meredian
    for (const int i : Meredians[iX][iY].iNodes)
    {
        if (i < 0) continue; // skip invalid node indexes

        const int iNode = i;

		// if (Nodes[iNode].origin.z > (vOrigin.z + 32)) continue; // do not pick nodes higher than us

        const float distanceFromTo = func_distance(vOrigin, Nodes[iNode].origin);
        if (distanceFromTo < fDist && // within range
            distanceFromTo > dist) { // but furthest so far
            dist = distanceFromTo;

            TraceResult tr;
            Vector nodeVector = Nodes[iNode].origin;

            //Using TraceHull to detect de_aztec bridge and other entities.
            //DONT_IGNORE_MONSTERS, we reached it only when there are no other bots standing in our way!
            //UTIL_TraceHull(vOrigin, nodeVector, dont_ignore_monsters, point_hull, pEdict, &tr);
            //UTIL_TraceHull(vOrigin, nodeVector, dont_ignore_monsters, human_hull, pEdict, &tr);
            if (pEdict) {
                UTIL_TraceHull(vOrigin, nodeVector, dont_ignore_monsters,
                               head_hull, pEdict->v.pContainingEntity,
                               &tr);
            } else {
                UTIL_TraceHull(vOrigin, nodeVector, dont_ignore_monsters,
                               head_hull, nullptr, &tr);
            }

            // if nothing hit:
            if (tr.flFraction >= 1.0f && pEdict != nullptr) {
                // Regardless of conditions, set iFarNode to iNode
                iFarNode = iNode;
            }
        }
    }
    return iFarNode;
}

// Adds a neighbour connection to a node ID
bool cNodeMachine::add_neighbour_node(int iNode, int iToNode) {
    if (iNode < 0)
        return false;

    tNode *node = getNode(iNode);
    const int iNeighbourId = freeNeighbourNodeIndex(node);
    if (iNeighbourId > -1) {
        node->iNeighbour[iNeighbourId] = iToNode;
        return true;
    }

    return false;
}

/**
 * Removes a neighbour connection from a node ID. Do note that we pass in node id's, ie not the actual indexes
 * of the iNeighbour[] array.
 * @param iFrom
 * @param iTo
 * @return
 */
bool cNodeMachine::removeConnection(int iFrom, int iTo) {
    if (iFrom < 0 || iTo < 0) {
        return false;
    }

    char msg[255] = {};

    tNode *node = getNode(iFrom);
    if (!node) {
        snprintf(msg, sizeof(msg), "(trouble) cNodeMachine::removeConnection | From %d, to %d has no node! (error)\n", iFrom, iTo);
        rblog(msg);
        return false;
    }

    bool removedOneOrMoreNeighbours = false;

    for (int i = 0; i < MAX_NEIGHBOURS; i++) {
	    const int neighbourNode = node->iNeighbour[i];

        snprintf(msg, sizeof(msg),
                "(trouble) removeConnection(from->%d, to->%d), evaluating neighbour [%d] = node %d\n",
                iFrom,
                iTo,
                i,
                neighbourNode);

        rblog(msg);

        if (neighbourNode == iTo) {
            rblog("(trouble) this is the connection to remove\n");
            node->iNeighbour[i] = -1;
            removedOneOrMoreNeighbours = true;
        }
    }

    if (removedOneOrMoreNeighbours) {
        ClearTroubledConnection(iFrom, iTo);
    }

    return removedOneOrMoreNeighbours;
}

// Removes ALL neighbour connections on iNode
bool cNodeMachine::remove_neighbour_nodes(int iNode) {
    if (iNode < 0)
        return false;

    for (int& i : Nodes[iNode].iNeighbour)
	    i = -1;

    return true;
}

// returns the next free 'neighbour id' for that node
int cNodeMachine::freeNeighbourNodeIndex(const tNode *Node) {
    for (int i = 0; i < MAX_NEIGHBOURS; i++) {
        if (Node->iNeighbour[i] < 0) {
            return i;
        }
    }

    return -1;
}

int cNodeMachine::is_neighbour_node(const tNode& node, int iNode)
{
    for (int i = 0; i < MAX_NEIGHBOURS; i++) {
		if (node.iNeighbour[i] == iNode) {
			return i;
		}
	}

	return -1;
}

// Return the node id from bot path on Index NR
int cNodeMachine::getNodeIndexFromBotForPath(int botIndex, int pathNodeIndex) const
{
    if (botIndex > -1 && botIndex < MAX_BOTS &&
        pathNodeIndex > -1 && pathNodeIndex < MAX_PATH_NODES) {
        return iPath[botIndex][pathNodeIndex];
    }

    return -1;                   // nothing found
}

// Compute the horizontal distance between A and B (ignoring z coordinate)
static float horizontal_distance(const Vector& a, const Vector& b) {
    return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

#define STEP    20              //Incremental move

// Return the floor below V
// TO BE IMPROVED use pEntityCOntaining
static Vector FloorBelow(const Vector& V) {
    static TraceResult tr;       // Keep it available even outside of the call

    // First use this hull
    int HullNumber = human_hull;
    float HullHeight = 36.0f;

    // Bump V a little higher (to allow for a steep climb)
    Vector UpALittle = V + Vector(0, 0, HullHeight);
    Vector ReallyDown = V + Vector(0, 0, -500);
    UTIL_TraceHull(UpALittle, ReallyDown, ignore_monsters, HullNumber, nullptr, &tr);
    //printf("      Floor %.0f -> %.0f, TraceHull fraction = %.2f, vecEndPos.z=%.0f %s %s\n",
    //UpALittle.z,ReallyDown.z,tr.flFraction,tr.vecEndPos.z,
    //(tr.fAllSolid) ? "AllSolid" : "",
    //(tr.fStartSolid) ? "StartSolid" : "") ;
    if (tr.fStartSolid) {        // Perhaps we where too high and hit the ceiling
        UpALittle = V + Vector(0, 0, 0);
        ReallyDown = V + Vector(0, 0, -500);
        UTIL_TraceHull(UpALittle, ReallyDown, ignore_monsters, HullNumber, nullptr, &tr);
        //printf("      Floor without raising %.0f -> %.0f, TraceHull fraction = %.2f, vecEndPos.z=%.0f %s %s\n",
        //UpALittle.z,ReallyDown.z,tr.flFraction,tr.vecEndPos.z,
        //(tr.fAllSolid) ? "AllSolid" : "",
        //(tr.fStartSolid) ? "StartSolid" : "") ;
        if (tr.fStartSolid) {     // Fall back to a point_hull
            HullNumber = point_hull;
            HullHeight = 0;
            UpALittle = V + Vector(0, 0, STEP);
            ReallyDown = V + Vector(0, 0, -500);
            UTIL_TraceHull(UpALittle, ReallyDown, ignore_monsters, HullNumber, nullptr, &tr);
            //printf("      Floor with point hull %.0f -> %.0f, TraceHull fraction = %.2f, vecEndPos.z=%.0f %s %s\n",
            //UpALittle.z,ReallyDown.z,tr.flFraction,tr.vecEndPos.z,
            //(tr.fAllSolid) ? "AllSolid" : "",
            //(tr.fStartSolid) ? "StartSolid" : "") ;
            if (tr.fStartSolid) {
                tr.vecEndPos = V;
                tr.vecEndPos.z = -4000;
            }
        }
    }

    tr.vecEndPos.z -= HullHeight;        //Look at feet position (depends on used hull)
    return tr.vecEndPos;
}

// Check whether we can reach End from Start being a normal player
// It uses (or should use) different heuristics:
// - plain walking on a floor with upward jumps or downward falls
// - downward falls either from solid ground or from a point in the air
//   to a point in water or a point in the air
// - ducking and walking (assuming flat ground)
// - swimming
int cNodeMachine::Reachable(const int iStart, const int iEnd) const
{
    Vector IncMove, Check, Floor;
    float Dist, Height, PreviousHeight;
    TraceResult tr;

    const Vector Start = Nodes[iStart].origin;
    const Vector End = Nodes[iEnd].origin;
#ifdef DEBUG_REACHABLE
    printf("Reachable %d(%.0f,%.0f,%.0f)%s", iStart, Start.x, Start.y,
           Start.z,
           (Nodes[iStart].iNodeBits & BIT_LADDER) ? "OnLadder" : "");
    printf(" >> %d(%.0f,%.0f,%.0f)%s\n", iEnd, End.x, End.y, End.z,
           (Nodes[iEnd].iNodeBits & BIT_LADDER) ? "OnLadder" : "");
#endif
    // Just in case
    if (static_cast<int>(Start.x) == 9999 || static_cast<int>(End.x) == 9999)
        return false;

    // Quick & dirty check whether we can go through...
    // This is simply to quickly decide whether the move is impossible
    UTIL_TraceHull(Start, End, ignore_monsters, point_hull, nullptr, &tr);
#ifdef DEBUG_REACHABLE

    printf("TraceHull --> tr.flFraction = %.2f\n", tr.flFraction);
#endif

    if (tr.flFraction < 1.0f)
        return false;

    // If either start/end is on ladder, assume we can fly without falling
    // but still check whether a human hull can go through
    if (Nodes[iStart].iNodeBits & BIT_LADDER ||
        Nodes[iEnd].iNodeBits & BIT_LADDER)
    {
        UTIL_TraceHull(Start, End, ignore_monsters, human_hull, nullptr, &tr);
        return tr.flFraction >= 1.0f;
    }

    // Heurestic for falling
    // TODO

    // Heurestic for walking/jumping/falling on the ground
    // Now check whether we have ground while going from Start to End...

    // Initialize IncMove to a STEP units long vector from Start to End
    IncMove = End - Start;
    IncMove = IncMove.Normalize();
    IncMove = IncMove * STEP;

    // Move Check towards End by small increment and check Z variation
    // Assuming that the move is a pure line TO BE IMPROVED !!

    Check = Start;
    Dist = (End - Check).Length();
    Floor = FloorBelow(Check);
    PreviousHeight = Floor.z;
    while (Dist > STEP) {
        Floor = FloorBelow(Check);
        Height = Floor.z;
#ifdef DEBUG_REACHABLE

        printf
        ("   in loop Check=(%.0f,%.0f,%.0f), Dist = %.0f, Height = %.0f\n",
         Check.x, Check.y, Check.z, Dist, Height);
#endif

        if (Height > PreviousHeight) {    // Going upwards
            if (Height - PreviousHeight > MAX_JUMPHEIGHT) {
                //printf("   too high for upward jump\n") ;
                return false;
            }
        } else {                  // Going downwards
            if (PreviousHeight - Height > MAX_FALLHEIGHT)
                if (UTIL_PointContents(Floor + Vector(0, 0, 5))
                    != CONTENTS_WATER)
                    //{printf("   too high for a downward fall not in water\n") ;
                    return false;    // Falling from too high not in water
            //}
            //else printf("     ouf we are in water\n") ;
        }
        // TO BE IMPROVED should check whether the surface is walkable & is not LAVA
        PreviousHeight = Height;
        Check = Check + IncMove;
        Dist = (End - Check).Length();
    }
    Height = End.z - 36.0f;
#ifdef DEBUG_REACHABLE

    printf("   after loop End=(%.0f,%.0f,%.0f), Height = %.0f\n",
           End.x, End.y, End.z, Height);
#endif

    if (Height > PreviousHeight) {       // Going upwards
        if (Height - PreviousHeight > MAX_JUMPHEIGHT) {
            //printf("   too high for upward jump\n") ;
            return false;
        }
    } else {                     // Going downwards
        if (PreviousHeight - Height > MAX_FALLHEIGHT)
            if (UTIL_PointContents(End) != CONTENTS_WATER) {
                //printf("   too high for a downward fall not in water\n") ;
                return false;       // Falling from too high not in water
            }
        //else printf("     ouf we are in water\n") ;
    }
    // TODO TO BE IMPROVED should check whether the surface is walkable & is not LAVA

    // Heurestic for ducking
    // TODO

    // Heurestic for swimming
    // TODO

    // Success !
    return true;
}

// Adding a node: another way...
int cNodeMachine::add2(const Vector& vOrigin, int iType, edict_t *pEntity) {
    // Do not add a node when there is already one close
    if (getClosestNode(vOrigin, NODE_ZONE, pEntity) > -1)
        return -1;

    const int newNodeIndex = getFreeNodeIndex();

    if (newNodeIndex >= MAX_NODES || newNodeIndex < 0) {
        return -1;
    }

    Nodes[newNodeIndex].origin = vOrigin;
    iMaxUsedNodes = std::max(newNodeIndex, iMaxUsedNodes);

    // Set different flags about the node
    Nodes[newNodeIndex].iNodeBits = iType;  // EVY's extension
    if (pEntity) {
        // ladder
        if (FUNC_IsOnLadder(pEntity))
            Nodes[newNodeIndex].iNodeBits |= BIT_LADDER;

        // water at origin (=waist) or at the feet (-30)
        if (UTIL_PointContents(pEntity->v.origin) == CONTENTS_WATER ||
            UTIL_PointContents(pEntity->v.origin - Vector(0, 0, 30)) ==
            CONTENTS_WATER)
            Nodes[newNodeIndex].iNodeBits |= BIT_WATER;

        // record jumping
        if (pEntity->v.button & IN_JUMP)
            Nodes[newNodeIndex].iNodeBits |= BIT_JUMP;
    } else {
        // water at origin (=waist) or at the feet (-30)
        if (UTIL_PointContents(vOrigin) == CONTENTS_WATER ||
            UTIL_PointContents(vOrigin - Vector(0, 0, 30)) == CONTENTS_WATER)
            Nodes[newNodeIndex].iNodeBits |= BIT_WATER;
    }

    // add to meredians
    int iX, iY;
    VectorToMeredian(Nodes[newNodeIndex].origin, &iX, &iY);

    if (iX > -1 && iY > -1) {
        AddToMeredian(iX, iY, newNodeIndex);
    } else {
        rblog("ERROR: Could not add node, no meredian found to add to.\n");
        return -1;
    }

    // Connect this node to other nodes (and vice versa)
    // TODO should use the Meredian structure to only check for nodes in adjacents meredians... (faster algo)
    int MyNeighbourCount, j;
    for (j = 0, MyNeighbourCount = 0;
         j < iMaxUsedNodes && MyNeighbourCount < MAX_NEIGHBOURS; j++) {

        if (j == newNodeIndex)
            continue;              // Exclude self

        if (Nodes[j].origin == Vector(9999, 9999, 9999))
            continue;              // Skip non existing nodes

        // When walking the human player can't pass a certain speed and distance
        // however, when a human is falling, the distance will be bigger.
        constexpr int maxDistance = 3 * NODE_ZONE;

        if (horizontal_distance(Nodes[newNodeIndex].origin, Nodes[j].origin) > maxDistance)
            continue;

        // j is a potential candidate for a neighbour
        // Let's do further tests

        if (Reachable(newNodeIndex, j)) {             // Can reach j from newNodeIndex ?
            add_neighbour_node(newNodeIndex, j);      // Add j as a neighbour of mine
            MyNeighbourCount++;
        }

        if (Reachable(j, newNodeIndex)) {             // Can reach newNodeIndex from j ?
            add_neighbour_node(j, newNodeIndex);      // Add i as a neighbour of j
        }
    }

    return newNodeIndex;
}

/**
 * Returns a free node index, this is not bound to a meridian (subcluster)!
 * @return
 */
int cNodeMachine::getFreeNodeIndex() const
{
	for (int i = 0; i < MAX_NODES; i++) {
        if (Nodes[i].origin == INVALID_VECTOR) {
            return i;
            //break;
        }
    }
    return -1; // no free node found
}

// Adding a node
int cNodeMachine::addNode(const Vector& vOrigin, edict_t *pEntity) {

    // Do not add a node when there is already one close
    if (getClosestNode(vOrigin, NODE_ZONE, pEntity) > -1)
        return -1;

    const int currentIndex = getFreeNodeIndex();

    // failed to find free node, bail
    if (currentIndex < 0) {
        return -1;
    }

    Nodes[currentIndex].origin = vOrigin;

    // SET BITS:
    bool bIsInWater = false;
    bool indexNodeFloats;
    bool bIndexOnCrate;

    if (pEntity) {
        // Does this thing float?
        indexNodeFloats = node_float(Nodes[currentIndex].origin, pEntity);
        bIndexOnCrate = node_on_crate(Nodes[currentIndex].origin, pEntity);

        if (FUNC_IsOnLadder(pEntity))
            Nodes[currentIndex].iNodeBits |= BIT_LADDER;

        // water
        if (UTIL_PointContents(pEntity->v.origin) == CONTENTS_WATER) {
            Nodes[currentIndex].iNodeBits |= BIT_WATER;
            bIsInWater = true;
        }

        // record jumping
        // FIXED: removed ; , thanks Whistler
        if (pEntity->v.button & IN_JUMP)
            Nodes[currentIndex].iNodeBits |= BIT_JUMP;

        if (FUNC_IsOnLadder(pEntity))
            indexNodeFloats = false;

        if (pEntity->v.button & IN_DUCK) 
            // ??
            Nodes[currentIndex].iNodeBits |= BIT_DUCK;

    	//Experimental - This new node requires for bots to vault onto crates and edges? [APG]RoboCop[CL]
        if (pEntity->v.button & BIT_DUCKJUMP)
            Nodes[currentIndex].iNodeBits |= BIT_DUCKJUMP;

    }                            // do only check pEntity when its not NULL
    else {
        // Does this thing float?
        indexNodeFloats = node_float(Nodes[currentIndex].origin, nullptr);
        bIndexOnCrate = node_on_crate(Nodes[currentIndex].origin, nullptr);
    }

    // add to subcluster
    int iX, iY;
    VectorToMeredian(Nodes[currentIndex].origin, &iX, &iY);

    if (iX > -1 && iY > -1) {
        AddToMeredian(iX, iY, currentIndex);
    }
    // done.

    // determine which other nodes are to be connected
    TraceResult tr;              // for traceresults
    int neighbourId = 0;                 // for neighbours

    // Connect this node to other nodes (and vice versa)
    for (int nodeIndex = 0; nodeIndex < MAX_NODES; nodeIndex++) {

        if (nodeIndex == currentIndex || // exclude self
            Nodes[nodeIndex].origin == INVALID_VECTOR // exclude invalid Nodes (no origin set)
                )
            continue;

        // remove the z-axis, so we just compare x,y now
        Vector vNormalizedOrigin = Nodes[nodeIndex].origin;
        Vector vNormalizedIndex = Nodes[currentIndex].origin;

        vNormalizedOrigin.z = 0;
        vNormalizedIndex.z = 0;

        // When walking the human player can't pass a certain speed and distance
        // however, when a human is falling (or walking a slope), the distance will be bigger.
        const float distanceBetweenVectorsWithoutZAxis = func_distance(vNormalizedOrigin, vNormalizedIndex);
        if (distanceBetweenVectorsWithoutZAxis > NODE_ZONE * 3) { // allow up to 3 times the boundary we use normally
            continue;
        }

        // ----------------------
        // The node is close enough, is valid, and not the same as the created one.
        // ----------------------

        // Traceline from nodeIndex to index
        bool bNeighbourFloats = node_float(Nodes[nodeIndex].origin, pEntity);
        const bool bNeighOnCrate = node_on_crate(Nodes[nodeIndex].origin, pEntity);
        bool bNeighbourWater = false;

        // when pEntity is on ladder, it is NOT floating!
        if (FUNC_IsOnLadder(pEntity))
            bNeighbourFloats = false;

        if (Nodes[nodeIndex].iNodeBits & BIT_LADDER)
            bNeighbourFloats = false;

        if (Nodes[nodeIndex].iNodeBits & BIT_WATER)
            bNeighbourWater = true;

        // Check if Index is LOWER then J, if so, the height should be jumpable!
        // When height does not differ to much we can add this connection.
        //
        // 14/06/04 - fix: Some slopes are not counted in here..., de_dust jump from crates connects now
        const bool nodeIsHigherThanOrigin = Nodes[nodeIndex].origin.z > Nodes[currentIndex].origin.z;

        // this assumes 'our' position is lower, this determines 'slope' distance
        const vec_t slopeDistance = Nodes[nodeIndex].origin.z - Nodes[currentIndex].origin.z;

        // this assumes 'our' position is higher, so we can determine fall distance
        const vec_t fallDistance = Nodes[currentIndex].origin.z - Nodes[nodeIndex].origin.z;

        if (indexNodeFloats
            && fallDistance > MAX_FALLHEIGHT
            && (!bNeighbourWater && !bIsInWater)) {
            // skip nodes that are not in water and too low (ie will cause fall damage)
            continue;
        }

        // skip nodes which are to high (fall damage)
        if (fallDistance > MAX_FALLHEIGHT) {
            continue;
        }

        // Index is not floating, we should check if we can jump it
        // 14/06/04 - temporary fix: We distinguish 2 kind of checks
        // when the nodes are on crates or not.
        if (bIndexOnCrate || bNeighOnCrate) {
            if (indexNodeFloats == false &&   // we stand on something
                nodeIsHigherThanOrigin &&       // we MUST jump (not fall)
                bNeighbourWater == false &&
                std::fabs(slopeDistance) > MAX_JUMPHEIGHT)      // and cannot jump to it
            {
                //                        SERVER_PRINT("Cannot jump to it");
                continue;           // next neighbour
            }
        } else {
            // both are on steep, do a bit simplified check
            if (indexNodeFloats == false &&   // we stand on something
                nodeIsHigherThanOrigin &&       // we MUST jump (not fall)
                bNeighbourWater == false &&
                std::fabs(slopeDistance) > MAX_FALLHEIGHT)      // and cannot jump to it
            {
                //                        SERVER_PRINT("Insanity jump not possible either!\n");
                continue;           // next neighbour
            }
        }

        int hull_type = head_hull;

        if (FUNC_IsOnLadder(pEntity)) {
            hull_type = point_hull;
        } else {
            // falling
            // 14/06/05 - this code does not cause bad connections! - Stefan
            // 20/06/05 - oops, it does... because it blocks when we are NOT falling...
            if (slopeDistance > MAX_FALLHEIGHT) {
                hull_type = human_hull; // when we fall, be sure we can freely fall.
            }

            // when we go steep, we have a point hull!!
            if ((bIndexOnCrate && bNeighOnCrate == false)
                || (bIndexOnCrate == false && bNeighOnCrate)) {
                // 20/06/05 - head_hull -> point_hull, somehow 'down stairs' does not get connected well
                // probably because head_hull is the size of a ducking player. So lets do a point_hull
                // then...

                // it seems that when going down the human_hull is used,
                hull_type = point_hull;
            }
        }

        // trace
        UTIL_TraceHull(Nodes[currentIndex].origin, Nodes[nodeIndex].origin, ignore_monsters,
			hull_type, pEntity->v.pContainingEntity, &tr);

        // if nothing hit:
        if (tr.flFraction >= 1.0f) {
            // Add this to the neighbouring list
            Nodes[currentIndex].iNeighbour[neighbourId] = nodeIndex;

            // Do a reverse check
            bool bCanConnect = true;
            // TODO: Stefan 30/08/2019 - this can be refactored into a function "canConnect" or something!?

            // ---- the same checks as above, but reversed ----

            // Check if J is LOWER then Index, if so, the height should be jumpable!
            // When height does not differ to much we can add this connection.
            if (indexNodeFloats && bNeighbourFloats
                && (bNeighbourWater == false && bIsInWater == false)
                && Nodes[currentIndex].origin.z >= Nodes[nodeIndex].origin.z) {
                char msg[80];
                snprintf(msg, sizeof(msg), "J.Z = %f, INDEX.Z = %f\n", Nodes[nodeIndex].origin.z,
                        Nodes[currentIndex].origin.z);
                //              UTIL_ClientPrintAll( HUD_PRINTNOTIFY, msg);
                bCanConnect = false;        // cannot connect
            }

            // Index is not floating, we should check if we can jump it
            if (bNeighbourFloats == false &&       // we stand on something
                Nodes[currentIndex].origin.z > Nodes[nodeIndex].origin.z &&       // we MUST jump (not fall)
                bIsInWater == false &&
                std::fabs(fallDistance) > MAX_JUMPHEIGHT)   // and cannot jump to it
                bCanConnect = false;        // cannot connect

            // All water stuff can connect to each other
            if (bNeighbourWater && bIsInWater)
                bCanConnect = true;

            if (bCanConnect) {
	            const int jNeigh = freeNeighbourNodeIndex(&Nodes[nodeIndex]);
                if (jNeigh > -1)
                    Nodes[nodeIndex].iNeighbour[jNeigh] = currentIndex;     // reversed also possible
            }

            neighbourId++;
            if (neighbourId > MAX_NEIGHBOURS)
                break;              // found enough neighbours

        } else {
            //              UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "NodeMachine: Hit something!!!!\n");
            //                if (tr.pHit != NULL)
            //                        UTIL_ClientPrintAll(HUD_PRINTNOTIFY, STRING(tr.pHit->v.classname));
        }

    }

    iMaxUsedNodes = std::max(currentIndex, iMaxUsedNodes);

    //UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "NodeMachine: Succesfully added node\n");
    return currentIndex;
}

// Players initialization
void cNodeMachine::init_players() {
    for (tPlayer& Player : Players)
    {
	    Player.vPrevPos = Vector(9999, 9999, 9999);
	    Player.iNode = -1;
    }
}

// Init on roundstart
void cNodeMachine::init_round() {
    //UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "NodeMachine: Roundstart\n");
    for (int index = 1; index <= gpGlobals->maxClients; index++) {
	    const edict_t *pPlayer = INDEXENT(index);

        // skip invalid players
        if (pPlayer && !pPlayer->free)
            Players[(index - 1)].vPrevPos = pPlayer->v.origin;
    }

    // Init paths
    for (int p = 0; p < 32; p++)
        path_clear(p);

    // decrease bad score for bad goal nodes
    for (tGoal& Goal : Goals)
	    if (Goal.iBadScore > 0)
		    Goal.iBadScore--;
}

/**
 * Look at players and plot nodes if not any nearby.
 */
void cNodeMachine::addNodesForPlayers() {
    for (int index = 1; index <= gpGlobals->maxClients; index++) {
        edict_t *pPlayer = INDEXENT(index);

        // skip invalid (dead, not playing) players
        if (pPlayer && !pPlayer->free) {
            if (pPlayer->free) continue;
            if (!IsAlive(pPlayer)) continue;

            const int iPlayerIndex = index - 1;

            // within a certain distance no node found? add one
            if (func_distance(pPlayer->v.origin, Players[iPlayerIndex].vPrevPos) > NODE_ZONE) {
                Players[iPlayerIndex].vPrevPos = pPlayer->v.origin;
                add2(pPlayer->v.origin, 0, pPlayer);
            }
        }
    }
}

// Draw connections of the node we are standing on
void cNodeMachine::connections(edict_t *pEntity) const
{
    int closeNode;
    char msg[75] = {};
    if (draw_nodepath > -1 && draw_nodepath < 32) {
        cBot botPointer = bots[draw_nodepath];
        if (botPointer.bIsUsed) {
            closeNode = botPointer.determineCurrentNodeWithTwoAttempts();
            if (closeNode > -1) {
	            const Vector &vector = Nodes[closeNode].origin;
                snprintf(msg, sizeof(msg), "Bot [%s|%d] is at node %d (%f,%f,%f)\n", botPointer.name, draw_nodepath, closeNode, vector.x, vector.y, vector.z);
            } else {
                snprintf(msg, sizeof(msg), "Bot [%s|%d] is at node %d\n", botPointer.name, draw_nodepath, closeNode);
            }
        } else {
            closeNode = getClosestNode(pEntity->v.origin, NODE_ZONE, pEntity);
            if (closeNode > -1) {
	            const Vector &vector = Nodes[closeNode].origin;
                snprintf(msg, sizeof(msg), "No bot used for slot [%d], YOU are at node %d (%f,%f,%f)\n", draw_nodepath, closeNode, vector.x, vector.y, vector.z);
            } else {
                snprintf(msg, sizeof(msg), "No bot used for slot [%d], YOU are at node %d\n", draw_nodepath, closeNode);
            }
        }
    } else {
        closeNode = getClosestNode(pEntity->v.origin, NODE_ZONE, pEntity);
        snprintf(msg, sizeof(msg), "YOU are at node %d\n", closeNode);
    }

    CenterMessage(msg);

    if (closeNode > -1) {
        for (int j = 0; j < MAX_NEIGHBOURS; j++) {
	        const tNode &node = Nodes[closeNode];
            const int neighbourNode = node.iNeighbour[j];

            if (neighbourNode > -1) {
	            const Vector start = node.origin;
	            const Vector end = Nodes[neighbourNode].origin;

                int red = 0;
                int green = 255;
                int blue = 0;

	            const int troubleIndex = GetTroubleIndexForConnection(closeNode, neighbourNode);

                if (troubleIndex > -1) {
	                const int tries = Troubles[troubleIndex].iTries;
                    if (tries <= 1) {
                        red = 255;
                        green = 250;
                        blue = 0;
                    }

                    if (tries == 2) {
                        red = 255;
                        green = 0;
                        blue = 0;
                    }

                    if (tries > 2) {
                        red = 150;
                        green = 0;
                        blue = 0;
                    }
                } else {
                    // other info?
                }

                DrawBeam(pEntity, start, end, 2, 0, red, green, blue, 255, 1);
            }
        }

    }
}

// Draw
void cNodeMachine::draw(edict_t* pEntity) const
{
    // DebugOut("waypoint: waypoint_draw()\n");
    int max_drawn = 0;

    // Declare 'start' vector outside the loop
    Vector start;

    for (const tNode& Node : Nodes)
    {
        if (Node.origin != Vector(9999, 9999, 9999)) {
            start = Node.origin - Vector(0, 0, 36);
            Vector end = Node.origin;

            const bool good = VectorIsVisibleWithEdict(pEntity, end, "none");

            const int angle_to_waypoint =
                FUNC_InFieldOfView(pEntity, end - pEntity->v.origin);

            if (good && angle_to_waypoint < 65) {

                int g, b, l;
                int r = g = b = l = 250;

                // l = 250; // Normally light is 250

                if (Node.iNodeBits & BIT_LADDER)
                    b = g = 0;

                if (Node.iNodeBits & BIT_WATER)
                    r = g = 0;

                if (Node.iNodeBits & BIT_DUCK)
                    r = b = 50;
                // Jump and DuckJump were missing for those nodes? [APG]RoboCop[CL]
                if (Node.iNodeBits & BIT_JUMP)
                    r = b = 100;

                if (Node.iNodeBits & BIT_DUCKJUMP)
                    r = b = 150;

                if (Node.iNeighbour[0] < 0)
                    r = 0;

                if (max_drawn < 39) {
                    DrawBeam(pEntity, start, end, 4, 0, r, g, b, l, 1);
                    max_drawn++;
                }
            }
        } // for
    }

    const int iNodeClose = getClosestNode(pEntity->v.origin, NODE_ZONE, pEntity);
    if (iNodeClose >= static_cast<int>(std::size(Nodes))) {
        char msg[50];
        snprintf(msg, sizeof(msg), "Node %d\nMe: (%.0f,%.0f,%.0f)\n", iNodeClose,
            pEntity->v.origin.x, pEntity->v.origin.y,
            pEntity->v.origin.z);
        CenterMessage(msg);
        return;
    }

    char msg[50];
    char Flags[10];

    Flags[0] = 0;

    if (iNodeClose >= 0)
    {
        if (Nodes[iNodeClose].iNodeBits & BIT_LADDER)
            std::strcat(Flags, "L");
        if (Nodes[iNodeClose].iNodeBits & BIT_WATER)
            std::strcat(Flags, "W");
        if (Nodes[iNodeClose].iNodeBits & BIT_JUMP)
            std::strcat(Flags, "J");
        if (Nodes[iNodeClose].iNodeBits & BIT_DUCK)
            std::strcat(Flags, "D");
        // Experimental DuckJump added for this new node [APG]RoboCop[CL]
        if (Nodes[iNodeClose].iNodeBits & BIT_DUCKJUMP)
            std::strcat(Flags, "h");
    }
    snprintf(msg, sizeof(msg), "Node %d(%.0f,%.0f,%.0f)%s\nMe: (%.0f,%.0f,%.0f)\n",
        iNodeClose, iNodeClose < 0 ? -1 : Nodes[iNodeClose].origin.x,
        iNodeClose < 0 ? -1 : Nodes[iNodeClose].origin.y,
        iNodeClose < 0 ? -1 : Nodes[iNodeClose].origin.z, Flags,
        pEntity->v.origin.x, pEntity->v.origin.y, pEntity->v.origin.z);
    CenterMessage(msg);
}

// Save Experience
void cNodeMachine::experience_save() {
    char dirname[256];
    char filename[256];

    // Set Directory name
    std::strcpy(dirname, "data/cstrike/exp/");
    std::strcat(dirname, STRING(gpGlobals->mapname));
    std::strcat(dirname, ".rbx");     // nodes file

    // writes whole path into "filename", Linux compatible
    UTIL_BuildFileNameRB(dirname, filename);

    // Only save if lock type is < 1
    FILE* rbl = std::fopen(filename, "wb");

    if (rbl != nullptr) {
	    constexpr int iVersion = FILE_EXP_VER2;
        std::fwrite(&iVersion, sizeof(int), 1, rbl);

        struct Node {
            Vector fDanger[2];
            Vector fContact[2];
        };

	    //TODO: To resolve the overflow of the buffer issue [APG]RoboCop[CL]
        for (int i = 0; i < MAX_NODES; i++) {
	        Node localInfoNodes[MAX_NODES];

	        std::fwrite(&localInfoNodes[i].fDanger[0], sizeof(Vector), 1, rbl);
            std::fwrite(&localInfoNodes[i].fDanger[1], sizeof(Vector), 1, rbl);
        	
            std::fwrite(&localInfoNodes[i].fContact[0], sizeof(Vector), 1, rbl);
            std::fwrite(&localInfoNodes[i].fContact[1], sizeof(Vector), 1, rbl);
        }

        if (iMaxUsedNodes > MAX_NODES)
            iMaxUsedNodes = MAX_NODES;

        // Here write down the MAX amounts of nodes used from vis table!
        const unsigned long iSize = iMaxUsedNodes * MAX_NODES / 8;

        std::fwrite(&iMaxUsedNodes, sizeof(int), 1, rbl);

        // Write down 512 bytes chunks, and when a remaining piece is left over
        // also write that down. Only when 'size' is 0 we quit the loop
        unsigned long iPos = 0;
        unsigned long iChunk = 512;

        // When we exceed the size we make sure that the chunk is not to big
        if (iSize < iChunk)
            iChunk = 512 - iSize;

        unsigned long iRemaining = iSize - iChunk;        // unsigned caused SEGV while below -- BERKED

        // While we still have bytes remaining to write to disk.
        while (iRemaining > 0) {
            std::fwrite(&cVisTable[iPos], iChunk, 1, rbl);

            iChunk = 512;          // keep the size 512

            // When we exceed the size we make sure that the chunk is not to big
            if (iRemaining < iChunk)
                iChunk = 512 - iRemaining;

            iRemaining -= iChunk;
            iPos += iChunk;        // increase position to read from cVisTable

            if (iRemaining < 1)
                break;              // escape
        }

        // write down the checked vis
        std::fwrite(iVisChecked, sizeof(iVisChecked), 1, rbl);
        std::fclose(rbl);
    } else
        std::fprintf(stderr, "Cannot write experience file %s\n", filename);
}

// Load Danger
void cNodeMachine::experience_load() {
    char dirname[256];
    char filename[256];

    // Set Directory name
    std::strcpy(dirname, "data/cstrike/exp/");
    std::strcat(dirname, STRING(gpGlobals->mapname));
    std::strcat(dirname, ".rbx");     // nodes file

    // writes whole path into "filename", Linux compatible
    UTIL_BuildFileNameRB(dirname, filename);

    FILE* rbl = std::fopen(filename, "rb");

    if (rbl != nullptr) {
	    int i;
	    int iVersion = FILE_EXP_VER1;
        std::fread(&iVersion, sizeof(int), 1, rbl);

        struct Node {
            Vector fDanger[2];
            Vector fContact[2];
        };
        Node localInfoNodes[MAX_NODES];

        if (iVersion == FILE_EXP_VER1) {

            //TODO: To resolve the overflow of the buffer issue [APG]RoboCop[CL]
            for (i = 0; i < MAX_NODES; i++) {
                std::fread(&localInfoNodes[i].fDanger[0], sizeof(Vector), 1, rbl);
                std::fread(&localInfoNodes[i].fDanger[1], sizeof(Vector), 1, rbl);
                std::fread(&localInfoNodes[i].fContact[0], sizeof(Vector), 1, rbl);
                std::fread(&localInfoNodes[i].fContact[1], sizeof(Vector), 1, rbl);
            }

            std::fread(&iMaxUsedNodes, sizeof(int), 1, rbl);

            // make sure we never exceed the limit
            if (iMaxUsedNodes > MAX_NODES)
                iMaxUsedNodes = MAX_NODES;

            const unsigned long iSize = iMaxUsedNodes * MAX_NODES / 8;

            // Read table from what we know
            std::fread(cVisTable, iSize, 1, rbl);
            std::fread(iVisChecked, sizeof(iVisChecked), 1, rbl);
        }
        else if (iVersion == FILE_EXP_VER2) {
            //TODO: To resolve the overflow of the buffer issue [APG]RoboCop[CL]
            for (i = 0; i < MAX_NODES; i++) {
                std::fread(&localInfoNodes[i].fDanger[0], sizeof(Vector), 1, rbl);
                std::fread(&localInfoNodes[i].fDanger[1], sizeof(Vector), 1, rbl);
                std::fread(&localInfoNodes[i].fContact[0], sizeof(Vector), 1, rbl);
                std::fread(&localInfoNodes[i].fContact[1], sizeof(Vector), 1, rbl);
            }

            std::fread(&iMaxUsedNodes, sizeof(int), 1, rbl);
            // make sure we never exceed the limit
            if (iMaxUsedNodes > MAX_NODES)
                iMaxUsedNodes = MAX_NODES;

            const unsigned long iSize = iMaxUsedNodes * MAX_NODES / 8;

            // Now read the cVisTable from what we know
            ClearVisibilityTable();        // clear first

            unsigned long iPos = 0;
            unsigned long iChunk = 512;
            unsigned long iRemaining = iSize;

            while (iRemaining > 0) {
                // Emergency, when iPos is getting to big, we get the heck out of here
                if (iPos > iSize)
                    break;

                // Read table from what we know
                std::fread(&cVisTable[iPos], iChunk, 1, rbl);

                // When we exceed the size we make sure that the chunk is not to big
                if (iRemaining < iChunk)
                    iChunk = 512 - iRemaining;

                iRemaining -= iChunk;
                iPos += iChunk;     // increase position to read from cVisTable

                if (iRemaining < 1)
                    break;           // escape
            }

            std::fread(iVisChecked, sizeof(iVisChecked), 1, rbl);
        }

        std::fclose(rbl);
    }
}

// Save
void cNodeMachine::save() const
{
    char dirname[256];
    char filename[256];

    // Set Directory name
    std::strcpy(dirname, "data/cstrike/maps/");
    std::strcat(dirname, STRING(gpGlobals->mapname));
    std::strcat(dirname, ".rbn");     // nodes file

    // writes whole path into "filename", Linux compatible
    UTIL_BuildFileNameRB(dirname, filename);

    // Only save if lock type is < 1
    FILE* rbl = std::fopen(filename, "wb");

    if (rbl != nullptr) {
        // Write down version number
        constexpr int iVersion = FILE_NODE_VER1;
        std::fwrite(&iVersion, sizeof(int), 1, rbl);
        for (const tNode& Node : Nodes)
        {
            std::fwrite(&Node.origin, sizeof(Vector), 1, rbl);
            for (const int& n : Node.iNeighbour)
	            std::fwrite(&n, sizeof(int), 1, rbl);

            // save bit flags
            std::fwrite(&Node.iNodeBits, sizeof(int), 1, rbl);
        }
        std::fclose(rbl);
    } else
        std::fprintf(stderr, "Cannot write file %s\n", filename);
}

void cNodeMachine::save_important() const
{
    char dirname[256];
    char filename[256];

    // Set Directory name
    std::strcpy(dirname, "data/cstrike/ini/");
    std::strcat(dirname, STRING(gpGlobals->mapname));
    std::strcat(dirname, ".ini");     // nodes file

    // writes whole path into "filename", Linux compatible
    UTIL_BuildFileNameRB(dirname, filename);

    // Only save if lock type is < 1
    FILE *rbl = std::fopen(filename, "w+t");

    if (rbl) {
        std::fprintf(rbl,
                "; RealBot : Important Area Definition file\n; Do not hand-edit, this is _not_ an editable ini file!\n;\n\n");

        // save important areas:
        for (const tGoal& Goal : Goals)
        {
            if (Goal.iType == GOAL_IMPORTANT) {
                // save this area
                std::fprintf(rbl, "[AREA]\n");
                const Vector iGoalVector = node_vector(Goal.iNode);
                std::fprintf(rbl, "X=%f\n", iGoalVector.x);
                std::fprintf(rbl, "Y=%f\n", iGoalVector.y);
                std::fprintf(rbl, "Z=%f\n\n", iGoalVector.z);

            }
        }

        std::fprintf(rbl, "; Eof");
        std::fclose(rbl);
    }
}

// Load
void cNodeMachine::load() {
    char dirname[256];
    char filename[256];

    // Set Directory name
    std::strcpy(dirname, "data/cstrike/maps/");

    std::strcat(dirname, STRING(gpGlobals->mapname));
    std::strcat(dirname, ".rbn");     // nodes file

    // writes whole path into "filename", Linux compatible
    UTIL_BuildFileNameRB(dirname, filename);

    FILE* rbl = std::fopen(filename, "rb");

    if (rbl != nullptr) {
	    int i;
	    int iVersion = FILE_NODE_VER1;
        std::fread(&iVersion, sizeof(int), 1, rbl);

        // Version 1.0
        if (iVersion == FILE_NODE_VER1) {
            for (i = 0; i < MAX_NODES; i++) {
                std::fread(&Nodes[i].origin, sizeof(Vector), 1, rbl);
                for (int& n : Nodes[i].iNeighbour)
                {
                    std::fread(&n, sizeof(int), 1, rbl);
                }

                // save bit flags
                std::fread(&Nodes[i].iNodeBits, sizeof(int), 1, rbl);

                if (Nodes[i].origin != Vector(9999, 9999, 9999))
                    iMaxUsedNodes = i;
            }
        }
        // Add nodes to meredians
        for (i = 0; i < MAX_NODES; i++)
            if (Nodes[i].origin != Vector(9999, 9999, 9999)) {
                int iX, iY;
                VectorToMeredian(Nodes[i].origin, &iX, &iY);
                if (iX > -1 && iY > -1)
                    AddToMeredian(iX, iY, i);
            }

        std::fclose(rbl);

        // 04/07/04
        // Check for full nodes table

        if (Nodes[MAX_NODES - 1].origin != Vector(9999, 9999, 9999))
            rblog("!!! Nodes table is full\n");

        char msg[80];
        snprintf(msg, sizeof(msg), "After NodeMachine::load iMaxUsedNodes=%d\n",
                iMaxUsedNodes);
        rblog(msg);
        SERVER_PRINT("Going to load IAD file : ");
        INI_PARSE_IAD();
    } else {
        //    char msg[128];
        //    sprintf(msg, "Notice: Could not open %s.\n");
        //    SERVER_PRINT(msg);
    }
}

void cNodeMachine::ClearImportantGoals() {
    for (tGoal& Goal : Goals)
    {
        if (Goal.iType == GOAL_IMPORTANT && Goal.iNode > -1) {
	        Goal.iType = -1;
	        Goal.iNode = -1;
	        Goal.pGoalEdict = nullptr;
            std::memset(Goal.name, 0, sizeof(Goal.name));
        }
    }
}

// Draw path 0 (user)
void cNodeMachine::path_draw(edict_t* pEntity) const
{
    //DebugOut("waypoint: waypoint_draw()\n");
    int max_drawn = 0;
	
    // Declare 'start' vector outside the loop
    Vector start;

    for (int i = 0; i < MAX_NODES; i++) {
        const int iNode = iPath[draw_nodepath][i];
        int iNextNode = -1; // Initialize to -1
    	
        if (i < MAX_NODES - 1) { // Check if i is less than MAX_NODES - 1
            iNextNode = iPath[draw_nodepath][(i + 1)];
        }
    	
        if (iNode > -1 && iNextNode > -1) {
	        start = Nodes[iNode].origin;
            Vector end = Nodes[iNextNode].origin;

	        const bool good = VectorIsVisibleWithEdict(pEntity, end, "none");
	        const int angle_to_waypoint =
                FUNC_InFieldOfView(pEntity, end - pEntity->v.origin);

            if (max_drawn < 39 && good && angle_to_waypoint < 50) {
	            constexpr int red = 255;
                int green = 0;
                int blue = 255;
                int width = 15;

                if (Nodes[iNode].iNodeBits & BIT_JUMP) {
                    width = 25;
                    green = 255;
                }

                if (Nodes[iNode].iNodeBits & BIT_DUCK) {
                    width = 5;
                    blue = 0;
                }

                DrawBeam(pEntity, start, end, width, 0, red, green, blue, 255, 5);
                max_drawn++;
            }
        } // if iNode
    } // for

//    int iNodeClose = getCloseNode(pEntity->v.origin, 35, pEntity);

//    char msg[30];
//    sprintf(msg, "At node %d\n", iNodeClose);
//    CenterMessage(msg);
}

// Spread contact areas
void cNodeMachine::contact(int iNode, int iTeam) {
    if (iNode < 0 || iNode >= MAX_NODES)
        return;

    if (iTeam < 0 || iTeam > 1)
        return;

    // Nodes near have more danger, increase with:
    // ((DISTANCE / NODE_CONTACT_DIST) * NODE_CONTACT_STEP)
    // example:
    // 200 / 512 = 0.39 * 0.2 = 0.07

    // Spot itself is + NODE_CONTACT_STEP;
    InfoNodes[iNode].fContact[iTeam] += NODE_CONTACT_STEP;

    // Go through all valid nodes, except iNode, and increase danger if needed.
    for (int i = 0; i < MAX_NODES; i++) {
        if (Nodes[i].origin != Vector(9999, 9999, 9999) && i != iNode) {
	        const float fDist = func_distance(Nodes[i].origin, Nodes[iNode].origin);
            if (fDist < NODE_CONTACT_DIST) {
                //Using TraceHull to detect de_aztec bridge and other entities.
                TraceResult tr;

                //UTIL_TraceHull(Nodes[iNode].origin, Nodes[i].origin, ignore_monsters, human_hull, NULL, &tr);
                UTIL_TraceHull(Nodes[iNode].origin, Nodes[i].origin,
                               ignore_monsters, point_hull, nullptr, &tr);

                // within distance and 'reachable'
                if (tr.flFraction >= 1.0f) {
	                const double costIncrease = fDist / NODE_CONTACT_DIST * NODE_CONTACT_STEP;
                    InfoNodes[i].fContact[iTeam] += static_cast<float>(costIncrease);
                }
            }
        }
    }
}

// Spread danger around
void cNodeMachine::danger(int iNode, int iTeam) {
    if (iNode < 0 || iNode >= MAX_NODES)
        return;

    if (iTeam < 0 || iTeam > 1)
        return;

    // Nodes near have more danger, increase with:
    // ((DISTANCE / NODE_DANGER_DIST) * NODE_DANGER_STEP)
    // example:
    // 200 / 512 = 0.39 * 0.2 = 0.07

    // The further away, the less danger increased. At actual
    // danger spot the danger increases with 10% (0.10)

    // Spot itself is + NODE_DANGER_STEP;
    InfoNodes[iNode].fDanger[iTeam] += NODE_DANGER_STEP;

    // Go through all valid nodes, except iNode, and increase danger if needed.
    for (int i = 0; i < MAX_NODES; i++) {
        if (Nodes[i].origin != Vector(9999, 9999, 9999) && i != iNode) {
	        const float fDist = func_distance(Nodes[i].origin, Nodes[iNode].origin);
            if (fDist < NODE_DANGER_DIST) {
                //Using TraceHull to detect de_aztec bridge and other entities.
                TraceResult tr;
                UTIL_TraceHull(Nodes[iNode].origin, Nodes[i].origin,
                               ignore_monsters, point_hull, nullptr, &tr);

                // within distance and reachable
                if (tr.flFraction >= 1.0f) {
	                const float costIncrease = fDist / NODE_DANGER_DIST * NODE_DANGER_STEP;
                    InfoNodes[i].fDanger[iTeam] += costIncrease;
                }
            }
        }
    }
}

// Adds a new goal to the array
void cNodeMachine::addGoal(edict_t *pEdict, int goalType, const Vector& vVec) {
    //
    // 14/06/04
    // Be carefull with adding SERVER_PRINT messages here
    // if so, only use them with maps that have NO RBN FILE! When loading
    // and RBN file with a corresponding INI file this will crash HL.EXE
    //

    // Note: pEdict can be NULL (ie 'important' goal nodes do not have an edict)
    if (hasGoalWithEdict(pEdict)) {
        return; // do not add goal that is already in our list
    }

    const int index = getFreeGoalIndex();
    if (index < 0) {
        return;
    }

    float distance = NODE_ZONE * 2;

    // some goals require very close nodes
    if (goalType == GOAL_HOSTAGE ||
        goalType == GOAL_VIPSAFETY ||
        goalType == GOAL_RESCUEZONE ||
        goalType == GOAL_BOMBSPOT) {
        distance = NODE_ZONE * 0.8f;
    }

    int nNode = getClosestNode(vVec, distance, pEdict);

    if (nNode < 0) {
        rblog("Cannot find near node, adding one");
        // 11/06/04 - stefan - when no goal exists, add it.
        nNode = addNode(vVec, pEdict);
        if (nNode < 0) {
            return;                // no node near
        }
    }

    tGoal *goal = getGoal(index);
    if (goal == nullptr) {
        rblog("No valid goal index found - bailing\n");
        return;
    }
    goal->iNode = nNode;
    goal->index = index;
    goal->pGoalEdict = pEdict;
    goal->iType = goalType;
    //strcpy(goal->name, getGoalTypeAsText(*goal)); //That appears to trigger crash [APG]RoboCop[CL]

    char msg[255] = {};
    snprintf(msg, sizeof(msg), "Adding goal at index %d of type %s, with nearby node %d\n", index, goal->name, nNode);
    rblog(msg);
}

tGoal *cNodeMachine::getGoal(int index) {
    if (index < 0 || index >= MAX_GOALS) {
        rblog("ERROR: Asking to retrieve goal with invalid index! Returning goal NULL\n");
        return nullptr;
    }
//    char msg[255];
//    sprintf(msg, "Getting goal by index [%d]\n", index);
//    rblog(msg);
    return &Goals[index];
}

/**
 * Looks into array of Goals and finds a non-used goal. If not found it will return -1, else it returns
 * a valid int between 0 and MAX_GOALS.
 * @return
 */
int cNodeMachine::getFreeGoalIndex() const {
    int index = -1;
    for (int g = 0; g < MAX_GOALS; g++) {
        if (Goals[g].iType == GOAL_NONE) {
            index = g;
            break;
        }
    }
    return index;
}

/**
 * Given a non-null pEdict, it will check if any goal already has this edict assigned, if so it returns true.
 * If pEdict argument provided is NULL, this function returns false.
 *
 * @param pEdict
 * @return
 */
bool cNodeMachine::hasGoalWithEdict(edict_t *pEdict) const
{
    if (pEdict == nullptr) return false; // no edict == by default no

    for (const tGoal& Goal : Goals)
    {
        if (Goal.pGoalEdict == pEdict) {
            return true;
        }
    }

    // Does not exist
    return false;
}

void cNodeMachine::resetCheckedValuesForGoals() {
    for (tGoal& Goal : Goals)
    {
	    Goal.iChecked = std::min(Goal.iChecked, 0);
    }
}

// returns goal type from node, -1 for unknown
int cNodeMachine::getGoalIndexFromNode(int iNode) const
{
    for (const tGoal& Goal : Goals)
	    if (Goal.iNode == iNode)
            return Goal.iType;

    return -1;
}

void cNodeMachine::updateGoals() {
    rblog("cNodeMachine::updateGoals - START\n");
    for (int i = 0; i < MAX_GOALS; i++) {
	    const tGoal *goal = getGoal(i);
        if (goal == nullptr || goal->iNode < 0) continue;

        if (goal->iType == GOAL_HOSTAGE) {
            initGoal(i);
        }

        if (goal->iType == GOAL_VIP) {
            initGoal(i);
        }
    }

    edict_t *pent = nullptr;

    // re-add goals for hostages so we have the latest information about them
    // GOAL #5 - Hostages (this is the 'starting' position):
    while ((pent = UTIL_FindEntityByClassname(pent, "hostage_entity")) != nullptr) {
        // verify hostage is still rescueable
        if (isHostageRescued(nullptr, pent) || !FUNC_EdictIsAlive(pent)) {
            continue; // skip dead or already rescued hostages
        }

        addGoal(pent, GOAL_HOSTAGE, pent->v.origin+ Vector(0,0,32));
    }

    // SEARCH PLAYERS FOR ENEMIES
    for (int i = 1; i <= gpGlobals->maxClients; i++) {
        edict_t* pPlayer = INDEXENT(i);
    	
        // skip invalid players and skip self (i.e. this bot)
        if (pPlayer && !pPlayer->free) {
            // skip this player if not alive (i.e. dead or dying)
            if (!IsAlive(pPlayer))
                continue;
            if (UTIL_IsVip(pPlayer)) {
                addGoal(pPlayer, GOAL_VIP, pPlayer->v.origin + Vector(0, 0, 32));
            }
        }
    }
    rblog("cNodeMachine::updateGoals - FINISHED\n");
}

/**
 * Called at start of map
 */
void cNodeMachine::setUpInitialGoals() {
    rblog("cNodeMachine::goals() - START\n");

    // make sure to initialize goals at start
    initGoals();

    // Called on round-start, will update goals,
    // because Nodes get expanded all the time the bot should eventually learn
    // how to reach other goals.

    edict_t *pent = nullptr;

    // GOAL #1 - Counter Terrorist Spawn points.
    while ((pent = UTIL_FindEntityByClassname(pent, "info_player_start")) != nullptr) {
        addGoal(pent, GOAL_SPAWNCT, pent->v.origin);
    }

    // GOAL #2 - Terrorist Spawn points.
    while ((pent = UTIL_FindEntityByClassname(pent, "info_player_deathmatch")) != nullptr) {
        addGoal(pent, GOAL_SPAWNT, pent->v.origin);
    }

    // GOAL #3 - Hostage rescue zone
    while ((pent = UTIL_FindEntityByClassname(pent, "func_hostage_rescue")) != nullptr) {
        addGoal(pent, GOAL_RESCUEZONE, VecBModelOrigin(pent));
    }

    // rescue zone can also be an entity of info_hostage_rescue
    while ((pent = UTIL_FindEntityByClassname(pent, "info_hostage_rescue")) != nullptr) {
        addGoal(pent, GOAL_RESCUEZONE, VecBModelOrigin(pent));
    }

    // GOAL #4 - Bombspot zone
    // Bomb spot
    while ((pent = UTIL_FindEntityByClassname(pent, "func_bomb_target")) != nullptr) {
        addGoal(pent, GOAL_BOMBSPOT, VecBModelOrigin(pent));
    }

    while ((pent = UTIL_FindEntityByClassname(pent, "info_bomb_target")) != nullptr) {
        addGoal(pent, GOAL_BOMBSPOT, VecBModelOrigin(pent));
    }

    // GOAL #5 - Hostages (this is the 'starting' position):
    while ((pent = UTIL_FindEntityByClassname(pent, "hostage_entity")) != nullptr) {
        addGoal(pent, GOAL_HOSTAGE, pent->v.origin + Vector(0,0,32));
    }

    // GOAL  #6 - VIP (this is the 'starting' position) (EVY)
    while ((pent = UTIL_FindEntityByClassname(pent, "info_vip_start")) != nullptr) {
        addGoal(pent, GOAL_VIP, VecBModelOrigin(pent));
    }

    // GOAL  #7 - VIP safety (this is the 'rescue' position) (EVY)
    while ((pent = UTIL_FindEntityByClassname(pent, "func_vip_safetyzone")) != nullptr) {
        addGoal(pent, GOAL_VIPSAFETY, VecBModelOrigin(pent));
    }

    // GOAL  #8 - Escape zone for es_ (EVY)
    while ((pent = UTIL_FindEntityByClassname(pent, "func_escapezone")) != nullptr) {
        addGoal(pent, GOAL_ESCAPEZONE, VecBModelOrigin(pent));
    }

    // 05/07/04
    // GOAL  #9 - Free weapons on the ground EVY
    while ((pent = UTIL_FindEntityByClassname(pent, "armoury_entity")) != nullptr) {
        addGoal(pent, GOAL_WEAPON, VecBModelOrigin(pent));
    }

    // TODO: Add important goals: (from ini file)

    rblog("cNodeMachine::goals() - END\n");
}

// Find a goal, and return the node close to it
tGoal* cNodeMachine::getRandomGoalByType(int goalType) {
    if (goalType == GOAL_NONE)
        return nullptr;
	
    int possibleGoalNodes[MAX_GOALS];
    for (int& possibleGoalNode : possibleGoalNodes)
    {
	    possibleGoalNode = -1;
    }
	
    int possibleCandidateIndex = 0;
    for (int goalIndex = 0; goalIndex < MAX_GOALS; goalIndex++) {
        if (Goals[goalIndex].iType == goalType && // type equals requested type
            Goals[goalIndex].iNode > -1) { // and it has a node
			// possibleGoalNodes[possibleCandidateIndex] = Goals[goalIndex].iNode;
            possibleGoalNodes[possibleCandidateIndex] = goalIndex;
            possibleCandidateIndex++;
        }
    }
	
    if (possibleCandidateIndex == 0)
        return nullptr;                // nothing found :(
	
    // we have an amount of goals, pick one randomly
    const int randomGoalIndex = RANDOM_LONG(0, possibleCandidateIndex - 1);
	
    char msg[255];
    snprintf(msg, sizeof(msg), "cNodeMachine::getRandomGoalByType() - Found %d nodes of type %d and picked %d\n",
        possibleCandidateIndex, goalType, randomGoalIndex);
    rblog(msg);
	
    return getGoal(possibleGoalNodes[randomGoalIndex]);
}

// Contact scaler (on round start)
void cNodeMachine::scale_contact() {
    // On round start: Search for highest and make that 1.0 in scale
    // rescale all other values to stay correct.
    int iTeam = 0;
    while (iTeam < 2) {
        float fHighest = 0.0f;
        int i;                // <-- ADDED BY PMB ELSE LINUX COMPILER ISNT HAPPY
        for (i = 0; i < MAX_NODES; i++)
	        fHighest = std::max(InfoNodes[i].fContact[iTeam], fHighest);

        if (fHighest < 1.0f) {
            iTeam++;
            continue;              // no need to rescale
        }
        // Check how much we passed the limit
        const float fLimit = 1.0f / fHighest;

        // Now rescale all
        for (i = 0; i < MAX_NODES; i++)
            if (InfoNodes[i].fContact[iTeam] > 0.0f)
                InfoNodes[i].fContact[iTeam] *= fLimit;     // rescale

        iTeam++;
        if (iTeam > 1)
            break;
    }
}

// Danger scaler (on round start)
void cNodeMachine::scale_danger() {
    // On round start: Search for highest danger and make that 1.0 in scale
    // rescale all other danger values to stay correct.
    int iTeam = 0;
    while (iTeam < 2) {
        float fHighest = 0.0f;
        int i;                // ADDED BY PMB FOR COMPILING UNDER LINUX
        for (i = 0; i < MAX_NODES; i++)
	        fHighest = std::max(InfoNodes[i].fDanger[iTeam], fHighest);

        if (fHighest < 0.8f) {
            iTeam++;
            continue;              // no need to rescale
        }
        // Check how much we passed the limit
        const float fLimit = 0.7f / fHighest;

        // Now rescale all
        for (i = 0; i < MAX_NODES; i++)
            if (InfoNodes[i].fDanger[iTeam] > 0.0f)
                InfoNodes[i].fDanger[iTeam] *= fLimit;      // rescale

        iTeam++;
        if (iTeam > 1)
            break;
    }
}

// Pathfinder
bool cNodeMachine::createPath(int nodeStartIndex, int nodeTargetIndex, int botIndex, cBot *pBot, int iFlags) {
    // Will create a path from nodeStartIndex to nodeTargetIndex, and store it into index number iPathId

    if (pBot) {
        char msg[255] = {};
        snprintf(msg, sizeof(msg), "createPath(from->%d, to->%d, botIndex->%d)", nodeStartIndex, nodeTargetIndex, botIndex);
        pBot->rprint("cNodeMachine::createPath", msg);
    }

    if (nodeStartIndex < 0 || nodeTargetIndex < 0 || botIndex < 0)
        return false; // do not create a path when invalid params given

    if (nodeStartIndex > iMaxUsedNodes || nodeTargetIndex > iMaxUsedNodes)
        return false; // do not create a path when invalid params given

    if (pBot) {
        UTIL_GetTeam(pBot->pEdict); // Stefan: yes we use 0-1 based, not 1-2 based
    }

    const Vector &INVALID_VECTOR = Vector(9999, 9999, 9999);

    // start or target vector may not be invalid
    if (Nodes[nodeStartIndex].origin == INVALID_VECTOR ||
        Nodes[nodeTargetIndex].origin == INVALID_VECTOR) {
        rblog("Invalid start and target index\n");
        return false;
    }

    int nodeIndex;

    path_clear(botIndex);

    // INIT: Start
    makeAllWaypointsAvailable();

    // Our start waypoint is open
    constexpr float gCost = 0.0f; // distance from starting node
    const float hCost = func_distance(Nodes[nodeStartIndex].origin,
                                      Nodes[nodeTargetIndex].origin); // distance from end node to node
    const float cost = gCost + hCost;
    closeNode(nodeStartIndex, nodeStartIndex, cost);
    openNeighbourNodes(nodeStartIndex, nodeStartIndex, nodeTargetIndex, -1);

    bool pathFound = false;           // is it still valid to loop through the lists for pathfinding?

    int nodesEvaluated = 0;
    // INIT: End
    // PATHFINDER: Start
    while (!pathFound) {
        float lowestScore = 99999999.0f;
        int nodeToClose = -1;

        // go through all OPEN waypoints
        for (nodeIndex = 0; nodeIndex < MAX_NODES; nodeIndex++) {
            nodesEvaluated++; // increment nodesEvaluated each time a node is evaluated [APG]RoboCop[CL]
        	tNodestar &nodeStar = astar_list[nodeIndex];

            if (nodeStar.state == CLOSED || nodeStar.state == AVAILABLE) continue;

            // OPEN waypoint
            tNode &node = Nodes[nodeIndex];
            if (node.origin == INVALID_VECTOR) {
                rblog("Evaluating an INVALID vector!?!\n");
                nodeStar.state = CLOSED;
                continue; // it is an invalid vector
            }

            // nodeIndex is target, so target found
            if (nodeIndex == nodeTargetIndex) {
                pathFound = true;
                rblog("Target found\n");
                break; // Get out of here
            }

            // open is not target, so go over its neighbours (which have been opened) and find the lowest cost
            if (nodeStar.cost < lowestScore) {
                nodeToClose = nodeIndex;
                lowestScore = nodeStar.cost;
            }
        }

        // a node that should be closed is an evaluated node and the most preferred one.
        // open up all neighbouring nodes, and close this one
        if (nodeToClose > -1) {
            astar_list[nodeToClose].state = CLOSED;
            int botTeam = -1;
            if (pBot) {
                botTeam = pBot->iTeam;
            }

            openNeighbourNodes(nodeStartIndex, nodeToClose, nodeTargetIndex, botTeam);
        } else {
            break;
        }
    }
    // PATHFINDER: End

    // RESULT: Success
    if (!pathFound) {
        if (pBot) {
            pBot->rprint("cNodeMachine::createPath", "Failed to create path");
        }
        return false;
    }

    for (nodeIndex = 0; nodeIndex < MAX_PATH_NODES; nodeIndex++) {
	    const tNodestar &nodeStar = astar_list[nodeIndex];
        if (nodeStar.state == AVAILABLE) continue;
    }

    // Build path (from goal to start, read out parent waypoint to backtrace)
    int temp_path[MAX_PATH_NODES];

    // INIT: Start
    for (nodeIndex = 0; nodeIndex < MAX_PATH_NODES; nodeIndex++)
        temp_path[nodeIndex] = -1;

    // The path has been built yet?
    bool built = false;

    // The variables needed to backtrace
    // wpta = waypoint we use to backtrace (starting at goal)
    // p = index for temp_path (the path will be GOAL-START, reversed later)
    int wpta = nodeTargetIndex, p = 0;

    // INIT: End

    // START: When path is not built yet
    while (!built) {
        temp_path[p] = wpta;   // Copy the waypoint into temp_path[index]

        // IF: At current (start) waypoint
        if (wpta == nodeStartIndex) {
            // Check if we did not already had this waypoint before
            built = true;       // We finished building this path.
        } else {
            // Whenever wpta is containing bad information...
            if (wpta < 0 || wpta > MAX_NODES)
                break;           // ...get out aswell
        }

        // waypoint we use to backtrace will be set to parent waypoint.
        wpta = astar_list[wpta].parent;

        // Increase index for temp_path
        p++;

        // Whenever we reach the limit, get out.
        if (p >= MAX_PATH_NODES)
            break;

    }

    // INIT: Start
    int path_index = 0;           // done above, but done again to be sure
    // INIT: End

    // Now set the path up correctly
    for (nodeIndex = MAX_NODES - 1; nodeIndex > -1; nodeIndex--) {
	    const int node = temp_path[nodeIndex];
        if (node < 0)
            continue;

        iPath[botIndex][path_index] = node;

        // print out full path so we know what the order is
        if (pBot != nullptr) {
            char pathMsg[255] = {};
            snprintf(pathMsg, sizeof(pathMsg), "Bot [%d] path index [%d] has node [%d]", botIndex, path_index, node);
            pBot->rprint("cNodeMachine::createPath", pathMsg);
        }

        path_index++;
    }

    // Finally there is the goal
    iPath[botIndex][path_index] = nodeTargetIndex;

    // terminate path
    path_index++;
    iPath[botIndex][path_index] = -1;

    // And set bot in motion
    if (pBot != nullptr) {
        pBot->beginWalkingPath();
        pBot->setTimeToMoveToNode(2); // set timer (how much time do we allow ourselves to reach the following node)
        pBot->rprint("cNodeMachine::createPath", "Path creation finished successfully");
    } else {
        rblog("createPath (without bot) - path creation finished\n");
    }

    return true; // path found
}

/**
 * A closed node means it has been evaluated.
 * @param nodeIndex
 * @param parent
 * @param cost
 */
void cNodeMachine::closeNode(int nodeIndex, int parent, float cost) {
    astar_list[nodeIndex].state = CLOSED;
    astar_list[nodeIndex].parent = parent;
    astar_list[nodeIndex].cost = cost;
}

/**
 * Open all neighbouring nodes, calculate costs for each neighbouring node
 * @param startNodeIndex
 * @param nodeToOpenNeighboursFrom
 * @param destinationNodeIndex
 * @param botTeam
 * @param nodeStartIndex
 * @param parent
 * @param cost
 */
void cNodeMachine::openNeighbourNodes(int startNodeIndex, int nodeToOpenNeighboursFrom, int destinationNodeIndex, int botTeam) const
{
	const tNode &startNode = Nodes[startNodeIndex]; // very start of path
	const tNode &destNode = Nodes[destinationNodeIndex]; // destination for path

	const tNode &node = Nodes[nodeToOpenNeighboursFrom]; // node evaluating neighbours

    for (const int neighbourNode : node.iNeighbour)
    {
	    if (neighbourNode < 0) continue; // skip invalid nodes
        if (Nodes[neighbourNode].origin == INVALID_VECTOR) continue; // skip nodes with invalid vector

	    const float gCost = func_distance(startNode.origin, destNode.origin); // distance from starting node
	    const float hCost = func_distance(node.origin, destNode.origin); // distance from end node to node
        float cost = gCost + hCost;

        if (botTeam > -1) {
	        const float dangerCost = InfoNodes[neighbourNode].fDanger[botTeam] * cost;
//            double contactCost = InfoNodes[neighbourNode].fContact[botTeam] * cost;

            cost += dangerCost;
//            cost += contactCost;
        }

        // TODO: Add costs for how easy it is to navigate (ie some kind of arrival speed score? if given?)

        tNodestar &nodeStar = astar_list[neighbourNode];
//        char msg[255];
        if (nodeStar.state == AVAILABLE) {
//            sprintf(msg, "Found AVAILABLE node to OPEN [%d]\n", neighbourNode);
//            rblog(msg);
            nodeStar.state = OPEN;
            nodeStar.parent = nodeToOpenNeighboursFrom;
            nodeStar.cost = cost;
        } else if (nodeStar.state == OPEN) {
            // only overwrite when cost < current cost remembered
            if (nodeStar.cost > cost) {
//                sprintf(msg, "Found OPEN node to overwrite ([%d]), because our cost [%f] is lower than nodeStar cost [%f]\n", neighbourNode, cost, nodeStar.cost);
//                rblog(msg);
                nodeStar.parent = nodeToOpenNeighboursFrom;
                nodeStar.cost = cost;
            }
        }
    }
}

/**
 * This marks all waypoints to be available to be evaluated again
 * @param nodeIndex
 */
void cNodeMachine::makeAllWaypointsAvailable()
{
	for (tNodestar& nodeIndex : astar_list)
	{
		nodeIndex.cost = 0;
		nodeIndex.parent = -1;
		nodeIndex.state = AVAILABLE;
    }
//    rblog("All nodes set to AVAILABLE\n");
}

// Find a node which has almost no danger!
int cNodeMachine::node_camp(const Vector& vOrigin, int iTeam) const
{
    // Use Meredians to search for nodes
    int iX, iY;
    VectorToMeredian(vOrigin, &iX, &iY);
    int iBestNode = -1;

    // Theory:
    // Find a node, close, and less danger...
    // and with less visibility

    if (iX > -1 && iY > -1)
    {
	    int iVisibility = 9999;
	    float fDistance = 9999.0f;
	    float fDanger = 2.0f;
	    // Search in this meredian
        for (const int i : Meredians[iX][iY].iNodes)
	        if (i > -1) {
	            const int iNode = i;

                if (Nodes[iNode].iNodeBits & BIT_WATER)
                    continue;        // next node, do not camp under water!

                if (InfoNodes[iNode].fDanger[iTeam] < fDanger)
                    if (func_distance(vOrigin, Nodes[iNode].origin) < fDistance) {
                        // Calculate visibility
                        int iVis = -1;

                        for (int iVisNr = 0; iVisNr < iNode; iVisNr++)
                            if (GetVisibilityFromTo(iNode, iVisNr) == VIS_VISIBLE)     // BERKED
                                iVis++;

                        if (iVis < 0)
                            iVis = 99999;

                        if (iVis < iVisibility) {
                            iBestNode = iNode;
                            fDistance =
                                    func_distance(vOrigin, Nodes[iNode].origin);
                            fDanger = InfoNodes[iNode].fDanger[iTeam];
                            iVisibility = iVis;
                        }
                    }
            }
    }

    return iBestNode;

}

// Check if iFrom is visible from other nodes (and opposite)
void cNodeMachine::vis_calculate(int iFrom) {
    // Check around your area to see what is visible

    for (int i = 0; i < MAX_NODES; i++)
        if (i != iFrom && Nodes[i].origin != Vector(9999, 9999, 9999)) {
	        constexpr float fClosest = 1024.0f;
	        const float fDistance = func_distance(Nodes[i].origin, Nodes[iFrom].origin);
            if (fDistance < fClosest) {
                TraceResult tr;

                // Visibility is not yet calculated, so determine now
                if (GetVisibilityFromTo(iFrom, i) == VIS_UNKNOWN)   // BERKED
                {
                    UTIL_TraceHull(Nodes[iFrom].origin, Nodes[i].origin, ignore_monsters, point_hull, nullptr, &tr);

                    if (tr.flFraction < 1.0f) {
                        SetVisibilityFromTo(iFrom, i, false);
                        SetVisibilityFromTo(i, iFrom, false);
                    } else {
                        SetVisibilityFromTo(iFrom, i, true);
                        SetVisibilityFromTo(i, iFrom, true);
                    }
                }
            }
        }
}

// Find a node to look at when camping
int cNodeMachine::node_look_camp(const Vector& vOrigin, int iTeam,
                                 edict_t *pEdict) {

    rblog("node_look_camp - start\n");
    float fDanger = -0.1f;
    float fDistance = 0;
    int iBestNode = -2;

    // Theory:
    // Find a node, far, and a lot danger...
    const int iFrom = getClosestNode(vOrigin, 75, pEdict);
    // Search in this meredian
    for (int i = 0; i < MAX_NODES; i++) {
	    const int iNode = i;
        if (InfoNodes[iNode].fDanger[iTeam] > fDanger)
            if (func_distance(vOrigin, Nodes[iNode].origin) > fDistance) {
                // all nodes within this range may be tracelined
                bool bVisible = true;
                TraceResult tr;
                if (GetVisibilityFromTo(iFrom, iNode) == VIS_UNKNOWN)       // BERKED
                {
                    UTIL_TraceLine(Nodes[iFrom].origin, Nodes[iNode].origin,
                                   ignore_monsters, ignore_glass, pEdict, &tr);
                    //UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "NODE, MEREDIAN: VISIBILITY NOT KNOWN \n");
                    if (tr.flFraction < 1.0f) {
                        bVisible = false;
                        // Set to false
                        SetVisibilityFromTo(iFrom, iNode, false);
                        SetVisibilityFromTo(iNode, iFrom, false);
                    } else {
                        SetVisibilityFromTo(iFrom, iNode, true);
                        SetVisibilityFromTo(iNode, iFrom, true);
                    }

                } else {
                    if (GetVisibilityFromTo(iFrom, iNode) == VIS_BLOCKED)    // BERKED
                    {
                        bVisible = false;
                        //UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "NODE, MEREDIAN: VISIBILITY KNOWN / NOT VISIBLE\n");
                    }
                }

                // Visible
                if (bVisible) {
                    //UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "NODE, MEREDIAN: FOUND LOOK@ NODE \n");
                    iBestNode = iNode;
                    fDistance = func_distance(vOrigin, Nodes[iNode].origin);
                    fDanger = InfoNodes[iNode].fDanger[iTeam];
                }
            }
    }
    char msg[255];
    snprintf(msg, sizeof(msg), "Found best node to camp at %d\n", iBestNode);
    rblog(msg);
    return iBestNode;
}

/**
 * This function moves the bot from one node to the next. Considering distanceMoved and timers to unstuck himself or learn
 * from bad connections.
 * @param pBot
 * @param distanceMoved
 */
void cNodeMachine::path_walk(cBot *pBot, float distanceMoved) {
    pBot->rprint("cNodeMachine::path_walk", "START");
    const int BotIndex = pBot->iBotIndex;

    // Check if path is valid
    if (iPath[BotIndex][0] < 0) {
        pBot->rprint("cNodeMachine::path_walk", "Bot has no path. ([0] path index < 0)");
        pBot->stopMoving();
        return;
    }

    if (!pBot->shouldBeAbleToMove()) {
        pBot->rprint("cNodeMachine::path_walk", "Finished - shouldBeAbleToMove is false");
        return;
    }

    if (pBot->f_strafe_time > gpGlobals->time) {
        pBot->rprint_trace("cNodeMachine::path_walk", "Also strafing!");
    }

    // When longer not stuck for some time we clear stuff
    if (pBot->fNotStuckTime < gpGlobals->time)
    {
        pBot->iDuckTries = 0;
        pBot->iJumpTries = 0;
        pBot->rprint_trace("cNodeMachine::path_walk", "Reset duck and jump tries because not stuck timer is expired");
    }

    pBot->setMoveSpeed(pBot->f_max_speed);

    // Walk the path
    const int currentNodeToHeadFor = pBot->getCurrentPathNodeToHeadFor();        // Node we are heading for

    // possibly end of path reached, overshoot destination?
    if (currentNodeToHeadFor < 0) {
        pBot->rprint_trace("cNodeMachine::path_walk", "Finished - there is no current node to head for");
        pBot->forgetGoal();
        pBot->forgetPath();
        return;
    }

    // when pButtonEdict is filled in, we check if we are close!
    if (pBot->pButtonEdict) {
        pBot->rprint("cNodeMachine::path_walk", "Interacting with button logic");
        const Vector vButtonVector = VecBModelOrigin(pBot->pButtonEdict);

        float fDistance = 90;
        bool bTrigger = false;

        if (std::strcmp(STRING(pBot->pButtonEdict->v.classname), "trigger_multiple") == 0) {
            fDistance = 32;
            bTrigger = true;
        }

        if (func_distance(pBot->pEdict->v.origin, vButtonVector) < fDistance) {
            TraceResult trb;
            // TRACELINE ON PURPOSE!
            if (bTrigger) {
                UTIL_TraceLine(pBot->pEdict->v.origin, vButtonVector,
                               dont_ignore_monsters, dont_ignore_glass,
                               pBot->pEdict, &trb);
            } else {
                UTIL_TraceLine(pBot->pEdict->v.origin, vButtonVector,
                               ignore_monsters, dont_ignore_glass,
                               pBot->pEdict, &trb);
            }

            bool isGood = false;

            // if nothing hit:
            if (trb.flFraction >= 1.0f)
                isGood = true;
            else {
                // we hit this button we check for
                if (trb.pHit == pBot->pButtonEdict)
                    isGood = true;
            }

            if (isGood || bTrigger) {
                pBot->vHead = vButtonVector;
                pBot->vBody = pBot->vHead;

                // kill edict in memory
                pBot->pButtonEdict = nullptr;

                // press use
                if (!bTrigger)
                    UTIL_BotPressKey(pBot, IN_USE);

                pBot->rprint_trace("cNodeMachine::path_walk", "This is the button I was looking for, it is close and I can use it");

                // wait a little
                pBot->setTimeToWait(0.5f);
                pBot->fButtonTime = gpGlobals->time + 5.0f;
                pBot->setTimeToMoveToNode(3);
                pBot->forgetPath();
                return;
            }
            pBot->rprint_trace("cNodeMachine::path_walk", "Finished - close to button to interact with");
            return;
        }
    }

    if (pBot->f_strafe_time < gpGlobals->time) {
        pBot->vBody = Nodes[currentNodeToHeadFor].origin;
    }

    // Overwrite vBody when we are 'very close' to the bomb.
    // FIXED: Terrorists only
    if (pBot->isTerrorist()) {
        if (Game.isC4Dropped()) {
            if (VectorIsVisibleWithEdict(pBot->pEdict, Game.vDroppedC4, "weaponbox")) {
                if (func_distance(pBot->pEdict->v.origin, Game.vDroppedC4) < 200) {
                    pBot->vBody = Game.vDroppedC4;
                    pBot->rprint_trace("cNodeMachine::path_walk", "Finished - C4 dropped!?");
                    return; // this basically enforces a T bot to pick up the bomb nearby
                }
            }
        }
    }

    // Near Node
    bool bNearNode; //Variable Reassigned [APG]RoboCop[CL]
    if (pBot->isOnLadder()) {
        pBot->rprint("Bot is on ladder");
        // Set touch radius
        pBot->f_strafe_speed = 0.0f;       // we may not strafe
        pBot->setMoveSpeed(pBot->f_max_speed / 2);
        //pBot->pEdict->v.button |= IN_DUCK;                    // duck

        // Look at the waypoint we are heading to.
        pBot->lookAtNode(currentNodeToHeadFor);
        pBot->setBodyToNode(currentNodeToHeadFor);

        // Press forward key to move on ladder
        UTIL_BotPressKey(pBot, IN_FORWARD);

        if (BotShouldDuck(pBot)) {
            UTIL_BotPressKey(pBot, IN_DUCK);
            pBot->f_hold_duck = gpGlobals->time + 0.2f;
        }

        bNearNode = pBot->getDistanceToNextNode() < 25;
    } else {
        pBot->rprint_trace("cNodeMachine::path_walk", "not on ladder");
        float requiredDistance = NODE_ZONE;

        if (Nodes[currentNodeToHeadFor].iNodeBits & BIT_LADDER) {
            pBot->rprint_trace("cNodeMachine::path_walk", "going to ladder");
            // Going to a ladder waypoint
            requiredDistance = 25;
        } else {
            if (pBot->isHeadingForGoalNode()) {
                pBot->rprint_trace("cNodeMachine::path_walk", "Heading to goal node");

                tGoal *goalData = pBot->getGoalData();
                if (goalData) {
                    char msg[255];
                    snprintf(msg, sizeof(msg), "Heading for goal node of type [%s]", goalData->name);
                    pBot->rprint_trace("cNodeMachine::path_walk (bNear)", msg);
                    if (goalData->iType == GOAL_HOSTAGE) {
                        pBot->rprint_normal("cNodeMachine::path_walk (bNear)", "next node is destination and GOAL_HOSTAGE, so need to get really close");
                        requiredDistance = 25; // get really close to hostage
                    } else if (goalData->iType == GOAL_VIPSAFETY) {
                        pBot->rprint_normal("cNodeMachine::path_walk (bNear)", "next node is destination and GOAL_VIPSAFETY, so need to get really close");
                        requiredDistance = 25; // get really close to safety zone
                    }
                }
            } else {
                pBot->rprint_trace("cNodeMachine::path_walk (bNear)", "Heading to normal node");
            }
        }

        char msg[255];
        snprintf(msg, sizeof(msg), "Heading for node %d, required distance is %f, actual distance is %f, time remaining %f",
                pBot->getCurrentPathNodeToHeadFor(), requiredDistance, pBot->getDistanceToNextNode(),
                pBot->getMoveToNodeTimeRemaining());
        pBot->rprint_trace("cNodeMachine::path_walk (bNear)", msg);

        bNearNode = pBot->getDistanceToNextNode() < requiredDistance;

        // If we should duck, duck.
        if (BotShouldDuck(pBot)) {
            UTIL_BotPressKey(pBot, IN_DUCK);
            pBot->f_hold_duck = gpGlobals->time + 0.2f;
        }
    }

    bool shouldDrawWaypointBeamsFromBot = false;

    if (shouldDrawWaypointBeamsFromBot) {
	    const tNode *nodeHeadingFor = this->getNode(currentNodeToHeadFor);

	    for (int player_index = 1; player_index <= gpGlobals->maxClients;
	         player_index++) {
            edict_t *pPlayer = INDEXENT(player_index);

            if (pPlayer && !pPlayer->free) {
                if (FBitSet(pPlayer->v.flags, FL_CLIENT)) { // do not draw for now

                    DrawBeam(
                            pPlayer, // player sees beam
                            pBot->pEdict->v.origin, // + Vector(0, 0, 32) (head?)
                            nodeHeadingFor->origin,
                            255, 255, 255
                    );

                    const int currentNode = pBot->determineCurrentNodeWithTwoAttempts();
                    if (currentNode > -1) {
	                    const tNode *node = getNode(currentNode);

                        // close node
                        DrawBeam(
                                pPlayer, // player sees beam
                                pBot->pEdict->v.origin + Vector(0, 0, 32),
                                node->origin + Vector(0, 0, 32),
                                25, 1,
                                255, 0, 0,
                                255,
                                1
                        );
                    }

                    break;
                }
            }
        }
    } // Draw waypoint beams

    // reached node
    if (bNearNode) {
        ExecuteNearNodeLogic(pBot);
        pBot->rprint_trace("cNodeMachine::path_walk", "Finished - near node logic executed");
        return;
    }

    // TODO TODO TODO Water Navigation

    const int nextNodeToHeadFor = pBot->getNextPathNode();              // the node we will head for after reaching currentNode

    // NO ENEMY, CHECK AROUND AREA
    // This determines where to look at, no enemy == look at next node
    if (!pBot->hasEnemy()) {
        // look towards node after we have reached 'current' node, or else look at 'current' node.
        if (nextNodeToHeadFor > -1) {
            pBot->vHead = Nodes[nextNodeToHeadFor].origin;
        } else {
            pBot->vHead = Nodes[currentNodeToHeadFor].origin;
        }
    }

    edict_t *pEntityHit = pBot->getEntityBetweenMeAndCurrentPathNodeToHeadFor();

    // Do note, at this point we are *not* near the node yet.

    // When blocked by an entity, we should figure out why:
    if (pEntityHit && // we are blocked by an entity
        !isEntityWorldspawn(pEntityHit)) // and it is not worldspawn (ie, the map itself)
    {
        char msg[255];
        snprintf(msg, sizeof(msg), "Entity [%s] between me and next node.", STRING(pEntityHit->v.classname));
        pBot->rprint_trace("cNodeMachine::path_walk", msg);

        // hit by a door?
        // a door is blocking us
        if (isEntityDoor(pEntityHit)) {
            pBot->rprint_trace("cNodeMachine::path_walk", "Entity between me and next node is a door.");
            ExecuteDoorInteractionLogic(pBot, pEntityHit);
            return;
        }

        // hit by a hostage?, hostage is blocking our path
        if (isEntityHostage(pEntityHit)) {
            pBot->rprint_trace("cNodeMachine::path_walk", "Entity between me and next node is a hostage.");
            return;
        }

        if (std::strcmp(STRING(pEntityHit->v.classname), "player") == 0) {
            pBot->rprint_trace("cNodeMachine::path_walk", "Another player between me and next node.");
            if (pBot->hasTimeToMoveToNode()) {
                pBot->strafeRight(0.2f);
                pBot->rprint_trace("cNodeMachine::path_walk", "Time left to move to node, so lets try strafing to unstuck.");
                return;
            }
            pBot->rprint_trace("cNodeMachine::path_walk", "No time left to move to node, so we continue and let stuck logic kick in");
        }

        pBot->rprint_trace("cNodeMachine::path_walk", "Finished - entity hit end block");
    } // entity between me and node

    // When not moving (and we should move):
    // - learn from mistakes
    // - unstuck
    // - go back in path...

    constexpr float timeEvaluatingMoveSpeed = 0.1f;
    const bool notStuckForAWhile = pBot->fNotStuckTime + timeEvaluatingMoveSpeed < gpGlobals->time;

    constexpr double fraction = 0.7;    // 0.7 is an arbitrary number based on several tests to consider stuck at a more sane moment.
										// Else it would trigger stuck logic too soon, too often.

    const double speedInOneTenthOfASecond = static_cast<double>(pBot->f_move_speed * timeEvaluatingMoveSpeed) * fraction;
    double expectedMoveDistance = speedInOneTenthOfASecond;
    if (pBot->isFreezeTime()) expectedMoveDistance = 0;
    if (pBot->isWalking()) expectedMoveDistance = speedInOneTenthOfASecond / 3.0;
    if (pBot->isDucking()) expectedMoveDistance = speedInOneTenthOfASecond / 3.0;
    if (pBot->isJumping()) expectedMoveDistance = speedInOneTenthOfASecond / 3.0;
    // no need for 'is walking' because walking time influence `f_move_speed` hence it is already taken care of

    char msg[255] = {};
    snprintf(msg, sizeof(msg), "Distance moved %f, expected %f, should be able to move yes, notStuck for a while %d", distanceMoved,
             expectedMoveDistance, notStuckForAWhile);
    pBot->rprint_trace("cNodeMachine::path_walk", msg);

    const bool isStuck = distanceMoved < expectedMoveDistance && pBot->shouldBeAbleToMove() && notStuckForAWhile; // also did not evaluate this logic for 0.5 second
    const Vector &vector = Nodes[currentNodeToHeadFor].origin;

    if (isStuck) {
        pBot->rprint_trace("cNodeMachine::path_walk", "!!!STUCK STUCK STUCK STUCK STUCK STUCK STUCK!!!");
        ExecuteIsStuckLogic(pBot, currentNodeToHeadFor, vector);
        pBot->rprint_trace("cNodeMachine::path_walk", "Finished - ExecuteIsStuckLogic");
        return;
    }

    if (pBot->getMoveToNodeTimeRemaining() < 0) {
        pBot->rprint_trace("cNodeMachine::path_walk/timeRemaining", "Time is up!");

        const int iFrom = pBot->getPreviousPathNodeToHeadFor();
        const int iTo = currentNodeToHeadFor;

        IncreaseAttemptsForTroubledConnectionOrRemoveIfExceeded(iFrom, iTo);

        pBot->forgetPath();
        pBot->rprint_trace("cNodeMachine::path_walk/timeRemaining", "Finished");
        return;
    }

    pBot->rprint_trace("cNodeMachine::path_walk", "Finished - really the end of the method");
}

void cNodeMachine::ExecuteIsStuckLogic(cBot *pBot, int currentNodeToHeadFor, const Vector &vector) {
    pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "START");
    pBot->fNotStuckTime = gpGlobals->time + 0.25f; // give some time to unstuck

    const int iFrom = pBot->getPreviousPathNodeToHeadFor();
    const int iTo = currentNodeToHeadFor;

    // JUMP & DUCK // TODO: Add a proper and reliable DuckJump Node [APG]RoboCop[CL]
    const tNode &currentNode = Nodes[currentNodeToHeadFor];
    if (BotShouldJumpIfStuck(pBot) || currentNode.iNodeBits & BIT_JUMP) {
        pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "Jump tries increased, increase node time - START");
        pBot->doJump(vector);
        pBot->iJumpTries++;
        pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "Jump tries increased, increase node time  - END");
        pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "Finished!");
        return;
    }
    if (BotShouldDuck(pBot) || currentNode.iNodeBits & BIT_DUCK) {
	    pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "Duck tries increased, increase node time - START");
	    pBot->doDuck();
	    pBot->iDuckTries++;
	    pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "Duck tries increased, increase node time - END");
	    pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "Finished!");
	    return;
    }
    if (BotShouldDuckJump(pBot) || currentNode.iNodeBits & BIT_DUCKJUMP) {
        pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "DuckJump tries increased, increase node time - START");
        pBot->doDuckJump();
        pBot->iDuckJumpTries++;
        pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "DuckJump tries increased, increase node time  - END");
        pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "Finished!");
        return;
    }
    if (pBot->isOnLadder()) {
	    pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "Is stuck on ladder, trying to get of the ladder by jumping");
	    pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "DuckJump tries increased");
	    pBot->doJump(vector);
	    pBot->iJumpTries++;
	    pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "Finished!");
	    return;
    }

    pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "No need to duck or to jump");

    char msg[255] = {};
    const float timeRemaining = pBot->getMoveToNodeTimeRemaining();
    snprintf(msg, sizeof(msg), "I still have %f seconds to go to node before considered 'stuck' for connection", timeRemaining);
    pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", msg);

    const cBot *pBotStuck = getCloseFellowBot(pBot);
//        edict_t *playerNearbyInFOV = getPlayerNearbyBotInFOV(pBot);
    edict_t *entityNearbyInFOV = getEntityNearbyBotInFOV(pBot);

    const edict_t *playerNearbyInFOV = nullptr;
    edict_t *hostageNearbyInFOV = nullptr;
    if (entityNearbyInFOV) {

        if (std::strcmp(STRING(entityNearbyInFOV->v.classname), "player") == 0) {
            playerNearbyInFOV = entityNearbyInFOV;
            pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "A player is in front of me");
        }

        if (std::strcmp(STRING(entityNearbyInFOV->v.classname), "hostage_entity") == 0) {
            hostageNearbyInFOV = entityNearbyInFOV;
            pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "A hostage is in front of me");
        }
    }

    std::memset(msg, 0, sizeof(msg));
    snprintf(msg, sizeof(msg), "Player in FOV? %d, hostage in FOV? %d bot close ? %d, time remaining? %f",
            playerNearbyInFOV != nullptr, hostageNearbyInFOV != nullptr, pBotStuck != nullptr, timeRemaining);
    pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", msg);

    if (playerNearbyInFOV) {
        if (pBotStuck != nullptr) {
            if (pBotStuck->pEdict == playerNearbyInFOV) {
                pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "The player nearby in FOV is the close bot as well: it is a fellow bot that blocks me");
            } else {
                pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "The player nearby in FOV is not the same as a bot close to me: another player blocks me");
            }
        } else {
            pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "A player nearby in FOV is not a bot, and there are no bots nearby: another player blocks me");
        }
        pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "playerNearbyInFOV - finished (TODO: Unstuck from player)");
        // unstuck from player?
        return;
    }

    if (hostageNearbyInFOV) {
        pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "A hostage is in front of me.");
        if (pBot->getHostageToRescue() == hostageNearbyInFOV) {
            pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "The hostage I want to rescue is blocking me, so ignore");
        }
        if (pBot->isUsingHostage(hostageNearbyInFOV)) {
            pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "This is a hostage that is (should be) following me, so ignore");
        }
        // Depending on how the other bot looks, act
        // pBotStuck -> faces pBot , do same
        // pBotStuck -> cannot see pBot, do opposite
        // Check if pBotStuck can see pBot (pBot can see pBotStuck!)
        pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "hostageNearbyInFOV - finished");
        return;
    }

    if (timeRemaining < 0) {
        pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic/timeRemaining", "Time is up!");

        IncreaseAttemptsForTroubledConnectionOrRemoveIfExceeded(iFrom, iTo);

        pBot->forgetPath();
        pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic/timeRemaining", "Finished!");
        return;
    }

    // should move, but no nearby bot found that could cause us to get stuck
    // - when no players are close (could be blocked by them, do not learn stupid things)
    if (pBotStuck == nullptr) {
        pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "There is no other BOT around making me go stuck");

        // check if the connection we want is going up
        // - when going up
        // - when we should move
        const bool nodeToHeadForIsHigher = currentNode.origin.z > pBot->pEdict->v.origin.z;
        if (nodeToHeadForIsHigher) {
            pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "Node to head for is higher than me");
            // check if the next node is floating (skip self)

            if (node_float(currentNode.origin, pBot->pEdict)) {
                pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "Node to head for is higher than me - and is floating, that is not good - removing");

                removeConnection(iFrom, iTo);

                pBot->forgetPath();
            } else {
                pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "I probably missed a connection now, so lets check...");
                if (pBot->canSeeVector(currentNode.origin)) {
                    pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "I can still see the current node to head for");
                    // make sure we head to the current node
                    pBot->vBody = currentNode.origin;
                    pBot->vHead = currentNode.origin;
                    pBot->fNotStuckTime = gpGlobals->time + 0.5f; // give a bit more time
                } else {
                    pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "I can no longer see the current node to head for");
                    IncreaseAttemptsForTroubledConnectionOrRemoveIfExceeded(iFrom, iTo);
                    pBot->forgetPath();
                }
                // probably our movement went wrong in between, troubled connection?
            }
        } else {
            pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "no bot holding me back, must be bad connection");

            IncreaseAttemptsForTroubledConnectionOrRemoveIfExceeded(iFrom, iTo);

            pBot->forgetPath();
        }
    } else {
        pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "pBotStuck pointer available - stuck by 'world'");
//                // we are stuck by 'world', we go back one step?
        pBot->CheckAround();
//        pBot->prevPathIndex();

//                pBot->f_stuck_time = gpGlobals->time + 0.2;
    }

    // normal stuck (not duck/jump)
    pBot->rprint_trace("cNodeMachine::ExecuteIsStuckLogic", "Finished");
}

void cNodeMachine::ExecuteNearNodeLogic(cBot *pBot) {
    pBot->rprint_trace("cNodeMachine::ExecuteNearNodeLogic", "Start");
    const int currentNodeToHeadFor = pBot->getCurrentPathNodeToHeadFor();

    // first determine if we where heading for a goal node
    const bool isHeadingForGoalNode = pBot->isHeadingForGoalNode();

    // increase index on path, so we will go to next node
    pBot->nextPathIndex();

    // Calculate vis table from here
    vis_calculate(currentNodeToHeadFor);

    // give time (2 seconds) to move to next node
    pBot->setTimeToMoveToNode(2.0f);


    if (!isHeadingForGoalNode) {
        char msg[255] = {};

        const int currentPathNode = pBot->getCurrentPathNodeToHeadFor();

        if (currentPathNode > -1) {
	        const int troubleIndex = GetTroubleIndexForConnection(pBot->getPreviousPathNodeToHeadFor(), currentNodeToHeadFor);
            if (troubleIndex > -1) {
	            const tTrouble &trouble = Troubles[troubleIndex];
                snprintf(msg, sizeof(msg), "Heading to next node: %d, trouble (tries) %d", currentPathNode, trouble.iTries);
            } else {
                snprintf(msg, sizeof(msg), "Heading to next node: %d - with no trouble", currentPathNode);
            }
        } else {
            snprintf(msg, sizeof(msg), "Heading to next node: %d", currentPathNode);
        }
        pBot->rprint("cNodeMachine::path_walk()", msg);
    }

    // When our target was our goal, we are there
    if (isHeadingForGoalNode) {
        pBot->rprint_trace("cNodeMachine::path_walk()", "Executing destination logic");

        // reached the end
        pBot->iPreviousGoalNode = pBot->getGoalNode();
        pBot->forgetGoal();
        pBot->forgetPath();

        // Do logic when arriving at destination now:

        // When f_cover_time is > gpGlobals, we are taking cover
        // so we 'wait'
//            if (pBot->f_cover_time > gpGlobals->time) {
//                if (pBot->hasEnemy() && pBot->lastSeenEnemyVector != Vector(0, 0, 0)) {
//                    pBot->vHead = pBot->lastSeenEnemyVector;
//                }
//
//                pBot->f_wait_time = pBot->f_cover_time - RANDOM_FLOAT(0.0, 3.0);
//                pBot->f_cover_time = gpGlobals->time;
//
//                if (RANDOM_LONG(0, 100) < 75) {
//                    pBot->f_hold_duck = gpGlobals->time + RANDOM_FLOAT(1.0, 4.0);
//                }
//                pBot->rprint("cNodeMachine::path_walk", "Taking cover time");
//            }
//            else {
        if (pBot->iPathFlags == PATH_CAMP) {
            pBot->rprint_trace("cNodeMachine::path_walk()", "Arrived at destination and marked as camp. So we camp");

            // Camp for a few seconds
            pBot->f_camp_time = gpGlobals->time + RANDOM_FLOAT(1, 10);

            if (RANDOM_LONG(0, 100) < pBot->ipFearRate) {
                pBot->iPathFlags = PATH_DANGER;
            } else {
                pBot->iPathFlags = PATH_NONE;
            }

            // RADIO: I am in position!
            if (FUNC_DoRadio(pBot)) {

            }
        }

        // At destination, bomb is planted and we have not discovered the C4 yet...
        // TODO: Remember which goals/bomb spots have been visited so bots won't visit this bomb spot again?
        const int iGoalType = getGoalIndexFromNode(pBot->getGoalNode());

        if (pBot->isCounterTerrorist()) {
            if (Game.bBombPlanted && !Game.isPlantedC4Discovered()) {
                // call out sector clear
                if (iGoalType == GOAL_BOMBSPOT) {
                    if (!pBot->Defuse()) {
                        // sector clear!
                        if (FUNC_DoRadio(pBot))
                            UTIL_BotRadioMessage(pBot, 3, "4", "");
                    }
                }
                else if (iGoalType == GOAL_RESCUEZONE) {
                    pBot->rprint("Arrived at goal rescue zone, forgetting hostages");
                    pBot->clearHostages(); // clear hostages
                }
            }
        } else {
            // nothing to do here it seems... (bomb planting logic is elsewhere)
        }
        return;
    }
}

/**
 * Returns true when entity is a door that can be used
 * @param pEntityHit
 * @return
 */
bool cNodeMachine::isDoorThatOpensWhenPressingUseButton(const edict_t *pEntityHit)
{
    return FBitSet(pEntityHit->v.spawnflags, SF_DOOR_USE_ONLY) &&
    !(FBitSet(pEntityHit->v.spawnflags, SF_DOOR_NO_AUTO_RETURN));
}

bool cNodeMachine::isEntityDoor(const edict_t *pEntityHit)
{
    return  std::strcmp(STRING(pEntityHit->v.classname), "func_door") == 0 || // normal door (can be used as an elevator)
            std::strcmp(STRING(pEntityHit->v.classname), "func_wall") == 0 || // I am not 100% sure about func_wall, but include it anyway
            std::strcmp(STRING(pEntityHit->v.classname), "func_door_rotating") == 0; // rotating door
}

bool cNodeMachine::isEntityHostage(const edict_t *pEntityHit)
{
    return  std::strcmp(STRING(pEntityHit->v.classname), "hostage_entity") == 0; // hostage
}

bool cNodeMachine::isEntityWorldspawn(const edict_t *pEntityHit)
{
    return  std::strcmp(STRING(pEntityHit->v.classname), "worldspawn") == 0; // the world
}

// Think about path creation here
void cNodeMachine::path_think(cBot *pBot, float distanceMoved) {
    pBot->rprint_trace("cNodeMachine::path_think", "START");
    if (pBot->shouldBeWandering()) {
        int currentNode = -1;
        for (int attempts = 1; attempts < 5; attempts++) {
	        const float distance = NODE_ZONE + static_cast<float>(attempts * NODE_ZONE);
            currentNode = pBot->determineCurrentNode(distance); // this also sets current node in bot state
            if (currentNode > -1) break;
        }
        pBot->rprint("shouldBeWandering found a current node");
        pBot->setGoalNode(currentNode);
        pBot->setBodyToNode(currentNode);
        pBot->lookAtNode(currentNode);
        return;
    }

    // Heading for hostage (directly) so do not do things with paths
    if (pBot->hasHostageToRescue()) {
        pBot->rprint_trace("cNodeMachine::path_think", "hasHostageToRescue");
        if (pBot->canSeeHostageToRescue()) {
            pBot->rprint_trace("cNodeMachine::path_think", "has hostage and can see hostage, will not do anything");
            return; // bot has hostage, can see hostage
        }
        pBot->rprint_trace("cNodeMachine::path_think", "has hostage to rescue, but can't see it");
    }

    if (pBot->shouldActWithC4()) {
        pBot->rprint("cNodeMachine::path_think", "Time to do things with a C4");
        return;
    }

    // When camping we do not think about paths, we think where to look at
    if (pBot->shouldCamp()) {
        pBot->rprint("cNodeMachine::path_think", "Time to camp");
        if (!pBot->hasGoal()) {
            pBot->rprint("cNodeMachine::path_think", "Setting goal to look for camping");
            const int noteToLookAt = node_look_camp(pBot->pEdict->v.origin, UTIL_GetTeam(pBot->pEdict), pBot->pEdict);
            pBot->setGoalNode(noteToLookAt);
        }
        return;
    }

    // If we should wait, we do nothing here
    if (pBot->shouldWait()) {
        pBot->rprint("cNodeMachine::path_walk", "Time to wait");
        return;
    }

    // After above checks where we may not walk or anything, we come here and only execute when we have a path
    if (pBot->isWalkingPath()) {
        pBot->rprint_trace("cNodeMachine::path_think", "isWalkingPath");
        path_walk(pBot, distanceMoved);  // walk the path
        return;
    }

    pBot->rprint_trace("cNodeMachine::path_think", "I am not walking a path.");

    const int thisBotIndex = pBot->iBotIndex;

    // No path
    pBot->stopMoving();

    if (pBot->hasGoal()) {
        // there is no path, but there is a goal to work to
        const int iCurrentNode = pBot->determineCurrentNodeWithTwoAttempts();

        if (iCurrentNode < 0) {
            pBot->forgetGoal();
            pBot->startWandering(1); // 1 second wandering, hoping to reach a 'currentNode' there
            pBot->rprint("cNodeMachine::path_think()", "I have a goal but not a current node.");
            return;
        }

        // Finally create a path
        pBot->rprint("cNodeMachine::path_think()", "Create path to goal node");
        createPath(iCurrentNode, pBot->getGoalNode(), thisBotIndex, pBot, pBot->iPathFlags);

        if (pBot->getPathIndex() < 0) {
            pBot->rprint("cNodeMachine::path_think()",
                         "there is a goal, no path, and we could not get a path to our goal; so i have choosen to find a node further away and try again");
            pBot->setGoalNode(getFurthestNode(pBot->pEdict->v.origin, 400, pBot->pEdict));
        }
        return;
    }

    // There is no goal
    pBot->rprint_trace("cNodeMachine::path_think", "No goal yet");

    // Depending on team we have a goal
    int iCurrentNode = pBot->determineCurrentNodeWithTwoAttempts();

    if (iCurrentNode < 0) {
        iCurrentNode = addNode(pBot->pEdict->v.origin, pBot->pEdict);
    }

    if (iCurrentNode < 0) {
        pBot->startWandering(3);
        pBot->forgetGoal();
        pBot->rprint_trace("cNodeMachine::path_think", "No node nearby, start wandering.");
        return;
    }

    // DETERMINE GOAL / FIND GOAL

    // Loop through all goals.
    float highestScore = 0.0f;

    int iFinalGoalNode = -1;
    int iFinalGoalIndex = -1;

    constexpr float MAX_DISTANCE = 16384.0f; // theoretical max distance
    constexpr float MAX_GOAL_DISTANCE = MAX_DISTANCE / 2.0f;

    // 01-07-2008; Instead of using 'scores', use a normalized score.
    // We do:
    // (current score + gained score) / 2.0;
    // Since both scores can be max 1.0, meaning we keep it between 0.0 and 1.0
    // A score of 1.0 is max.

    pBot->rprint_normal("cNodeMachine::path_think", "going to choose goal");
    for (int goalIndex = 0; goalIndex < MAX_GOALS; goalIndex++) {
	    constexpr int maxCheckedScore = 5;

	    // Make sure this goal is valid
        if (Goals[goalIndex].iNode < 0) {
            continue;
        }

        const float fDistanceToGoal = func_distance(
                pBot->pEdict->v.origin,
                node_vector(Goals[goalIndex].iNode)
        );

        // First score is distance, so just set it:
        float score = fDistanceToGoal / MAX_GOAL_DISTANCE;

        if (Goals[goalIndex].iChecked > maxCheckedScore) {
            // it has been very popular, reset
            Goals[goalIndex].iChecked = 0;
        }

        // A bit off randomness
        float weight = 50.0f / static_cast<float>(pBot->ipRandom); // (yes, this will give us 1 or higher score)
        weight *= score;

        score += weight;

        // Take into consideration how many times this goal has been selected
        score = (score + (1.0f - static_cast<float>(Goals[goalIndex].iChecked) / static_cast<float>(maxCheckedScore))) / 2.0f;

        // Danger (is important)
        score = (score + InfoNodes[Goals[goalIndex].iNode].fDanger[UTIL_GetTeam(pBot->pEdict)]) / 1.5f;

        // How many bots have already taken this goal?
        float goalAlreadyUsedScore = 0.0f;
        float teamMembers = 1.0f; // count self by default
		
        for (cBot& bot : bots)
        {
            // not a bot
            const cBot* botPointer = &bot;
            if (!botPointer->bIsUsed ||
                botPointer == pBot) { // skip self
                continue;
            }
        	
            // real bots..
            if (pBot->isOnSameTeamAs(botPointer)) {
                teamMembers++;
                if (pBot->getGoalNode() == Goals[goalIndex].iNode) {
                    goalAlreadyUsedScore++;
                }
            }
        }

        // add favoriteness
        score = (score + (1.0f - goalAlreadyUsedScore / teamMembers)) / 2.0f;

        // Goals regardless of map/game type
        const int goalType = Goals[goalIndex].iType;

        if (goalType == GOAL_SPAWNCT || goalType == GOAL_SPAWNT) {
	        const float goalscore = fDistanceToGoal / MAX_GOAL_DISTANCE;
            score = (score + goalscore) / 2.0f;
        }

        if (Game.bHostageRescueMap) {
            // Deal with CS_ASSAULT case, where no rescue zone is given
            if (!Game.bHostageRescueZoneFound && goalType == GOAL_SPAWNCT) {
                if (pBot->isCounterTerrorist()) {
                    if (pBot->isEscortingHostages()) {
                        pBot->rprint("I am escorting hostages - assuming ct spawn is rescue zone and prioritizing");
                        // highest priority
                        score = 2.0f;
                    }
                }
            }

            if (goalType == GOAL_HOSTAGE) {
                // counter-terrorist should
                float goalscore = 0.0f;
                if (pBot->isCounterTerrorist()) {
                    if (pBot->isEscortingHostages()) {
                        pBot->rprint("I am escorting hostages - should ignore existing hostages");
                        // already escorting hostages, low interest for other hostages
                        goalscore = 0.5f;
                    } else {
                        // always go to the most furthest hostage spot, and add some randomness here, else
                        // all bots go to there.
                        const float mul = MAX_GOAL_DISTANCE / fDistanceToGoal;
                        goalscore = 1 + mul;
                    }
                } else {
                    // Terrorist pick randomly this location
                    if (RANDOM_LONG(0, 100) < 25) {
                        goalscore = RANDOM_FLOAT(0.1f, 0.6f);
                    }
                }

                score = (score + goalscore) / 2.0f;
            } else if (goalType == GOAL_RESCUEZONE) {
                if (pBot->isCounterTerrorist()) {
                    if (pBot->isEscortingHostages()) {
                        pBot->rprint("I am escorting hostages - prioritizing for rescue zone");
                        // highest priority
                        score = 2.0f;
                    } else {
                        score = 0.2f;
                    }
                } else if (pBot->isTerrorist()) {
                    // TODO: when hostages are being rescued, go to a rescue zone to catch CT's and
                    // prevent rescue
                    score = 0.2f;
                }
            }
        } else { // it is a DE_ map
            if (goalType == GOAL_BOMBSPOT) {
                float goalscore = 0.0f;
                if (pBot->isTerrorist()) {
                    if (pBot->hasBomb()) {
                        goalscore = 2.0f; // plant it!
                    } else {
	                    const float mul = fDistanceToGoal / MAX_GOAL_DISTANCE;
                        goalscore = 0.7f * mul;
                    }

                } else if (pBot->isCounterTerrorist()) {
                    if (Game.bBombPlanted) {
                        if (Game.isPlantedC4Discovered()) {
                            pBot->rprint_trace("path_think/determine goal", "I know where the C4 is planted, evaluating if this is the closest bombspot.");
                            char msg[255] = {};
                            snprintf(msg, sizeof(msg), "C4 is located at %f, %f, %f", Game.vPlantedC4.x, Game.vPlantedC4.y, Game.vPlantedC4.z);
                            pBot->rprint_trace("path_think/determine goal", msg);

                            // find a node close to the C4
                            const int nodeCloseToC4 = getClosestNode(Game.vPlantedC4, NODE_ZONE * 2, nullptr);
                            if (nodeCloseToC4 > -1) {
                                // measure distance compared to goal node we are evaluating
                                const float distanceToC4FromCloseNode = func_distance(
                                        Game.vPlantedC4,
                                        node_vector(Goals[nodeCloseToC4].iNode)
                                );

                                // the distance from this goal node
                                const float distanceToC4FromThisGoalNode = func_distance(
                                        Game.vPlantedC4,
                                        node_vector(Goals[goalIndex].iNode)
                                );

                                const float score = distanceToC4FromCloseNode / distanceToC4FromThisGoalNode;
                                goalscore = 1.5f + score;
                                std::memset(msg, 0, sizeof(msg));
                                snprintf(msg, sizeof(msg), "Distance from C4 to closest node is %f, distance from evaluating node to C4 is %f, resulting into score of %f",
                                        distanceToC4FromCloseNode,
                                        distanceToC4FromThisGoalNode,
                                        goalscore);
                                pBot->rprint_trace("path_think/determine goal", msg);
                            } else {
                                pBot->rprint_trace("path_think/determine goal", "We know where the C4 is planted, but unfortunately we can't find a close node to the planted c4.");
                                // we can't find a node close to the c4, so we gamble
                                goalscore = 2.0f; // pick
                            }
                        } else {
                            pBot->rprint_trace("path_think/determine goal", "No clue where bomb is, picking bombspot to evaluate");
                            goalscore = 2.0f; // pick any bombspot
                        }
                    } else {
                        pBot->rprint_trace("path_think/determine goal", "Bomb is not planted");
                        const float mul = fDistanceToGoal / MAX_GOAL_DISTANCE;
                        goalscore = 0.7f * mul;
                    }
                }
                // this is weird? what?
                score = (score + goalscore) / 2.0f;
            } // GOAL is a bombspot
        } // bomb plant map

        // 17/07/04
        // Basic attempts to handle other kind of goals...

        if (goalType == GOAL_VIPSAFETY)        // basic goals added for as_ maps
        {
            // Maximum importance when acting as CT should check whether we are the VIP!
            if (pBot->isCounterTerrorist()) {
                if (pBot->vip) {
                    // VIP wants to get out
                    score = 2.0f;
                }
            } else {
                // terrorists pick random
                score = (score + RANDOM_FLOAT(0.0f, 1.0f)) / 2.0f;
            }
        }
    	
        if (goalType == GOAL_VIP) {
            if (pBot->isCounterTerrorist()) {
                if (pBot->vip) {
                    score = 0; // do not chase yourself
                } else {
                    score = 0; // don't care about VIP
                	// if distance is too big, go to it. (guard the VIP)
                    constexpr int maxDistanceWeKeepToVIP = 500;
                    const float goalScore = maxDistanceWeKeepToVIP / fDistanceToGoal;
					score = (score + goalScore) / 2.0f;
                }
            }
        }

        if (goalType == GOAL_ESCAPEZONE)     // basic goals added for es_  maps
        {
            // Maximum importance when acting as T
            if (pBot->iTeam == 1) {
                score = 2.0f;
            }
        }

        // checked , used for bombspots, when the checked value is filled, we will make the
        // score drasticly lower!
        /*
           if (Goals[iGn].iChecked > 0)
           {
           iScore /= Goals[iGn].iChecked;

           if (Goals[iGn].iChecked > 10)
           Goals[iGn].iChecked = RANDOM_LONG(0,10);
           }
         */


        // was previous goal as well, don't go there
        if (pBot->iPreviousGoalNode == goalIndex) {
            score *= 0.2f; // low chance
        }


        // UNTIL HERE THE GOAL 'SCORE' IS CALCULATED
        // the next job is to determine if it is 'more important' to pick than the one before
        // Refactor this please...

        // even though its float comparison, it can h appen since we hard-set it to 2.0 at some places, making
        // some scores the same
        char msg[255] = {};
	    snprintf(msg, sizeof(msg), "Evaluating goal %s gives a score of %f, highest score so far is %f",
	             Goals[goalIndex].name, score, highestScore);
        pBot->rprint_trace("path_think/determine goal", msg);
        if (score == highestScore && RANDOM_LONG(0,100) < 50) {
            pBot->rprint_trace("path_think/determine goal", "SCORE == HIGHEST SCORE and chosen to override randomly.");
            highestScore = score;
            iFinalGoalNode = Goals[goalIndex].iNode;
            iFinalGoalIndex = goalIndex;
        } else if (score > highestScore) {
            // accept duplication for now for the sake of logging
            pBot->rprint_trace("path_think/determine goal", "determine goal: SCORE > HIGHEST SCORE");
            highestScore = score;
            iFinalGoalNode = Goals[goalIndex].iNode;
            iFinalGoalIndex = goalIndex;
        }
    }

    if (iFinalGoalNode < 0) {
        pBot->rprint("path_think/determine goal", "I cannot determine a goal, bugged?");
    }

    // when afraid , use path_danger
    if (RANDOM_LONG(0, 100) < pBot->ipFearRate) {
        pBot->iPathFlags = PATH_DANGER;
    }

    // a few situations override the final goal node... (Stefan, what!?)
    if (pBot->pButtonEdict) {
        iFinalGoalNode = getClosestNode(VecBModelOrigin(pBot->pButtonEdict), NODE_ZONE, pBot->pButtonEdict);
    }

    // Well it looks like we override the 'final' goal node (not so final after all huh?) to the dropped c4
    if (Game.vDroppedC4 != Vector(9999, 9999, 9999) && // c4 dropped somewhere
        pBot->pButtonEdict == nullptr) { // not using button

        // randomly, if we 'feel like picking up the bomb' just override the 'final' goal node
        if (RANDOM_LONG(0, 100) < pBot->ipDroppedBomb) {
            iFinalGoalNode = getClosestNode(Game.vDroppedC4, 75, nullptr);
        }
    }

    // When camp flag is set, so we should camp at our 'destination' then we should determine
    // where to camp there.
    /*
       if (pBot->iPathFlags == PATH_CAMP && iGoalNode > -1)
       {
       int iSearchNode = node_dangerous (UTIL_GetTeam (pBot->pEdict), Nodes[iGoalNode].origin, 300);

       if (iSearchNode > -1)
       iGoalNode = node_camp (Nodes[iSearchNode].origin,  UTIL_GetTeam (pBot->pEdict));
       else
       iGoalNode = node_camp (Nodes[pBot->iDiedNode].origin, UTIL_GetTeam (pBot->pEdict));
       }
     */


    if (iFinalGoalIndex > -1) {
        Goals[iFinalGoalIndex].iChecked++;
    } else {
        pBot->startWandering(1);
    }

    // Finally, we set it for real
    pBot->rprint("path_think/determine goal", "Setting final goal after choosing a goal from goals list");
    pBot->setGoalNode(iFinalGoalNode, iFinalGoalIndex);
    tGoal *goalData = pBot->getGoalData();

    char msg[255] = {};

    if (goalData != nullptr) {
        snprintf(msg, sizeof(msg), "I have chosen a goal: Node [%d], Goal type [%s], checked [%d], score [%f], distance [%f]",
                iFinalGoalNode,
                goalData->name,
                goalData->iChecked,
                highestScore,
                pBot->getDistanceTo(iFinalGoalNode)
        );
    } else {
        snprintf(msg, sizeof(msg), "I have chosen a goal: Node [%d], - NO GOAL DATA - score [%f], distance [%f]",
                iFinalGoalNode,
                highestScore,
                pBot->getDistanceTo(iFinalGoalNode)
        );
    }

    pBot->rprint("path_think/determine goal", msg);

    pBot->rprint("path_think/determine goal", "GOAL PICKING FINISHED");
    //////////////////////////////////////// GOAL PICKING FINISHED //////////////////////////////////////////
    //////////////////////////////////////// GOAL PICKING FINISHED //////////////////////////////////////////

//    pBot->rprint("path_think", "Determining if I should camp at this goal or not...");
//    // Set on camp mode
//    if (RANDOM_LONG(0, 100) < pBot->ipCampRate &&
//        pBot->f_camp_time + ((100 - pBot->ipCampRate) / 2) <
//        gpGlobals->time &&
//        !pBot->isEscortingHostages() &&
//        !pBot->vip) {
//        pBot->iPathFlags = PATH_CAMP;
//        pBot->rprint("path_think", "Set camp flag for path");
//    }

    // create path
    const bool success = createPath(iCurrentNode, pBot->getGoalNode(), thisBotIndex, pBot, pBot->iPathFlags);

    // If we still did not find a path, we set wander time
    // for 1 second we wait before a new attempt to find a goal and create a path.
    if (!success) {
        pBot->rprint("cNodeMachine::path_think()", "Unable to create path to destination.");
        pBot->rprint("cNodeMachine::path_think()", "Finding node nearby to move to");
        pBot->setGoalNode(getClosestNode(pBot->pEdict->v.origin, NODE_ZONE * 5, pBot->pEdict));

        const bool successToAlternative = createPath(iCurrentNode, pBot->getGoalNode(), thisBotIndex, pBot, pBot->iPathFlags);

        if (!successToAlternative) {
            pBot->rprint("cNodeMachine::path_think()", "Unable to create path to node nearby.");
            pBot->forgetPath();
            pBot->forgetGoal();
        }
    }
}

tNode *cNodeMachine::getNode(int index) {
    // safe-guard
    if (index < 0 || index >= MAX_NODES) return nullptr;
    return &Nodes[index];
}

std::string cNodeMachine::getGoalTypeAsText(const tGoal& goal)
{
    switch (goal.iType) {
		case GOAL_SPAWNT:
			return "GOAL_SPAWNT";
		case GOAL_SPAWNCT:
			return "GOAL_SPAWNCT";
        case GOAL_BOMBSPOT:
            return "GOAL_BOMBSPOT";
        case GOAL_BOMB:
            return "GOAL_BOMB";
        case GOAL_HOSTAGE:
            return "GOAL_HOSTAGE";
        case GOAL_RESCUEZONE:
            return "GOAL_RESCUEZONE";
        case GOAL_CONTACT:
            return "GOAL_CONTACT";
        case GOAL_IMPORTANT:
            return "GOAL_IMPORTANT";
        case GOAL_VIP:
            return "GOAL_VIP";
        case GOAL_VIPSAFETY:
            return "GOAL_VIPSAFETY";
        case GOAL_ESCAPEZONE:
            return "GOAL_ESCAPEZONE";
        case GOAL_WEAPON:
            return "GOAL_WEAPON";
        case GOAL_NONE:
            return "GOAL_NONE";
        default:
            return "GOAL UNKNOWN";
    }
}

// Find cover
int cNodeMachine::node_cover(int iFrom, int iTo, edict_t *pEdict) {
    if (iFrom == iTo)
        return iFrom;             // cover at current position

    if (iFrom < 0)
        return -1;

    if (iTo < 0)
        return -1;

    // From here the difficult part starts.

    // We don't want to scan the entire node table, > 4000 nodes
    // Using meredians, we should take care that some nodes are
    // not in the same meredian. So we should scan meredian x, and x+1
    // but also x - 1. Same goes for y, and y+1, y-1. That means we have
    // 9 meredians. 9*80 = ~720 nodes at max. Instead of 4000. This is
    // already a speedup increasement.

    // Another problem, firing 720 tracelines is way to much. Visibilty
    // table calculation for 4000 nodes, (4000x4000 nodes table is way to much)
    // is no option.

    // Note: this function is only called once for finding some node to take cover from
    // Most bad situation would be that 31 bots call this function in 1 frame.

    // TEMP sollution:
    float fClosest = 512;
    int iClosest = -1;
    for (int i = 0; i < MAX_NODES; i++)
        if (i != iTo && Nodes[i].origin != Vector(9999, 9999, 9999)) {
	        const float fDistance =
                    func_distance(Nodes[i].origin, Nodes[iFrom].origin);
            if (fDistance < fClosest) {
                // all nodes within this range may be tracelined
                bool bVisible = true;
                TraceResult tr;

                if (GetVisibilityFromTo(iFrom, i) == VIS_UNKNOWN)   // BERKED
                {
                    UTIL_TraceLine(Nodes[iFrom].origin, Nodes[i].origin,
                                   ignore_monsters, ignore_glass, pEdict, &tr);

                    if (tr.flFraction < 1.0f)
                        bVisible = false;
                } else {
                    if (GetVisibilityFromTo(iFrom, i) == VIS_BLOCKED)        // BERKED
                        bVisible = false;
                }

                // Hit something
                if (bVisible == false) {
                    // Update VisTable
                    SetVisibilityFromTo(iFrom, i, false);
                    SetVisibilityFromTo(i, iFrom, false);
                    fClosest = fDistance;
                    iClosest = i;
                } else {
                    SetVisibilityFromTo(iFrom, i, true);
                    SetVisibilityFromTo(i, iFrom, true);
                }
            }
        }
    // We have found something?
    return iClosest;

}

int cNodeMachine::node_look_at_hear(int iFrom, int iOrigin,
                                    edict_t *pEdict) {
    if (iFrom == iOrigin)
        return iFrom;             // impossible

    if (iFrom < 0) {
        SERVER_PRINT("iFrom is lower then 0\n");
        return -1;
    }

    if (iOrigin < 0) {
        SERVER_PRINT("iOrigin is lower then 0\n");
        return -1;
    }
    // From here the difficult part starts...
    // .. see bs at above function ..

    // what we do:
    // find a CLOSE node that has almost NO DANGER _and_
    // that can SEE iFROM.

    // TEMP sollution:
    int iClosest = -1;

    // we search for a node that can see the sound node (iFrom) and the origin node (iOrigin)
    for (int i = 0; i < MAX_NODES; i++)
        if (i != iOrigin && i != iFrom
            && Nodes[i].origin != Vector(9999, 9999, 9999)) {
            if (func_distance(Nodes[i].origin, Nodes[iOrigin].origin) >
                BOT_HEARDISTANCE)
                continue;

            // all nodes within this range may be tracelined
            bool bVisible = true;  // visible from I to iFrom (= sound origin)
            bool bVisibleTo = true;        // visible from I to iTo (= where we are located)

            TraceResult tr;

            if (GetVisibilityFromTo(iFrom, i) == VIS_UNKNOWN)      // BERKED
            {
                UTIL_TraceLine(Nodes[iFrom].origin, Nodes[i].origin,
                               ignore_monsters, ignore_glass, pEdict, &tr);

                if (tr.flFraction < 1.0f) {
                    SetVisibilityFromTo(iFrom, i, false);
                    SetVisibilityFromTo(i, iFrom, false);
                }
            }

            if (GetVisibilityFromTo(iOrigin, i) == VIS_UNKNOWN)    // BERKED
            {
                UTIL_TraceLine(Nodes[iOrigin].origin, Nodes[i].origin,
                               ignore_monsters, ignore_glass, pEdict, &tr);

                if (tr.flFraction < 1.0f) {
                    SetVisibilityFromTo(iOrigin, i, false);
                    SetVisibilityFromTo(i, iOrigin, false);
                }
            }
            // only when really blocked, count it as false.
            if (GetVisibilityFromTo(iFrom, i) == VIS_BLOCKED)
                bVisible = false;

            if (GetVisibilityFromTo(iOrigin, i) == VIS_BLOCKED)
                bVisibleTo = false;

            // Hit something
            if (bVisible && bVisibleTo) {
                SERVER_PRINT
                        ("Found one node that could be a possible 'look to'\n");
                iClosest = i;
            }
        }
    // We have found something?
    return iClosest;
}

void cNodeMachine::dump_goals() const
{
	rblog("Dump of all goals\n");
    for (int i = 0; i < MAX_GOALS && Goals[i].iNode >= 0; i++) {
	    char buffer[100];
	    const Vector v = Nodes[Goals[i].iNode].origin;
        snprintf(buffer, sizeof(buffer),
            "Goal#%d is at node %d (%.0f, %.0f, %.0f), iChecked= %d, ",
            i + 1, Goals[i].iNode, v.x, v.y, v.z, Goals[i].iChecked);
        switch (Goals[i].iType) {
            case GOAL_SPAWNCT:
                std::strcat(buffer, "GOAL_SPAWNCT");
                break;
            case GOAL_SPAWNT:
                std::strcat(buffer, "GOAL_SPAWNT");
                break;
            case GOAL_BOMBSPOT:
                std::strcat(buffer, "GOAL_BOMBSPOT");
                break;
            case GOAL_BOMB:
                std::strcat(buffer, "GOAL_BOMB");
                break;
            case GOAL_HOSTAGE:
                std::strcat(buffer, "GOAL_HOSTAGE");
                break;
            case GOAL_RESCUEZONE:
                std::strcat(buffer, "GOAL_RESCUEZONE");
                break;
            case GOAL_CONTACT:
                std::strcat(buffer, "GOAL_CONTACT");
                break;
            case GOAL_IMPORTANT:
                std::strcat(buffer, "GOAL_IMPORTANT");
                break;
            case GOAL_VIP:
                std::strcat(buffer, "GOAL_VIP");
                break;
            case GOAL_VIPSAFETY:
                std::strcat(buffer, "GOAL_VIPSAFETY");
                break;
            case GOAL_ESCAPEZONE:
                std::strcat(buffer, "GOAL_ESCAPEZONE");
                break;
            case GOAL_WEAPON:
                std::strcat(buffer, "GOAL_WEAPON");
                break;
            case GOAL_NONE:
                std::strcat(buffer, "GOAL_NONE");
                break;
            default:
                std::strcat(buffer, "unknown type");
        }
        std::strcat(buffer, "\n");
        rblog(buffer);
    }

}

// EVY: another dump
void cNodeMachine::dump_path(int iBot, int CurrentPath) const
{
    char buffer[80];
    int i, CurrentNode;

    if (CurrentPath >= 0)
        CurrentNode = iPath[iBot][CurrentPath];
    else
        CurrentNode = -1;
    rblog("  Path is: ");
    for (i = 0; i < MAX_NODES && iPath[iBot][i] >= 0; i++) {
        if (i == CurrentPath)
            snprintf(buffer, sizeof(buffer), "<%d> ", iPath[iBot][i]);
        else
            snprintf(buffer, sizeof(buffer), "%d ", iPath[iBot][i]);
        rblog(buffer);
    }
    rblog("\n");
    if (CurrentNode < 0)
        return;
    rblog("  Current direct neighbours are:\n");
    for (i = 0; i < MAX_NEIGHBOURS; i++)
        if (Nodes[CurrentNode].iNeighbour[i] >= 0) {
	        const int j = Nodes[CurrentNode].iNeighbour[i];
	        const Vector v = Nodes[j].origin;
            snprintf(buffer, sizeof(buffer), "      %d (%.0f, %.0f, %.0f)\n", j, v.x, v.y, v.z);
            rblog(buffer);
        }
    rblog("\n");
}

// EVY a lot of things to draw: nodes, neighbours, goals, paths, ...
// Graphs from PMB & Botman

// width and height of the debug bitmap image
enum : std::uint16_t
{
	DEBUG_BMP_WIDTH = 2048,
	DEBUG_BMP_HEIGHT = 2048
};

static char *bmp_buffer;
static float maxx, maxy, minx, miny;
static float scale;

static void InitDebugBitmap() {
    // this function allocates memory and clears the debug bitmap buffer

//    if (bmp_buffer)
        free(bmp_buffer);         // reliability check, free BMP buffer if already allocated

    bmp_buffer = nullptr;
    bmp_buffer = static_cast<char*>(malloc(DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT));    // allocate memory
    if (bmp_buffer == nullptr) {
        std::fprintf(stderr,
                "InitDebugBitmap(): unable to allocate %d kbytes for BMP buffer!\n",
                DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT / 1024);
        exit(1);
    }

    std::memset(bmp_buffer, 14, DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT);  // Set all to all white (and allow for darker palette)
}

// Draw a small cross
static void DrawPoint(const Vector& v, unsigned char color) {
	if (bmp_buffer == nullptr) {
        std::fprintf(stderr,
                "DrawLineInDebugBitmap(): function called with NULL BMP buffer!\n");
        return;                   // reliability check: cancel if bmp buffer unallocated
    }
    // translate the world coordinates in image pixel coordinates
	const int x0 = static_cast<int>((v.x - minx) / scale);
	const int y0 = static_cast<int>((v.y - miny) / scale);

	const int offset = y0 * DEBUG_BMP_WIDTH + x0;
    if (offset < 0 || offset >= DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT) {
        std::fprintf(stderr,
                "DrawLineInDebugBitmap(): bad BMP buffer index %d (range 0 - %d)\n",
                offset, DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT);
        exit(1);
    }

    bmp_buffer[offset] = color;  // draw the point itself
    if (offset + 1 < DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT)
        bmp_buffer[offset + 1] = color;   // make a small star on the right
    if (offset - 1 >= 0)
        bmp_buffer[offset - 1] = color;   // make a small star on the left
    if (offset + DEBUG_BMP_WIDTH < DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT)
        bmp_buffer[offset + DEBUG_BMP_WIDTH] = color;     // make a small star below
    if (offset - DEBUG_BMP_WIDTH >= 0)
        bmp_buffer[offset - DEBUG_BMP_WIDTH] = color;     // make a small star above
}

// From PMB and Botman's code

static void DrawLineInDebugBitmap(const Vector& v_from, const Vector& v_to, unsigned char color) {
    // blind copy of botman's Bresenham(). This function prints a vector line into a bitmap dot
    // matrix. The dot matrix (bmp_buffer) is a global array. The size of the bitmap is always
    // assumed to be DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT pixels (currently 2000 * 2000 to fit with
    // the size of the universe, with an adaptative unit scale, up to 1 pixel = 10 vector units).

    int stepx, stepy;
    int fraction;

    if (bmp_buffer == nullptr) {
        std::fprintf(stderr,
                "DrawLineInDebugBitmap(): function called with NULL BMP buffer!\n");
        return;                   // reliability check: cancel if bmp buffer unallocated
    }
    // translate the world coordinates in image pixel coordinates
    int x0 = static_cast<int>((v_from.x - minx) / scale);
    int y0 = static_cast<int>((v_from.y - miny) / scale);
    const int x1 = static_cast<int>((v_to.x - minx) / scale);
    const int y1 = static_cast<int>((v_to.y - miny) / scale);

    int dx = (x1 - x0) * 2;
    int dy = (y1 - y0) * 2;
    if (dx < 0) {
        dx = -dx;
        stepx = -1;
    } else
        stepx = 1;
    if (dy < 0) {
        dy = -dy;
        stepy = -1;
    } else
        stepy = 1;

    int offset = y0 * DEBUG_BMP_WIDTH + x0;
    if (offset < 0 || offset >= DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT) {
        std::fprintf(stderr,
                "DrawLineInDebugBitmap(): bad BMP buffer index %d (range 0 - %d)\n",
                offset, DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT);
        exit(1);
    }

    bmp_buffer[offset] = color;  // draw the first point of the line

    // is the line rather horizontal than vertical ? We need to know this to determine the step
    // advance in the Bresenham grid, either we draw y = f(x), or x = f(y).
    if (dx > dy) {
        // the line is rather horizontal, we can draw it safely for incremental values of x

        fraction = 2 * dy - dx;   // fraction of height in x0 pixel's 'square' where y0 should be

        // while we've not reached the end of the segment...
        while (x0 != x1) {
            // if y0 should rather be drawn on a different height than its previous height...
            if (fraction >= 0) {
                y0 += stepy;        // draw it one pixel aside, then (depending on line orientation)
                fraction -= 2 * dx; // and reset its fraction (Bresenham, not sure I get the math)
            }
            x0 += stepx;           // in either case, draw x0 one pixel aside its previous position
            fraction += 2 * dy;    // and update y0's fraction (not sure I get the math - but whatever)

            // compute the offset in the BMP buffer corresponding to this point
            offset = y0 * DEBUG_BMP_WIDTH + x0;
            if (offset < 0
                || offset >= DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT) {
                std::fprintf(stderr,
                        "DrawLineInDebugBitmap(): bad BMP buffer index %d (range 0 - %d)\n",
                        offset, DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT);
                exit(1);
            }

            bmp_buffer[offset] = color;    // set this point to have the specified color
        }
    } else {
        // else the line is rather vertical, we NEED to draw it for incremental values of y (if we
        // did it for incremental values of x instead, we would drop half the pixels).

        fraction = 2 * dx - dy;   // fraction of width in y0 pixel's 'square' where x0 should be

        // while we've not reached the end of the segment...
        while (y0 != y1) {
            // if x0 should rather be drawn on a different width than its previous width...
            if (fraction >= 0) {
                x0 += stepx;        // draw it one pixel aside, then (depending on line orientation)
                fraction -= 2 * dy; // and reset its fraction (Bresenham, not sure I get the math)
            }
            y0 += stepy;           // in either case, draw y0 one pixel aside its previous position
            fraction += 2 * dx;    // and update x0's fraction (not sure I get the math - but whatever)

            // compute the offset in the BMP buffer corresponding to this point
            offset = y0 * DEBUG_BMP_WIDTH + x0;
            if (offset < 0
                || offset >= DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT) {
                std::fprintf(stderr,
                        "DrawLineInDebugBitmap(): bad BMP buffer index %d (range 0 - %d)\n",
                        offset, DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT);
                exit(1);
            }
            bmp_buffer[offset] = color;    // set this point to have the specified color
        }
    }
    //return;                      // finished, segment has been printed into the BMP dot matrix
}

// from PMB & Botman code

static void WriteDebugBitmap(const char *filename) {
	int data_start, file_size;
    unsigned long dummy;

    if (bmp_buffer == nullptr) {
        std::fprintf(stderr,
                "WriteDebugBitmap(): function called with NULL BMP buffer!\n");
        return;                   // reliability check: cancel if bmp buffer unallocated
    }
    // open (or create) the .bmp file for writing in binary mode...
    FILE* fp = std::fopen(filename, "wb");
    if (fp == nullptr) {
        std::fprintf(stderr, "WriteDebugBitmap(): unable to open BMP file!\n");
	// if (bmp_buffer)
            free(bmp_buffer);      // cannot open file, free DXF buffer
        bmp_buffer = nullptr;
        return;                   // cancel if error creating file
    }
    // write the BMP header
    fwrite("BM", 2, 1, fp);      // write the BMP header tag
    fseek(fp, sizeof(unsigned long), SEEK_CUR);  // skip the file size field (will write it last)
    fwrite("\0\0", sizeof(short), 1, fp);        // dump zeros in the first reserved field (unused)
    fwrite("\0\0", sizeof(short), 1, fp);        // dump zeros in the second reserved field (unused)
    fseek(fp, sizeof(unsigned long), SEEK_CUR);  // skip the data start field (will write it last)

    // write the info header
    dummy = 40;
    std::fwrite(&dummy, sizeof(unsigned long), 1, fp);        // write the info header size (does 40 bytes)
    dummy = DEBUG_BMP_WIDTH;
    std::fwrite(&dummy, sizeof(long), 1, fp); // write the image width (2000 px)
    dummy = DEBUG_BMP_HEIGHT;
    std::fwrite(&dummy, sizeof(long), 1, fp); // write the image height (2000 px)
    dummy = 1;
    std::fwrite(&dummy, sizeof(short), 1, fp);        // write the # of planes (1)
    dummy = 8;
    std::fwrite(&dummy, sizeof(short), 1, fp);        // write the bit count (8)
    dummy = 0;
    std::fwrite(&dummy, sizeof(unsigned long), 1, fp);        // write the compression id (no compression)
    dummy = DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT;
    std::fwrite(&dummy, sizeof(unsigned long), 1, fp);        // write the image size (2000 * 2000)
    dummy = 0;
    std::fwrite(&dummy, sizeof(long), 1, fp); // write the X pixels per meter (not specified)
    std::fwrite(&dummy, sizeof(long), 1, fp); // write the Y pixels per meter (not specified)
    dummy = 256;
    std::fwrite(&dummy, sizeof(unsigned long), 1, fp);        // write the # of colors used (all)
    std::fwrite(&dummy, sizeof(unsigned long), 1, fp);        // write the # of important colors (wtf ?)

    // write the color palette (B, G, R, reserved byte)
    fputc(0x00, fp);
    fputc(0x00, fp);
    fputc(0x00, fp);
    fputc(0x00, fp);             // 0=BLACK
    fputc(0xFF, fp);
    fputc(0xFF, fp);
    fputc(0xFF, fp);
    fputc(0x00, fp);             // 1=WHITE
    fputc(0x80, fp);
    fputc(0x80, fp);
    fputc(0x80, fp);
    fputc(0x00, fp);             // 2=GREY
    fputc(0xC0, fp);
    fputc(0xC0, fp);
    fputc(0xC0, fp);
    fputc(0x00, fp);             // 3=SILVER
    fputc(0x80, fp);
    fputc(0x00, fp);
    fputc(0x00, fp);
    fputc(0x00, fp);             // 4=DARK BLUE
    fputc(0xFF, fp);
    fputc(0x00, fp);
    fputc(0x00, fp);
    fputc(0x00, fp);             // 5=BLUE
    fputc(0x80, fp);
    fputc(0x80, fp);
    fputc(0x00, fp);
    fputc(0x00, fp);             // 6=DARK YELLOW
    fputc(0xFF, fp);
    fputc(0xFF, fp);
    fputc(0x00, fp);
    fputc(0x00, fp);             // 7=YELLOW ? LIGHT BLUE
    fputc(0x00, fp);
    fputc(0x80, fp);
    fputc(0x00, fp);
    fputc(0x00, fp);             // 8=DARK GREEN
    fputc(0x00, fp);
    fputc(0xFF, fp);
    fputc(0x00, fp);
    fputc(0x00, fp);             // 9=GREEN
    fputc(0x00, fp);
    fputc(0x00, fp);
    fputc(0x80, fp);
    fputc(0x00, fp);             // 10=DARK RED
    fputc(0x00, fp);
    fputc(0x00, fp);
    fputc(0xFF, fp);
    fputc(0x00, fp);             // 11=RED
    fputc(0x80, fp);
    fputc(0x00, fp);
    fputc(0x80, fp);
    fputc(0x00, fp);             // 12=DARK PURPLE
    fputc(0xFF, fp);
    fputc(0x00, fp);
    fputc(0xFF, fp);
    fputc(0x00, fp);             // 13=PURPLE
    fputc(0xFF, fp);
    fputc(0xFF, fp);
    fputc(0xFF, fp);
    fputc(0x00, fp);             // 14=WHITE
    fputc(0xEF, fp);
    fputc(0xEF, fp);
    fputc(0xEF, fp);
    fputc(0x00, fp);             // 15=WHITE-GREY
    fputc(0xDF, fp);
    fputc(0xDF, fp);
    fputc(0xDF, fp);
    fputc(0x00, fp);             // 16=GREY
    fputc(0xCF, fp);
    fputc(0xCF, fp);
    fputc(0xCF, fp);
    fputc(0x00, fp);             // 17=DARKGREY
    fputc(0xBF, fp);
    fputc(0xBF, fp);
    fputc(0xBF, fp);
    fputc(0x00, fp);             // 18=DARKGREY
    fputc(0xAF, fp);
    fputc(0xAF, fp);
    fputc(0xAF, fp);
    fputc(0x00, fp);             // 19=DARKGREY

    for (dummy = 20; dummy < 256; dummy++) {
        // fill out the rest of the palette with zeros
        fputc(0x00, fp);
        fputc(0x00, fp);
        fputc(0x00, fp);
        fputc(0x00, fp);
    }

    // write the actual image data
    data_start = ftell(fp);      // get the data start position (that's where we are now)
    std::fwrite(bmp_buffer, DEBUG_BMP_WIDTH * DEBUG_BMP_HEIGHT, 1, fp);       // write the image
    file_size = ftell(fp);       // get the file size now that the image is dumped

    // now that we've dumped our data, we know the file size and the data start position

    fseek(fp, 0, SEEK_SET);      // rewind
    fseek(fp, 2, SEEK_CUR);      // skip the BMP header tag "BM"
    std::fwrite(&file_size, sizeof(unsigned long), 1, fp);    // write the file size at its location
    fseek(fp, sizeof(short), SEEK_CUR);  // skip the first reserved field
    fseek(fp, sizeof(short), SEEK_CUR);  // skip the second reserved field
    std::fwrite(&data_start, sizeof(unsigned long), 1, fp);   // write the data start at its location

    std::fclose(fp);                  // finished, close the BMP file

	// if (bmp_buffer)
        free(bmp_buffer);         // and free the BMP buffer
    bmp_buffer = nullptr;

    //return;                      // and return
}


// Find the border of all nodes to scale the bitmap

void cNodeMachine::FindMinMax() const
{
	minx = miny = 9999.0f;
    maxx = maxy = -9999.0f;
    for (int i = 0;
         i < MAX_NODES && Nodes[i].origin != Vector(9999, 9999, 9999);
         i++) {
	    maxx = std::max(Nodes[i].origin.x, maxx);
	    maxy = std::max(Nodes[i].origin.y, maxy);
	    minx = std::min(Nodes[i].origin.x, minx);
	    miny = std::min(Nodes[i].origin.y, miny);
    }

    // Avoid having lines/points just on the bitmap border, add some more spaces
    maxx += NODE_ZONE;
    minx -= NODE_ZONE;
    maxy += NODE_ZONE;
    miny -= NODE_ZONE;

    // first compute the X and Y divider scale, and take the greatest of both
	const float scalex = (1 + maxx - minx) / DEBUG_BMP_WIDTH;
	const float scaley = (1 + maxy - miny) / DEBUG_BMP_WIDTH;
    if (scalex > scaley)
        scale = scalex + scalex / 100;    // add a little offset (margin) for safety
    else
        scale = scaley + scaley / 100;    // add a little offset (margin) for safety
}

// 05/07/04
// Mark meridians as slighly darker in alternance
// Palette is defined such that increasing the palette index
// Makes a slightly darker dark

void cNodeMachine::MarkAxis() {
	const int x0 = static_cast<int>((0 - minx) / scale);
	const int y0 = static_cast<int>((0 - miny) / scale);

    // Mark X axis by keeping X to 0 and varying Y
    if (minx < 0 && 0 < maxx)
        for (int y = 0; y < DEBUG_BMP_HEIGHT; y++)
            bmp_buffer[y * DEBUG_BMP_WIDTH + x0] += 2;

    // Mark Y axis by keeping Y to 0 and varying X
    if (miny < 0 && 0 < maxy)
        for (int x = 0; x < DEBUG_BMP_WIDTH; x++)
            bmp_buffer[y0 * DEBUG_BMP_WIDTH + x] += 2;
}

// 05/07/04
// Mark each meredians (default 256 units) of alternating color

void cNodeMachine::MarkMeredians() {
    int x, y;
    int Meredian;

    // Mark some meredians
    for (x = 0; x < DEBUG_BMP_WIDTH; x++) {
        Meredian =
                (x * scale + minx +
	                8192.0f) / SIZE_MEREDIAN;
        if (Meredian & 0x01) {
            for (y = 0; y < DEBUG_BMP_HEIGHT; y++)
                bmp_buffer[y * DEBUG_BMP_WIDTH + x]++;
        }
    }

    // Mark some meredians
    for (y = 0; y < DEBUG_BMP_HEIGHT; y++) {
        Meredian =
                (y * scale + miny +
	                8192.0f) / SIZE_MEREDIAN;
        if (Meredian & 0x01) {
            for (x = 0; x < DEBUG_BMP_HEIGHT; x++)
                bmp_buffer[y * DEBUG_BMP_WIDTH + x]++;
        }
    }
}

// Put a cross on all nodes in RBN + draw lines to all neighbours

void cNodeMachine::PlotNodes(int NeighbourColor, int NodeColor) const
{
    int i;

    // Draw all neighbours
    for (i = 0;
         i < MAX_NODES && Nodes[i].origin != Vector(9999, 9999, 9999);
         i++)
        for (int j = 0; j < MAX_NEIGHBOURS && Nodes[i].iNeighbour[j] >= 0;
             j++)
            DrawLineInDebugBitmap(Nodes[i].origin,
                                  Nodes[Nodes[i].iNeighbour[j]].origin,
                                  NeighbourColor);
    // Draw all nodes
    for (i = 0;
         i < MAX_NODES && Nodes[i].origin != Vector(9999, 9999, 9999);
         i++)
        DrawPoint(Nodes[i].origin, NodeColor);
}

// Put a small cross at all goal points

void cNodeMachine::PlotGoals(int color) const
{
	for (int i = 0; i < MAX_GOALS && Goals[i].iNode >= 0; i++) {
		const Vector v = Nodes[Goals[i].iNode].origin;
        DrawPoint(v, color);
    }
}

// Plot the computed paths for all life bots
void cNodeMachine::PlotPaths(int Tcolor, int CTcolor) const
{
	for (int iBot = 0; iBot < 32; iBot++) {
        if (bots[iBot].bIsUsed) {
            int From = iPath[iBot][0];
            if (From < 0)
                continue;           // This bot has not path

            for (int i = 1; i < MAX_NODES && iPath[iBot][i] >= 0; i++) {
	            const int To = iPath[iBot][i];
                DrawLineInDebugBitmap(Nodes[From].origin, Nodes[To].origin,
                                      bots[iBot].iTeam ==
                                      1 ? Tcolor : CTcolor);
                From = To;
            }
        }
    }
}

// Called from 'realbot debug nodes dumpbmp' command in dll.cpp
// Creates a .BMP file with all nodes (blue cross), goals (green cross), neighbour (black lines),
// goals (light blue lines for CT and red lines for T)
// 05/07/04
// Marking Axis, Meredians, other colors, other filenames (linked to map names)

void cNodeMachine::Draw() const
{
    static int Count = 0;        // Static to create filenames like cs_siege0000.bmp, cs_siege0001.bmp, ...
    char Filename[80];

    FindMinMax();
    InitDebugBitmap();
    MarkMeredians();
    MarkAxis();
    PlotNodes(0, 5);             // 0 = black, 5 = blue
    PlotPaths(11, 7);            // 11 = Red 7 = light blue ?
    PlotGoals(9);                // 9 = green
    snprintf(Filename, sizeof(Filename), "%s%4.4d.bmp", STRING(gpGlobals->mapname), Count++);
    WriteDebugBitmap(Filename);
}

void cNodeMachine::ExecuteDoorInteractionLogic(cBot *pBot, edict_t *pEntityHit) const
{
    pBot->rprint_trace("cNodeMachine::ExecuteDoorInteractionLogic", "Start");

    // check if we have to 'use' it
    if (!pBot->hasButtonToInteractWith() && // we do not have an interaction with a button set up
        pBot->shouldBeAbleToInteractWithButton() && // we have the time to interact
        isDoorThatOpensWhenPressingUseButton(pEntityHit)) { // and it is a door that requires button interaction

        // use only, press use and wait
        pBot->vHead = VecBModelOrigin(pEntityHit);
        pBot->vBody = pBot->vHead;
        UTIL_BotPressKey(pBot, IN_USE);
        pBot->setTimeToWait(0.5f);
        pBot->fButtonTime = gpGlobals->time + 5;
        pBot->pButtonEdict = nullptr;

        pBot->rprint_trace("cNodeMachine::ExecuteDoorInteractionLogic", "I have pressed USE to open a door - finished");
        // TODO: when this door is opened by a trigger_multiple (on touch)
        // then we do not have to wait and press USE at all.
        return;
    }

    // check if door has a button you have to use to open it
    const char *doorButtonToLookFor = STRING(pEntityHit->v.targetname);

    if (doorButtonToLookFor) {
        char msg[255] = {};
        snprintf(msg, sizeof(msg), "There is a target button , named %s, to open this door [%s] - going to search for it.", doorButtonToLookFor, STRING(pEntityHit->v.classname));
        pBot->rprint_trace("cNodeMachine::ExecuteDoorInteractionLogic", msg);

        // find this entity
        edict_t *pButtonEdict = nullptr;
        edict_t *pent = nullptr;
        TraceResult trb;

        // search for all buttons
        while ((pent = UTIL_FindEntityByClassname(pent, "func_button")) != nullptr) {
            // skip anything that could be 'self' (unlikely)
            if (pent == pEntityHit) continue;

            // found button entity matching target
            if (std::strcmp(STRING(pent->v.target), doorButtonToLookFor) == 0) {
                Vector buttonVector = VecBModelOrigin(pent);

                UTIL_TraceLine(pBot->pEdict->v.origin, buttonVector,
                               ignore_monsters, dont_ignore_glass,
                               pBot->pEdict, &trb);

                // if nothing hit:
                if (trb.flFraction >= 1.0f) {
                    // Button found to head for!
                    pButtonEdict = pent;
                    break;
                }
                // we hit this button we check for
                if (trb.pHit == pent) {
	                // Button found to head for!
	                pButtonEdict = pent;
	                break;
                }
            }
        } // while (func_button)

        // still nothing found
        if (pButtonEdict == nullptr) {
            // TOUCH buttons (are not func_button!)
            pent = nullptr;

            // search for all buttons
            while ((pent = UTIL_FindEntityByClassname(pent, "trigger_multiple")) != nullptr) {
                // skip anything that could be 'self' (unlikely)
                if (pent == pEntityHit)
                    continue;

                // found button entity
                if (std::strcmp(STRING(pent->v.target), doorButtonToLookFor) == 0) {
                    // get vectr
                    Vector vPentVector = VecBModelOrigin(pent);

                    UTIL_TraceHull(pBot->pEdict->v.origin, vPentVector,
                                   dont_ignore_monsters, point_hull,
                                   pBot->pEdict, &trb);

                    bool isGood = false;

                    // if nothing hit:
                    if (trb.flFraction >= 1.0f) {
                        pButtonEdict = pent;
                        break;
                    }
                    // we hit this button we check for
                    if (trb.pHit == pent)
	                    isGood = true;

                    if (isGood) {
                        // Button found to head for!
                    } else {
                        // we failed here
                        // it is probably a button 'on the other side of the wall'
                        // as most doors have 2 buttons to access it (ie prodigy)

                        // hits by worldspawn here
                        if (std::strcmp(STRING(trb.pHit->v.classname), "worldspawn") == 0) {

                            // DE_PRODIGY FIX:
                            // Somehow the button is not detectable. Find a node, that is close to it.
                            // then retry the traceline. It should NOT hit a thing now.
                            // On success, it is still our button
                            const int iClose = getClosestNode(vPentVector, NODE_ZONE, pent);

                            if (iClose > -1) {
                                // retry the tracehull
                                UTIL_TraceHull(pBot->pEdict->v.origin,
                                               node_vector(iClose),
                                               dont_ignore_monsters,
                                               point_hull,
                                               pBot->pEdict, &trb);

                                // if nothing hit:
                                if (trb.flFraction >= 1.0f) {
                                    pButtonEdict = pent;
                                    break;
                                }
                            }
                        }

                    }
                }
            } // while
        }

        // We have found a button to go to
        if (pButtonEdict) {
            std::memset(msg, 0, sizeof(msg));
            // Get its vector
            const Vector vButtonVector = VecBModelOrigin(pButtonEdict);

            // Search a node close to it
            const int iButtonNode = getClosestNode(vButtonVector, NODE_ZONE, pButtonEdict);

            // When node found, create path to it
            if (pBot->createPath(iButtonNode, PATH_NONE)) {
                snprintf(msg, sizeof(msg), "Found a button at node %d and created a path to it.", iButtonNode);
                pBot->pButtonEdict = pButtonEdict;
            } else {
                if (iButtonNode > -1) {
                    snprintf(msg, sizeof(msg), "Found a button at node %d but failed to create a path to it.", iButtonNode);
                } else {
                    snprintf(msg, sizeof(msg), "Found a button, but there is no nearby node :/");
                }
            }
            pBot->rprint_trace("cNodeMachine::ExecuteDoorInteractionLogic", msg);
        } else {
            pBot->rprint_trace("cNodeMachine::ExecuteDoorInteractionLogic", "There is a button to look for, but I could'nt find it?");
        }
    }
}

// $Log: NodeMachine.cpp,v $
// Revision 1.20  2004/09/07 15:44:34  eric
// - bumped build nr to 3060
// - minor changes in add2 (to add nodes for Bsp2Rbn utilities)
// - if compiled with USE_EVY_ADD, then the add2() function is used when adding
//   nodes based on human players instead of add()
// - else, it now compiles mostly without warnings :-)
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
// Revision 1.15  2004/07/08 09:04:57  stefan
// - fixed some shield detection bug... hopefully fixes improper buy behaviour
//
// Revision 1.14  2004/07/05 21:32:29  eric
// - bumped to build 3053
// - minor modifications in 'realbot debug nodes dumpbmp'...
// -    .BMP filename linked to the map name
// -    X and Y axis are now marked by darker line
// -    Meredians (256 units wide per default) are of alternating colors to get
//      idea of the map size and node position
// - added a new goal type GOAL_WEAPON for spawned weapons
//      BUT goal system does not use them yet...
//
// Revision 1.13  2004/07/02 16:43:34  stefan
// - upped to build 3051
// - changed log() into rblog()
// - removed BOT.CFG code that interpets old RB V1.0 commands
// - neater respons of the RealBot console
// - more help from RealBot console (ie, type realbot server broadcast ... with no arguments it will tell you what you can do with this, etc)
// - removed message "bot personality loaded from file"
// - in overal; some cleaning done, no extra features added
//
// Revision 1.12  2004/07/01 18:09:45  stefan
// - fixed skill 10 bots not causing memory bugger on re-adding (respawning)
// - added extra check for respawning bots so auto-add function cannot crash
// - fixed 2 nitpicks pointed out on the forums
//
// Revision 1.11  2004/06/25 07:39:00  stefan
// - upped to build 3050
// - fixed reaction time (instant reaction time) bug
// - added evy's goals, but they are not used yet
// - fixed some radio responses here and there for swat behaviour.
// - swat leader automaticly assigned again when one dies
// - HINT: you can see any changes made by me, by looking at DD/MM/YY - Stefan (ie, 22/06/04 - Stefan, will let you find all changes i made that day)
//
// Revision 1.10  2004/06/23 08:24:13  stefan
// - upped to build 3049
// - added swat behaviour (team leader assignment, radio response change and leaders command team-mates) - THIS IS EXPERIMENTAL AND DOES NOT ALWAYS WORK AS I WANT IT TO.
// - changed some conditions in nodemachine
// - sorry evy, still not added your new goals ;) will do next time, i promise
//
// Revision 1.9  2004/06/20 10:24:13  stefan
// - fixed another steep/stair thingy
// - changed a bit of the aiming code
//
// Revision 1.8  2004/06/19 21:06:14  stefan
// - changed distance check in nodemachine
// - fixed some 'steep' bug in nodemachine
//
// Revision 1.7  2004/06/18 12:20:05  stefan
// - fixed another bug in chatting, CS 1.5 won't crash now
// - added some limit to bots searching for goals
//
// Revision 1.6  2004/06/17 21:23:22  stefan
// - fixes several connection problems with nodes. Going down from steep + crates (de_dust) PLUS going up/down from very steep slopes on as_oilrig.. 0wnage and thx to PMB and Evy
// - fixed chat bug in CS 1.6, its still CS 1.5 & CS 1.6 compatible though
//
// Revision 1.5  2004/06/13 20:08:21  stefan
// - 'bad score for goals' added
// - bmp dump info (Thanks Evy)
// - added 'realbot server players', so you can keep a server full at NR players at all times
// - slightly adjusted goal selection code
// - wander code disabled
// - lots of debug info introduced, do not use this source for REAL USAGE!
//
// Revision 1.4  2004/05/29 19:05:46  stefan
// - upped to BUILD 3044
// - removed several debug messages on screen
// - changed default 'chatrate (max sentences)' to 3
// - removed copyright notice, which is not valid due GPL license
//
// i know, nothing special :)
//
// Revision 1.3  2004/05/12 13:24:44  stefan
// fixed ; in jump recording
//
// Revision 1.2  2004/04/18 17:39:19  stefan
// - Upped to build 2043
// - REALBOT_PRINT() works properly now
// - Log() works properly now
// - Clearing in dll.cpp of reallog.txt at dll init
// - Logging works now, add REALBOT_PRINT() at every point you want to log something.
//
