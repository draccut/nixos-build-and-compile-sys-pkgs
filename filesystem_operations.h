#ifndef COPY_FILE_H
#define COPY_FILE_H

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <libgen.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <limits.h>

int mkdir_p(const char *path);
int copy_binary_file(const char *src_path, const char *dst_path);
int check_file_exists(const char *full_path);

#ifdef _TEST_BUILD

#include "test_functions.h"

#endif
#endif
