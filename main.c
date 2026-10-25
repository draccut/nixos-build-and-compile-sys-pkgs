#include "deploy_pkgs.h"
#include "recreate_dir.h"
#include "compile_projects.h"
#include "config_parser.h"

#ifndef _TEST_BUILD

static int start_deployment_prozess(void) 
{
    if (check_all_binerys_exists() == 1) {
        printf("Fehlende Datein\n");
        return 1;
    } else {
        printf("Alle Binerys vorhanden\n");
    }
    char *overlay_string = build_overlay_nix();
    if (!overlay_string) return 1;
    write_overlay_nix(OVERLAY_FILE, overlay_string);
    free(overlay_string);
    recreate_dir();
    SystemProgrammStruct progs[NROF];
    size_t i;
    int errors = 0;
    memset(progs, 0, sizeof(progs));
    if (geteuid() != 0) {
        fprintf(stderr, "Warnung: nicht root. Schreiben nach %s schlägt oft fehl.\n", PKG_ROOT);
    }
    for (i = 0; i < NROF; i++) {
        if (init_program(&progs[i], PKG_NAMES[i], i) != 0) {
            fprintf(stderr, "init_program failed for %s\n", PKG_NAMES[i]);
            errors++;
        }
    }
    for (i = 0; i < NROF; i++) {
        if (progs[i].name == NULL) continue;
        if (deploy_package(&progs[i]) != 0) {
            fprintf(stderr, "deploy_package failed: %s\n", progs[i].name);
            errors++;
        }
    }
    for (i = 0; i < NROF; i++) {
        if (progs[i].path_pkg_dir == NULL) continue;
        if (!file_is_regular(progs[i].path_output_default_nix)) {
            fprintf(stderr, "skip nix-build %s: keine default.nix\n", progs[i].name ? progs[i].name : "?");
            errors++;
            continue;
        }
        if (!file_is_regular(progs[i].path_binery_for_nix)) {
            fprintf(stderr, "skip nix-build %s: Binary fehlt (%s)\n", progs[i].name ? progs[i].name : "?", progs[i].path_binery_for_nix);
            errors++;
            continue;
        }
        if (run_nix_build(progs[i].path_pkg_dir) != 0) {
            fprintf(stderr, "run_nix_build failed: %s\n", progs[i].name);
            errors++;
        }
    }
    for (i = 0; i < NROF; i++) {
        free_program(&progs[i]);
    }
    if (errors != 0) {
        fprintf(stderr, "Fertig mit %d Fehler(n).\n", errors);
        return 1;
    }
    printf("Alle %ld Pakete deployed und gebaut.\n", NROF);
    return 0;
}

#endif

static int start_compile_prozess_c(const char *project_name, char *src_file_path, char *project_path, size_t index)
{
    snprintf(src_file_path, 0x1000, "%s/%s.c", SRC_PROJECT_PATH[index], project_name);
    snprintf(project_path, 0x1000, "%s", SRC_PROJECT_PATH[index]);
    if (check_file_exists(src_file_path)) {
        int ret = run_compile_commands(project_name, project_path, ID_C_LANG);
        return ret;
    }
    return 0;
}

static int start_compile_prozess_rust(const char *project_name, char *src_file_path, char *project_path, size_t index)
{
    snprintf(project_path, 0x1000, "%s", SRC_PROJECT_PATH[index]);
    snprintf(src_file_path, 0x1000, "%s/Cargo.toml", SRC_PROJECT_PATH[index]);
    if (check_file_exists(src_file_path)) {
        int ret = run_compile_commands(project_name, project_path, ID_RUST_LANG);
        return ret;
    }
    return 0;
}

static int clear_compile_prozess_c(const char *project_name, char *bin_file_path, char *project_path, size_t index)
{
    snprintf(bin_file_path, 0x1000, "%s/%s", SRC_PROJECT_PATH[index], project_name);
    snprintf(project_path, 0x1000, "%s", SRC_PROJECT_PATH[index]);
    if (check_file_exists(bin_file_path)) {
        int ret = clear_compile_commands(project_name, project_path, ID_C_LANG);
        return ret;
    }
    return 0;
}

static int clear_compile_prozess_rust(const char *project_name, char *bin_file_path, char *project_path, size_t index)
{
    snprintf(project_path, 0x1000, "%s", SRC_PROJECT_PATH[index]);
    snprintf(bin_file_path, 0x1000, "%s/target/release/%s", SRC_PROJECT_PATH[index], project_name);
    if (check_file_exists(bin_file_path)) {
        int ret = clear_compile_commands(project_name, project_path, ID_RUST_LANG);
        return ret;
    }
    return 0;
}

