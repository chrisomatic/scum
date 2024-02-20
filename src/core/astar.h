#pragma once

#define ASTAR_MAX_X      256 // maximum grid dimension
#define ASTAR_MAX_NODES  (ASTAR_MAX_X*ASTAR_MAX_X)

typedef
{
    int x;
    int y;
} AStarNode_t;

typedef
{
    // initial data
    int node_size;
    int width;
    int height;

    // intermediate use
    AStarNode_t nodemap[ASTAR_MAX_X][ASTAR_MAX_X];

    int openset_count;
    AStarNode_t* openset[ASTAR_MAX_NODES];

    int camefrom_count;
    AStarNode_t* camefrom[ASTAR_MAX_NODES];

    int gscores[ASTAR_MAX_X][ASTAR_MAX_X];
    int fscores[ASTAR_MAX_X][ASTAR_MAX_X];

    // function pointers
    astar_tfunc_t* is_traversable;
    astar_hfunc_t* heuristic;
    
    // result
    int pathlen;
    AStarNode_t path[ASTAR_MAX_NODES];

} AStar_t;

void astar_create(AStar_t* asd, int width, int height, int node_size);
bool astar_traverse(AStarData* asd, int start_x, int start_y, int goal_x, int goal_y);
