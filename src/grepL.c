#define _XOPEN_SOURCE 700
#define _GNU_SOURCE 1
#include <dirent.h>
#include <err.h>
#include <fcntl.h>
#include <ftw.h>
#include <pthread.h>
#include <rationl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
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
    char linenumber;
    char entire_text;
    char change_text;
    char string;
    char fast;

} Options;

static reg_t regex;

char buffer[1000] = {0};

static Options option = {
.recursion = 0,
.occurences = 0,
.linenumber = 0,
.entire_text = 0,
.change_text = 0,
.string = 0,
.fast = 0,
};

static pthread_mutex_t STDOUT_MUTEX = PTHREAD_MUTEX_INITIALIZER;

static int finished = 0;


void help()
{
    printf("Usage : grepL [OPTIONS]... PATTERN [FILE/FOLDER]\n");
    printf("\n Options\n");
    printf("   -h : display this help\n");
    printf("   -r : recursive search\n");
    printf("   -o : print the number of occurences of the regexp\n");
    printf("   -n : print line numbers\n");
    printf("   -e : print the entire text\n");
    printf("   -c : change the regexp with the new string\n");
    printf("        grepL -c PATTERN REPLACEMENT [FILE/FOLDER]\n");
    printf("   -s : a string is given, not a regexp\n");
    printf("   -f : fast. printf every occurence of the regexp. Disables the other options, except -s\n");
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

void search_lines(reg_t regexp, char *path)
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
            if (option.recursion)
                printf("\033[0;35m%s:"WHITE, path);
            if (option.linenumber)
                printf("\033[0;32m%i:"WHITE, line_number);
            option.occurences += match_size;
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
        else if (option.entire_text)
        {
            printf("\033[0;35m%s:"WHITE, path);
            if (option.linenumber)
                printf("\033[0;32m%i:"WHITE, line_number);
            printf("%s", line);
        }
        line_number++;
    }

    free(line);

    fclose(f);
}

void get_files(char *path, reg_t regexp)
{
    DIR *d = opendir(path);
    if (d == NULL)
    {
        search_lines(regexp, path);
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
            search_lines(regexp, new_path);
        }
        if (option.recursion && dir->d_type == DT_DIR
            && strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0
            && dir->d_name[0] != '.')
        {
            char new_path[512];
            if (sprintf(new_path, "%s/%s", path, dir->d_name) == -1)
                return;
            get_files(new_path, regexp);
        }
    }
    closedir(d);
}



struct file_data
{
    struct stat *sb;
    char *path;
    reg_t regexp;
};

void *read_file(void *data)
{
    struct file_data *d = data;
    char *lhs;
    char *rhs;
    char *cp;
    int len;
    int linecnt;
    int linemax;
    FILE *f = fopen(d->path, "r");
    if (f == NULL)
        return NULL;
    char *fbuf = malloc(sizeof(char) * (d->sb->st_size + 1));
    fread(fbuf, d->sb->st_size, 1, f);
    fclose(f);
    match **match_list;
    size_t match_size = 0;
    size_t position = 0;
    if ((match_size = regex_search(d->regexp, fbuf, &match_list)) != 0)
    {
        option.occurences += match_size;
        for (size_t i = 0; i < match_size; i++)
        {
            if (option.recursion)
            {
                printf("%s:", d->path);
            }
            match *match = match_list[i];
            printf(RED "%.*s\n" WHITE, (int)(match->length),
                   fbuf + match->start);
        }
    }
    else if (option.entire_text)
        printf("%s\n", fbuf);
    free(fbuf);
    free(data);
    return NULL;
}

int process_file(const char *filepath, const struct stat *info,
                 const int typeflag, struct FTW *pathinfo)
{
    if (typeflag == FTW_F && info->st_size != 0)
    {
        struct file_data *arg = malloc(sizeof(struct file_data));
        char *copy = malloc(1 + strlen(filepath));
        struct stat *info_cpy = malloc(sizeof(struct stat));
        memcpy(info_cpy, info, sizeof(struct stat));
        arg->sb = info_cpy;
        strcpy(copy, filepath);
        arg->path = copy;
        arg->regexp = regex;
        pthread_t thr;
        pthread_create(&thr, NULL, read_file, arg);
        pthread_detach(thr);
    }
    return 0;
}

void recurse_directories(const char *const dirpath)
{
    nftw(dirpath, process_file, 200, FTW_PHYS);
    finished = 1;
}

int main(int argc, char **argv)
{
    int opt;

    if (argc <= 2 && !(argc == 2 && strcmp(argv[1], "-h") == 0))
    {
        printf("missing arguments\n");
        USAGE;
        HELP;
        return 0;
    }

    while ((opt = getopt(argc, argv, "hronec:sf")) != -1)
    {
        switch (opt)
        {
            case 'h':
                help();
                return 0;
                break;
            case 'r':
                option.recursion = 1;
                break;
            case 'o':
                option.occurences = 1;
                break;
            case 'n':
                option.linenumber = 1;
                break;
            case 'e':
                option.entire_text = 1;
                break;
            case 'c':
                option.change_text = optind - 1;
                break;
            case 's':
                option.string = 1;
                break;
            case 'f':
                option.fast = 1;
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

        if (option.change_text)
        {
            printf("After -c, indicate the new string:\n");
            printf("    grepL -c PATTERN REPLACEMENT [FILE/FOLDER]\n");
        }
        return 0;
    }

    if (option.string)
        regex = regexp_compile_string(argv[optind]);
    else
        regex = regex_compile(argv[optind]);

    char occ = option.occurences;

    if (option.change_text)
    {
        replace(argv[optind + 1], regex, argv[option.change_text]);
        return 0;
    }

    if (option.fast)
    {
        recurse_directories(argv[optind + 1]);
    }
    else
        get_files(argv[optind+1], regex);

    option.occurences--;
    if (occ)
        printf("%li occurences\n", option.occurences);
    pthread_exit(0);
}
