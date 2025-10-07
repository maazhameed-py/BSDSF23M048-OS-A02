#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main() {
    DIR *dir = opendir(".");
    if (!dir) {
        perror("opendir");
        return EXIT_FAILURE;
    }

    struct dirent *entry;
    char **filenames = NULL;
    int file_count = 0;
    int max_len = 0;

    // Step 1: Read all filenames and find the longest
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        filenames = realloc(filenames, sizeof(char*) * (file_count + 1));
        filenames[file_count] = strdup(entry->d_name);

        int len = strlen(entry->d_name);
        if (len > max_len) max_len = len;

        file_count++;
    }
    closedir(dir);

    if (file_count == 0) {
        printf("No files found.\n");
        return 0;
    }

    // Step 2: Get terminal width
    struct winsize w;
    int term_width = 80; // fallback
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) != -1) {
        term_width = w.ws_col;
    }

    // Step 3: Calculate columns and rows
    int spacing = 2; // spaces between columns
    int cols = term_width / (max_len + spacing);
    if (cols < 1) cols = 1;
    int rows = (file_count + cols - 1) / cols; // ceiling division

    // Step 4: Print filenames "down then across"
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = r + c * rows;
            if (idx < file_count) {
                printf("%-*s", max_len + spacing, filenames[idx]);
            }
        }
        printf("\n");
    }

    // Step 5: Free memory
    for (int i = 0; i < file_count; i++) free(filenames[i]);
    free(filenames);

    return 0;
}

