#pragma once

#include "core/io.h"

#include "creature.h"
#include "item.h"

#define FORMAT_VERSION 1

static bool get_next_section(FILE* fp, char* section);
static int __line_num;

static char room_files[100][32] = {0};

extern char* tile_names[TILE_MAX];
extern char* creature_names[CREATURE_TYPE_MAX];
extern char* item_names[ITEM_MAX];

typedef struct
{
    // header info
    uint8_t version;
    Vector2i size;
    uint8_t type;
    uint8_t rank;
    
    // tile
    TileType tile_types[32][32];

    // creatures
    CreatureType creature_types[100];
    Vector2i     creature_locations[100];
    int          creature_count;

    // items
    ItemType item_types[100];
    Vector2i item_locations[100];
    int      item_count;

    // doors
    bool doors[4];

} RoomFileData;

static void room_file_save(RoomFileData* rfd, char* path, ...)
{
    va_list args;
    va_start(args, path);
    char fpath[256] = {0};
    vsprintf(fpath, path, args);
    va_end(args);

    FILE* fp = fopen(fpath, "w");
    if(fp)
    {

        fprintf(fp, "[%d] # version\n\n", rfd->version);

        fputs("; Room Dimensions\n", fp);
        fprintf(fp, "%d,%d\n\n", rfd->size.x, rfd->size.y);

        fputs("; Room Type\n", fp);
        fprintf(fp, "%d   # %s\n\n", rfd->type, get_room_type_name(rfd->type));

        fputs("; Room Rank\n", fp);
        fprintf(fp, "%d\n\n", rfd->rank);

        fputs("; Tile Mapping\n", fp);
        for(int i = 0; i < TILE_MAX; ++i)
        {
            fprintf(fp, "%s\n", tile_names[i]);
        }
        fputs("\n", fp);

        fputs("; Tile Data\n", fp);
        for(int y = 0; y < rfd->size.y; ++y)
        {
            for(int x = 0; x < rfd->size.x; ++x)
            {
                int tt = rfd->tile_types[x][y];

                fprintf(fp, "%d", tt);

                if(x == rfd->size.x-1)
                    fputs("\n", fp);
                else
                    fputs(",", fp);
            }
        }

        fputs("\n", fp);

        fputs("; Creature Mapping\n", fp);
        for(int i = 0; i < CREATURE_TYPE_MAX; ++i)
        {
            fprintf(fp, "%s\n", creature_names[i]);
        }
        fputs("\n", fp);

        fputs("; Creatures\n", fp);
        for(int i = 0; i < rfd->creature_count; ++i)
        {
            fprintf(fp, "%d,%d,%d\n", rfd->creature_types[i], rfd->creature_locations[i].x, rfd->creature_locations[i].y);
        }
        fputs("\n", fp);

        fputs("; Item Mapping\n", fp);
        for(int i = 0; i < ITEM_MAX; ++i)
        {
            fprintf(fp, "%s\n", item_names[i]);
        }
        fputs("\n", fp);

        fputs("; Items\n", fp);
        for(int i = 0; i < rfd->item_count; ++i)
        {
            fprintf(fp, "%d,%d,%d\n", rfd->item_types[i], rfd->item_locations[i].x, rfd->item_locations[i].y);
        }
        fputs("\n", fp);

        fputs("; Doors\n", fp);
        for(int i = 0; i < 4; ++i)
        {
            fprintf(fp, "%d", rfd->doors[i] ? 1 : 0);
            if(i == 3)
                fputs("\n", fp);
            else
                fputs(",", fp);
        }

        fputs("\n", fp);

        fclose(fp);
    }
}

