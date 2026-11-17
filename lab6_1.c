#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#define ROOT "lab6_12"

static int dir_count = 0;
static int file_count = 0;

static void make_dir(const char *path) {
    if (mkdir(path, 0755) == -1 && errno != EEXIST) {
        perror(path);
    }
}

static void make_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        perror(path);
        return;
    }
    fputs(content, f);
    fclose(f);
}

static void create_structure(void) {
    make_dir(ROOT);
    make_dir(ROOT "/.git");
    make_file(ROOT "/.git/config", "[core]\n\trepositoryformatversion = 0\n");
    make_dir(ROOT "/src");
    make_file(ROOT "/src/main.c",
              "#include <stdio.h>\n\nint main(void) {\n    printf(\"Hello\\n\");\n    return 0;\n}\n");
    make_dir(ROOT "/bin");
    make_dir(ROOT "/docs");
    make_file(ROOT "/docs/manual.txt", "Manual\n");
    make_file(ROOT "/docs/presentation.pdf", "%PDF-1.4 placeholder\n");
    make_dir(ROOT "/docs/img");
    make_file(ROOT "/docs/img/cat.jpg", "cat placeholder\n");
    make_file(ROOT "/docs/img/dog.png", "dog placeholder\n");
    make_file(ROOT "/README.md", "# lab6_12\n");
}

static int cmp_names(const void *a, const void *b) {
    return strcmp(*(char *const *)a, *(char *const *)b);
}

static void print_tree(const char *path, const char *prefix) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror(path);
        return;
    }

    char **names = NULL;
    size_t count = 0, capacity = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue; // приховані елементи, а також "." та ".."
        }
        if (count == capacity) {
            capacity = capacity ? capacity * 2 : 8;
            char **tmp = realloc(names, capacity * sizeof(char *));
            if (tmp == NULL) {
                perror("realloc");
                break;
            }
            names = tmp;
        }
        names[count] = strdup(entry->d_name);
        if (names[count] == NULL) {
            perror("strdup");
            break;
        }
        count++;
    }
    closedir(dir);

    if (count > 0) {
        qsort(names, count, sizeof(char *), cmp_names);
    }

    for (size_t i = 0; i < count; i++) {
        int last = (i == count - 1);
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, names[i]);

        struct stat st;
        if (stat(full_path, &st) == -1) {
            perror(full_path);
            free(names[i]);
            continue;
        }

        printf("%s%s%s\n", prefix, last ? "└── " : "├── ", names[i]);

        if (S_ISDIR(st.st_mode)) {
            dir_count++;
            char new_prefix[1024];
            snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, last ? "    " : "│   ");
            print_tree(full_path, new_prefix);
        } else {
            file_count++;
        }
        free(names[i]);
    }
    free(names);
}

int main(int argc, char *argv[]) {
    const char *start = (argc > 1) ? argv[1] : ROOT;

    if (argc == 1) {
        create_structure();
    }

    printf("%s\n", start);
    print_tree(start, "");
    printf("\n%d directories, %d files\n", dir_count, file_count);
    return 0;
}