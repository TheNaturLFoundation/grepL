#include <stdio.h>
#include <err.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <rationl.h>

#define RED "\033[0;31m"
#define WHITE "\033[0;37m"

void search_lines(reg_t regexp, char *path)
{
    FILE *f = fopen(path, "r");
    if (f == NULL)
        exit(EXIT_FAILURE);

    ssize_t r = 0;
    size_t len = 0;
    char *line = NULL;

    while ( (r = getline(&line, &len, f)) != -1)
    {
        match **match_list;
        size_t match_size = 0;
        size_t position = 0;
        if ((match_size = regex_search(regexp, line, &match_list)))
        {
            char *l = line;
            for (size_t i = 0; i<match_size; i++)
            {
                match *match = match_list[i];
                printf("%.*s", (int)(match->start-position), l);
                l = line + match->start;
                position = match->start;
                printf(RED"%.*s"WHITE, (int)(match->length), l);
                l += match->length;
                position += match->length;
                if (i+1 >= match_size)
                {
                    printf("%s", l);
                }
            }
        }
    }
    free(line);

    fclose(f);
}

void get_files(char *path, reg_t regexp, int option)
{
    struct stat path_stat;
    stat(path, &path_stat);
    if (path_stat.st_mode != S_IFDIR)
    {
        search_lines(regexp, path);
        option -= 1;
    }
    DIR *d = opendir(path);
    if (d == NULL)
        exit(EXIT_FAILURE);
    struct dirent *dir;
    while ((dir = readdir(d)) != NULL)
    {
        if (dir-> d_type != DT_DIR)
        {
            char new_path[512];
            if (sprintf(new_path, "%s/%s", path, dir->d_name) == -1)
                exit(EXIT_FAILURE);
            search_lines(regexp, new_path);
        }
        if (option>=0 && dir -> d_type == DT_DIR &&
            strcmp(dir->d_name,".")!=0 && strcmp(dir->d_name,"..")!=0)
        {
            char new_path[512];
            if (sprintf(new_path, "%s/%s", path, dir->d_name) == -1)
                exit(EXIT_FAILURE);
            get_files(new_path, regexp, option-1);
        }
    }
    closedir(d);
}


int main(int argc, char **argv)
{
    if (argc < 3)
        errx(1, "Please give a file or folder and a regexp");
    reg_t regexp = regex_compile(argv[1]);
    int option = 0;
    if (argc == 4)
        option = atoi(argv[3]);
    get_files(argv[2], regexp, option);
    regex_free(regexp);
    return 0;
}
