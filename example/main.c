#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/tinyconfig.h"

// This shows how tinyconfig saves each key-value pair.
void print_config(tc_config *config) {
    for (size_t i = 0; i < config->size; i++) {
        void *current_location = &config->buffer[TC_LINE_TOTAL_SIZE * i];
        size_t *header_size = (size_t *) current_location;
        printf("%zi ", *header_size);
        char *line_location = (char *) current_location + TC_HEADER_SIZE;
        printf("%s\n", line_location);
    }
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

    // More unsafe way
    char *player_power = tc_get_value(&config, "player_power");
    printf("player_power: %i\n", atoi(player_power));

    // Safer
    char *player_int = tc_get_value(&config, "base_attack");
    if (player_int != NULL) {
        printf("base_attack: %f\n", atof(player_int));
    }

    // Negative values:
    char *player_charisma = tc_get_value(&config, "player_charisma");
    printf("player_charisma: %i kinda low...\n", atoi(player_charisma));

    // Use helper functions to get ints
    int player_pow = get_int(&config, "player_power", 0);
    printf("player_power with helper function: %i", player_pow);

    // You can print every value as they are all null terminated strings.
    char *player_destination = tc_get_value(&config, "player_destination");
    printf("player_destination: %s\n", player_destination);

    // Set a value to a certain an already existing key.
    // Even if the new value is created or just updated,
    // tc_set_value will return the pointer to the value.
    char *new_player_power = tc_set_value(&config, "player_power", "330");
    printf("modified player_power: %i\n", atoi(new_player_power));

    // Save the modifications back to a file.
    tc_save_to_file(&config, "modified.conf");

    printf("\nConfig layout:\n");
    print_config(&config);

    return 0;
}
