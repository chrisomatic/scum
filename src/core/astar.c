#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <math.h>

#include "astar.h"

// openset queue handling functions
static void         openset_add(AStar_t* asd, AStarNode_t* n);
static AStarNode_t* openset_get(AStar_t* asd);
static bool         openset_in(AStar_t* asd, AStarNode_t* n);

// default functions
static int manhatten_distance(AStarNode_t* a, AStarNode_t* b);
static int default_traversable(int x, int y);

// helpers
static int get_neighbors(AStar_t* asd, AStarNode_t* n, AStarNode_t* neighbors[]);
static void reconstruct_path(AStar_t* asd, AStarNode_t* start, AStarNode_t* goal);

// -----------------------
// Global functions
// -----------------------

void astar_create(AStar_t* asd, int width, int height)
{
    asd->width = width;
    asd->height = height;
    asd->traversable = default_traversable;
    asd->heuristic = manhatten_distance;

    for(int j = 0; j < asd->height; ++j)
    {
        for(int i = 0; i < asd->width; ++i)
        {
            asd->nodemap[i][j].x = i;
            asd->nodemap[i][j].y = j;
        }
    }
}

bool astar_traverse(AStar_t* asd, int start_x, int start_y, int goal_x, int goal_y)
{

    // initialize variables for traversal
    asd->openset_count = 0;
    asd->pathlen = 0;

    AStarNode_t* start = &asd->nodemap[start_x][start_y];
    AStarNode_t* goal  = &asd->nodemap[goal_x][goal_y];

    // begin with start node
    openset_add(asd, start);

    // map gscores and fscores with infinity, except starting node
    for(int j = 0; j < asd->height; ++j)
    {
        for(int i = 0; i < asd->width; ++i)
        {
            asd->gscores[i][j] = INT_MAX;
            asd->fscores[i][j] = INT_MAX;
        }
    }

    asd->gscores[start_x][start_y] = 0;
    asd->fscores[start_x][start_y] = 0;

    while(asd->openset_count > 0)
    {
        asd->prev = asd->curr;
        asd->curr = openset_get(asd); // gets lowest fscore node

        if(asd->curr == goal)
        {
            reconstruct_path(asd, start, goal);
            return true;
        }

        AStarNode_t* n[4];
        int neighbor_count = get_neighbors(asd, asd->curr, n);

        for(int i = 0; i < neighbor_count; ++i)
        {
            int traversability = asd->traversable(n[i]->x, n[i]->y);

            if(traversability == 0)
                continue; // can't travel on this neighbor

            float d = 1.0; // using 1 since manhatten distance
            int tentative_gscore = asd->gscores[asd->curr->x][asd->curr->y] + d*traversability;

            if(tentative_gscore < asd->gscores[n[i]->x][n[i]->y])
            {
                // this path is better to the neighbor than previous
                asd->camefrom[ASINDEX(n[i]->x, n[i]->y, asd->width)] = asd->curr;

                asd->gscores[n[i]->x][n[i]->y] = tentative_gscore;
                asd->fscores[n[i]->x][n[i]->y] = tentative_gscore + asd->heuristic(n[i], goal);

                if(!openset_in(asd, n[i]))
                {
                    openset_add(asd,n[i]);
                }
            }
        }
    }
    
    return false;
}

void astar_print_path_graph(AStar_t* asd)
{
    for(int j = 0; j < asd->height; ++j)
    {
        for(int i = 0; i < asd->pathlen; ++i)
        {
            bool in_path = false;
            for(int n = 0; n < asd->pathlen; ++n)
            {
                if(asd->path[n].x == i && asd->path[n].y == j)
                {
                    in_path = true;
                    break;
                }
            }

            int traversability = asd->traversable(i, j);
            printf("%c", in_path ? '*' : (traversability > 0 ? 'x' : '_'));
        }
        printf("\n");
    }
}

void astar_set_traversable_func(AStar_t* asd, astar_tfunc_t func)
{
    asd->traversable = func;
}

