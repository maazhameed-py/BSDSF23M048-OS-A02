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




