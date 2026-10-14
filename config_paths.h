#ifndef CONFIG_PATHS_H
#define CONFIG_PATHS_H

#ifndef _TEST_BUILD

#define SRC_ROOT "/dat/src"
#define PKG_ROOT "/etc/nixos/pkgs"
#define OVERLAY_FILE "/etc/nixos/overlays/my-overlay.nix"

#else

#define SRC_ROOT "/tmp/home/draccut/dev"
#define PKG_ROOT "/tmp/etc/nixos/pkgs"
#define OVERLAY_FILE "/tmp/etc/nixos/overlays/my-overlay.nix"

#endif

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

#ifndef _TEST_BUILD

static const int LANG_IDS[] = {
    ID_RUST_LANG,
    ID_C_LANG,
    ID_RUST_LANG,
    ID_C_LANG,
    ID_RUST_LANG,
    ID_RUST_LANG,
    ID_RUST_LANG,
    ID_RUST_LANG,
    ID_RUST_LANG
};

/**
 * Paketnamen Ordner unter PKG_ROOT und SRC_ROOT heißen gleich,
 * Binery heißt wie der Paketname.
 * Also wie das Verzeichnis in dem die Binärdatei liegt.
 */
static const char *PKG_NAMES[] = {
    "LLL",
    "REP",
    "LLM",
    "CALC",
    "CCAT",
    "TERMINE",
    "CHECK",
    "HOSTS",
    "KANBAN",
};

#else

static const int LANG_IDS[] = {
    ID_RUST_LANG,
    ID_C_LANG,
    ID_RUST_LANG,
    ID_C_LANG,
};

static const char *PKG_NAMES[] = {
    "TEST_RUST_BINERY_1",
    "TEST_C_BINERY_1",
    "AAAAAAAAAAA",
    "BBBBBBBBBBBB",
};

#endif

static const size_t NROF = sizeof(PKG_NAMES) / sizeof(PKG_NAMES[0]);
static const size_t LANG_IDS_LEN = sizeof(LANG_IDS) / sizeof(LANG_IDS[0]);

int init_program(SystemProgrammStruct *prog, const char *name);
void free_program(SystemProgrammStruct *prog);
int check_all_binerys_exists();
unsigned char *replace_by_split(const ReplaceArgs *args);
int file_is_regular(const char *path);
int resolve_src_binary(const char *name, char *out, size_t out_sz);

#ifdef _TEST_BUILD
void run_tests_config_paths();
#endif

#endif
