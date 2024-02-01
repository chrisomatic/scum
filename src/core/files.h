#pragma once

#include "headers.h"

#ifdef _WIN32
#define F_OK 0
#define access _access
#define MAX_SIZE 32
#include <io.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif


static BOOL io_file_exists(char* file_path)
{
    return (access(file_path, F_OK) == 0);
}

static char* io_get_filename(char* full_path)
{
    char* pfile;
    pfile = full_path + strlen(full_path);
    for (; pfile > full_path; pfile--)
    {
        if ((*pfile == '\\') || (*pfile == '/'))
        {
            pfile++;
            break;
        }
    }
    return pfile;
}

static char* remove_extension(char* filename)
{
    char* pfile;
    pfile = filename;
    int len = strlen(filename);

    for(; pfile < filename + len; pfile++)
    {
        if(*pfile == '.')
        {
            while(*pfile)
            {
                *pfile = '\0';
                pfile++;
            }
        }
    }
    return filename;
}

static char* io_str_is_num(char* str)
{

}

static char* io_str_eat_whitespace(char* str)
{
    while(str && *str == ' ' || *str == '\t' || *str == '\r' || *str == '\n')
        str++;

    return str;
}

static char* io_get_string_from_file(char* filename)
{
    char* buffer = NULL;

    FILE* fp = fopen(filename,"rb");
    if(!fp) return NULL;
        
    if(fp)
    {
        fseek(fp, 0, SEEK_END);
        int length = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        buffer = (char*)malloc(length);
        if(buffer)
        {
            fread(buffer, 1, length, fp);
            buffer[length] = '\0';
        }
        fclose(fp);
    }

    return buffer;
}

static void io_replace_char(char* str, char original, char replace)
{
    while(*str)
    {
        if(*str == original)
            *str = replace;
        ++str;
    }
}

static int io_get_files_in_dir(char* dir_path, char* match_str, char files[32][32])
{
#ifdef _WIN32

    // const int max_size = MAX_PATH;

    WIN32_FIND_DATA ffd;
    TCHAR szDir[MAX_SIZE];
    HANDLE hFind = INVALID_HANDLE_VALUE;
    size_t length_of_arg = 0;
    int num_files = 0;

    StringCchLength(dir_path, MAX_SIZE, &length_of_arg);

    if (length_of_arg > (MAX_SIZE - 3))
    {
        printf(TEXT("\nDirectory path is too long.\n"));
        return (-1);
    }

    StringCchCopy(szDir, MAX_SIZE, dir_path);
    StringCchCat(szDir, MAX_SIZE, TEXT("\\*"));
    StringCchCat(szDir, MAX_SIZE, match_str);

    hFind = FindFirstFile(szDir, &ffd);

    if (INVALID_HANDLE_VALUE == hFind)
    {
        printf(TEXT("\nError getting first file.\n"));
        return -1;
    } 

    /*
    char full_file_path[MAX_SIZE] = {0};
    StringCchCopy(full_file_path, MAX_SIZE,dir_path);
    StringCchCat(full_file_path, MAX_SIZE, "\\");
    StringCchCat(full_file_path, MAX_SIZE, ffd.cFileName);
    StringCchCopy(files[num_files], MAX_SIZE,full_file_path);
    */
    StringCchCopy(files[num_files], MAX_SIZE,ffd.cFileName);

    do
    {
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // directory
        }
        else
        {
            /*
            StringCchCopy(full_file_path, MAX_SIZE,dir_path);
            StringCchCat(full_file_path, MAX_SIZE, "\\");
            StringCchCat(full_file_path, MAX_SIZE, ffd.cFileName);
            StringCchCopy(files[num_files++], MAX_SIZE,full_file_path);
            */
            StringCchCopy(files[num_files++], MAX_SIZE,ffd.cFileName);
        }
    } while (FindNextFile(hFind, &ffd) != 0);

    FindClose(hFind);
    return num_files;

#else

    DIR *dir = opendir(dir_path);

    if(!dir)
        return 0;

    int num_files = 0;
    struct dirent *ent;

    while ((ent = readdir(dir)) != NULL)
    {
        if(strstr(ent->d_name,match_str) == NULL || strcmp(ent->d_name,".") ==  0 || strcmp(ent->d_name,"..") == 0)
            continue;

        strcpy(files[num_files],ent->d_name);
        num_files++;
    }

    closedir(dir);
    return num_files;

#endif
}
