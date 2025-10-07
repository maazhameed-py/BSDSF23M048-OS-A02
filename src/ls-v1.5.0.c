/* src/ls-v1.6.0.c
 * ls clone with: -l, -x, colorized output (v1.5.0), and -R recursion (v1.6.0)
 *
 * Compile: make (provided Makefile)
 * Author: generated helper
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <limits.h>
#include <fcntl.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[0;34m"
#define COLOR_GREEN   "\033[0;32m"
#define COLOR_RED     "\033[0;31m"
#define COLOR_MAGENTA "\033[0;35m"
#define COLOR_REVERSE "\033[7m"

static int flag_long = 0;
static int flag_horizontal = 0;
static int flag_recursive = 0;

/* Helpers */
static int ends_with(const char *s, const char *suf) {
    if (!s || !suf) return 0;
    size_t ls = strlen(s), lsu = strlen(suf);
    if (ls < lsu) return 0;
    return strcmp(s + ls - lsu, suf) == 0;
}
static int is_archive_name(const char *name) {
    /* check common archive extensions (check longer ones first) */
    const char *exts[] = {".tar.gz", ".tar.bz2", ".tar.xz", ".tar", ".tgz", ".tar.Z", ".zip", ".gz", ".bz2", ".xz", NULL};
    for (int i = 0; exts[i]; ++i) if (ends_with(name, exts[i])) return 1;
    return 0;
}
static void mode_to_perm(mode_t m, char *out) {
    /* output must be at least 11 bytes */
    out[0] = S_ISDIR(m) ? 'd' :
             S_ISLNK(m) ? 'l' :
             S_ISCHR(m) ? 'c' :
             S_ISBLK(m) ? 'b' :
             S_ISFIFO(m) ? 'p' :
             S_ISSOCK(m) ? 's' : '-';
    /* user */
    out[1] = (m & S_IRUSR) ? 'r' : '-';
    out[2] = (m & S_IWUSR) ? 'w' : '-';
    out[3] = (m & S_IXUSR) ? ((m & S_ISUID) ? 's' : 'x') : ((m & S_ISUID) ? 'S' : '-');
    /* group */
    out[4] = (m & S_IRGRP) ? 'r' : '-';
    out[5] = (m & S_IWGRP) ? 'w' : '-';
    out[6] = (m & S_IXGRP) ? ((m & S_ISGID) ? 's' : 'x') : ((m & S_ISGID) ? 'S' : '-');
    /* others */
    out[7] = (m & S_IROTH) ? 'r' : '-';
    out[8] = (m & S_IWOTH) ? 'w' : '-';
    out[9] = (m & S_IXOTH) ? ((m & S_ISVTX) ? 't' : 'x') : ((m & S_ISVTX) ? 'T' : '-');
    out[10] = '\0';
}
static const char *choose_color(const char *name, const struct stat *st) {
    if (S_ISLNK(st->st_mode)) return COLOR_MAGENTA;
    if (S_ISDIR(st->st_mode)) return COLOR_BLUE;
    if (S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode) || S_ISFIFO(st->st_mode) || S_ISSOCK(st->st_mode))
        return COLOR_REVERSE;
    if (st->st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) return COLOR_GREEN;
    if (is_archive_name(name)) return COLOR_RED;
    return NULL;
}

/* Terminal width */
static int get_terminal_width(void) {
    struct winsize ws;
    if (ioctl(1, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) return (int)ws.ws_col;
    return 80;
}

/* Sorting comparator */
static int cmpstrptr(const void *a, const void *b) {
    const char *sa = *(const char **)a;
    const char *sb = *(const char **)b;
    return strcmp(sa, sb);
}

/* Read directory entries into a dynamic array of strings; skip hidden files by default */
static char **read_dir_names(const char *dirpath, int *count_out) {
    DIR *d = opendir(dirpath);
    if (!d) {
        fprintf(stderr, "opendir(%s): %s\n", dirpath, strerror(errno));
        *count_out = 0;
        return NULL;
    }
    size_t cap = 128;
    char **names = malloc(cap * sizeof(char *));
    size_t n = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        const char *nme = ent->d_name;
        if (nme[0] == '.') continue; /* skip hidden */
        if (n + 1 >= cap) { cap *= 2; names = realloc(names, cap * sizeof(char*)); }
        names[n++] = strdup(nme);
    }
    closedir(d);
    qsort(names, n, sizeof(char*), cmpstrptr);
    *count_out = (int)n;
    return names;
}

