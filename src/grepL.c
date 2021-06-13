#include <dirent.h>
#include <err.h>
#include <rationl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define RED "\033[0;31m"
#define WHITE "\033[0;37m"
#define USAGE printf("Usage : grepL [OPTIONS]... PATTERN [FILE/FOLDER]\n")
#define HELP printf("Try 'grepL -h' for more information\n")

typedef struct Options
{
    char recursion;
    size_t occurences;
    char printlines;
    char entire_text;
    char change_text;

} Options;

Options *init_options()
{
    Options *op = malloc(sizeof(Options));
    op->recursion = 0;
    op->occurences = 0;
    op->printlines = 0;
    op->entire_text = 0;
    op->change_text = 0;

    return op;
}

void help()
{
    printf("Usage : grepL [OPTIONS]... PATTERN [FILE/FOLDER]\n");
    printf("\n Options\n");
    printf("   -h : display this help\n");
    printf("   -r : recursive search\n");
    printf("   -o : print the number of occurences of the regexp\n");
    printf("   -l : print line numbers\n");
    printf("   -e : print the entire text\n");
    printf("   -c : change the regexp with the new string\n");
    printf("        grepL -c PATTERN REPLACEMENT [FILE/FOLDER]\n");

}


void replace(char *path, reg_t regexp, char *text)
{
    FILE *f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *string = malloc(fsize + 1);
    fread(string, 1, fsize, f);
    fclose(f);
    string[fsize] = 0;

    char *new_text = regex_sub(regexp, string, text);
    FILE *file = fopen(path, "w");
    fputs(new_text, file);
    fclose(file);
    printf("done\n");
}

void search_lines(reg_t regexp, char *path, Options *options)
{
    FILE *f = fopen(path, "r");
    if (f == NULL)
    {
        printf("invalid file path\n");
        return;
    }

    ssize_t r = 0;
    size_t len = 0;
    char *line = NULL;
    int line_number = 0;

    while ((r = getline(&line, &len, f)) != -1)
    {
        match **match_list;
        size_t match_size = 0;
        size_t position = 0;

        if ((match_size = regex_search(regexp, line, &match_list)) != 0)
        {
            if (options->recursion)
                printf("\033[0;35m%s:"WHITE, path);
            if (options->printlines)
                printf("\033[0;32m%i:"WHITE, line_number);
            options->occurences += match_size;
            char *l = line;
            for (size_t i = 0; i < match_size; i++)
            {
                match *match = match_list[i];
                printf("%.*s", (int)(match->start - position), l);
                l = line + match->start;
                position = match->start;
                printf(RED "%.*s" WHITE, (int)(match->length), l);
                l += match->length;
                position += match->length;
                if (i + 1 >= match_size)
                {
                    printf("%s", l);
                }

            }
        }
        else if (options->entire_text)
            printf("%s", line);
        line_number++;
    }

    free(line);

    fclose(f);
}

void get_files(char *path, reg_t regexp, Options *options)
{
    DIR *d = opendir(path);
    if (d == NULL)
    {
        search_lines(regexp, path, options);
        return;
    }
    struct dirent *dir;
    while ((dir = readdir(d)) != NULL)
    {
        if (dir->d_type != DT_DIR)
        {
            char new_path[512];
            if (sprintf(new_path, "%s/%s", path, dir->d_name) == -1)
                return;
            search_lines(regexp, new_path, options);
        }
        if (options->recursion && dir->d_type == DT_DIR
            && strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0
            && dir->d_name[0] != '.')
        {
            char new_path[512];
            if (sprintf(new_path, "%s/%s", path, dir->d_name) == -1)
                return;
            get_files(new_path, regexp, options);
        }
    }
    closedir(d);
}

int main(int argc, char **argv)
{
    Options *options = init_options();
    int opt;

    if (argc <= 2 && !(argc == 2 && strcmp(argv[1], "-h") == 0))
    {
        printf("missing arguments\n");
        USAGE;
        HELP;
        return 0;
    }

    while ((opt = getopt(argc, argv, "hrolec:")) != -1)
    {
        switch (opt)
        {
            case 'h':
                help();
                return 0;
                break;
            case 'r':
                options->recursion = 1;
                break;
            case 'o':
                options->occurences = 1;
                break;
            case 'l':
                options->printlines = 1;
                break;
            case 'e':
                options->entire_text = 1;
                break;
            case 'c':
                options->change_text = optind-1;

                break;
            case '?':
                printf("unknown option %c\n", opt);
                HELP;
                break;
        }
    }

    if ((argc - optind) != 2)
    {
        printf("missing arguments\n");
        USAGE;
        HELP;

        if (options->change_text)
        {
            printf("After -c, indicate the new string:\n");
            printf("    grepL -c PATTERN REPLACEMENT [FILE/FOLDER]\n");
        }
        return 0;
    }

    reg_t regexp = regex_compile(argv[optind]);

    char occ = options->occurences;
    if (options->change_text)
    {
        replace(argv[optind+1], regexp, argv[options->change_text]);
        return 0;
    }

    get_files(argv[optind + 1], regexp, options);

    regex_free(regexp);

    options->occurences--;
    if (occ)
        printf("%li occurences\n", options->occurences);

    return 0;
}
