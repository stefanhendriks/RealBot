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
  * NODE MACHINE
  * COPYRIGHTED BY STEFAN HENDRIKS (C) 
  **/

#ifndef NODEMACHINE_H
#define NODEMACHINE_H

#include <string>

#include "bot.h"

#include "NodeDataTypes.h"

// CLASS: NodeMachine
class cNodeMachine {
public:
    // -----------------
    int addNode(const Vector& vOrigin, edict_t *pEntity);

    int Reachable(int iStart, int iEnd) const;

    int add2(const Vector& vOrigin, int iType, edict_t *pEntity);

    int getClosestNode(const Vector& vOrigin, float fDist, edict_t *pEdict) const;    // returns a close node
    int getFurthestNode(const Vector& vOrigin, float fDist, const edict_t *pEdict) const;    // returns a node within dist, but most far away
    int getFreeNodeIndex() const;

    // -----------------
    bool add_neighbour_node(int iNode, int iToNode);

    bool removeConnection(int iFrom, int iTo);

    bool remove_neighbour_nodes(int iNode);

    static int freeNeighbourNodeIndex(const tNode *Node);

    static int is_neighbour_node(const tNode& node, int iNode);

    // -----------------
    void init();                 // Init (info)nodes
    void save() const;           // Save nodes on disk
    void load();                 // Load nodes on disk
    void save_important() const;

    // -----------------
    Vector node_vector(int iNode) const;

    // -----------------
    int GetTroubleIndexForConnection(int iFrom, int iTo) const;

    int AddTroubledConnection(int iFrom, int iTo);


    bool IncreaseAttemptsForTroubledConnectionOrRemoveIfExceeded(int iFrom, int iTo);

    bool hasAttemptedConnectionTooManyTimes(int index) const;
    void IncreaseAttemptsForTroubledConnection(int index);

    bool ClearTroubledConnection(int iFrom, int iTo);

    // -----------------
    void setUpInitialGoals();                // find new goals and attach them to the nodes
    void updateGoals();          // update moving goals (ie hostages)

    int getGoalIndexFromNode(int iNode) const;

    void resetCheckedValuesForGoals();

    void ClearImportantGoals();

    bool hasGoalWithEdict(edict_t *pEdict) const;

    void addGoal(edict_t *pEdict, int goalType, const Vector& vVec);

    tGoal * getRandomGoalByType(int goalType);    // return a node close to a iType goal (random)
    static bool node_float(const Vector& vOrigin, edict_t *pEdict);

    static bool node_on_crate(const Vector& vOrigin, edict_t *pEdict);

    static int node_dangerous(int iTeam, const Vector& vOrigin, float fMaxDistance);

    int node_look_camp(const Vector& vOrigin, int iTeam, edict_t *pEdict);

    // -----------------
    void danger(int iNode, int iTeam);   // Make spot dangerous
    void scale_danger();

    // -----------------
    void contact(int iNode, int iTeam);  // Add contact area
    void scale_contact();

    // -----------------
    void experience_save();

    void experience_load();

    // -----------------
    int node_cover(int iFrom, int iTo, edict_t *pEdict);

    int node_look_at_hear(int iFrom, int iOrigin, edict_t* pEdict);
	
    int node_camp(const Vector& vOrigin, int iTeam) const;

    void vis_calculate(int iFrom);

    // -----------------
    bool createPath(int nodeStartIndex, int nodeTargetIndex, int botIndex, cBot *pBot, int iFlags);   // know the path
    void path_draw(edict_t *pEntity) const;   // draw the path
    void path_walk(cBot *pBot, float distanceMoved);   // walk the path
    void path_think(cBot *pBot, float distanceMoved);  // think about paths
    void path_clear(int botIndex);

    void ExecuteNearNodeLogic(cBot *pBot);

    int getNodeIndexFromBotForPath(int botIndex, int pathNodeIndex) const;

    // -----------------
    static void VectorToMeredian(const Vector& vOrigin, int *iX, int *iY);     // Input: origin, output X and Y Meredians
    void AddToMeredian(int iX, int iY, int iNode);

    // -----------------
    void draw(edict_t *pEntity) const;        // Draw nodes
    void connections(edict_t *pEntity) const; // Draw neighbours

    // -----------------
    void addNodesForPlayers();         // Players plot around!
    void init_players();         // Initialize players (dll load)
    void init_round();           // Initialize on round start

    // -------------------
    // From cheesemonster:
    int GetVisibilityFromTo(int iFrom, int iTo) const; // BERKED
    void ClearVisibilityTable() const;

    void SetVisibilityFromTo(int iFrom, int iTo, bool bVisible);

    void FreeVisibilityTable() const;

    // Some debugging by EVY
    void dump_goals() const;

    void dump_path(int iBot, int CurrentPath) const;

    void Draw() const;

    tNode *getNode(int index);

    tGoal *getGoal(int index);

private:
    tNode Nodes[MAX_NODES] = {};                              // Nodes
    tInfoNode InfoNodes[MAX_NODES] = {};                      // Info for Nodes (metadata)
    tPlayer Players[32] = {};                                 // Players to keep track of, for node plotting
    tGoal Goals[MAX_GOALS] = {};                              // Goals to pursue in the game
    tMeredian Meredians[MAX_MEREDIANS][MAX_MEREDIANS] = {};   // Meredian lookup search for Nodes, squared

    int iPath[MAX_BOTS][MAX_PATH_NODES] = {};                 // 32 bots, with max waypoints paths (TODO: move to bot class?)

    int iMaxUsedNodes = 0;

    byte iVisChecked[MAX_NODES] = {};
    unsigned char * cVisTable = nullptr;
    tTrouble Troubles[MAX_TROUBLE] = {};

    void FindMinMax() const;

    static void MarkAxis();

    static void MarkMeredians();

    void PlotNodes(int NeighbourColor, int NodeColor) const;

    void PlotPaths(int Tcolor, int CTcolor) const;

    void PlotGoals(int GoalColor) const;

    static void makeAllWaypointsAvailable();

    static void closeNode(int nodeIndex, int parent, float cost);
    void openNeighbourNodes(int startNodeIndex, int nodeToOpenNeighboursFrom, int destinationNodeIndex, int botTeam) const;

    static std::string getGoalTypeAsText(const tGoal& goal);

    int getFreeGoalIndex() const;

    void initGoals();

    void initGoal(int g);

    static bool isEntityDoor(const edict_t *pEntityHit);
    static bool isEntityHostage(const edict_t *pEntityHit);
    static bool isEntityWorldspawn(const edict_t *pEntityHit);

    static bool isDoorThatOpensWhenPressingUseButton(const edict_t *pEntityHit);

    void ExecuteIsStuckLogic(cBot *pBot, int currentNodeToHeadFor, const Vector &vector);

    void ExecuteDoorInteractionLogic(cBot* pBot, edict_t* pEntityHit) const;
};

#endif // NODEMACHINE_H
