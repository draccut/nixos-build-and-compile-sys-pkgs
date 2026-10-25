#include "deploy_pkgs.h"

/**
 * Ersetzt: manuelles mkdir -p
 * +
 * speichern einer Datei (z.B. default.nix) unter filepath. Gespeichert wird der vec: buffer
 */
int write_buffer_to_file(const unsigned char *buffer, size_t size, const char *filepath)
{
    char path_copy[0x1000];
    char *dir;
    FILE *f;
    if (buffer == NULL || filepath == NULL) return -1;
    snprintf(path_copy, sizeof(path_copy), "%s", filepath);
    dir = dirname(path_copy);
    if (mkdir_p(dir) != 0) {
        fprintf(stderr, "mkdir_p für '%s': %s\n", filepath, strerror(errno));
        return -1;
    }
    f = fopen(filepath, "wb");
    if (f == NULL) {
        fprintf(stderr, "fopen dest '%s': %s\n", filepath, strerror(errno));
        return -1;
    }
    if (fwrite(buffer, 1, size, f) != size) {
        fprintf(stderr, "fwrite '%s': %s\n", filepath, strerror(errno));
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

/**
 * Ersetzt: manuelles Schreiben von overlay.nix
 */
char *build_overlay_nix(void)
{
    const char *header = "self: super: {\n";
    const char *footer = "}\n";
    const char *prefix = "  ";
    const char *call = " = super.callPackage /etc/nixos/pkgs/";
    const char *suffix = " {};\n";
    size_t total_len = 0;
    total_len += strlen(header);
    total_len += strlen(footer);
    for (size_t i = 0; i < NROF; ++i) {
        total_len += strlen(prefix);
        total_len += strlen(PKG_NAMES[i]);
        total_len += strlen(call);
        total_len += strlen(PKG_NAMES[i]);
        total_len += strlen(suffix);
    }
    char *out = malloc(total_len + 1);
    if (!out) return NULL;
    size_t pos = 0;
    pos += sprintf(out + pos, "%s", header);
    for (size_t i = 0; i < NROF; ++i) {
        pos += sprintf(out + pos,
                       "%s%s%s%s%s",
                       prefix,
                       PKG_NAMES[i],
                       call,
                       PKG_NAMES[i],
                       suffix);
    }
    sprintf(out + pos, "%s", footer);
    return out;
}

/**
 * Manuelles Anlegen/Überschreiben der Datei unter path.
 */
int write_overlay_nix(const char *path, const char *content)
{
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fputs(content, f);
    fclose(f);
    return 0;
}

/**
 * Was die Sammlung an Projekten für das System ist oder das bekanntmachen in den pkgs Packeten.
 * Mit einer Liste.
 */
char *build_default_nix(const char *name)
{
	#define TEMPLATE_NIX "" \
		"{ pkgs ? import <nixpkgs> {} }:\n" \
		"\n" \
		"pkgs.stdenv.mkDerivation {\n" \
		"  pname = \"QQQQ\";\n" \
		"  version = \"1.0\";\n" \
		"\n" \
		"  # Wenn die Binärdatei lokal liegt, referenziere sie relativ oder absolut\n" \
		"  src = /etc/nixos/pkgs/QQQQ/QQQQ;\n" \
		"\n" \
		"  # Hook, damit Nix patchelf automatisch anwendet (wichtig für ELF-Binaries)\n" \
		"  nativeBuildInputs = [ pkgs.autoPatchelfHook ];\n" \
		"\n" \
		"  # Nur Install-Phase nötig: kopiere die Binärdatei nach $out/bin\n" \
		"  phases = [ \"installPhase\" ];\n" \
		"  installPhase = ''\n" \
		"    mkdir -p $out/bin\n" \
		"    cp -a \"$src\" $out/bin/QQQQ\n" \
		"    chmod +x $out/bin/QQQQ\n" \
		"  '';\n" \
		"\n" \
		"  meta = with pkgs.lib; {\n" \
		"    description = \"binary packaged for NixOS\";\n" \
		"    license = licenses.mit; # oder licenses.mit / other, je nach Lizenz\n" \
		"    platforms = [ \"x86_64-linux\" ];\n" \
		"  };\n" \
		"}\n\n"
    const char *REPLACE_THIS = "QQQQ";
    char *templ = strdup(TEMPLATE_NIX);
    if (!templ) return NULL;
    size_t templ_len = strlen(templ);
    size_t buffer_len = templ_len + 1;
    unsigned char *buffer = malloc(buffer_len);
    if (!buffer) { free(templ); return NULL; }
    memcpy(buffer, templ, buffer_len);
    const unsigned char *old_bytes = (const unsigned char *)REPLACE_THIS;
    size_t old_len = strlen(REPLACE_THIS);
    const unsigned char *new_bytes = (const unsigned char *)name;
    size_t new_len = strlen(name);
    size_t new_buffer_len = 0;
    ReplaceArgs args = {
        .haystack = buffer,
        .haystack_len = templ_len,
        .old = old_bytes,
        .old_len = old_len,
        .new_bytes = new_bytes,
        .new_len = new_len,
        .out_len = &new_buffer_len
    };
    unsigned char *new_buffer = replace_by_split(&args);
    if (!new_buffer) {
        free(buffer);
        free(templ);
        return NULL;
    }
    char *res = malloc(new_buffer_len + 1);
    if (!res) {
        free(new_buffer);
        free(buffer);
        free(templ);
        return NULL;
    }
    memcpy(res, new_buffer, new_buffer_len);
    res[new_buffer_len] = '\0';
    free(new_buffer);
    free(buffer);
    free(templ);
    #undef TEMPLATE_NIX
    return res;
}

/**
 * Ordner anlegen, Binary kopieren, default.nix schreiben, +x setzen.
 */
int deploy_package(SystemProgrammStruct *prog)
{
    char *nix_content;
    size_t nix_len;
    int rc = 0;
    if (prog == NULL) return -1;
    printf("==> Deploy %s\n", prog->name);
    printf("    src : %s\n", prog->path_binery_in_src);
    printf("    dest: %s\n", prog->path_binery_for_nix);
    printf("    nix : %s\n", prog->path_output_default_nix);
    if (mkdir_p(prog->path_pkg_dir) != 0) {
        fprintf(stderr, "mkdir_p '%s': %s\n", prog->path_pkg_dir, strerror(errno));
        return -1;
    }
    printf("    copy %s -> %s\n", prog->path_binery_in_src, prog->path_binery_for_nix);
    if (copy_binary_file(prog->path_binery_in_src, prog->path_binery_for_nix) != 0) {
        fprintf(stderr, "copy_binary_file failed for %s\n", prog->name);
        rc = -1;
    } else if (chmod(prog->path_binery_for_nix, 0755) != 0) {
        fprintf(stderr, "chmod '%s': %s\n", prog->path_binery_for_nix, strerror(errno));
        rc = -1;
    }
    nix_content = build_default_nix(prog->name);
    if (nix_content == NULL) {
        fprintf(stderr, "build_default_nix failed for %s\n", prog->name);
        return -1;
    }
    nix_len = strlen(nix_content);
    printf("    write %s\n", prog->path_output_default_nix);
    if (write_buffer_to_file((unsigned char *)nix_content, nix_len,
                             prog->path_output_default_nix) != 0) {
        fprintf(stderr, "write default.nix failed for %s\n", prog->name);
        free(nix_content);
        return -1;
    }
    free(nix_content);
    return rc;
}

/**
 * Exportiere die Binärdatei in den für den
 * Userspace unveränderbaren Bereich
 */
int run_nix_build(const char *pkg_dir)
{
    pid_t pid;
    int status;
    if (pkg_dir == NULL) return -1;
    printf("    nix-build in %s\n", pkg_dir);
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork: %s\n", strerror(errno));
        return -1;
    }
    if (pid == 0) {
        if (chdir(pkg_dir) != 0) {
            fprintf(stderr, "chdir '%s': %s\n", pkg_dir, strerror(errno));
            _exit(127);
        }
        execlp("nix-build", "nix-build", "default.nix", (char *)NULL);
        fprintf(stderr, "execlp nix-build: %s\n", strerror(errno));
        _exit(127);
    }
    if (waitpid(pid, &status, 0) < 0) {
        fprintf(stderr, "waitpid: %s\n", strerror(errno));
        return -1;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "nix-build failed in %s (status=%d)\n",
                pkg_dir, WIFEXITED(status) ? WEXITSTATUS(status) : -1);
        return -1;
    }
    printf("    nix-build OK: %s\n", pkg_dir);
    return 0;
}