static int sum_ret(size_t len, int *ret)
{
    int res = 0;
    for (size_t i = 0; i < len; ++i) {
        res = res + ret[i];
    }
    return res;
}

#ifndef _TEST_BUILD

/**
 * Führt den Befehl "nixos-rebuild switch" aus.
 * Gibt den Exit-Status des Befehls zurück.
 */
static int nixos_rebuild_switch(void)
{
    return system("nixos-rebuild switch");
}

static void print_config_vector(const ConfigVector *vec)
{
    if (!vec) {
        printf("(ConfigVector is NULL)\n");
        return;
    }
    printf("ConfigVector: %zu Einträge\n", vec->count);
    printf("----------------------------------------\n");
    for (size_t i = 0; i < vec->count; i++) {
        const ConfigEntry *e = &vec->entries[i];
        printf("[%zu]\n", i);
        printf("  lang:    %s\n", e->lang);
        printf("  source:  %s\n", e->source);
        printf("  command: %s\n", e->command);
    }
    printf("----------------------------------------\n");
}

static int fill_up_global_vectors(const ConfigVector *vec)
{
    if (!vec) {
        printf("(ConfigVector is NULL)\n");
        return 1;
    }
    NROF = vec->count;
    LANG_IDS_LEN = vec->count;
    LANG_IDS = malloc(vec->count * sizeof(int));
    PKG_NAMES = malloc(vec->count * sizeof(char *));
    SRC_PROJECT_PATH = malloc(vec->count * sizeof(char *));
    for (size_t i = 0; i < vec->count; i++) {
        const ConfigEntry *e = &vec->entries[i];
        if (strcmp(e->lang, LANG_C_FILE) == 0) {
            LANG_IDS[i] = ID_C_LANG;
        } else if (strcmp(e->lang, LANG_MAKE) == 0) {
            LANG_IDS[i] = ID_MAKE;
        } else if (strcmp(e->lang, LANG_RUST) == 0) {
            LANG_IDS[i] = ID_RUST_LANG;
        } else if (strcmp(e->lang, LANG_BINERY) == 0) {
            LANG_IDS[i] = ID_BINERY_FILE;
        } else {
            return 1;
        }
        PKG_NAMES[i] = strdup(e->command);
        SRC_PROJECT_PATH[i] = strdup(e->source);
    }
    return 0;
}

static void free_global_vectors()
{
    for (size_t i = 0; i < NROF; i++) {
        free(PKG_NAMES[i]);
	free(SRC_PROJECT_PATH[i]);
    }
    free(PKG_NAMES);
    free(SRC_PROJECT_PATH);
    free(LANG_IDS);
}

int main(void)
{
    ConfigVector vec;
    if (parse_config("config.json", &vec) == 0) {
        print_config_vector(&vec);
    }
    int res_fugv = fill_up_global_vectors(&vec);
    if (res_fugv != 0) {
        return 1;
    }
    int *ret = malloc((NROF * 2 + 2) * sizeof(int));
    size_t index_ret = 0;
    for (size_t index = 0; index < NROF; ++index) {
        char *sbuf1 = malloc(0x1000);
        char *sbuf2 = malloc(0x1000);
        if (!sbuf1 || !sbuf2) {
            fprintf(stderr, "malloc failed\n");
            free(sbuf1);
            free(sbuf2);
            return 1;
        }
        if (LANG_IDS[index] == ID_C_LANG) {
            ret[index_ret] = start_compile_prozess_c(PKG_NAMES[index], sbuf1, sbuf2, index);
        } else if (LANG_IDS[index] == ID_RUST_LANG) {
            ret[index_ret] = start_compile_prozess_rust(PKG_NAMES[index], sbuf1, sbuf2, index);
        } else {
            char bin_file_path_check[0x1000];
            snprintf(bin_file_path_check, sizeof(bin_file_path_check), "%s/%s", SRC_PROJECT_PATH[index], PKG_NAMES[index]);
            ret[index_ret] = check_binery_exists(bin_file_path_check);
        }
        free(sbuf1);
        free(sbuf2);
        ++index_ret;
    }
    ret[index_ret] = start_deployment_prozess();
    if (ret[index_ret] != 0) {
        return 1;
    }
    ++index_ret;
    ret[index_ret] = nixos_rebuild_switch();
    ++index_ret;
    for (size_t index = 0; index < NROF; ++index) {
        char *cbuf1 = malloc(0x1000);
        char *cbuf2 = malloc(0x1000);
        if (!cbuf1 || !cbuf2) {
            fprintf(stderr, "malloc failed\n");
            free(cbuf1);
            free(cbuf2);
            return 1;
        }
        if (LANG_IDS[index] == ID_C_LANG) {
            ret[index_ret] = clear_compile_prozess_c(PKG_NAMES[index], cbuf1, cbuf2, index);
        } else if (LANG_IDS[index] == ID_RUST_LANG) {
            ret[index_ret] = clear_compile_prozess_rust(PKG_NAMES[index], cbuf1, cbuf2, index);
        } else {
            ret[index_ret] = 0;
        }
        free(cbuf1);
        free(cbuf2);
        ++index_ret;
    }
    int return_value = sum_ret(NROF * 2 + 2, ret);
    free(ret);
    free_global_vectors();
    return return_value;
}


