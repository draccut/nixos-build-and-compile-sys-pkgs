#include "deploy_pkgs.h"
#include "recreate_dir.h"
#include "compile_projects.h"

static int start_deployment_prozess(void) 
{
    if (check_all_binerys_exists() == 1) {
        printf("Fehlende Datein\n");
        return 0;
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
        if (init_program(&progs[i], PKG_NAMES[i]) != 0) {
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

static int start_compile_prozess_c(const char *project_name, char *src_file_path, char *project_path)
{
    snprintf(src_file_path, 0x1000, "%s/%s/%s.c", SRC_ROOT, project_name, project_name);
    snprintf(project_path, 0x1000, "%s/%s", SRC_ROOT, project_name);
    if (check_file_exists(src_file_path)) {
        int ret = run_compile_commands(project_name, project_path, ID_C_LANG);
        return ret;
    }
    return 0;
}

static int start_compile_prozess_rust(const char *project_name, char *src_file_path, char *project_path)
{
    snprintf(project_path, 0x1000, "%s/%s", SRC_ROOT, project_name);
    snprintf(src_file_path, 0x1000, "%s/%s/Cargo.toml", SRC_ROOT, project_name);
    if (check_file_exists(src_file_path)) {
        int ret = run_compile_commands(project_name, project_path, ID_RUST_LANG);
        return ret;
    }
    return 0;
}

static int clear_compile_prozess_c(const char *project_name, char *bin_file_path, char *project_path)
{
    snprintf(bin_file_path, 0x1000, "%s/%s/%s", SRC_ROOT, project_name, project_name);
    snprintf(project_path, 0x1000, "%s/%s", SRC_ROOT, project_name);
    if (check_file_exists(bin_file_path)) {
        int ret = clear_compile_commands(project_name, project_path, ID_C_LANG);
        return ret;
    }
    return 0;
}

static int clear_compile_prozess_rust(const char *project_name, char *bin_file_path, char *project_path)
{
    snprintf(project_path, 0x1000, "%s/%s", SRC_ROOT, project_name);
    snprintf(bin_file_path, 0x1000, "%s/%s/target/release/%s", SRC_ROOT, project_name, project_name);
    if (check_file_exists(bin_file_path)) {
        int ret = clear_compile_commands(project_name, project_path, ID_RUST_LANG);
        return ret;
    }
    return 0;
}

/**
 * Führt den Befehl "nixos-rebuild switch" aus.
 * Gibt den Exit-Status des Befehls zurück.
 */
static int nixos_rebuild_switch(void) 
{
    return system("nixos-rebuild switch");
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

int main(void)
{
    if (NROF != LANG_IDS_LEN) {
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
            ret[index_ret] = start_compile_prozess_c(PKG_NAMES[index], sbuf1, sbuf2);
        } else {
            ret[index_ret] = start_compile_prozess_rust(PKG_NAMES[index], sbuf1, sbuf2);
        }
        free(sbuf1);
        free(sbuf2);
        ++index_ret;
    }
    ret[index_ret] = start_deployment_prozess();
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
            ret[index_ret] = clear_compile_prozess_c(PKG_NAMES[index], cbuf1, cbuf2);
        } else {
            ret[index_ret] = clear_compile_prozess_rust(PKG_NAMES[index], cbuf1, cbuf2);
        }
        free(cbuf1);
        free(cbuf2);
        ++index_ret;
    }
    int return_value = sum_ret(NROF * 2 + 2, ret);
    free(ret);
    return return_value;
}


#else

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
    int ret = start_compile_prozess_c("TEST_C_BINERY_1", src_path, proj_path);
    test_assert_eq_int(ret, 0, "start_compile_prozess_c TEST_C_BINERY_1 ret");
    test_assert_eq_string(src_path, "/tmp/home/draccut/dev/TEST_C_BINERY_1/TEST_C_BINERY_1.c",
                          sizeof("/tmp/home/draccut/dev/TEST_C_BINERY_1/TEST_C_BINERY_1.c") - 1);
    test_assert_eq_string(proj_path, "/tmp/home/draccut/dev/TEST_C_BINERY_1",
                          sizeof("/tmp/home/draccut/dev/TEST_C_BINERY_1") - 1);
    ret = start_compile_prozess_c("BBBBBBBBBBBB", src_path, proj_path);
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
    int ret = start_compile_prozess_rust("TEST_RUST_BINERY_1", src_path, proj_path);
    test_assert_eq_int(ret, 0, "start_compile_prozess_rust TEST_RUST_BINERY_1 ret");
    test_assert_eq_string(src_path, "/tmp/home/draccut/dev/TEST_RUST_BINERY_1/Cargo.toml",
                          sizeof("/tmp/home/draccut/dev/TEST_RUST_BINERY_1/Cargo.toml") - 1);
    test_assert_eq_string(proj_path, "/tmp/home/draccut/dev/TEST_RUST_BINERY_1",
                          sizeof("/tmp/home/draccut/dev/TEST_RUST_BINERY_1") - 1);
    ret = start_compile_prozess_rust("AAAAAAAAAAA", src_path, proj_path);
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
    int ret = clear_compile_prozess_c("TEST_C_BINERY_1", bin_path, proj_path);
    test_assert_eq_int(ret, 0, "clear_compile_prozess_c TEST_C_BINERY_1 ret");
    test_assert_eq_string(bin_path, "/tmp/home/draccut/dev/TEST_C_BINERY_1/TEST_C_BINERY_1",
                          sizeof("/tmp/home/draccut/dev/TEST_C_BINERY_1/TEST_C_BINERY_1") - 1);
    test_assert_eq_string(proj_path, "/tmp/home/draccut/dev/TEST_C_BINERY_1",
                          sizeof("/tmp/home/draccut/dev/TEST_C_BINERY_1") - 1);
    ret = clear_compile_prozess_c("BBBBBBBBBBBB", bin_path, proj_path);
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
    int ret = clear_compile_prozess_rust("TEST_RUST_BINERY_1", bin_path, proj_path);
    test_assert_eq_int(ret, 0, "clear_compile_prozess_rust TEST_RUST_BINERY_1 ret");
    test_assert_eq_string(bin_path, "/tmp/home/draccut/dev/TEST_RUST_BINERY_1/target/release/TEST_RUST_BINERY_1",
                          sizeof("/tmp/home/draccut/dev/TEST_RUST_BINERY_1/target/release/TEST_RUST_BINERY_1") - 1);
    test_assert_eq_string(proj_path, "/tmp/home/draccut/dev/TEST_RUST_BINERY_1",
                          sizeof("/tmp/home/draccut/dev/TEST_RUST_BINERY_1") - 1);
    ret = clear_compile_prozess_rust("AAAAAAAAAAA", bin_path, proj_path);
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
    print_test_summary();
    return 0;
}

#endif
