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

#define C_RESET   "\033[0m"
#define C_RED     "\033[31m"
#define C_GREEN   "\033[32m"
#define C_YELLOW  "\033[33m"
#define C_BOLD    "\033[1m"

static int counter_failed_tests = 0;

/**
 * actual   = Rückgabewert der zu testenden Funktion
 * expected = Sollwert
 * len      = compare offset
 */
static inline void test_assert_eq_string(char *actual, char *expected, size_t len)
{
    if (actual == NULL || expected == NULL) {
        fprintf(stderr, C_RED "assert: NULL params" C_RESET "\n");
    }
    int result_compare = 0;
    char message_buffer[0x64];
    size_t i = 0;
    for (i = 0; i < len; ++i) {
        result_compare = result_compare + (actual[i] ^ expected[i]);
        if (i < 0x63) {
            message_buffer[i] = actual[i];
        }
    }
    size_t null_terminator_index;
    if (i < 0x63) {
        null_terminator_index = i;
    } else {
        null_terminator_index = 0x63;
    }
    message_buffer[null_terminator_index] = '\0';
    if (result_compare == 0) {
        printf("  %s[OK]%s  #%s . . . \n", C_GREEN, C_RESET, message_buffer);
    } else {
        printf("  %s[FAIL]%s #%s\n", C_RED, C_RESET, actual);
        printf("  actual=%s expected=%s\n", actual, expected);
        counter_failed_tests += 1;
    }
}

static inline void test_assert_eq_int(int actual, int expected, const char *label)
{
    if (actual == expected) {
        printf("  %s[OK]%s  #%s . . . \n", C_GREEN, C_RESET, label ? label : "");
    } else {
        printf("  %s[FAIL]%s #%s\n", C_RED, C_RESET, label ? label : "");
        printf("  actual=%d expected=%d\n", actual, expected);
        counter_failed_tests += 1;
    }
}

/**
 * Am Ende: Gesamstatus
 * @return not 0: CI/make failed
 */
static inline void print_test_summary()
{
    printf("\n%s======== TEST SUMMARY ========%s\n", C_BOLD, C_RESET);
    if (counter_failed_tests == 0) {
        printf("%sAlles okay: %d Test(s) fehlgeschlagen.%s\n", C_GREEN, 0, C_RESET);
    } else {
        printf("%sEs gab Fehler %d Test(s) fehlgeschlagen.%s\n", C_RED, counter_failed_tests, C_RESET);
    }
}

#endif
#endif
