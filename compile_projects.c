#include "compile_projects.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

#define NAME_DEV_USER "draccut"

#ifdef _TEST_BUILD

/**
 * Mock-Funktion für run_command_as in Test-Builds
 * Validiert Parameter basierend auf aufrufender Funktion
 *
 * @param argv     NULL-terminiertes Argument-Array
 * @param username Name identifiziert aufrufende Funktion
 * @return         Immer 0 für erfolgreiche Tests
 */
static int run_command_as(char *const argv[], const char *username)
{
    size_t len = strlen(username);
    if (strcmp(username, "build_c") == 0) {
        test_assert_eq_string(argv[0], "gcc", len);
        test_assert_eq_string(argv[1], "-Og", len);
        test_assert_eq_string(argv[2], "-fstack-protector-strong", len);
    } else if (strcmp(username, "build_rust") == 0) {
        test_assert_eq_string(argv[0], "cargo", len);
        test_assert_eq_string(argv[1], "build", len);
        test_assert_eq_string(argv[2], "--release", len);
        test_assert_eq_string(argv[3], "--manifest-path", len);
    } else if (strcmp(username, "clean_rust") == 0) {
        test_assert_eq_string(argv[0], "cargo", len);
        test_assert_eq_string(argv[1], "clean", len);
        test_assert_eq_string(argv[2], "--manifest-path", len);
    }
    return 0;
}

#else

/**
 * Führt ein Kommando als der angegebene Benutzer aus.
 * Elternprozess (root) wartet auf Beendigung des Kindes.
 *
 * @param argv     NULL-terminiertes Argument-Array (wie für execvp)
 * @param username Name des Benutzers, unter dem das Kommando laufen soll.
 * @return         Exit-Code des Kommandos, -1 bei Fehlern im Elternprozess.
 */
static int run_command_as(char *const argv[], const char *username)
{
    if ((argv == NULL) || (username == NULL)) {
        return -1;
    }
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    if (pid == 0) {
        struct passwd *pw = NULL; 
        if (*username != '\0') {
            pw = getpwnam(username);
            if (pw == NULL) {
                fprintf(stderr, "run_command_as: Benutzer '%s' nicht gefunden\n", username);
                _exit(1);
            }
            if (initgroups(username, pw->pw_gid) != 0) {
                perror("initgroups");
                _exit(1);
            }
            if (setgid(pw->pw_gid) != 0) {
                perror("setgid");
                _exit(1);
            }
            if (setuid(pw->pw_uid) != 0) {
                perror("setuid");
                _exit(1);
            }
            if (setuid(0) != -1 || getuid() != pw->pw_uid) {
                fprintf(stderr, "run_command_as: Rechte konnten nicht vollständig reduziert werden\n");
                _exit(1);
            }
        }
        if (pw != NULL) {
            setenv("HOME", pw->pw_dir, 1);
            setenv("USER", pw->pw_name, 1);
            setenv("LOGNAME", pw->pw_name, 1);
        }
        execvp(argv[0], argv);
        perror("execvp");
        _exit(127);
    }
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return -1;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
}

static int build_make(const char *project_dir)
{
    char *const argv[] = {
        "make",
        "-C", (char *)project_dir,
        NULL
    };
    printf("Baue Make-Projekt...\n");
    return run_command_as(argv, NAME_DEV_USER);
}

static int clean_make(const char *project_dir)
{
    char *const argv[] = {
        "make",
        "-C", (char *)project_dir,
        "clean",
        NULL
    };
    printf("Bereinige Make-Projekt (make clean)...\n");
    return run_command_as(argv, NAME_DEV_USER);
}

#endif

/**
 * Kompiliert C-Quellcode mit sicherheitsorientierten Compiler-Flags
 * Binary wird im Projektordner erstellt
 *
 * @param project_dir  Pfad zum Projektverzeichnis
 * @param binery_name  Name der zu erstellenden Binärdatei
 * @return             Exit-Code der Kompilierung
 */
