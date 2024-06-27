### tinyconfig

tinyconfig is a minimal `ini` like configuration file with only key-value pairs. The file specification
is suited for basically all use cases, as it can be used as a read-only configuration, or a dynamic
configuration that can update values on the file.

Internally, tinyconfig uses only static allocations to store the key-value pairs, meaning that its 
size is predictable and can be altered (see [Flags](#flags)) for maximum efficiency.

The library name was inspired by [tinyxml2](https://github.com/leethomason/tinyxml2)

## Simple example
```txt
# This is a line comment
project_name=tinyconfig
version=2.0
number_of_versions=6

# You can use whitepaces before and after equals =
whitespace = 34

# Strings and numbers are flexible
ip_address = 172.165.10.1
some_string = Just random text
domain_name = com.example.domain
```

### Using tinyconfig on your project
tinyconfig is a simple header and source file that can be included on any build system easily, or
just copied inside your project via download or just cloning this repository inside your project.
The two files you'll are `src/tinyconfig.c` and `include/tinyconfig.h`.

If you're using CMake, you can simply pull this repository inside your project and configure your 
CMakeList.txt 
file like the following:
```cmake
add_subdirectory(path_to_tinyconfig/)
target_link_libraries(your_executable tinyconfig)
target_include_directories(your_executable path_to_tinyconfig/include/)
```

**You can create your on wrappers easily too, provided you have C interop in your language of choice.**

### How to use
For a C example, head to the [example](/example) folder that contains a fully working example and 
some example utilities that you may want to use alongside tinyconfig.

### Flags
You can manually define the config maximum size and line maximum size, these are used to determine
the static buffer size on compile time.

A CMake example that will change the value or just set the flags:
```cmake
add_compile_definitions(TC_CONFIG_MAX_SIZE=10)
add_compile_definitions(TC_MAX_LINE_SIZE=100)
```

The available flags are:

| Flag name          | Description                                                                               |
|--------------------|-------------------------------------------------------------------------------------------|
| TC_LINE_MAX_SIZE   | The maximum line buffer size used to store the key-value pair from the configuration file |
| TC_CONFIG_MAX_SIZE | The maximum lines that can be stored in the configuration file                            |

When setting a different `TC_LINE_MAX_SIZE`, prefer setting the numbers to **powers of two**, so 
that it can be correctly aligned in memory.

### Caveats
When using the function `tc_save_to_file`, all the comments and spaces present on the original file 
will vanish, as they are naturally ignored by the lexer (described at [Lexer rules](#Lexer)).

tinyconfig doesn't check for duplicates, which means that if you use `tc_get_value` with a key that 
is duplicated inside the config, it will return the first value encountered.

Be careful while storing numbers, a big number stored as a string may overflow when converted to an 
improper numeric type.

### Lexer
Each key-value pair is evaluated by line, one line means one key and one value, the delimiter to 
define what is a key and what is a value, is the equal sign '='.

**Keys** can be underscore separated strings like: `some_key`, normal non-spaced strings like:
`SomeKey`, or just an integer like: `1`. Notice that keys are **case-sensitive**, so a key `someKey` 
is different from `SomeKey`.

**Values** can start with any alphanumeric character, dot '.', or hyphen '-', some examples:
- Integers: `1`, `100`, `-5`.
- Floats: `5.56`, `.5`, `-10.2`
- Strings: `Some string`, `my.domain.example`, `underscore_separated`

After reading the first valid character on the value, tinyconfig accepts any symbols, spaces, and
all valid characters until it finds an end of line '\n' or end of file (EOF). 

Whitespaces at the beginning and end are trimmed out of the value.

**Comments:**
- A # represents a line comment (the only available in tinyconfig).
- Everything in front of the # will be ignored until a new line is found or an end of file.

**Whitespaces:**
- All whitespaces are ignored when reading the key, equal sign, and before reading a value.
- When a value string is being read, all whitespaces are respected, when the program encounters an
  ending delimiter (such as `\n` or EOF), the value reading is completed, and we trim extra whitespaces  
  at the end of the string.

**Errors:**
- If tinyconfig detects some inconsistency on a line, it'll report an error on standard error and
  finish the execution of the program, returning false from `tc_load_config`.
