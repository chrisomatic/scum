#pragma once

#define ASTAR_MAX_X      32 // maximum grid dimension
#define ASTAR_MAX_NODES  (ASTAR_MAX_X*ASTAR_MAX_X)

typedef struct
{
    int x;
    int y;
} AStarNode_t;

typedef int (*astar_tfunc_t)(int x, int y);
typedef int (*astar_hfunc_t)(AStarNode_t* a, AStarNode_t* b);

typedef struct
{
    // initial data
    int width;
    int height;

    // intermediate use
    AStarNode_t nodemap[ASTAR_MAX_X][ASTAR_MAX_X];

    int openset_count;
    AStarNode_t* openset[ASTAR_MAX_NODES];
    AStarNode_t* camefrom[ASTAR_MAX_NODES];

    int gscores[ASTAR_MAX_X][ASTAR_MAX_X];
    int fscores[ASTAR_MAX_X][ASTAR_MAX_X];

    // function pointers
    astar_tfunc_t traversable;
    astar_hfunc_t heuristic;
    
    // result
    int pathlen;
    AStarNode_t path[ASTAR_MAX_NODES];

} AStar_t;

void astar_create(AStar_t* asd, int width, int height);
bool astar_traverse(AStar_t* asd, int start_x, int start_y, int goal_x, int goal_y);
void astar_print_path_graph(AStar_t* asd);
void astar_test();

void astar_set_traversable_func(AStar_t* asd, astar_tfunc_t func);
void astar_set_heuristic_func(AStar_t* asd, astar_hfunc_t func);