void astar_set_heuristic_func(AStar_t* asd, astar_hfunc_t func)
{
    asd->heuristic = func;
}

static int _test_traversable_func(int x, int y)
{
    if(x == 1 && y == 0) return 0;
    if(x == 1 && y == 1) return 0;
    if(x == 1 && y == 2) return 0;
    if(x == 1 && y == 3) return 0;
    if(x == 1 && y == 5) return 0;

    return 1;
}

void astar_test()
{
    AStar_t asd;

    astar_create(&asd, 7, 7);
    astar_set_traversable_func(&asd, _test_traversable_func);

    if(!astar_traverse(&asd,0,0,5,0))
    {
        printf("astar test failed\n");
        return;
    }

    printf("astar test succeeded\n");
    astar_print_path_graph(&asd);
}

// -----------------------
// Static functions
// -----------------------

static bool openset_in(AStar_t* asd, AStarNode_t* n)
{
    for(int i = 0; i < asd->openset_count; ++i)
    {
        if(asd->openset[i] == n)
            return true;
    }
    return false;
}

static void openset_add(AStar_t* asd, AStarNode_t* n)
{
    int open_count = asd->openset_count;
    int total_count = asd->width*asd->height;

    if(open_count >= total_count)
    {
        printf("Failed to add to openset, exceeded max count (%d >= %d)\n", open_count, total_count);
        return;
    }

    asd->openset[asd->openset_count++] = n;
}

static AStarNode_t* openset_get(AStar_t* asd)
{
    // @TODO: This has O(N) time, and can be improved
    // return node with smallest fscore
    int fscore_min = INT_MAX;
    int min_index = -1;
    
    for(int i = 0; i < asd->openset_count; ++i)
    {
        AStarNode_t* n = asd->openset[i];
        if(asd->fscores[n->x][n->y] < fscore_min)
        {
            fscore_min = asd->fscores[n->x][n->y];
            min_index = i;
        }
    }

    if(min_index == -1)
        return NULL;

    AStarNode_t* min = asd->openset[min_index];

    memcpy(&asd->openset[min_index],&asd->openset[asd->openset_count-1], sizeof(AStarNode_t*));
    --asd->openset_count;

    return min;
}

static int manhatten_distance(AStarNode_t* a, AStarNode_t* b)
{
    int dx = abs(a->x - b->x);
    int dy = abs(a->y - b->y);

    return (dx+dy);
}

static int default_traversable(int x, int y)
{
    return 1;
}

static int get_neighbors(AStar_t* asd, AStarNode_t* n, AStarNode_t* neighbors[])
{
    int x = n->x;
    int y = n->y;

    AStarNode_t up    = {.x = x, .y = y - 1};
    AStarNode_t down  = {.x = x, .y = y + 1};
    AStarNode_t left  = {.x = x - 1, .y = y};
    AStarNode_t right = {.x = x + 1, .y = y};

    AStarNode_t dirs[] = {up,down,left,right};

    int neighbor_count = 0;

    for(int i = 0; i < 4; ++i)
    {
        AStarNode_t* dir = &dirs[i];

        if(dir->x >= 0 && dir->x < asd->width)
            if(dir->y >= 0 && dir->y < asd->height)
                neighbors[neighbor_count++] = &asd->nodemap[dir->x][dir->y];
    }

    return neighbor_count;
}

static void reconstruct_path(AStar_t* asd, AStarNode_t* start, AStarNode_t* goal)
{
    AStarNode_t* n = goal;

    for(;;)
    {
        // add to path
        memcpy(&asd->path[1],&asd->path[0],asd->pathlen*sizeof(AStarNode_t*));
        asd->path[0].x = n->x;
        asd->path[0].y = n->y;
        asd->pathlen++;

        if(n == start)
            break;

        int index = ASINDEX(n->x, n->y, asd->width);
        n = asd->camefrom[index];
    }
}
