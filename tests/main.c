#include <string.h>

#include "tinyconfig.h"

#define GREEN(string) "\033[0;32m"string"\033[0m"
#define RED(string)   "\033[0;31m"string"\033[0m"

#define TEST(test_name, condition)    \
    printf("TEST " test_name "... "); \
    if (condition) printf(GREEN("SUCCESS")"\n"); else printf(RED("FAILED")"\n")

int main(void) {
    printf("INIT TESTS\n");
    printf("\nINIT Config tests\n");

    tc_config config = {};
    bool ret = tc_load_config(&config, "test.conf");
    TEST("tc_load_config success return", ret == true);
    TEST("config->size = 6", config.size == 6);
    TEST("TC_CONFIG_MAX_SIZE = 6", TC_CONFIG_MAX_SIZE == 6);

    // --------------------
    // Check value parsing.
    printf("\nINIT tc_get_value tests\n");
    const char * file_name = tc_get_value(&config, "file_name");
    TEST("string main.c", strcmp(file_name, "main.c") == 0);
    
    const char * number_of_macros = tc_get_value(&config, "numberOfMacros");
    TEST("int 2", strcmp(number_of_macros, "2") == 0);

    const char * program_safety = tc_get_value(&config, "programsafety");
    TEST("raw string unsafe", strcmp(program_safety, "unsafe") == 0);

    const char * time_to_run = tc_get_value(&config, "time_to_run");
    TEST("dot float .1", strcmp(time_to_run, ".1") == 0);

    const char * random_float = tc_get_value(&config, "random_float");
    TEST("float 5.56", strcmp(random_float, "5.56") == 0);

    const char *code_quality = tc_get_value(&config, "code_quality");
    TEST("negative int -50", strcmp(code_quality, "-50") == 0);

    const char *empty_line = tc_get_value(&config, "");
    TEST("emtpy string", empty_line == NULL);

    // --------------------
    printf("\nINIT tc_set_value tests\n");

    // Set a new value to an existing one and assert the result.
    tc_set_value(&config, "programsafety", "very_safe");
    const char *new_safety = tc_get_value(&config, "programsafety");
    TEST("raw string very_safe", strcmp(new_safety, "very_safe") == 0);

    return 0;
}
