#pragma once

#define ASTAR_MAX_NODES 128*128

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
    AStarNode_t nodemap[ASTAR_MAX_NODES];

    int gscores[ASTAR_MAX_NODES];
    int hscores[ASTAR_MAX_NODES];

    int openset_count;
    AStarNode_t* openset[ASTAR_MAX_NODES];

    int camefrom_count;
    AStarNode_t* camefrom[ASTAR_MAX_NODES];

    // function pointers
    astar_tfunc_t* is_traversable;
    astar_hfunc_t* heuristic;
    
    // result
    int pathlen;
    AStarNode_t path[ASTAR_MAX_NODES];

} AStar_t;

void astar_create(AStar_t* asd, int width, int height, int node_size);
bool astar_traverse(AStar_t* asd, int start_x, int start_y, int end_x, int end_y);
