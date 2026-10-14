#ifndef DEPLOY_PKGS_H
#define DEPLOY_PKGS_H

#include "config_paths.h"

int write_buffer_to_file(const unsigned char *buffer, size_t size, const char *filepath);
char *build_overlay_nix(void);
int write_overlay_nix(const char *path, const char *content);
char *build_default_nix(const char *name);
int deploy_package(SystemProgrammStruct *prog);
int run_nix_build(const char *pkg_dir);

#ifdef _TEST_BUILD
void run_tests_deploy_pkgs(void);
#endif

#endif