#else

static void run_test_sum_ret(void)
{
    size_t len_list = 4;
    int list[] = {1, 2, 3, 4};
    int ret = sum_ret(len_list, list);
    test_assert_eq_int(ret, 10, "sum_ret");
}

static void run_tests_start_compile_prozess_c(void)
{
    char *src_path = malloc(0x1000);
    char *proj_path = malloc(0x1000);
    if (!src_path || !proj_path) {
        fprintf(stderr, "malloc failed\n");
        free(src_path);
        free(proj_path);
        return;
    }
    int ret = start_compile_prozess_c("TEST_C_BINERY_1", src_path, proj_path, 1);
    test_assert_eq_int(ret, 0, "start_compile_prozess_c TEST_C_BINERY_1 ret");
    test_assert_eq_string(src_path, "/tmp/home/draccut/dev/TEST_C_BINERY_1/TEST_C_BINERY_1.c",
                          sizeof("/tmp/home/draccut/dev/TEST_C_BINERY_1/TEST_C_BINERY_1.c") - 1);
    test_assert_eq_string(proj_path, "/tmp/home/draccut/dev/TEST_C_BINERY_1",
                          sizeof("/tmp/home/draccut/dev/TEST_C_BINERY_1") - 1);
    ret = start_compile_prozess_c("BBBBBBBBBBBB", src_path, proj_path, 3);
    test_assert_eq_int(ret, 0, "start_compile_prozess_c BBBBBBBBBBBB ret");
    test_assert_eq_string(src_path, "/tmp/home/draccut/dev/BBBBBBBBBBBB/BBBBBBBBBBBB.c",
                          sizeof("/tmp/home/draccut/dev/BBBBBBBBBBBB/BBBBBBBBBBBB.c") - 1);
    test_assert_eq_string(proj_path, "/tmp/home/draccut/dev/BBBBBBBBBBBB",
                          sizeof("/tmp/home/draccut/dev/BBBBBBBBBBBB") - 1);
    free(src_path);
    free(proj_path);
}

static void run_tests_start_compile_prozess_rust(void)
{
    char *src_path = malloc(0x1000);
    char *proj_path = malloc(0x1000);
    if (!src_path || !proj_path) {
        fprintf(stderr, "malloc failed\n");
        free(src_path);
        free(proj_path);
        return;
    }
    int ret = start_compile_prozess_rust("TEST_RUST_BINERY_1", src_path, proj_path, 0);
    test_assert_eq_int(ret, 0, "start_compile_prozess_rust TEST_RUST_BINERY_1 ret");
    test_assert_eq_string(src_path, "/tmp/home/draccut/dev/TEST_RUST_BINERY_1/Cargo.toml",
                          sizeof("/tmp/home/draccut/dev/TEST_RUST_BINERY_1/Cargo.toml") - 1);
    test_assert_eq_string(proj_path, "/tmp/home/draccut/dev/TEST_RUST_BINERY_1",
                          sizeof("/tmp/home/draccut/dev/TEST_RUST_BINERY_1") - 1);
    ret = start_compile_prozess_rust("AAAAAAAAAAA", src_path, proj_path, 2);
    test_assert_eq_int(ret, 0, "start_compile_prozess_rust AAAAAAAAAAA ret");
    test_assert_eq_string(src_path, "/tmp/home/draccut/dev/AAAAAAAAAAA/Cargo.toml",
                          sizeof("/tmp/home/draccut/dev/AAAAAAAAAAA/Cargo.toml") - 1);
    test_assert_eq_string(proj_path, "/tmp/home/draccut/dev/AAAAAAAAAAA",
                          sizeof("/tmp/home/draccut/dev/AAAAAAAAAAA") - 1);
    free(src_path);
    free(proj_path);
}

