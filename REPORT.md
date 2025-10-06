## Feature-2 Report Questions

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
