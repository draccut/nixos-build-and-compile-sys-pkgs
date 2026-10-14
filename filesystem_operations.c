#include "filesystem_operations.h"

/**
 * mkdir_p legt ein Verzeichnis inkl. aller fehlenden Elternverzeichnisse an (ähnlich „mkdir -p“).
 * Der Pfad wird schrittweise zerlegt; existierende Verzeichnisse werden toleriert, Dateien
 * an gleicher Stelle führen zu einem Fehler. Rückgabe 0 bei Erfolg, -1 bei Fehler.
 */
int mkdir_p(const char *path)
{
    char tmp[0x1000];
    char *p;
    size_t len;
    struct stat st;
    if (path == NULL || path[0] == '\0') return -1;
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (len == 0) return -1;
    if (tmp[len - 1] == '/') tmp[len - 1] = '\0';
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (stat(tmp, &st) != 0) {
                if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
                    return -1;
            } else if (!S_ISDIR(st.st_mode)) {
                return -1;
            }
            *p = '/';
        }
    }
    if (stat(tmp, &st) != 0) {
        if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
            return -1;
    } else if (!S_ISDIR(st.st_mode)) {
        return -1;
    }
    return 0;
}

/**
 * copy_binary_file kopiert eine Datei binär von src_path nach dst_path.
 * Das Zielverzeichnis wird bei Bedarf mit mkdir_p angelegt; gelesen und geschrieben
 * wird blockweise. Bei Fehlern (Öffnen, Lesen, Schreiben) wird -1 zurückgegeben, sonst 0.
 */
int copy_binary_file(const char *src_path, const char *dst_path)
{
    FILE *src = NULL;
    FILE *dst = NULL;
    unsigned char buf[0x40 * 0x400];
    size_t n;
    char path_copy[0x1000];
    char *dir;
    if (src_path == NULL || dst_path == NULL) return -1;
    printf("Copy from %s to %s\n", src_path, dst_path);
    src = fopen(src_path, "rb");
    if (src == NULL) {
        fprintf(stderr, "fopen src '%s': %s\n", src_path, strerror(errno));
        return -1;
    }
    snprintf(path_copy, sizeof(path_copy), "%s", dst_path);
    dir = dirname(path_copy);
    if (mkdir_p(dir) != 0) {
        fprintf(stderr, "mkdir_p dest dir '%s': %s\n", dir, strerror(errno));
        fclose(src);
        return -1;
    }
    dst = fopen(dst_path, "wb");
    if (dst == NULL) {
        fprintf(stderr, "fopen dest '%s': %s\n", dst_path, strerror(errno));
        fclose(src);
        return -1;
    }
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
        if (fwrite(buf, 1, n, dst) != n) {
            fprintf(stderr, "fwrite '%s': %s\n", dst_path, strerror(errno));
            fclose(src);
            fclose(dst);
            return -1;
        }
    }
    if (ferror(src)) {
        fprintf(stderr, "fread '%s': %s\n", src_path, strerror(errno));
        fclose(src);
        fclose(dst);
        return -1;
    }
    fclose(src);
    fclose(dst);
    return 0;
}

#ifndef _TEST_BUILD

/**
 * @return 1 if file exists
 */
int check_file_exists(const char *full_path)
{
    printf("Teste: %s\n", full_path);
    struct stat file_stat;
    int result = (stat(full_path, &file_stat) == 0) ? 1 : 0;
    return result;
}

#else

int check_file_exists(const char *full_path)
{
    (void)full_path;
    return 1;
}

#endif
