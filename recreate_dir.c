#include "recreate_dir.h"

#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ftw.h>
#include <errno.h>

static int remove_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    (void)sb;
    (void)typeflag;
    (void)ftwbuf;
    int rv = remove(fpath);
    if (rv) {
        perror(fpath);
    }
    return rv;
}

static int remove_directory(const char *path)
{
    return nftw(path, remove_cb, 64, FTW_DEPTH | FTW_PHYS);
}

int recreate_dir()
{
    char dir[sizeof(PKG_ROOT_FOLDERNAME)];
    strncpy(dir, PKG_ROOT_FOLDERNAME, sizeof(dir));
    struct stat st;
    if (stat(dir, &st) != 0) {
        if (errno == ENOENT) {
            fprintf(stderr, "Ordner '%s' existiert nicht.\n", dir);
        } else {
            perror("stat");
        }
        return 1;
    }
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "'%s' ist kein Verzeichnis.\n", dir);
        return 1;
    }
    if (remove_directory(dir) != 0) {
        fprintf(stderr, "Fehler beim Löschen von '%s'\n", dir);
        return 1;
    }
    printf("Ordner '%s' wurde gelöscht.\n", dir);
    if (mkdir(dir, 0755) != 0) {
        perror("mkdir");
        return 1;
    }
    printf("Ordner '%s' wurde neu erstellt.\n", dir);
    return 0;
}
