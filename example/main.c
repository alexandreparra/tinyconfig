#include <stdio.h>
#include <stdlib.h>

#include "../include/tinyconfig.h"

int main() {
    tc_config *config = NULL;
    int err = tc_load_config(&config, "tiny.conf");
    if (err) {
        printf("Error loading config\n");
        return 1;
    }

    // More unsafe way
    char *player_power = tc_get_value(config, "player_power");
    printf("player_power: %i\n", atoi(player_power));

    // Safer
    char *player_int = tc_get_value(config, "base_attack");
    if (player_int != NULL) {
        printf("base_attack: %f\n", atof(player_int));
    }

    // Negative values:
    char *player_charisma = tc_get_value(config, "player_charisma");
    printf("player_charisma: %i kinda low...\n", atoi(player_charisma));

    // String values are printed normally
    char *player_destination = tc_get_value(config, "player_destination");
    printf("palayer_destination: %s\n", player_destination);

    // Set a value to a certain key, if the value doesn't exist it'll be created.
    // Even if the new value is created or just updated, tc_set_value will return the pointer to the value.
    char *new_player_power = tc_set_value(config, "player_power", "330");
    printf("modified player_power: %i\n", atoi(new_player_power));

    // Create a value that doesn't exist.
    char *player_dex = tc_set_value(config, "player_dex", "50");
    printf("new value player_dex: %i\n", atoi(player_dex));

    // Save the modifications back to a file.
    tc_save_to_file(config, "modified.conf");

    tc_free(config);

    return 0;
}