static bool room_file_load(RoomFileData* rfd, char* path, ...)
{
    va_list args;
    va_start(args, path);
    char filename[256] = {0};
    vsprintf(filename, path, args);
    va_end(args);

    FILE* fp = fopen(filename,"r");
    if(!fp) return false;

    // clear rfd
    memset(rfd,0,sizeof(RoomFileData));

    __line_num = 0;

    char line[100] = {0};

    fgets(line,sizeof(line),fp); __line_num++;

    // parse version
    if(line[0] == '[')
    {
        int matches = sscanf(line,"[%d]",&rfd->version);
        if(matches == 0)
        {
            LOGW("Failed to room file version");
        }
    }

    int tile_mapping[TILE_MAX] = {0};
    int tmi = 0;

    int creature_mapping[CREATURE_TYPE_MAX] = {0};
    int cmi = 0;

    int item_mapping[ITEM_MAX] = {0};
    int imi = 0;

    for(;;)
    {
        char section[100] = {0};
        bool check = get_next_section(fp,section);

        if(!check)
            break;

        if(STR_EQUAL(section,"Room Dimensions"))
        {
            fgets(line,sizeof(line),fp); __line_num++;
            sscanf(line,"%d,%d",&rfd->size.x,&rfd->size.y);
        }
        else if(STR_EQUAL(section,"Room Type"))
        {
            fgets(line,sizeof(line),fp); __line_num++;
            sscanf(line,"%d",&rfd->type);
        }
        else if(STR_EQUAL(section,"Room Rank"))
        {
            fgets(line,sizeof(line),fp); __line_num++;
            sscanf(line,"%d",&rfd->rank);
        }
        else if(STR_EQUAL(section,"Tile Mapping"))
        {
            for(;;)
            {
                char* check = fgets(line,sizeof(line),fp); __line_num++;
                line[strcspn(line, "\r\n")] = 0; // remove newline

                if(!check || STR_EMPTY(line))
                    break;

                for(int i = 0; i < TILE_MAX; ++i)
                {
                    if(STR_EQUAL(line, tile_names[i]))
                    {
                        tile_mapping[tmi++] = i;
                        break;
                    }
                }
            }
        }
        else if(STR_EQUAL(section,"Tile Data"))
        {
            for(int i = 0; i < rfd->size.y; ++i)
            {
                fgets(line,sizeof(line),fp); __line_num++;

                char* p = &line[0];

                char num_str[4];
                int ni;

                for(int j = 0; j < rfd->size.x; ++j)
                {
                    ni = 0;
                    memset(num_str,0,4*sizeof(char));

                    while(p && *p != ',' && *p != '\n')
                        num_str[ni++] = *p++;

                    p++;

                    int num = atoi(num_str);
                    if(num < 0 || num >= TILE_MAX)
                    {

                        LOGW("Failed to load tile, out of range (index: %d); line_num: %d",num,__line_num);
                        continue;
                    }

                    rfd->tile_types[j][i] = tile_mapping[num];
                }
            }
        }
        else if(STR_EQUAL(section,"Creature Mapping"))
        {
            for(;;)
            {
                char* check = fgets(line,sizeof(line),fp); __line_num++;
                line[strcspn(line, "\r\n")] = 0; // remove newline

                if(!check || STR_EMPTY(line))
                    break;

                for(int i = 0; i < CREATURE_TYPE_MAX; ++i)
                {
                    if(STR_EQUAL(line, creature_names[i]))
                    {
                        creature_mapping[cmi++] = i;
                        break;
                    }
                }
            }
        }
        else if(STR_EQUAL(section,"Creatures"))
        {
            int index;
            int x,y;

            for(;;)
            {
                fgets(line,sizeof(line),fp); __line_num++;
                int matches = sscanf(line,"%d,%d,%d",&index,&x,&y);

                if(matches != 3)
                    break;

                if(index < 0 || index >= CREATURE_TYPE_MAX)
                {
                    LOGW("Failed to load creature, out of range (index: %d); line_num: %d",index, __line_num);
                    continue;
                }

                CreatureType type = creature_mapping[index];

                rfd->creature_types[rfd->creature_count] = type;
                rfd->creature_locations[rfd->creature_count].x = x;
                rfd->creature_locations[rfd->creature_count].y = y;
                rfd->creature_count++;
            }
        }
        else if(STR_EQUAL(section,"Item Mapping"))
        {
            for(;;)
            {
                char* check = fgets(line,sizeof(line),fp); __line_num++;
                line[strcspn(line, "\r\n")] = 0; // remove newline

                if(!check || STR_EMPTY(line))
                    break;

                for(int i = 0; i < ITEM_MAX; ++i)
                {
                    if(STR_EQUAL(line, item_names[i]))
                    {
                        item_mapping[imi++] = i;
                        break;
                    }
                }

            }
        }
        else if(STR_EQUAL(section,"Items"))
        {
            int index;
            int x,y;

            for(;;)
            {
                fgets(line,sizeof(line),fp); __line_num++;
                int matches = sscanf(line,"%d,%d,%d",&index,&x,&y);

                if(matches != 3)
                    break;

                if(index < 0 || index >= ITEM_MAX)
                {
                    LOGW("Failed to load item, out of range (index: %d); line_num: %d",index, __line_num);
                    continue;
                }

                ItemType type = item_mapping[index];

                rfd->item_types[rfd->item_count] = type;
                rfd->item_locations[rfd->item_count].x = x;
                rfd->item_locations[rfd->item_count].y = y;
                rfd->item_count++;
            }
        }
        else if(STR_EQUAL(section,"Doors"))
        {
            fgets(line,sizeof(line),fp); __line_num++;

            int up,right,down,left;
            sscanf(line,"%d,%d,%d,%d",&up,&right,&down,&left);

            rfd->doors[DIR_UP]    = (up == 1);
            rfd->doors[DIR_RIGHT] = (right == 1);
            rfd->doors[DIR_DOWN]  = (down == 1);
            rfd->doors[DIR_LEFT]  = (left == 1);
        }
        else
        {
            LOGW("Unhandled Section: %s; line_num: %d",section, __line_num);
        }
    }

    fclose(fp);

    return true;
}

static int room_file_get_all(char* _files[])
{
    int num_files = io_get_files_in_dir("src/rooms",".room", room_files);

    for(int i = 0; i < num_files; ++i)
        _files[i] = room_files[i];

    LOGI("room files count: %d", num_files);
    for(int i = 0; i < num_files; ++i)
        LOGI("  %d) %s", i+1, room_files[i]);

    return num_files;
}

static bool get_next_section(FILE* fp, char* section)
{
    if(!fp)
        return false;

    char line[100] = {0};
    char* s = section;

    for(;;)
    {
        memset(line,0,100);

        // get line
        char* check = fgets(line,sizeof(line),fp); __line_num++;

        if(!check) return false;

        if(line[0] == ';')
        {
            char* p = &line[1];
            p = io_str_eat_whitespace(p);

            while(p && *p != '\n')
                *s++ = *p++;
            break;
        }
    }

    return true;
}
