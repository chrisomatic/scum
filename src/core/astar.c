#include <stdio.h>

#define ASINDEX(x,y,w) ((y)*(w)+(x))

/*
 
   USAGE:

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
*/

static int h_manhatten_distance(AStarNode_t* a, AStarNode_t* b)
{
    int dx = ABS(a->x - b->x);
    int dy = ABS(a->y - b->y);

    return (dx+dy);
}

static void _init_sets(AStar_t* asd)
{
    asd->openset_count = 0;
    asd->camefrom_count = 0;
    asd->pathlen = 0;

    memset(asd->gscores, INT_MAX, ASTAR_MAX_NODES*sizeof(int));
    memset(asd->hscores, 0, ASTAR_MAX_NODES*sizeof(int));
}

void astar_create(AStar_t* asd, int width, int height, int node_size)
{
    asd->width = width;
    asd->height = height;
    asd->node_size = node_size;
    asd->heuristic = h_manhatten_distance;
}

static AStarNode_t* get_node_at_xy(AStar_t* asd, int x, int y)
{
    return &asd->nodemap[ASINDEX(x,y,asd->width)];
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

    return asd->openset[min_index];
}

bool astar_traverse(AStarData* asd, int start_x, int start_y, int end_x, int end_y)
{
    _init_sets(asd);

    openset_add(asd, get_node_at_xy(start_x, start_y));
    asd->gscores[ASINDEX(start_x,start_y,asd->width)] = 0;

    while(asd->openset_count > 0)
    {
    
    
    }
    
    return false;
}