#ifdef _TEST_BUILD

/**
 * Prüft, dass build_overlay_nix Header, eine callPackage-Zeile pro PKG_NAME
 * und den Footer exakt aus den Test-Paketnamen zusammenbaut.
 */
static void test_build_overlay_nix(void)
{
    char *actual;
    char *expected;
    size_t len;
    actual = build_overlay_nix();
    expected =
    "self: super: {\n"
    "  TEST_RUST_BINERY_1 = super.callPackage /etc/nixos/pkgs/TEST_RUST_BINERY_1 {};\n"
    "  TEST_C_BINERY_1 = super.callPackage /etc/nixos/pkgs/TEST_C_BINERY_1 {};\n"
    "  AAAAAAAAAAA = super.callPackage /etc/nixos/pkgs/AAAAAAAAAAA {};\n"
    "  BBBBBBBBBBBB = super.callPackage /etc/nixos/pkgs/BBBBBBBBBBBB {};\n"
    "}\n";
    len = strlen(expected);
    test_assert_eq_string(actual, expected, len);
    free(actual);
}

/**
 * Prüft, dass build_default_nix alle QQQQ-Platzhalter durch den Namen aus dem
 * per init_program aufgebauten SystemProgrammStruct ersetzt; free_program gibt
 * die Struct-Allokationen wieder frei.
 */
