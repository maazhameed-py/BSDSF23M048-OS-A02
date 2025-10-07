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

// -------------------- Utility Functions --------------------
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
void list_long(const char *dirname) {
    DIR *dir = opendir(dirname);
    if (!dir) { perror("opendir"); return; }

    struct dirent *entry;
    char **filenames = NULL;
    int file_count = 0;

    // Read filenames
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        filenames = realloc(filenames, sizeof(char*) * (file_count + 1));
        filenames[file_count] = strdup(entry->d_name);
        file_count++;
    }
    closedir(dir);

    // Sort alphabetically
    qsort(filenames, file_count, sizeof(char*), compare_names);

    // Display info for each file
    for (int i = 0; i < file_count; i++) {
        char path[1024];
        struct stat st;
        snprintf(path, sizeof(path), "%s/%s", dirname, filenames[i]);

        if (lstat(path, &st) == -1) { perror("lstat"); continue; }

        print_permissions(st.st_mode);
        printf("%2lu ", st.st_nlink);

        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);
        printf("%s %s ", pw ? pw->pw_name : "unknown", gr ? gr->gr_name : "unknown");

        printf("%5ld ", st.st_size);

        char time_buf[20];
        struct tm *tm_info = localtime(&st.st_mtime);
        strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", tm_info);
        printf("%s ", time_buf);

        printf("%s\n", filenames[i]);
        free(filenames[i]);
    }

    free(filenames);
}

void list_columns(const char *dirname) {
    DIR *dir = opendir(dirname);
    if (!dir) { perror("opendir"); return; }

    struct dirent *entry;
    char **filenames = NULL;
    int file_count = 0, max_len = 0;

    // Read filenames
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        filenames = realloc(filenames, sizeof(char*) * (file_count + 1));
        filenames[file_count] = strdup(entry->d_name);
        int len = strlen(entry->d_name);
        if (len > max_len) max_len = len;
        file_count++;
    }
    closedir(dir);

    if (file_count == 0) { printf("No files found.\n"); return; }

    // Sort alphabetically
    qsort(filenames, file_count, sizeof(char*), compare_names);

    // Terminal width detection
    struct winsize w;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != -1)
        term_width = w.ws_col;

    int spacing = 2;
    int cols = term_width / (max_len + spacing);
    if (cols < 1) cols = 1;
    int rows = (file_count + cols - 1) / cols;

    // Print across then down (human-expected order)
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = c + r * cols;  // <-- FIXED LINE
            if (idx < file_count)
                printf("%-*s", max_len + spacing, filenames[idx]);
        }
        printf("\n");
    }

    for (int i = 0; i < file_count; i++) free(filenames[i]);
    free(filenames);
}

void list_horizontal(const char *dirname) {
    DIR *dir = opendir(dirname);
    if (!dir) { perror("opendir"); return; }

    struct dirent *entry;
    char **filenames = NULL;
    int file_count = 0, max_len = 0;

    // Read filenames
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        filenames = realloc(filenames, sizeof(char*) * (file_count + 1));
        filenames[file_count] = strdup(entry->d_name);
        int len = strlen(entry->d_name);
        if (len > max_len) max_len = len;
        file_count++;
    }
    closedir(dir);

    if (file_count == 0) { printf("No files found.\n"); return; }

    // Sort alphabetically
    qsort(filenames, file_count, sizeof(char*), compare_names);

    // Terminal width detection
    struct winsize w;
    int term_width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != -1)
        term_width = w.ws_col;

    int spacing = 2;
    int col_width = max_len + spacing;
    int pos = 0;

    // Print left to right horizontally
    for (int i = 0; i < file_count; i++) {
        if (pos + col_width > term_width) {
            printf("\n");
            pos = 0;
        }
        printf("%-*s", col_width, filenames[i]);
        pos += col_width;
    }
    printf("\n");

    for (int i = 0; i < file_count; i++) free(filenames[i]);
    free(filenames);
}

// -------------------- Main Function --------------------
void do_ls(const char *dirname, enum DisplayMode mode) {
    switch(mode) {
        case LONG: list_long(dirname); break;
        case HORIZONTAL: list_horizontal(dirname); break;
        default: list_columns(dirname);
    }
}

int main(int argc, char *argv[]) {
    int opt;
    enum DisplayMode mode = DEFAULT;

    while ((opt = getopt(argc, argv, "lx")) != -1) {
        switch(opt) {
            case 'l': mode = LONG; break;
            case 'x': mode = HORIZONTAL; break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-x] [directory]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind == argc) {
        do_ls(".", mode);
    } else {
        for (int i = optind; i < argc; i++) {
            printf("Directory listing of %s:\n", argv[i]);
            do_ls(argv[i], mode);
            printf("\n");
        }
    }

    return 0;
}

