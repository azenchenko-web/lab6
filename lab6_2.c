#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define LOG_FILE "sort_report.log"

typedef struct {
    const char *ext;
    const char *category;
} ExtMap;

static const ExtMap ext_map[] = {
    {".txt", "Documents"}, {".md", "Documents"},
    {".c", "Sources"}, {".h", "Sources"}, {".go", "Sources"},
    {".jpg", "Images"}, {".png", "Images"},
    {".pdf", "PDFs"}
};

static FILE *logf = NULL;

// Виводить повідомлення в консоль і записує його в лог
static void log_msg(const char *fmt, ...) {
    va_list args, copy;
    va_start(args, fmt);
    va_copy(copy, args);
    vprintf(fmt, args);
    printf("\n");
    if (logf != NULL) {
        vfprintf(logf, fmt, copy);
        fprintf(logf, "\n");
    }
    va_end(copy);
    va_end(args);
}

static const char *get_category(const char *name) {
    const char *ext = strrchr(name, '.');
    if (ext == NULL || ext == name) {
        return "Other";
    }
    for (size_t i = 0; i < sizeof(ext_map) / sizeof(ext_map[0]); i++) {
        if (strcasecmp(ext, ext_map[i].ext) == 0) {
            return ext_map[i].category;
        }
    }
    return "Other";
}

static int ensure_dir(const char *path) {
    if (mkdir(path, 0755) == 0) {
        log_msg("Створено папку: %s", path);
        return 0;
    }
    if (errno == EEXIST) {
        return 0;
    }
    log_msg("Помилка створення папки %s: %s", path, strerror(errno));
    return -1;
}

static void move_file(const char *src_path, const char *name, const char *dest) {
    const char *category = get_category(name);
    char cat_dir[PATH_MAX];
    char dst_path[PATH_MAX];
    snprintf(cat_dir, sizeof(cat_dir), "%s/%s", dest, category);
    snprintf(dst_path, sizeof(dst_path), "%s/%s", cat_dir, name);

    if (ensure_dir(cat_dir) == -1) {
        return;
    }
    if (access(dst_path, F_OK) == 0) {
        log_msg("Помилка: %s вже існує, файл %s пропущено", dst_path, src_path);
        return;
    }
    if (rename(src_path, dst_path) == -1) {
        log_msg("Помилка переміщення %s -> %s: %s", src_path, dst_path, strerror(errno));
        return;
    }
    log_msg("Переміщено: %s -> %s", src_path, dst_path);
}

static void sort_dir(const char *path, const char *dest) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        log_msg("Помилка відкриття директорії %s: %s", path, strerror(errno));
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == -1) {
            log_msg("Помилка stat %s: %s", full_path, strerror(errno));
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            sort_dir(full_path, dest);
        } else if (S_ISREG(st.st_mode)) {
            move_file(full_path, entry->d_name, dest);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    const char *source = NULL;
    const char *dest = NULL;
    int opt;

    static struct option long_options[] = {
        {"source", required_argument, NULL, 's'},
        {"dest",   required_argument, NULL, 'd'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "", long_options, NULL)) != -1) {
        switch (opt) {
            case 's':
                source = optarg;
                break;
            case 'd':
                dest = optarg;
                break;
            default:
                fprintf(stderr, "Використання: %s --source=<директорія> --dest=<директорія>\n", argv[0]);
                return 1;
        }
    }

    if (source == NULL || dest == NULL) {
        fprintf(stderr, "Використання: %s --source=<директорія> --dest=<директорія>\n", argv[0]);
        return 1;
    }

    struct stat st;
    if (stat(source, &st) == -1 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Помилка: %s не є директорією\n", source);
        return 1;
    }

    logf = fopen(LOG_FILE, "w");
    if (logf == NULL) {
        perror(LOG_FILE);
        return 1;
    }

    log_msg("--- Програма сортування запущена ---");
    if (ensure_dir(dest) == 0) {
        sort_dir(source, dest);
    }
    log_msg("Сортування завершено! Див. '%s'.", LOG_FILE);

    fclose(logf);
    return 0;
}