static int build_c(const char *project_dir, const char *binery_name) 
{
    char out_path[0xFFF];
    char src_path[0xFFF];
    snprintf(out_path, sizeof(out_path), "%s/%s",   project_dir, binery_name);
    snprintf(src_path, sizeof(src_path), "%s/%s.c", project_dir, binery_name);
    char *const argv[] = {
        "gcc",
        "-Og",
        "-fstack-protector-strong",
        "-D_FORTIFY_SOURCE=3",
        "-fstrict-flex-arrays=3",
        "-fno-strict-aliasing",
        "-fno-strict-overflow",
        "-fno-delete-null-pointer-checks",
        "-fcf-protection=full",
        "-Wall",
        "-Wextra",
        "-Wformat",
        "-Wformat-security",
        "-fPIE",
        "-pie",
        "-Wl,-z,relro",
        "-Wl,-z,now",
        "-Wl,-z,noexecstack",
        "-o", out_path,
        src_path,
        NULL
    };
    printf("Baue C-Programm...\n");
    #ifdef _TEST_BUILD
    return run_command_as(argv, "build_c");
    #else
    return run_command_as(argv, NAME_DEV_USER);
    #endif
}

/**
 * Führt cargo build --release aus
 * Binary wird im target/release Verzeichnis erstellt
 *
 * @param project_dir Pfad zum Rust-Projektverzeichnis
 * @return            Exit-Code des Build-Prozesses
 */
static int build_rust(const char *project_dir) 
{
    char manifest[0x1000];
    char target_dir[0x1000];
    snprintf(manifest, sizeof(manifest), "%s/Cargo.toml", project_dir);
    snprintf(target_dir, sizeof(target_dir), "%s/target", project_dir);
    char *const argv[] = {
        "cargo",
        "build",
        "--release",
        "--manifest-path", manifest,
        "--target-dir", target_dir,
        NULL
    };
    printf("Baue Rust-Projekt...\n");
    #ifdef _TEST_BUILD
    return run_command_as(argv, "build_rust");
    #else
    return run_command_as(argv, NAME_DEV_USER);
    #endif
}

/**
 * Führt cargo clean aus zur Bereinigung von Build-Artefakten
 *
 * @param project_dir Pfad zum Rust-Projektverzeichnis
 * @return            Exit-Code des Clean-Prozesses
 */
static int clean_rust(const char *project_dir) 
{
    char manifest[0x1000];
    snprintf(manifest, sizeof(manifest), "%s/Cargo.toml", project_dir);
    char *const argv[] = {
        "cargo",
        "clean",
        "--manifest-path", manifest,
        NULL
    };
    printf("clean Rust-Projekt...\n");
    #ifdef _TEST_BUILD
    return run_command_as(argv, "clean_rust");
    #else
    return run_command_as(argv, NAME_DEV_USER);
    #endif
}

#ifndef _TEST_BUILD

/**
 * Löscht eine Datei sicher
 * Keine Fehlermeldung wenn Datei nicht existiert
 *
 * @param absolute_path Absoluter Pfad zur zu löschenden Datei
 */
static void delete_binery_file(const char* absolute_path) 
{
    if (unlink(absolute_path) == 0) {
        printf("Datei erfolgreich gelöscht: %s\n", absolute_path);
    } else {
        if (access(absolute_path, F_OK) == -1) {
            printf("Datei existiert nicht, nichts zu löschen: %s\n", absolute_path);
            return;
        }
        perror("Fehler beim Löschen der Datei");
    }
}

/**
 * Kopiert kompilierte Rust-Binärdatei in Projekt-Root
 * Ermöglicht Build-Prozess unabhängig von NixOS-Paketierung
 *
 * @param rust_binery_path Zielpfad für Binärdatei
 * @param project_dir      Projektverzeichnis
 * @param binery_name      Name der Binärdatei
 */
