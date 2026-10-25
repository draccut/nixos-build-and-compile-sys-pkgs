#ifndef CONFIG_PATHS_H
#define CONFIG_PATHS_H

#include "config_root_paths.h"
#include "filesystem_operations.h"

typedef struct {
    char *name;                    /* z.B. "LLL" */
    char *path_binery_for_nix;     /* /etc/nixos/pkgs/LLL/LLL */
    char *path_binery_in_src;      /* /dat/src/LLL/LLL (oder /dat/src/LLL) */
    char *path_pkg_dir;            /* /etc/nixos/pkgs/LLL */
    char *path_output_default_nix; /* /etc/nixos/pkgs/LLL/default.nix */
} SystemProgrammStruct;

typedef struct {
    const unsigned char *haystack;
    size_t haystack_len;
    const unsigned char *old;
    size_t old_len;
    const unsigned char *new_bytes;
    size_t new_len;
    size_t *out_len;
} ReplaceArgs;

static const int ID_C_LANG = 0;
static const int ID_RUST_LANG = 1;
static const int ID_BINERY_FILE = 2;
static const int ID_MAKE = 3;

extern char **SRC_PROJECT_PATH;
extern int   *LANG_IDS;
extern char **PKG_NAMES;
extern size_t NROF;
extern size_t LANG_IDS_LEN;

int init_program(SystemProgrammStruct *prog, const char *name, size_t index);
void free_program(SystemProgrammStruct *prog);
int check_all_binerys_exists();
unsigned char *replace_by_split(const ReplaceArgs *args);
int file_is_regular(const char *path);

#ifdef _TEST_BUILD
void run_tests_config_paths();
#endif

#endif
