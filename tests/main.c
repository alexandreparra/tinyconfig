#include <assert.h>
#include <string.h>

#include "tinyconfig.h"

#define GREEN(string) "\033[0;32m"string"\033[0m"
#define RED(string)   "\033[0;31m"string"\033[0m"

#define TEST(test_name, condition)    \
    printf("TEST " test_name "... "); \
    if (condition) printf(GREEN("SUCCESS")"\n"); else printf(RED("FAILED")"\n")

#define BIG_STR "a_very_big_string_with_at_least_more_than_fifty_characters_to_force_reallocation_of_a_line" 
#define HUGE_STR "a_huge_string_with_at_least_double_the_size_of_the_default_line_allocation_size_to_test_the_line_size_allocation_size"

int main(void) {
    tc_config * config = NULL;
    tc_load_config(&config, "test.conf");
    // Check initial config size.
    TEST("config->size", config->size == 6);

    // Check value parsing.
    const char * file_name = tc_get_value(config, "file_name");
    TEST("tc_get_value string \"test.conf\"", strcmp(file_name, "main.c") == 0);
    
    const char * number_of_macros = tc_get_value(config, "numberOfMacros");
    TEST("tc_get_value int 2", strcmp(number_of_macros, "2") == 0);

    const char * program_safety = tc_get_value(config, "programsafety");
    TEST("tc_get_value raw string unsafe", strcmp(program_safety, "unsafe") == 0);

    const char * time_to_run = tc_get_value(config, "time_to_run");
    TEST("tc_get_value dot float .1", strcmp(time_to_run, ".1") == 0);

    const char * random_float = tc_get_value(config, "random_float");
    TEST("tc_get_value float 5.56", strcmp(random_float, "5.56") == 0);

    const char * code_quality = tc_get_value(config, "code_quality");
    TEST("tc_get_value negative int -50", strcmp(code_quality, "-50") == 0);

    // Assert that the parser doesn't create an empty line.
    const char * empty_line = tc_get_value(config, "");
    TEST("tc_get_value emtpy string", empty_line == NULL);

    // Assert changes in TC_CONFIG_DEFAULT_SIZE using the build system.
    TEST("TC_CONFIG_DEFAULT_SIZE", config->capacity == 6);

    // Force a config realloc adding an extra value and assert grow size rule.
    tc_set_value(config, "overflow", "true");
    TEST("realloc config size", config->capacity == 12);

    // Set a new value to an existing one and assert the result.
    tc_set_value(config, "programsafety", "very_safe");
    const char * new_safety = tc_get_value(config, "programsafety");
    TEST("tc_set_value raw string very_safe", strcmp(new_safety, "very_safe") == 0);

    // Create a new value that has more characters then the default TC_CONFIG_DEFAULT_LINE_SIZE.
    tc_set_value(config, "big_string", BIG_STR);
    const char * bigass_str = tc_get_value(config, "big_string");
    TEST("tc_set_value new value with big string (len > 50)", strcmp(bigass_str, BIG_STR) == 0);

    // Set a new value to an existing one, having the new line size being bigger than TC_CONFIG_DEFAULT_LINE_SIZE.
    tc_set_value(config, "file_name", BIG_STR);
    const char * bigass_str_2 = tc_get_value(config, "file_name");
    TEST("tc_set_value change existing value with big string (len > 50)", strcmp(bigass_str_2, BIG_STR) == 0);   

    // Create a new value that has more characters than (2 * TC_CONFIG_DEFAULT_LINE_SIZE).
    tc_set_value(config, "huge_string", HUGE_STR);
    const char * huge_str = tc_get_value(config, "huge_string");
    TEST("tc_set_value new value with huge string (len > 100)", strcmp(huge_str, HUGE_STR) == 0);

    // Set a new value to an existing one, having the line size being bigger than (2 * TC_CONFIG_DEFAULT_LINE_SIZE).
    tc_set_value(config, "programsafety", HUGE_STR);
    const char * huge_str_2 = tc_get_value(config, "programsafety");
    TEST("tc_set_value change existing value with huge string (len > 100)", strcmp(huge_str_2, HUGE_STR) == 0);

    return 0;
}
