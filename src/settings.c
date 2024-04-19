#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "player.h"
#include "settings.h"

bool settings_save(char* filename, Settings* s)
{
    FILE* fp = fopen(filename, "w");

    if(!fp)
    {
        return false;
    }

    fprintf(fp, "name: %s\n", s->name);
    fprintf(fp, "class: %u\n", s->class);
    fprintf(fp, "color: %08X\n", s->color);

    fclose(fp);

    return true;
}

bool settings_load(char* filename, Settings* s)
{
    FILE* fp = fopen(filename, "r");

    if(!fp)
    {
        return false;
    }

    char label[100] = {0};
    char value[100] = {0};

    int lindex = 0;
    int vindex = 0;

    bool is_value = false;

    int c;

    for(;;)
    {
        c = fgetc(fp);

        if(c == EOF)
            break;

        if(c == ' ' || c == '\t')
            continue; // whitespace, don't care
        
        if(is_value && c == '\n')
        {
            is_value = !is_value;
            
            if(STR_EQUAL(label, "name"))
            {
                memset(s->name,0,PLAYER_NAME_MAX);
                memcpy(s->name,value,MIN(PLAYER_NAME_MAX, vindex));
            }
            else if(STR_EQUAL(label, "class"))
            {
                s->class = atoi(value);
            }
            else if(STR_EQUAL(label, "color"))
            {
                s->color = (uint32_t)strtol(value, NULL, 16);
            }

            memset(label,0,100); lindex = 0;
            memset(value,0,100); vindex = 0;

            continue;
        }

        if(!is_value && c == ':')
        {
            is_value = !is_value;
            lindex++;
            // label[lindex++] == c;
            continue;
        }

        if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (c == '-' || c == '_'))
        {
            // valid character

            if(is_value) value[vindex++] = c;
            else         label[lindex++] = c;
        }
    }

    return true;
}
