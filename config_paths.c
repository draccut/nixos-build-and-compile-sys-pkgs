#include "config_paths.h"

#ifndef _TEST_BUILD

/* Produktions-Build: leer, wird zur Laufzeit gefüllt */
char **SRC_PROJECT_PATH = NULL;
int   *LANG_IDS         = NULL;
char **PKG_NAMES        = NULL;
size_t NROF             = 0;
size_t LANG_IDS_LEN     = 0;

#else

/* Test-Build: feste Datensätze, genau eine Instanz für alle Module */
static char *pkg_names_data[] = {
    "TEST_RUST_BINERY_1",
    "TEST_C_BINERY_1",
    "AAAAAAAAAAA",
    "BBBBBBBBBBBB",
};

static char *src_project_path_data[] = {
    "/tmp/home/draccut/dev/TEST_RUST_BINERY_1",
    "/tmp/home/draccut/dev/TEST_C_BINERY_1",
    "/tmp/home/draccut/dev/AAAAAAAAAAA",
    "/tmp/home/draccut/dev/BBBBBBBBBBBB",
};

char **SRC_PROJECT_PATH = src_project_path_data;
char **PKG_NAMES        = pkg_names_data;
size_t NROF             = 4;

#endif

static int resolve_src_binary(const char *name, char *out, size_t out_sz, size_t index);

int init_program(SystemProgrammStruct *prog, const char *name, size_t index)
{
    char bin_path[0x20ff];
    char nix_path[0x20ff];
    char pkg_dir[0x1000];
    char src_path[0x1000];
    if (prog == NULL || name == NULL) return -1;
    memset(prog, 0, sizeof(*prog));
    prog->name = strdup(name);
    if (prog->name == NULL) return -1;
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/%s", PKG_ROOT, name);
    prog->path_pkg_dir = strdup(pkg_dir);
    if (prog->path_pkg_dir == NULL) return -1;
    if (resolve_src_binary(name, src_path, sizeof(src_path), index) != 0) return -1;
    prog->path_binery_in_src = strdup(src_path);
    if (prog->path_binery_in_src == NULL) return -1;
    snprintf(bin_path, sizeof(bin_path), "%s/%s", pkg_dir, name);
    prog->path_binery_for_nix = strdup(bin_path);
    if (prog->path_binery_for_nix == NULL) return -1;
    snprintf(nix_path, sizeof(nix_path), "%s/default.nix", pkg_dir);
    prog->path_output_default_nix = strdup(nix_path);
    if (prog->path_output_default_nix == NULL) return -1;
    return 0;
}

void free_program(SystemProgrammStruct *prog)
{
    if (prog == NULL) return;
    free(prog->name);
    free(prog->path_binery_for_nix);
    free(prog->path_binery_in_src);
    free(prog->path_pkg_dir);
    free(prog->path_output_default_nix);
    memset(prog, 0, sizeof(*prog));
}

int check_all_binerys_exists()
{
    for (size_t i = 0; i < NROF; ++i) {
        size_t path_length = strlen(SRC_PROJECT_PATH[i]) + strlen(PKG_NAMES[i]) + 2;
        char* full_path = malloc(path_length);
        if (full_path == NULL) {
            return 0;
        }
        snprintf(full_path, path_length, "%s/%s", SRC_PROJECT_PATH[i], PKG_NAMES[i]);
        if (!(check_file_exists(full_path))) {
            printf("FEHLER: %s existiert nicht\n", full_path);
            free(full_path);
            return 1;
        }
        free(full_path);
        full_path = NULL;
    }
    return 0;
}