/* Print helpers: prints name with color and optional padding for column layout */
static void print_name_colored_padded(const char *name, const char *fullpath, int pad) {
    struct stat st;
    if (lstat(fullpath, &st) != 0) {
        /* fall back to plain name */
        printf("%s", name);
        for (int i = 0; i < pad; ++i) putchar(' ');
        return;
    }
    const char *col = choose_color(name, &st);
    if (col) printf("%s%s%s", col, name, COLOR_RESET);
    else printf("%s", name);
    int nlen = (int)strlen(name);
    for (int i = 0; i < pad - nlen; ++i) putchar(' ');
}

/* Long listing for one directory */
static void do_long_listing(const char *dirpath, char **names, int n) {
    /* compute widths */
    int w_links = 0, w_user = 0, w_group = 0, w_size = 0;
    struct stat stbuf;
    for (int i = 0; i < n; ++i) {
        char full[PATH_MAX];
        snprintf(full, sizeof(full), "%s/%s", dirpath, names[i]);
        if (lstat(full, &stbuf) != 0) continue;
        char tmp[64];
        int t = snprintf(tmp, sizeof(tmp), "%lu", (unsigned long)stbuf.st_nlink);
        if (t > w_links) w_links = t;
        struct passwd *pw = getpwuid(stbuf.st_uid);
        struct group *gr = getgrgid(stbuf.st_gid);
        int ul = pw ? (int)strlen(pw->pw_name) : snprintf(tmp, sizeof(tmp), "%u", stbuf.st_uid);
        if (ul > w_user) w_user = ul;
        int gl = gr ? (int)strlen(gr->gr_name) : snprintf(tmp, sizeof(tmp), "%u", stbuf.st_gid);
        if (gl > w_group) w_group = gl;
        t = snprintf(tmp, sizeof(tmp), "%llu", (unsigned long long)stbuf.st_size);
        if (t > w_size) w_size = t;
    }
    /* print each */
    for (int i = 0; i < n; ++i) {
        char full[PATH_MAX];
        snprintf(full, sizeof(full), "%s/%s", dirpath, names[i]);
        if (lstat(full, &stbuf) != 0) {
            printf("????????? ????? ??? ??? ??????? %s\n", names[i]);
            continue;
        }
        char perms[11];
        mode_to_perm(stbuf.st_mode, perms);
        char timebuf[64];
        struct tm *mt = localtime(&stbuf.st_mtime);
        strftime(timebuf, sizeof(timebuf), "%b %e %H:%M", mt);
        struct passwd *pw = getpwuid(stbuf.st_uid);
        struct group *gr = getgrgid(stbuf.st_gid);
        char userbuf[64], groupbuf[64];
        if (pw) strncpy(userbuf, pw->pw_name, sizeof(userbuf)); else snprintf(userbuf, sizeof(userbuf), "%u", stbuf.st_uid);
        if (gr) strncpy(groupbuf, gr->gr_name, sizeof(groupbuf)); else snprintf(groupbuf, sizeof(groupbuf), "%u", stbuf.st_gid);
        /* print header columns */
        printf("%s %*lu %-*s %-*s %*llu %s ", perms,
               w_links, (unsigned long)stbuf.st_nlink,
               w_user, userbuf,
               w_group, groupbuf,
               w_size, (unsigned long long)stbuf.st_size,
               timebuf);
        /* name (with color). if symlink show -> target */
        const char *col = choose_color(names[i], &stbuf);
        if (col) printf("%s%s%s", col, names[i], COLOR_RESET);
        else printf("%s", names[i]);
        if (S_ISLNK(stbuf.st_mode)) {
            char linkt[PATH_MAX+1];
            ssize_t r = readlink(full, linkt, sizeof(linkt)-1);
            if (r >= 0) {
                linkt[r] = '\0';
                printf(" -> %s", linkt);
            }
        }
        printf("\n");
    }
}

