#include <string.h>
#include <stdbool.h>

#include "tinyconfig.h"

#define GREEN(string) "\033[0;32m"string"\033[0m"
#define RED(string)   "\033[0;31m"string"\033[0m"

#define TEST(test_name, condition)    \
    printf("TEST " test_name "... "); \
    if (condition) printf(GREEN("SUCCESS")"\n"); else printf(RED("FAILED")"\n")

#define STRING_COMPARE(x, y) strcmp(x, y) == 0

void test_config_values(tc_config *config) {
    const char *ip_address = tc_get_value(config, "ip_address");
    TEST("Dot separated numbers", STRING_COMPARE(ip_address, "172.165.10.02"));
    
    const char *number_of_macros = tc_get_value(config, "numberOfMacros");
    TEST("Integer", STRING_COMPARE(number_of_macros, "2"));

    const char *program_safety = tc_get_value(config, "programsafety");
    TEST("One word string", STRING_COMPARE(program_safety, "unsafe"));

    const char *time_to_run = tc_get_value(config, "time_to_run");
    TEST("Dotted float", STRING_COMPARE(time_to_run, ".1"));

    const char *random_float = tc_get_value(config, "random_float");
    TEST("Float number", STRING_COMPARE(random_float, "5.56"));

    const char *code_quality = tc_get_value(config, "code_quality");
    TEST("Negative integer", STRING_COMPARE(code_quality, "-50"));
    
    const char *random_text = tc_get_value(config, "random_text");
    TEST("Text with white spaces", STRING_COMPARE(random_text, "Some whitespaced random text"));

    const char *dotted_text = tc_get_value(config, "dotted_text");
    TEST("Dotted text", STRING_COMPARE(dotted_text, "com.domain.example"));
}

int main(void) {
    printf("INIT TESTS\n");
    printf("\nINIT Config tests\n");

    tc_config config = {};
    bool ret = tc_load_config(&config, "test.conf");
    TEST("tc_load_config success return", ret == true);
    TEST("config->size = 8", config.size == 8);

    // Test changes made on CMake
    TEST("TC_CONFIG_MAX_SIZE = 8", TC_CONFIG_MAX_SIZE == 8);

    // --------------------
    // tc_get_value
    // --------------------
    printf("\nINIT tc_get_value tests\n");
    test_config_values(&config);

    // --------------------
    // tc_save_to_file 
    // --------------------
    printf("\nINIT tc_save_to_file tests\n");
    printf("Save test.conf contents to new test2.conf\n");
    printf("Test if config was reset and correctly re-written to new file\n");
    tc_save_to_file(&config, "test2.conf");
    tc_load_config(&config, "test2.conf");
    test_config_values(&config);

    // --------------------
    // tc_set_value 
    // --------------------
    printf("\nINIT tc_set_value tests\n");

    // Set a new value to an existing one and assert the result.
    tc_set_value(&config, "programsafety", "very_safe");
    const char *new_safety = tc_get_value(&config, "programsafety");
    TEST("raw string very_safe", STRING_COMPARE(new_safety, "very_safe"));

    return 0;
}
