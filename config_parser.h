#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <stddef.h>

/* Maximal erlaubte Anzahl Einträge und Stringlängen (einfacher fester Speicher) */
#define MAX_ENTRIES   32
#define MAX_STR_LEN   256

/* Erlaubte lang-Werte */
#define LANG_C_FILE   "c_file"
#define LANG_MAKE     "make"
#define LANG_RUST     "rust"
#define LANG_BINERY   "binery"

typedef struct {
    char lang[MAX_STR_LEN];
    char source[MAX_STR_LEN];
    char command[MAX_STR_LEN];
} ConfigEntry;

typedef struct {
    ConfigEntry entries[MAX_ENTRIES];
    size_t      count;
} ConfigVector;

/**
 * Nicht-statische Parser-Funktion.
 * Liest die Datei, prüft Syntax und Keywords und füllt den Vector.
 * out_vec: Zeiger auf den Vector, der befüllt wird (wird gesetzt/geschrieben)
 * Rückgabe: 0 bei Erfolg, negativer Fehlercode bei Fehler.
 */
int parse_config(const char *filepath, ConfigVector *out_vec);

#ifdef _TEST_BUILD
void run_tests_parse_config_json();
#endif
#endif