/* Column (down then across) */
static void do_columns(const char *dirpath, char **names, int n) {
    if (n == 0) return;
    int maxlen = 0;
    for (int i = 0; i < n; ++i) if ((int)strlen(names[i]) > maxlen) maxlen = (int)strlen(names[i]);
    int width = get_terminal_width();
    int colw = maxlen + 2;
    int columns = width / colw;
    if (columns < 1) columns = 1;
    int rows = (n + columns - 1) / columns;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < columns; ++c) {
            int idx = r + c * rows;
            if (idx >= n) continue;
            char full[PATH_MAX];
            snprintf(full, sizeof(full), "%s/%s", dirpath, names[idx]);
            print_name_colored_padded(names[idx], full, colw);
        }
        putchar('\n');
    }
}

/* Horizontal (-x) printing (left-to-right) */
static void do_horizontal(const char *dirpath, char **names, int n) {
    if (n == 0) return;
    int maxlen = 0;
    for (int i = 0; i < n; ++i) if ((int)strlen(names[i]) > maxlen) maxlen = (int)strlen(names[i]);
    int colw = maxlen + 2;
    int width = get_terminal_width();
    int cur = 0;
    for (int i = 0; i < n; ++i) {
        if (cur + colw > width) { putchar('\n'); cur = 0; }
        char full[PATH_MAX];
        snprintf(full, sizeof(full), "%s/%s", dirpath, names[i]);
        print_name_colored_padded(names[i], full, colw);
        cur += colw;
    }
    if (cur != 0) putchar('\n');
}

/* Main directory listing function (recursive-aware) */
static void do_ls(const char *dirpath) {
    int n = 0;
    char **names = read_dir_names(dirpath, &n);
    if (!names) return;
    if (flag_recursive) printf("%s:\n", dirpath);
    if (flag_long) {
        do_long_listing(dirpath, names, n);
    } else if (flag_horizontal) {
        do_horizontal(dirpath, names, n);
    } else {
        do_columns(dirpath, names, n);
    }
    /* If recursive, descend into subdirectories */
    if (flag_recursive) {
        for (int i = 0; i < n; ++i) {
            char full[PATH_MAX];
            snprintf(full, sizeof(full), "%s/%s", dirpath, names[i]);
            struct stat st;
            if (lstat(full, &st) != 0) continue;
            if (S_ISDIR(st.st_mode)) {
                /* skip . and .. */
                if (strcmp(names[i], ".") == 0 || strcmp(names[i], "..") == 0) continue;
                putchar('\n');
                do_ls(full);
            }
        }
    }
    for (int i = 0; i < n; ++i) free(names[i]);
    free(names);
}

int main(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "lxR")) != -1) {
        switch (opt) {
            case 'l': flag_long = 1; break;
            case 'x': flag_horizontal = 1; break;
            case 'R': flag_recursive = 1; break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-x] [-R] [dir...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    int dirs = argc - optind;
    if (dirs == 0) {
        do_ls("."); /* default */
    } else if (dirs == 1) {
        do_ls(argv[optind]);
    } else {
        /* multiple dirs: print headers */
        for (int i = optind; i < argc; ++i) {
            printf("%s:\n", argv[i]);
            do_ls(argv[i]);
            if (i < argc - 1) putchar('\n');
        }
    }
    return 0;
}

