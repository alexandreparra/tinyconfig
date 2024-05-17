#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/tinyconfig.h"

// This shows how tinyconfig saves each key-value pair.
void print_config(tc_config *config) {
    for (size_t i = 0; i < config->size; i++) {
        void *current_line = (char *) config->buffer + (TC_LINE_TOTAL_SIZE * i);

        // Read the size_t at the beginning of the line.
        void *key_start = (char *) current_line;
        size_t *line_size = (size_t *) key_start;
        printf("%zi ", *line_size);

        // Read the rest of the line past the TC_HEADER_SIZE (which is just sizeof(size_t))
        char *key_value = (char *) key_start + TC_HEADER_SIZE;
        printf("%s\n", key_value);
    }
}

// Example helper function to parse a boolean value.
bool parse_boolean(char *source) {
    if (strcmp(source, "true") == 0) return true;
    if (strcmp(source, "false") == 0) return false;

    // This may not be the best approach, but defaulting to false is probably safer.
    return false;
}

// Example helper function to get data
int get_int(tc_config *config, const char *key, int default_value) {
    char *value = tc_get_value(config, key);
    if (value == NULL) return default_value;
    return atoi(value);
}

int main() {
    tc_config config = {};
    bool ok = tc_load_config(&config, "tiny.conf");
    if (!ok) {
        printf("Error loading config\n");
        return 1;
    }

    // Getting values from the config
    char *server_ip = tc_get_value(&config, "server_ip");
    printf("server_ip: %s\n", server_ip);

    // Safer
    char *character_name = tc_get_value(&config, "character_name");
    if (character_name != NULL) {
        printf("character_name: %s\n", character_name);
    }

    // Negative values:
    char *player_intelligence = tc_get_value(&config, "char_intelligence");
    printf("char_intelligence: %i (kinda low...)\n", atoi(player_intelligence));

    // An example helper functions to get an int from a value, where a default,
    // fallback value can be provided.
    int base_attack = get_int(&config, "base_attack", 0);
    printf("base_attack with helper function: %i\n", base_attack);

    // You can print every value as they are all null terminated strings.
    char *player_destination = tc_get_value(&config, "player_destination");
    printf("player_destination: %s\n", player_destination);

    char *boolean_example = tc_get_value(&config, "boolean_example");
    bool parsed_bool = parse_boolean(boolean_example);
    // Do whatever with parsed_bool

    // Numbers as keys
    char *one = tc_get_value(&config, "1");
    printf("Value from key 1: %s\n", one);

    // Set a new value to a certain existing key. If the new value overflows TC_LINE_MAX_SIZE or 
    // the provided key to update doesn't exist, tc_set_value will return NULL. If the operation is 
    // successful a valid void * will be returned.
    char *char_power = tc_set_value(&config, "char_power", "330");
    printf("modified char_power: %i\n", atoi(char_power));

    // Save the modifications back to a file.
    tc_save_to_file(&config, "modified.conf");

    printf("\nConfig layout:\n");
    print_config(&config);

    return 0;
}