unsigned char *replace_by_split(const ReplaceArgs *args)
{
    const unsigned char *haystack  = args->haystack;
    size_t haystack_len            = args->haystack_len;
    const unsigned char *old       = args->old;
    size_t old_len                 = args->old_len;
    const unsigned char *new_bytes = args->new_bytes;
    size_t new_len                 = args->new_len;
    size_t *out_len                = args->out_len;
    if (old_len == 0) {
        unsigned char *result = malloc(haystack_len);
        if (!result) return NULL;
        memcpy(result, haystack, haystack_len);
        *out_len = haystack_len;
        return result;
    }
    size_t capacity = haystack_len;
    unsigned char *result = malloc(capacity);
    if (!result) return NULL;
    size_t result_len = 0;
    size_t last = 0;
    while (last + old_len <= haystack_len) {
        unsigned char *match = memmem(&haystack[last], haystack_len - last, old, old_len);
        if (!match) {
            break;
        }
        size_t abs_pos = match - haystack;
        size_t needed = result_len + (abs_pos - last) + new_len;
        if (needed > capacity) {
            capacity = needed * 2;
            unsigned char *tmp = realloc(result, capacity);
            if (!tmp) { free(result); return NULL; }
            result = tmp;
        }
        memcpy(&result[result_len], &haystack[last], abs_pos - last);
        result_len += (abs_pos - last);
        memcpy(&result[result_len], new_bytes, new_len);
        result_len += new_len;
        last = abs_pos + old_len;
    }
    size_t remaining = haystack_len - last;
    if (remaining > 0) {
        size_t needed = result_len + remaining;
        if (needed > capacity) {
            unsigned char *tmp = realloc(result, needed);
            if (!tmp) { free(result); return NULL; }
            result = tmp;
        }
        memcpy(&result[result_len], &haystack[last], remaining);
        result_len += remaining;
    }
    *out_len = result_len;
    return result;
}

int file_is_regular(const char *path)
{
    struct stat st;
    if (path == NULL) return 0;
    if (stat(path, &st) != 0) return 0;
    return S_ISREG(st.st_mode);
}

static int resolve_src_binary(const char *name, char *out, size_t out_sz, size_t index)
{
    if (name == NULL || out == NULL || out_sz == 0) return -1;
    snprintf(out, out_sz, "%s/%s/%s", SRC_PROJECT_PATH[index], name, name);
    if (file_is_regular(out)) return 0;
    snprintf(out, out_sz, "%s/%s", SRC_PROJECT_PATH[index], name);
    if (file_is_regular(out)) return 0;
    #ifndef _TEST_BUILD
    fprintf(stderr,
            "Quelle nicht gefunden für '%s'. Gesucht:\n"
            "  %s/%s/%s\n"
            "  %s/%s\n",
            name, SRC_PROJECT_PATH[index], name, name, SRC_PROJECT_PATH[index], name);
    #endif
    return -1;
}


#ifdef _TEST_BUILD

#define SRC_ROOT "/tmp/home/draccut/dev"

static void test_free_program_clears_struct(void)
{
    SystemProgrammStruct prog;
    const char *name = "TEST_C_BINERY_1";
    char pkg_dir[0x1000];
    char bin_path[0x20ff];
    char nix_path[0x20ff];
    char src_path[0x1000];
    char zeros_marker[] = "CLEARED_OK";
    char actual_marker[16];
    memset(&prog, 0, sizeof(prog));
    prog.name = strdup(name);
    snprintf(pkg_dir, sizeof(pkg_dir), "%s/%s", PKG_ROOT, name);
    prog.path_pkg_dir = strdup(pkg_dir);
    snprintf(src_path, sizeof(src_path), "%s/%s/%s", SRC_ROOT, name, name);
    prog.path_binery_in_src = strdup(src_path);
    snprintf(bin_path, sizeof(bin_path), "%s/%s", pkg_dir, name);
    prog.path_binery_for_nix = strdup(bin_path);
    snprintf(nix_path, sizeof(nix_path), "%s/default.nix", pkg_dir);
    prog.path_output_default_nix = strdup(nix_path);
    free_program(&prog);
    if (prog.name == NULL &&
        prog.path_binery_for_nix == NULL &&
        prog.path_binery_in_src == NULL &&
        prog.path_pkg_dir == NULL &&
        prog.path_output_default_nix == NULL) {
        memcpy(actual_marker, zeros_marker, sizeof(zeros_marker));
        } else {
            memcpy(actual_marker, "CLEARED_FAIL", 13);
        }
        test_assert_eq_string(actual_marker, zeros_marker, strlen(zeros_marker));
}

static void test_replace_simple(void)
{
    const unsigned char haystack[] = "hello world";
    const unsigned char old_b[] = "world";
    const unsigned char new_b[] = "there";
    size_t out_len = 0;
    ReplaceArgs args;
    unsigned char *result;
    char actual_buf[64];
    char expected[] = "hello there";
    args.haystack = haystack;
    args.haystack_len = 11;
    args.old = old_b;
    args.old_len = 5;
    args.new_bytes = new_b;
    args.new_len = 5;
    args.out_len = &out_len;
    result = replace_by_split(&args);
    memset(actual_buf, 0, sizeof(actual_buf));
    if (result != NULL) {
        memcpy(actual_buf, result, out_len);
        free(result);
    }
    test_assert_eq_string(actual_buf, expected, strlen(expected));
}