static void copy_rust_binery(const char *rust_binery_path, const char *project_dir, const char *binery_name)
{
    delete_binery_file(rust_binery_path);
    char new_bin_path[0x1000];
    snprintf(new_bin_path, sizeof(new_bin_path), "%s/target/release/%s", project_dir, binery_name);
    copy_binary_file(new_bin_path, rust_binery_path);
}

/**
 * Kompiliert Projekt basierend auf Programmiersprache
 * Legt Binärdatei im Projektverzeichnis ab
 *
 * @param binery_name Name der zu erstellenden Binärdatei
 * @param project_dir Pfad zum Wurzelverzeichnis des Projekts
 * @param lang_id     ID der Programmiersprache
 * @return            0 bei Erfolg, 1 bei Fehlern
 */
int run_compile_commands(const char *binery_name, const char *project_dir, const int lang_id) 
{
    if (lang_id == ID_RUST_LANG) {
        clean_rust(project_dir);
        if (build_rust(project_dir) != 0) {
            fprintf(stderr, "Cargo Build fehlgeschlagen\n");
            return 1;
        }
        char rust_binery_path[0x1000];
        snprintf(rust_binery_path, sizeof(rust_binery_path), "%s/%s", project_dir, binery_name);
        printf("Old Rust bin at: %s\n", rust_binery_path);
        copy_rust_binery(rust_binery_path, project_dir, binery_name);
    }
    if (lang_id == ID_C_LANG) {
        char binery_path[0x1000];
        snprintf(binery_path, sizeof(binery_path), "%s/%s", project_dir, binery_name);
        delete_binery_file(binery_path);
        if (build_c(project_dir, binery_name) != 0) {
            fprintf(stderr, "C-Build fehlgeschlagen\n");
            return 1;
        }
    }
    if (lang_id == ID_MAKE) {
        clean_make(project_dir);
        if (build_make(project_dir) != 0) {
            fprintf(stderr, "make-Build fehlgeschlagen\n");
            return 1;
        }
    }
    printf("kompilierung erfolgreich: %s\nIn Projektverzeichnis: %s\n", binery_name, project_dir);
    return 0;
}

/**
 * Bereinigt Verzeichnisstrukturen der Kompilierungs-Zwischenstufen
 * Entfernt auch resultierende Binärdateien
 *
 * @param binery_name Name der Binärdatei
 * @param project_dir Projektverzeichnis
 * @param lang_id     Programmiersprachen-ID
 * @return            0 bei Erfolg, 1 bei Fehlern
 */
int clear_compile_commands(const char *binery_name, const char *project_dir, const int lang_id)
{
    char binery_path[0x1000];
    snprintf(binery_path, sizeof(binery_path), "%s/%s", project_dir, binery_name);
    if (lang_id == ID_RUST_LANG) {
        if (clean_rust(project_dir) != 0) {
            return 1;
        }
        delete_binery_file(binery_path);
    }
    if (lang_id == ID_C_LANG) {
        delete_binery_file(binery_path);
    }
    if (lang_id == ID_MAKE) {
        clean_make(project_dir);
    }
    printf("CleanUp erfolgreich: %s\nIn Projektverzeichnis: %s\n", binery_name, project_dir);
    return 0;
}


#else

int run_compile_commands(const char *binery_name, const char *project_dir, const int lang_id)
{
    (void)binery_name;
    (void)project_dir;
    (void)lang_id;
    return 0;
}

int clear_compile_commands(const char *binery_name, const char *project_dir, const int lang_id)
{
    (void)binery_name;
    (void)project_dir;
    (void)lang_id;
    return 0;
}

void run_tests_compile_projects()
{
    const char *project_dir = "/usr/src";
    const char *binery_name = "example_binery";
    printf("\n%s==== compile_projects.c tests ====%s\n", C_BOLD, C_RESET);
    build_c(project_dir, binery_name);
    build_rust(project_dir);
    clean_rust(project_dir);
}

#endif
