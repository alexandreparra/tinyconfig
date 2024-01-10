### tinyconfig

tinyconfig is a simple key-value pair configuration file that can hold
numbers (integers and floats) and strings.

The library name was inspired by [tinyxml2](https://github.com/leethomason/tinyxml2)

A simple config example:
```txt
# Line comments
project_name=tinyconfig
version=2.0
number_of_versions=6

# Numbers can be used as keys
42=key_number 

# You can use whitepaces before and after equals =
example = 34

# Strings are also accepted
entry_point="int main();"
```

### Using tinyconfig on your project
Tiny config is a simple header and source file that can be called from any
build system easily, you can pull this repository or simply copy the source/header file from
`src/tinyconfig.c` and `include/tinyconfig.h`.

If you're using CMake, you can simply pull this repository inside your project and configure your CMakeList.txt 
file like the following:
```cmake
add_subdirectory(path_to_tinyconfig/)
target_link_libraries(your_executable tinyconfig)
target_include_directories(your_executable path_to_tinyconfig/include/)
```

**You can create your on wrappers easily too.**

### How to use
#### C
Alongside this snippet, the [example](/example) folder contains a working example (that uses all functions) with a cmake build.

```c
#include "tinyconfig.h"

int main() {
    // Create the tc_config struct that is needed for all operations when using tinyconfig.
    tc_config *config = NULL;
    int err = tc_load_config(&config, "tiny.conf");
    // Loading the config can fail when opening/reading the file or scanning it, always check for errors.
    if (err) {
        printf("Error loading config\n");
        return 1;
    }
    
    // You can now retrieve values from the config file.
    // You need to bring in your own methods of conversion.
    // More unsafe way:
    char *player_power = tc_get_value(config, "player_power");
    printf("player_power: %i\n", atoi(player_power));

    // Safer:
    char *player_int = tc_get_value(config, "base_attack");
    if (player_int != NULL) {
        printf("base_attack: %f\n", atof(player_int));
    }
    
    // You can create new values if they don't exist or just update already existing ones:
    char *player_dex = tc_set_value(config, "player_dex", "14");
    
    // Now if you need you can persist the changes back to a file.
    int err = tc_save_to_file(config, "modified.conf");
    if (err) {
        // Handle possible error while saving the file.
    }
    
    // Don't forget to free the config structure.
    tc_free(config);

    return 0;
}
```

### Flags
You can manually define the initial config allocation size, line allocation size, (these can be useful 
to avoid reallocation's if your config has a constant size, check `tinyconfig.c` for the default values) 
and enable debug logs. 

A CMake example that will change the value or just set the flags:
```cmake
add_compile_definitions(TC_CONFIG_DEFAULT_SIZE=10)
add_compile_definitions(TC_CONFIG_DEFAULT_LINE_SIZE=100)
add_compile_definitions(TC_DEBUG_LOGS)
```

The available flags are:

| Flag name                    | Description                                                                                |
|------------------------------|--------------------------------------------------------------------------------------------|
| TC_CONFIG_DEFAULT_SIZE       | The initial config array size used to store each line in the configuration file            |
| TC_CONFIG_DEFAULT_LINE_SIZE  | The initial line buffer size used to store the key-value from the configuration file       |
| TC_DEBUG_LOGS                | Activate logs that print debug information like reallocation, line added and config status |

### Caveats
When using the function `tc_save_to_file`, all the comments and spaces present on the original file will vanish, because 
they are naturally ignored by the lexer (described at [Lexer rules](#lexer-rules)).

tinyconfig doesn't check for duplicates, which means that if you use `tc_get_value` with a key that is duplicated inside
the config, it will return the first value it encounters inside the config.

Even though the string type is provided, tinyconfig is best suited for small use cases, one could think of using it for
translation, but there are better solutions specifically for that.

Be careful while storing numbers, a big number stored as a string may overflow when converted to an improper numeric type.

### Valid types
There are only 4 valid types that tinyconfig can work with.

**Raw strings** don't need to be surrounded by quotes "", the only separator character is an underscore '\_'.

Examples: CamelCase, snake\_case, compactname

**Strings** are surrounded by double quotes "" and can contain any value inside it, spaces, numbers, special characters and so on.

Examples: "string with spaces", "symbols: !@#$%", "1+1=2"

**Integers** can be negative or not.

Examples: -1, 0, 42, -50, 100.

**Floats** are numbers that can contain a dot '.' to represent their decimal values, they can start with a dot '.',
can be negative or not.

Examples: 10.5, 1.2, .556, -5.5

### Lexer rules
Each key-value pair is evaluated by line, one line means one key and one value, the delimiter to define what is a key and
what is a value is the equal sign '='.

**Keys** can be raw strings or positive integers.

**Values** can be of all valid types (raw strings, strings, integers, floats).

**Comments:**
- A # represents a line comment (the only available in tinyconfig).
- Everything in front of the # will be ignored until a new line is found or end of file.

**Whitespaces:**
- All whitespaces are ignored unless inside a string type.

**Errors:**
- If tinyconfig detects some inconsistency on a line, it'll completely ignore that line and skip to the next.
