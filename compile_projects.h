#ifndef COMPILE_PROJECTS_H
#define COMPILE_PROJECTS_H

#include "config_paths.h"

int run_compile_commands(const char *binery_name, const char *project_dir, const int lang_id);
int clear_compile_commands(const char *binery_name, const char *project_dir, const int lang_id);

#ifdef _TEST_BUILD
void run_tests_compile_projects();
#endif

#endif