static void test_replace_multiple(void)
{
    const unsigned char haystack[] = "aaXbbXcc";
    const unsigned char old_b[] = "X";
    const unsigned char new_b[] = "YY";
    size_t out_len = 0;
    ReplaceArgs args;
    unsigned char *result;
    char actual_buf[64];
    char expected[] = "aaYYbbYYcc";
    args.haystack = haystack;
    args.haystack_len = 8;
    args.old = old_b;
    args.old_len = 1;
    args.new_bytes = new_b;
    args.new_len = 2;
    args.out_len = &out_len;
    result = replace_by_split(&args);
    memset(actual_buf, 0, sizeof(actual_buf));
    if (result != NULL) {
        memcpy(actual_buf, result, out_len);
        free(result);
    }
    test_assert_eq_string(actual_buf, expected, strlen(expected));
}

static void test_replace_no_match(void)
{
    const unsigned char haystack[] = "abcdef";
    const unsigned char old_b[] = "ZZ";
    const unsigned char new_b[] = "QQ";
    size_t out_len = 0;
    ReplaceArgs args;
    unsigned char *result;
    char actual_buf[64];
    char expected[] = "abcdef";
    args.haystack = haystack;
    args.haystack_len = 6;
    args.old = old_b;
    args.old_len = 2;
    args.new_bytes = new_b;
    args.new_len = 2;
    args.out_len = &out_len;
    result = replace_by_split(&args);
    memset(actual_buf, 0, sizeof(actual_buf));
    if (result != NULL) {
        memcpy(actual_buf, result, out_len);
        free(result);
    }
    test_assert_eq_string(actual_buf, expected, strlen(expected));
}

static void test_replace_empty_old(void)
{
    const unsigned char haystack[] = "keep-me";
    const unsigned char old_b[] = "";
    const unsigned char new_b[] = "IGNORED";
    size_t out_len = 0;
    ReplaceArgs args;
    unsigned char *result;
    char actual_buf[64];
    char expected[] = "keep-me";
    args.haystack = haystack;
    args.haystack_len = 7;
    args.old = old_b;
    args.old_len = 0;
    args.new_bytes = new_b;
    args.new_len = 7;
    args.out_len = &out_len;
    result = replace_by_split(&args);
    memset(actual_buf, 0, sizeof(actual_buf));
    if (result != NULL) {
        memcpy(actual_buf, result, out_len);
        free(result);
    }
    test_assert_eq_string(actual_buf, expected, strlen(expected));
}

static void test_replace_shorter_new(void)
{
    const unsigned char haystack[] = "fooBAR_baz";
    const unsigned char old_b[] = "BAR";
    const unsigned char new_b[] = "x";
    size_t out_len = 0;
    ReplaceArgs args;
    unsigned char *result;
    char actual_buf[64];
    char expected[] = "foox_baz";
    args.haystack = haystack;
    args.haystack_len = 10;
    args.old = old_b;
    args.old_len = 3;
    args.new_bytes = new_b;
    args.new_len = 1;
    args.out_len = &out_len;
    result = replace_by_split(&args);
    memset(actual_buf, 0, sizeof(actual_buf));
    if (result != NULL) {
        memcpy(actual_buf, result, out_len);
        free(result);
    }
    test_assert_eq_string(actual_buf, expected, strlen(expected));
}

static void test_replace_at_boundaries(void)
{
    const unsigned char haystack[] = "XXmiddleXX";
    const unsigned char old_b[] = "XX";
    const unsigned char new_b[] = "Y";
    size_t out_len = 0;
    ReplaceArgs args;
    unsigned char *result;
    char actual_buf[64];
    char expected[] = "YmiddleY";
    args.haystack = haystack;
    args.haystack_len = 10;
    args.old = old_b;
    args.old_len = 2;
    args.new_bytes = new_b;
    args.new_len = 1;
    args.out_len = &out_len;
    result = replace_by_split(&args);
    memset(actual_buf, 0, sizeof(actual_buf));
    if (result != NULL) {
        memcpy(actual_buf, result, out_len);
        free(result);
    }
    test_assert_eq_string(actual_buf, expected, strlen(expected));
}

void run_tests_config_paths(void)
{
    printf("\n%s==== config_paths.c tests ====%s\n", C_BOLD, C_RESET);
    test_free_program_clears_struct();
    test_replace_simple();
    test_replace_multiple();
    test_replace_no_match();
    test_replace_empty_old();
    test_replace_shorter_new();
    test_replace_at_boundaries();
}

#undef SRC_ROOT

#endif
