# Feature-2 Report Questions

### 1. Crucial difference between `stat()` and `lstat()`

- **`stat()`**: Returns metadata of the **target file**.  
  - If the file is a symbolic link, `stat()` gives information about the file it points to.  
- **`lstat()`**: Returns metadata of the **link itself**, not the target.  

**In the context of `ls`:**  
- Use **`lstat()`** when you want to display information about **symbolic links themselves**, rather than the file they reference.

---

### 2. Extracting file type and permissions from `st_mode`

- `st_mode` in `struct stat` contains **both file type** and **permission bits**.  
- Use **bitwise AND (`&`)** with predefined macros to extract information:

```c
struct stat st;

// File type
if (st.st_mode & S_IFDIR) { /* directory */ }
if (st.st_mode & S_IFREG) { /* regular file */ }
if (st.st_mode & S_IFLNK) { /* symbolic link */ }

// Permissions
if (st.st_mode & S_IRUSR) { /* owner read */ }
if (st.st_mode & S_IWUSR) { /* owner write */ }
if (st.st_mode & S_IXUSR) { /* owner execute */ }
```


# Feature 3: ls-v1.2.0 – Column Display (Down Then Across)

## 1. General Logic for Printing Items in "Down Then Across" Format

- **Goal:** Display files in multiple columns that adapt to terminal width.  
- **Steps:**
  1. **Read all filenames** into a dynamically allocated array.  
  2. **Determine the longest filename** for column width calculation.  
  3. **Get terminal width** using `ioctl`.  
  4. **Calculate number of columns:**  
     ```
     cols = terminal_width / (max_filename_length + spacing)
     ```
  5. **Calculate number of rows:**  
     ```
     rows = ceil(total_files / cols)
     ```
  6. **Print files row by row (down then across):**  
     - For each row `r`, print:
       ```
       filenames[r + 0*rows], filenames[r + 1*rows], filenames[r + 2*rows], ...
       ```
     - This fills columns vertically first, then moves across.

- **Why a simple loop is insufficient:**  
  - Iterating linearly would print files **left-to-right across rows**, which is **not how standard `ls` displays files**.  
  - Down-then-across ensures **even column filling and proper alignment**.

---

## 2. Purpose of `ioctl` System Call

- `ioctl(STDOUT_FILENO, TIOCGWINSZ, &w)` retrieves **terminal window size**:
  - `w.ws_col` → number of columns (width)  
  - `w.ws_row` → number of rows (height)

- **Importance:**  
  - Dynamically adjusts the number of columns based on **current terminal width**.  
  - Without `ioctl`, a fixed width (e.g., 80 columns) would:
    - Break alignment if terminal is wider or narrower.  
    - Possibly cause unnecessary wrapping or wasted space.

---

# Feature 4: ls-v1.3.0 –  Horizontal Column Display (Across)

### Q1: Compare the implementation complexity of "down then across" vs "across" printing logic.
- **Down then across (vertical):** Requires calculating rows and mapping filenames to column-row positions → more pre-calculation.  
- **Across (horizontal):** Prints sequentially left-to-right, wraps on terminal width → simpler, less calculation.

### Q2: Describe the strategy used to manage different display modes (-l, -x, default).
- Used an **integer flag** to track display mode.  
- After reading filenames:
  - `-l` → call **long listing** function  
  - `-x` → call **horizontal** function  
  - default → call **vertical column** function
 
# Feature-5: ls-v1.4.0 – Alphabetical Sort

## Why read all directory entries before sorting?

Sorting requires comparing all filenames, so the program must first **load every directory entry into memory** (e.g., into an array).  
Only after all entries are known can a sort function like `qsort()` arrange them.

### Drawbacks
- **High memory use:** Millions of files consume large RAM.  
- **Slow performance:** Sorting huge lists takes time.  
- **Scalability issues:** May cause swapping or crashes with very large directories.

---

## Purpose and Signature of `qsort()` Comparison Function

**Prototype:**
```c
void qsort(void *base, size_t nitems, size_t size,
           int (*compar)(const void *, const void *));

```

# Feature-6: ls-v1.5.0 – Colorized Output Based on File Type

## ANSI escape codes for color (Linux terminals)

- Format: ESC [ params m  (SGR: Select Graphic Rendition)
- ESC is byte 0x1B, written as `\x1b` or `\033`
- Reset with `0` → `\x1b[0m`

Green text sequences:
- Standard green: `\x1b[32m`
- Bright green: `\x1b[92m`
- Reset: `\x1b[0m`

Examples:
```sh
# Bash
echo -e "\033[32mThis is green\033[0m"
echo -e "\033[92mThis is bright green\033[0m"
echo -e "\033[1;32mThis is bold/bright green\033[0m"
```

```c
// C
printf("\x1b[32mThis is green\x1b[0m\n");
printf("\x1b[92mThis is bright green\x1b[0m\n");
printf("\x1b[1;32mThis is bold/bright green\x1b[0m\n");
```

---

## Executable permission bits in st_mode

Include:
```c
#include <sys/stat.h>
```

Bits to check:
- Owner executable: `S_IXUSR` (0100)
- Group executable: `S_IXGRP` (0010)
- Others executable: `S_IXOTH` (0001)

Check if a regular file is executable by anyone:
```c
struct stat st;
if (stat(path, &st) == 0) {
    int is_regular = S_ISREG(st.st_mode);
    int is_exec_any = (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) != 0;

    if (is_regular && is_exec_any) {
        printf("\x1b[32m%s\x1b[0m\n", name);  // color executables green
    } else {
        printf("%s\n", name);
    }
}
```

Check individually:
```c
int owner_exec  = (st.st_mode & S_IXUSR) != 0;
int group_exec  = (st.st_mode & S_IXGRP) != 0;
int others_exec = (st.st_mode & S_IXOTH) != 0;
```



# Feature 7: Recursive Listing (v1.6.0)

## 1. Base Case in a Recursive Function
A **base case** is the condition that stops recursion from continuing indefinitely. It prevents infinite loops by providing a terminating condition.  

**In the recursive `ls` implementation:**  
- The base case occurs when a directory contains **no subdirectories** or when all entries are `.` or `..`.  
- At this point, the recursion stops, and the function returns without making further recursive calls.

## 2. Importance of Constructing Full Paths
Before making a recursive call, it is essential to construct the **full path** of a directory (`parent_dir/subdir`) instead of just the subdirectory name.  

**Reason:**  
- Without the full path, the recursive call would look for the subdirectory in the **current working directory**, not in the nested location.  
- This would cause errors or incorrect listings if the program is traversing multiple levels of directories.  

**Example:**  
```c
char path[1024];
snprintf(path, sizeof(path), "%s/%s", parent_dir, entry_name);
do_ls(path, mode, recursive_flag);
**







