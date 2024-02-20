#include <stdio.h>
#include <stdlib.h>

#define ASINDEX(x,y,w) ((y)*(w)+x)

// openset queue handling functions
static void         openset_add(AStar_t* asd, AStarNode* n);
static AStarNode_t* openset_get(AStar_t* asd);
static bool         openset_in(AStar_t* asd, AStarNode* n);

// heuristic functions
static int h_manhatten_distance(AStarNode_t* a, AStarNode_t* b);

// helpers
static int get_neighbors(AStar_t* asd, AStarNode_t* n, AStarNode_t* neighbors[]);

// -----------------------
// Global functions
// -----------------------

void astar_create(AStar_t* asd, int width, int height, int node_size)
{
    asd->width = width;
    asd->height = height;
    asd->node_size = node_size;
    asd->heuristic = h_manhatten_distance; // default
}

bool astar_traverse(AStarData* asd, int start_x, int start_y, int goal_x, int goal_y)
{
    // initialize variables for traversal
    asd->openset_count = 0;
    asd->camefrom_count = 0;
    asd->pathlen = 0;

    // begin with start node
    openset_add(asd, &asd->nodemap[start_x][start_y]);

    // map gscores and fscores with infinity, except starting node
    memset(asd->gscores, INT_MAX, ASTAR_MAX_NODES*sizeof(int));
    memset(asd->fscores, INT_MAX, ASTAR_MAX_NODES*sizeof(int));

    asd->gscores[start_x][start_y] = 0;
    asd->fscores[start_x][start_y] = 0;

    AStarNode_t* goal = &asd->nodemap[goal_x][goal_y];

    while(asd->openset_count > 0)
    {
        AStarNode_t* curr = openset_get(asd);

        if(curr == goal)
        {
            // @TODO reconstruct path
            return true;
        }

        AStarNode_t* n[4];
        int neighbor_count = get_neighbors(asd, curr, n);

        for(int i = 0; i < neighbor_count; ++i)
        {
            if(!asd->traversable(n[i]))
                continue; // can't travel on this neighbor

            int tentative_gscore = asd->gscores[curr->x][curr->y] + 1; // using 1 as the distance between neighbors

            if(tentative_gscore < asd->gscores[n[i]->x][n[i]->y])
            {
                // this path is better to the neighbor than previous
                asd->camefrom[(n[i]->x, n[i]->y, asd->width)] = curr;

                asd->gscores[n[i]->x][n[i]->y] = tentative_gscore;
                asd->fscores[n[i]->x][n[i]->y] = tentative_gscore + asd->heuristic(n[i], goal);

                if(!openset_in(n[i]))
                    openset_add(asd,n[i]);
            }
        }
    }
    
    return false;
}

void astar_test()
{
    AStar_t asd;

    astar_create(&as, 7, 7, sizeof(int));
    astar_set_traversible_func(NULL);

    if(!astar_traverse(&asd,0,0,4,5))
    {
        printf("Failed to find a path!\n");
            return 1;
    }

    printf("Found a path!\n");
    for(int i = 0; i < asd->pathlen; ++i)
    {
        printf("%d: [%d, %d]\n", i, asd->path[i].x, asd->path[i].y);
    }
}

// -----------------------
// Static functions
// -----------------------

static bool openset_in(AStar_t* asd, AStarNode* n)
{
    for(int i = 0; i < asd->openset_count; ++i)
    {
        if(asd->openset[i] == n)
            return true;
    }
    return false;
}

static void openset_add(AStar_t* asd, AStarNode* n)
{
    if(asd->openset_count >= (asd->width*asd->height))
    {
        printf("Failed to add to openset, exceeded max count");
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
    
    for(int i = 0; i < asd->open_set_count; ++i)
    {
        AStarNode_t* n = asd->openset[i];
        if(n->fscore < fscore_min)
        {
            fscore_min = n->fscore;
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

static int h_manhatten_distance(AStarNode_t* a, AStarNode_t* b)
{
    int dx = ABS(a->x - b->x);
    int dy = ABS(a->y - b->y);

    return (dx+dy);
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
