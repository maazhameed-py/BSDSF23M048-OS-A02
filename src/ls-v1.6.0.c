/*
 * Feature 7: Recursive Listing (v1.6.0)
 * Retains previous features: -l (long), -x (horizontal), default (columns), colorized output
 * Adds recursive listing (-R)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>

enum DisplayMode { DEFAULT, LONG, HORIZONTAL };

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[0;34m"  // Directory
#define COLOR_GREEN   "\033[0;32m"  // Executable
#define COLOR_RED     "\033[0;31m"  // Archive
#define COLOR_MAGENTA "\033[0;35m"  // Symlink

// -------------------- Utility Functions --------------------
int ends_with(const char *s, const char *suf) {
    size_t ls = strlen(s), lsu = strlen(suf);
    if (ls < lsu) return 0;
    return strcmp(s + ls - lsu, suf) == 0;
}

int is_archive(const char *name) {
    const char *exts[] = {".zip", ".tar", ".gz", ".bz2", ".xz", ".tgz", NULL};
    for (int i = 0; exts[i]; ++i)
        if (ends_with(name, exts[i])) return 1;
    return 0;
}

const char *get_color(const char *name, const struct stat *st) {
    if (S_ISDIR(st->st_mode)) return COLOR_BLUE;
    if (S_ISLNK(st->st_mode)) return COLOR_MAGENTA;
    if (is_archive(name)) return COLOR_RED;
    if (st->st_mode & S_IXUSR) return COLOR_GREEN;
    return COLOR_RESET;
}

void print_permissions(mode_t mode) {
    char perms[11] = "----------";

    if (S_ISDIR(mode)) perms[0] = 'd';
    else if (S_ISLNK(mode)) perms[0] = 'l';
    else if (S_ISCHR(mode)) perms[0] = 'c';
    else if (S_ISBLK(mode)) perms[0] = 'b';
    else if (S_ISFIFO(mode)) perms[0] = 'p';
    else if (S_ISSOCK(mode)) perms[0] = 's';

    if (mode & S_IRUSR) perms[1] = 'r';
    if (mode & S_IWUSR) perms[2] = 'w';
    if (mode & S_IXUSR) perms[3] = 'x';
    if (mode & S_IRGRP) perms[4] = 'r';
    if (mode & S_IWGRP) perms[5] = 'w';
    if (mode & S_IXGRP) perms[6] = 'x';
    if (mode & S_IROTH) perms[7] = 'r';
    if (mode & S_IWOTH) perms[8] = 'w';
    if (mode & S_IXOTH) perms[9] = 'x';

    printf("%s ", perms);
}

// -------------------- Sorting Function --------------------
int compare_names(const void *a, const void *b) {
    const char *nameA = *(const char **)a;
    const char *nameB = *(const char **)b;
    return strcmp(nameA, nameB);
}

// -------------------- Display Functions --------------------
void list_long(const char *dirname, char **filenames, int file_count) {
    for (int i = 0; i < file_count; i++) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dirname, filenames[i]);
        struct stat st;
        if (lstat(path, &st) == -1) { perror("lstat"); continue; }

        print_permissions(st.st_mode);
        printf("%2lu ", st.st_nlink);

        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);
        printf("%s %s ", pw ? pw->pw_name : "unknown", gr ? gr->gr_name : "unknown");

        printf("%6ld ", st.st_size);

        char time_buf[20];
        struct tm *tm_info = localtime(&st.st_mtime);
        strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", tm_info);
        printf("%s ", time_buf);

        const char *color = get_color(filenames[i], &st);
        printf("%s%s%s\n", color, filenames[i], COLOR_RESET);
    }
}

void list_columns(const char *dirname, char **filenames, int file_count) {
    int max_len = 0;
    for (int i = 0; i < file_count; i++) {
        int len = strlen(filenames[i]);
        if (len > max_len) max_len = len;
    }

    struct winsize w;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != -1)
        term_width = w.ws_col;

    int spacing = 2;
    int cols = term_width / (max_len + spacing);
    if (cols < 1) cols = 1;
    int rows = (file_count + cols - 1) / cols;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = r + c * rows;
            if (idx < file_count) {
                char path[1024];
                snprintf(path, sizeof(path), "%s/%s", dirname, filenames[idx]);
                struct stat st;
                if (lstat(path, &st) == 0) {
                    const char *color = get_color(filenames[idx], &st);
                    printf("%s%-*s%s", color, max_len + spacing, filenames[idx], COLOR_RESET);
                }
            }
        }
        printf("\n");
    }
}

void list_horizontal(const char *dirname, char **filenames, int file_count) {
    int max_len = 0;
    for (int i = 0; i < file_count; i++) {
        int len = strlen(filenames[i]);
        if (len > max_len) max_len = len;
    }

    struct winsize w;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != -1)
        term_width = w.ws_col;

    int spacing = 2;
    int col_width = max_len + spacing;
    int pos = 0;

    for (int i = 0; i < file_count; i++) {
        char path[1024];
        snprintf(path, sizeof(path), "%s/%s", dirname, filenames[i]);
        struct stat st;
        if (lstat(path, &st) == 0) {
            const char *color = get_color(filenames[i], &st);
            if (pos + col_width > term_width) {
                printf("\n");
                pos = 0;
            }
            printf("%s%-*s%s", color, col_width, filenames[i], COLOR_RESET);
            pos += col_width;
        }
    }
    printf("\n");
}

// -------------------- Core Function (Recursive) --------------------
void do_ls(const char *dirname, enum DisplayMode mode, int recursive_flag) {
    DIR *dir = opendir(dirname);
    if (!dir) { perror(dirname); return; }

    struct dirent *entry;
    char **filenames = NULL;
    int file_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        filenames = realloc(filenames, sizeof(char*) * (file_count + 1));
        filenames[file_count++] = strdup(entry->d_name);
    }
    closedir(dir);

    qsort(filenames, file_count, sizeof(char*), compare_names);

    // Display according to mode
    switch (mode) {
        case LONG:       list_long(dirname, filenames, file_count); break;
        case HORIZONTAL: list_horizontal(dirname, filenames, file_count); break;
        default:         list_columns(dirname, filenames, file_count);
    }

    // Recursive descent
    if (recursive_flag) {
        for (int i = 0; i < file_count; i++) {
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dirname, filenames[i]);

            struct stat st;
            if (lstat(path, &st) == -1) continue;

            if (S_ISDIR(st.st_mode)) {
                // skip "." and ".."
                if (strcmp(filenames[i], ".") == 0 || strcmp(filenames[i], "..") == 0)
                    continue;

                printf("\n%s:\n", path);
                do_ls(path, mode, recursive_flag);
            }
        }
    }

    for (int i = 0; i < file_count; i++) free(filenames[i]);
    free(filenames);
}

// -------------------- Main Function --------------------
int main(int argc, char *argv[]) {
    int opt;
    enum DisplayMode mode = DEFAULT;
    int recursive_flag = 0;

    while ((opt = getopt(argc, argv, "lxR")) != -1) {
        switch (opt) {
            case 'l': mode = LONG; break;
            case 'x': mode = HORIZONTAL; break;
            case 'R': recursive_flag = 1; break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-x] [-R] [directory]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind == argc) {
        printf(".:\n");
        do_ls(".", mode, recursive_flag);
    } else {
        for (int i = optind; i < argc; i++) {
            printf("%s:\n", argv[i]);
            do_ls(argv[i], mode, recursive_flag);
            if (i < argc - 1) printf("\n");
        }
    }
    return 0;
}