static void run_tests_clear_compile_prozess_c(void)
{
    char *bin_path = malloc(0x1000);
    char *proj_path = malloc(0x1000);
    if (!bin_path || !proj_path) {
        fprintf(stderr, "malloc failed\n");
        free(bin_path);
        free(proj_path);
        return;
    }
    int ret = clear_compile_prozess_c("TEST_C_BINERY_1", bin_path, proj_path, 1);
    test_assert_eq_int(ret, 0, "clear_compile_prozess_c TEST_C_BINERY_1 ret");
    test_assert_eq_string(bin_path, "/tmp/home/draccut/dev/TEST_C_BINERY_1/TEST_C_BINERY_1",
                          sizeof("/tmp/home/draccut/dev/TEST_C_BINERY_1/TEST_C_BINERY_1") - 1);
    test_assert_eq_string(proj_path, "/tmp/home/draccut/dev/TEST_C_BINERY_1",
                          sizeof("/tmp/home/draccut/dev/TEST_C_BINERY_1") - 1);
    ret = clear_compile_prozess_c("BBBBBBBBBBBB", bin_path, proj_path, 3);
    test_assert_eq_int(ret, 0, "clear_compile_prozess_c BBBBBBBBBBBB ret");
    test_assert_eq_string(bin_path, "/tmp/home/draccut/dev/BBBBBBBBBBBB/BBBBBBBBBBBB",
                          sizeof("/tmp/home/draccut/dev/BBBBBBBBBBBB/BBBBBBBBBBBB") - 1);
    test_assert_eq_string(proj_path, "/tmp/home/draccut/dev/BBBBBBBBBBBB",
                          sizeof("/tmp/home/draccut/dev/BBBBBBBBBBBB") - 1);
    free(bin_path);
    free(proj_path);
}

static void run_tests_clear_compile_prozess_rust(void)
{
    char *bin_path = malloc(0x1000);
    char *proj_path = malloc(0x1000);
    if (!bin_path || !proj_path) {
        fprintf(stderr, "malloc failed\n");
        free(bin_path);
        free(proj_path);
        return;
    }
    int ret = clear_compile_prozess_rust("TEST_RUST_BINERY_1", bin_path, proj_path, 0);
    test_assert_eq_int(ret, 0, "clear_compile_prozess_rust TEST_RUST_BINERY_1 ret");
    test_assert_eq_string(bin_path, "/tmp/home/draccut/dev/TEST_RUST_BINERY_1/target/release/TEST_RUST_BINERY_1",
                          sizeof("/tmp/home/draccut/dev/TEST_RUST_BINERY_1/target/release/TEST_RUST_BINERY_1") - 1);
    test_assert_eq_string(proj_path, "/tmp/home/draccut/dev/TEST_RUST_BINERY_1",
                          sizeof("/tmp/home/draccut/dev/TEST_RUST_BINERY_1") - 1);
    ret = clear_compile_prozess_rust("AAAAAAAAAAA", bin_path, proj_path, 2);
    test_assert_eq_int(ret, 0, "clear_compile_prozess_rust AAAAAAAAAAA ret");
    test_assert_eq_string(bin_path, "/tmp/home/draccut/dev/AAAAAAAAAAA/target/release/AAAAAAAAAAA",
                          sizeof("/tmp/home/draccut/dev/AAAAAAAAAAA/target/release/AAAAAAAAAAA") - 1);
    test_assert_eq_string(proj_path, "/tmp/home/draccut/dev/AAAAAAAAAAA",
                          sizeof("/tmp/home/draccut/dev/AAAAAAAAAAA") - 1);
    free(bin_path);
    free(proj_path);
}

static void run_tests_start_clear_compile_prozess(void)
{
    printf("\n%s======== TEST main.c ========%s\n", C_BOLD, C_RESET);
    run_tests_start_compile_prozess_c();
    run_tests_start_compile_prozess_rust();
    run_tests_clear_compile_prozess_c();
    run_tests_clear_compile_prozess_rust();
}

int main(void)
{
    printf("Test startet\n");
    run_tests_compile_projects();
    run_tests_config_paths();
    run_tests_deploy_pkgs();
    run_tests_start_clear_compile_prozess();
    run_tests_parse_config_json();
    print_test_summary();
    run_test_sum_ret();
    return 0;
}

#endif

