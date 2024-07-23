#include <sys/stat.h>  //TODO: windows?

#include "core/files.h"
#include "creature.h"
#include "item.h"
#include "level.h"
#include "main.h"

#include "room_file.h"

static int __line_num;

char room_files[512][32] = {0};
char* p_room_files[512] = {0};
int  room_file_count = 0;

RoomFileData room_list[MAX_ROOM_LIST_COUNT] = {0};
int room_list_count = 0;

static bool get_next_section(FILE* fp, char* section);

void room_file_save(RoomFileData* rfd, char* path, ...)
{
    va_list args;
    va_start(args, path);
    char fpath[512] = {0};
    vsprintf(fpath, path, args);
    va_end(args);

    FILE* fp = fopen(fpath, "w");
    if(fp)
    {
        fprintf(fp, "[%d] # version\n\n", rfd->version);

        fputs("; Room Dimensions\n", fp);
        fprintf(fp, "%d,%d\n\n", rfd->size_x, rfd->size_y);

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
        for(int y = 0; y < rfd->size_y; ++y)
        {
            for(int x = 0; x < rfd->size_x; ++x)
            {
                int tt = rfd->tiles[x][y];

                fprintf(fp, "%d", tt);

                if(x == rfd->size_x-1)
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
            fprintf(fp, "%d,%d,%d,%d\n", rfd->creature_types[i], rfd->creature_locations_x[i], rfd->creature_locations_y[i], rfd->creature_orientations[i]);
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
            fprintf(fp, "%d,%d,%d\n", rfd->item_types[i], rfd->item_locations_x[i], rfd->item_locations_y[i]);
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

        fputs("; Commentary\n", fp);
        fprintf(fp, "%d", rfd->stars);
        fputs("\n", fp);
        fprintf(fp, "%s", rfd->comment);
        fputs("\n", fp);

        fclose(fp);

        struct stat stats = {0};
        if(stat(fpath, &stats) == 0)
        {
            rfd->mtime = stats.st_mtime;
            rfd->fsize = stats.st_size;
        }

        printf("saved file: %s (%ld, %ld)\n", fpath, rfd->mtime, stats.st_mtime);

    }
}

bool room_file_load(RoomFileData* rfd, bool force, bool print_errors, char* path, ...)
{
    va_list args;
    va_start(args, path);
    char filename[512] = {0};
    vsprintf(filename, path, args);
    va_end(args);

    struct stat stats = {0};
#if 0

    if(stat(filename, &stats) == 0)
    {
        // printf("size:  %d\n", stats.st_size);
        // printf("mtime: %ld\n", stats.st_mtime);
        if(rfd->mtime == stats.st_mtime && rfd->fsize == stats.st_size && !force)
        {
            return true;
        }
    }
    else
    {
        // file dne
        printf("DNE\n");
        return false;
    }
#endif

    // printf("loading file: %s\n", filename);

    io_replace_char(filename, '\\', '/');


    FILE* fp = fopen(filename,"r");
    if(!fp) 
    {
        printf("No file found (%s)\n", filename);
        return false;
    }

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
            if(print_errors) LOGW("Failed to room file version");
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
            sscanf(line,"%d,%d",&rfd->size_x,&rfd->size_y);
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
            for(int i = 0; i < rfd->size_y; ++i)
            {
                fgets(line,sizeof(line),fp); __line_num++;

                char* p = &line[0];

                char num_str[4];
                int ni;

                for(int j = 0; j < rfd->size_x; ++j)
                {
                    ni = 0;
                    memset(num_str,0,4*sizeof(char));

                    while(p && *p != ',' && *p != '\n')
                        num_str[ni++] = *p++;

                    p++;

                    int num = atoi(num_str);
                    if(num < 0 || num >= TILE_MAX)
                    {

                        if(print_errors) LOGW("Failed to load tile, out of range (index: %d); line_num: %d",num,__line_num);
                        continue;
                    }

                    rfd->tiles[j][i] = tile_mapping[num];
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
            int orientation = 0;

            for(;;)
            {
                fgets(line,sizeof(line),fp); __line_num++;
                int matches = sscanf(line,"%d,%d,%d,%d",&index,&x,&y,&orientation);

                if(matches != 4)
                {
                    matches = sscanf(line,"%d,%d,%d",&index,&x,&y);
                    if(matches != 3)
                        break;
                }

                if(index < 0 || index >= CREATURE_TYPE_MAX)
                {
                    if(print_errors) LOGW("Failed to load creature, out of range (index: %d); line_num: %d",index, __line_num);
                    continue;
                }

                if(x < 0 || x > OBJECTS_MAX_X)
                {
                    if(print_errors) LOGW("Failed to load creature, x coordinate (%d), out of range (index: %d); line_num: %d", x, index, __line_num);
                    continue;
                }
                if(y < 0 || y > OBJECTS_MAX_Y)
                {
                    if(print_errors) LOGW("Failed to load creature, y coordinate (%d), out of range (index: %d); line_num: %d", y, index, __line_num);
                    continue;
                }

                CreatureType type = creature_mapping[index];

                rfd->creature_types[rfd->creature_count] = type;
                rfd->creature_locations_x[rfd->creature_count] = x;
                rfd->creature_locations_y[rfd->creature_count] = y;
                rfd->creature_orientations[rfd->creature_count] = orientation;
                rfd->creature_count++;
            }
        }
        else if(STR_EQUAL(section,"Item Mapping"))
        {
            for(;;)
            {
                char* check = fgets(line,sizeof(line),fp); __line_num++;
                line[strcspn(line, "\r\n")] = 0; // remove newline

                //printf("item mapping. filename: %s\n", filename);

                if(!check || STR_EMPTY(line))
                    break;

                bool matched = false;
                for(int i = 0; i < ITEM_MAX; ++i)
                {
                    if(STR_EQUAL(line, item_names[i]))
                    {
                        matched = true;
                        item_mapping[imi++] = i;
                        break;
                    }
                }
                if(!matched)
                {
                    item_mapping[imi++] = -1;
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
                    if(print_errors) LOGW("Failed to load item, out of range (index: %d); line_num: %d",index, __line_num);
                    continue;
                }

                if(x < 0 || x > OBJECTS_MAX_X)
                {
                    if(print_errors) LOGW("Failed to load item, x coordinate (%d), out of range (index: %d); line_num: %d", x, index, __line_num);
                    continue;
                }
                if(y < 0 || y > OBJECTS_MAX_Y)
                {
                    if(print_errors) LOGW("Failed to load item, y coordinate (%d), out of range (index: %d); line_num: %d", y, index, __line_num);
                    continue;
                }

                ItemType type = item_mapping[index];

                if(type >= 0)
                {
                    rfd->item_types[rfd->item_count] = type;
                    rfd->item_locations_x[rfd->item_count] = x;
                    rfd->item_locations_y[rfd->item_count] = y;
                    rfd->item_count++;
                }
            }

            // if(STR_EQUAL("src/rooms/treasure1.room", filename))
            // {
            //     for(int i = 0; i < imi; ++i)
            //     {
            //         printf("item mapping %d: %d\n", i, item_mapping[i]);
            //     }
            // }
            // printf("item mapping. filename: %s\n", filename);

        }
        else if(STR_EQUAL(section,"Doors"))
        {
            fgets(line,sizeof(line),fp); __line_num++;

            int up,right,down,left;
            sscanf(line,"%d,%d,%d,%d",&right,&up,&left,&down);

            rfd->doors[DIR_RIGHT] = (right == 1);
            rfd->doors[DIR_UP]    = (up == 1);
            rfd->doors[DIR_LEFT]  = (left == 1);
            rfd->doors[DIR_DOWN]  = (down == 1);
        }
        else if(STR_EQUAL(section,"Commentary"))
        {
            fgets(line,sizeof(line),fp); __line_num++;
            sscanf(line,"%d",&rfd->stars);

            fgets(rfd->comment,sizeof(line),fp); __line_num++;
        }
        else
        {
            if(print_errors) LOGW("Unhandled Section: %s; line_num: %d",section, __line_num);
        }
    }

    fclose(fp);

    if(stat(filename, &stats) == 0)
    {
        rfd->mtime = stats.st_mtime;
        rfd->fsize = stats.st_size;
    }
    // printf("loaded file: %s (%ld, %ld)\n", filename, rfd->mtime, stats.st_mtime);

    return true;
}

bool room_file_load_all(bool force)
{
    bool file_diff = room_file_get_all(force);
    if(!file_diff && !force) return false;

    room_list_count = 0;

    for(int i = 0; i < room_file_count; ++i)
    {
        // printf("i: %d, Loading room %s\n",i, p_room_files[i]);
        bool success = room_file_load(&room_list[room_list_count], force, true, "src/rooms/%s", p_room_files[i]);
        if(!success)
        {
            printf("Failed to open room file: %s\n", p_room_files[i]);
            continue;
        }

        room_list[room_list_count].file_index = i;
        room_list_count++;
    }

    return true;
    // printf("room list count: %d\n",room_list_count);
}

bool room_file_get_all(bool force)
{
    char temp_room_files[512][32] = {0};
    int temp_room_file_count = io_get_files_in_dir("src/rooms",".room", temp_room_files);

    // insertion sort
    int i, j;
    char key[32];

    for (i = 1; i < temp_room_file_count; ++i) 
    {
        memcpy(key, temp_room_files[i], 32*sizeof(char));
        j = i - 1;

        while (j >= 0 && strncmp(key,temp_room_files[j],32) < 0)
        {
            memcpy(temp_room_files[j+1], temp_room_files[j], 32*sizeof(char));
            j = j - 1;
        }
        memcpy(temp_room_files[j+1], key, 32*sizeof(char));
    }

    if(temp_room_file_count != room_file_count || force)
    {
        if(memcmp(temp_room_files, room_files, sizeof(char)*512*32) != 0 || force)
        {
            room_file_count = temp_room_file_count;
            memcpy(room_files, temp_room_files, sizeof(char)*512*32);

            // copy to pointer array
            for(int i = 0; i < room_file_count; ++i)
                p_room_files[i] = room_files[i];

            // LOGI("room files count: %d", room_file_count);
            // for(int i = 0; i < room_file_count; ++i)
            //     LOGI("  %d) %s", i+1, room_files[i]);

            return true;
        }
    }

    return false;
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