static void test_build_default_nix(void)
{
    SystemProgrammStruct prog;
    char *actual;
    char *expected;
    size_t len;
    int init_rc;
    init_rc = init_program(&prog, "TEST_C_BINERY_1", 0);
    actual = build_default_nix(prog.name);
    expected =
    "{ pkgs ? import <nixpkgs> {} }:\n"
    "\n"
    "pkgs.stdenv.mkDerivation {\n"
    "  pname = \"TEST_C_BINERY_1\";\n"
    "  version = \"1.0\";\n"
    "\n"
    "  # Wenn die Binärdatei lokal liegt, referenziere sie relativ oder absolut\n"
    "  src = /etc/nixos/pkgs/TEST_C_BINERY_1/TEST_C_BINERY_1;\n"
    "\n"
    "  # Hook, damit Nix patchelf automatisch anwendet (wichtig für ELF-Binaries)\n"
    "  nativeBuildInputs = [ pkgs.autoPatchelfHook ];\n"
    "\n"
    "  # Nur Install-Phase nötig: kopiere die Binärdatei nach $out/bin\n"
    "  phases = [ \"installPhase\" ];\n"
    "  installPhase = ''\n"
    "    mkdir -p $out/bin\n"
    "    cp -a \"$src\" $out/bin/TEST_C_BINERY_1\n"
    "    chmod +x $out/bin/TEST_C_BINERY_1\n"
    "  '';\n"
    "\n"
    "  meta = with pkgs.lib; {\n"
    "    description = \"binary packaged for NixOS\";\n"
    "    license = licenses.mit; # oder licenses.mit / other, je nach Lizenz\n"
    "    platforms = [ \"x86_64-linux\" ];\n"
    "  };\n"
    "}\n\n";
    len = strlen(expected);
    (void)init_rc;
    test_assert_eq_string(actual, expected, len);
    free(actual);
    free_program(&prog);
}

/**
 * Zusätzlicher build_default_nix-Lauf mit anderem Paketnamen (Rust-Testbinär),
 * inkl. init_program/free_program für Struct-Aufbau und Freigabe.
 */
static void test_build_default_nix_rust_name(void)
{
    SystemProgrammStruct prog;
    char *actual;
    char *expected;
    size_t len;
    int init_rc;
    init_rc = init_program(&prog, "TEST_RUST_BINERY_1", 0);
    actual = build_default_nix(prog.name);
    expected =
    "{ pkgs ? import <nixpkgs> {} }:\n"
    "\n"
    "pkgs.stdenv.mkDerivation {\n"
    "  pname = \"TEST_RUST_BINERY_1\";\n"
    "  version = \"1.0\";\n"
    "\n"
    "  # Wenn die Binärdatei lokal liegt, referenziere sie relativ oder absolut\n"
    "  src = /etc/nixos/pkgs/TEST_RUST_BINERY_1/TEST_RUST_BINERY_1;\n"
    "\n"
    "  # Hook, damit Nix patchelf automatisch anwendet (wichtig für ELF-Binaries)\n"
    "  nativeBuildInputs = [ pkgs.autoPatchelfHook ];\n"
    "\n"
    "  # Nur Install-Phase nötig: kopiere die Binärdatei nach $out/bin\n"
    "  phases = [ \"installPhase\" ];\n"
    "  installPhase = ''\n"
    "    mkdir -p $out/bin\n"
    "    cp -a \"$src\" $out/bin/TEST_RUST_BINERY_1\n"
    "    chmod +x $out/bin/TEST_RUST_BINERY_1\n"
    "  '';\n"
    "\n"
    "  meta = with pkgs.lib; {\n"
    "    description = \"binary packaged for NixOS\";\n"
    "    license = licenses.mit; # oder licenses.mit / other, je nach Lizenz\n"
    "    platforms = [ \"x86_64-linux\" ];\n"
    "  };\n"
    "}\n\n";
    len = strlen(expected);
    (void)init_rc;
    test_assert_eq_string(actual, expected, len);
    free(actual);
    free_program(&prog);
}

void run_tests_deploy_pkgs(void)
{
    printf("\n%s======== TEST deploy_pkgs.c ========%s\n", C_BOLD, C_RESET);
    test_build_overlay_nix();
    test_build_default_nix();
    test_build_default_nix_rust_name();
}

#endif
