// Copyright 2023-2024 Alexandre Parra
// MIT License
// tinyconfig.

/*
    tinyconfig was created to solve most common usages of configuration files, with only 4 functions
    and one struct, it can be used in many scenarios, for example:
      1. As a read only configuration file for servers. You may want to document what each key=value
    pair actually do and their default values. In a read only configuration you can comment as much
    as you need and tinyconfig will maintain your program structure unchanged.
      2. As a dynamic configuration file that can read and update the file. For example, if you
    have a game that sets the window height and width by reading a file, and also provides settings
    so that the player change the resolution while playing, you can update the value on the config
    struct and save them back to the files. Notice that this removes all comments.

    Different usages comes with different drawbacks, but in general, read only files are the most
    useful.

Lexer:
    tinyconfig has a simple lexer, basically we have:
    - Line comments.
    - Numbers in the form of ints and floats.
    - Strings, which can be underscore '_' separated for keys or normal space separated strings for 
      values.
    
    Keys are case-sensitive, if you create two keys like the following: `SomeKey` and `somekey`
    tinyconfig is able to distinguish between them.
    
    Values need to start with an alphanumeric character, all other special characters are accepted
    inside the value until a new line is found. Basically this is valid '123&' but this is not
    '&123'.

Memory model:
    tinyconfig uses a static allocation to store values. It saves the whole line from the 
    configuration file alongside a header:

    header            line
    |-----------------|-------------------------------------|
    | 13              | player_power=5                      |
    |-----------------|-------------------------------------|
    ^                 ^
    offset (size_t)   configuration line (char *)

    The offset (stored in the header) represents the exact point in which the value starts, just
    after the '=' sign in the line, this is saved so that we can access the value easily. Each line
    is always null terminated meaning that you can print it in C with a simple printf("%s").

Hot reload:
    You can easily achieve hot reload in tinyconfig by running tc_load_config again. Two simple
    methods to implement hot reload are:
      1. Check the file stat every time and use tc_load_config to reload the configurations.
      2. Create a custom command to reload the file on demand.
*/

#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef NDEBUG
#include <time.h>
#endif

#include <tinyconfig.h>

//---------------------------------------------------------------------------
// #define
//---------------------------------------------------------------------------

#define internal static

// Modified #define names, originally from tinyxml2
// https://github.com/leethomason/tinyxml2
#if defined(_WIN64)
    #define TC_FSEEK _fseeki64
    #define TC_FTELL _ftelli64
#elif defined(__unix__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) \
   || defined(__NetBSD__) || defined(__DragonFly__) || (__CYGWIN__)
    #define TC_FSEEK fseeko
	#define TC_FTELL ftello
#else
	#define TC_FSEEK fseek
	#define TC_FTELL ftell
#endif

#define ERROR_REPORT(string, args...) fprintf(        \
        stderr,                                       \
        "\033[0;31m tinyconfig: " string "\033[0m\n", \
        args)

//---------------------------------------------------------------------------
// Util
//---------------------------------------------------------------------------

internal bool open_file(FILE** fp, const char* file_path, const char* mode)
{
#ifdef _MSC_VER
    int err = fopen_s(fp, file_path, mode);
    if (err) return false;
#else
    *fp = fopen(file_path, mode);
#endif

    if (*fp == NULL || ferror(*fp))
        return false;

    return true;
}

//---------------------------------------------------------------------------
// String manipulation
//---------------------------------------------------------------------------

/// Compare a slice from start to end from the compared string with the base string.
internal bool key_compare(const char *key, size_t key_end, const char *compared)
{
    size_t i;
    for (i = 0; i < key_end; i++)
    {
        if (key[i] != compared[i]) return false;
    }

    if (i < key_end) return false;

    return true;
}

/// Copy a slice from start to end from source into target starting from
/// target_start_from including a terminator character '\0' at the end.
internal void string_copy_slice_null(
    const char *source,
    size_t start,
    size_t end,
    char *target
) {
    assert((end - start) < TC_LINE_MAX_SIZE);
    size_t i;
    for (i = 0; i <= (end - start); i++)
        target[i] = source[i+start];
    target[i] = '\0';
}

/// Copy a slice from start to end from the source string into the target string.
internal void string_copy_slice(const char *source, size_t start, size_t end, char *target)
{
    assert((end - start) < TC_LINE_MAX_SIZE);
    for (size_t i = 0; i <= (end - start); i++)
        target[i] = source[i+start];
}

/// Find and return the position where the value really ends ignoring spaces at the end.
internal size_t string_trim_end(const char *source, size_t position)
{
    size_t value_end = position;
    do {
        if(source[value_end] != ' ') break;
    } while(value_end--);
    return value_end;
}

//---------------------------------------------------------------------------
// Allocation
//---------------------------------------------------------------------------

internal void *line_get(tc_config *config, size_t index)
{
    assert(index < TC_CONFIG_MAX_SIZE);
    return &config->buffer[TC_LINE_TOTAL_SIZE * index];
}

internal size_t line_offset_get(tc_config *config, size_t index)
{
    assert(index <= config->size);
    return *((size_t *) line_get(config, index));
}

internal void header_write(void *location, size_t key_value_offset)
{
    assert(location != NULL);
    size_t *header = location;
    *header = key_value_offset;
}

internal char *header_read(void *location)
{
    assert(location != NULL);
    void *key_start = (char *) location + TC_HEADER_SIZE;
    return key_start;
}

//---------------------------------------------------------------------------
// Lexer
//---------------------------------------------------------------------------

internal bool tc_parse_config(tc_config *config, char *file_buffer, size_t file_bytes_read)
{
    assert(file_bytes_read > 0);

    char c              = 0;
    size_t current_line = 0;
    size_t start_pos    = 0;
    size_t key_size     = 0;
    bool reading_value  = false;

    for (size_t pos = 0; pos < file_bytes_read; pos++)
    {
        switch (c = file_buffer[pos])
        {
        case '\r':
        case '\n':
        case ' ':
        case '\t': break;
        case '#':
        {
            while ((c = file_buffer[pos+1]) != '\n' || c == '\0') pos++;
            break;
        }
        case '=':
        {
            reading_value = true;
            break;
        }
        default:
        {
            start_pos = pos;

            if (reading_value)
            {
                if (isalpha(c) || isdigit(c) || c == '-' || c == '.')
                {
                    for (;;)
                    {
                        c = file_buffer[pos+1];
                        if (c == '\r' || c == '\n' || c == '\0' || c == '#') break;
                        pos++;
                    }
                }
                else
                {
                    // TODO error reading value
                }

                size_t value_size = pos - start_pos;

                // +2 for '=' and '\0'
                if ((value_size + key_size + 2) >= TC_LINE_MAX_SIZE)
                {
                    ERROR_REPORT("value at line %zi overflows default TC_LINE_MAX_SIZE (%i)",
                        current_line,
                        TC_LINE_MAX_SIZE
                    );
                }

                char *key_start   = header_read(line_get(config, current_line));
                char *value_start = &key_start[key_size + 2];
                size_t trim_end_position = string_trim_end(file_buffer, pos);
                string_copy_slice_null(
                    file_buffer,
                    start_pos,
                    trim_end_position,
                    value_start
                );

                current_line += 1;
                config->size += 1;
                reading_value = false;
            }
            else
            {
                if (isalpha(c))
                {
                    while (isalpha((c = file_buffer[pos+1])) || c == '_') pos++;
                }
                else
                {
                    // Invalid key, skip current_line.
                    while ((c = file_buffer[pos+1]) != '\n') pos++;
                    ERROR_REPORT(
                        "key at line %zi starts with illegal character",
                        current_line
                    );
                    goto error;
                }

                key_size = (pos - start_pos);

                if (config->size == TC_CONFIG_MAX_SIZE)
                {
                    ERROR_REPORT(
                        "key at line %zi overflows default TC_LINE_MAX_SIZE (%i)",
                        current_line,
                        TC_CONFIG_MAX_SIZE
                    );
                    goto error;
                }

                if (key_size >= TC_LINE_MAX_SIZE)
                {
                    ERROR_REPORT(
                        "amount of lines exceeds TC_CONFIG_MAX_SIZE (%i)",
                        TC_LINE_MAX_SIZE
                    );
                    goto error;
                }

                void *current_location = line_get(config, current_line);
                // + 2 to land correctly on the start of the value, just after the = sign.
                header_write(current_location, key_size + 2);
                assert( *((size_t *) current_location) == key_size + 2);

                void *key_location = header_read(current_location);
                string_copy_slice(file_buffer, start_pos, pos, key_location);

                char *key = (char *) key_location;
                key[key_size + 1] = '=';
            }

            break;
        }
        }
    }

    free(file_buffer);
    return true;

error:
    free(file_buffer);
    return false;
}

//---------------------------------------------------------------------------
// tinyconfig.h
//---------------------------------------------------------------------------

// TODO test memory alignment
typedef struct {
    size_t offset;
    char   line[TC_LINE_MAX_SIZE];
} tc_config_line;

internal void *buffer[TC_CONFIG_MAX_SIZE * TC_LINE_TOTAL_SIZE] = {0};

extern bool tc_load_config(tc_config *config, const char *file_path)
{
    assert(config != NULL);
#ifndef NDEBUG
    double startTime = (double) clock() / CLOCKS_PER_SEC;
#endif

    FILE *file;
    bool ok = open_file(&file, file_path, "rb");
    if (!ok)
        return false;

    TC_FSEEK(file, 0L, SEEK_END);
    size_t file_size = TC_FTELL(file);
    rewind(file);

    char *file_buffer = malloc(file_size + 1);
    if (!file_buffer)
    {
        free(file_buffer);
        return false;
    }

    size_t bytes_read = fread(file_buffer, sizeof(char), file_size, file);
    if (bytes_read == 0)
    {
        free(file_buffer);
        return false;
    }

    file_buffer[bytes_read] = '\0';
    fclose(file);

    config->buffer = buffer;
    config->size   = 0;
    bool success   = tc_parse_config(config, file_buffer, bytes_read);
#ifndef NDEBUG
    double elapsed = (double)clock() / CLOCKS_PER_SEC - startTime;
    printf("tinyconfig: load config time: %f seconds\n", elapsed);
#endif

    return success;
}

/// Iterates config->buffer to find the correct key and return its value.
extern char *tc_get_value(tc_config *config, const char *key)
{
    for (size_t i = 0; i < config->size; i += 1)
    {
        size_t offset = line_offset_get(config, i);
        char *key_start = header_read(line_get(config, i));
        if (key_compare(key, offset - 1, key_start))
        {
            return &key_start[offset];
        }
    }

    return NULL;
}

/// Iterates config->buffer to find the key and assign it a new value
/// if it doesn't overflow TC_LINE_MAX_SIZE. If the key doesn't exit, no value is updated.
extern void *tc_set_value(tc_config *config, char *key, char *new_value)
{
    size_t new_value_length = strlen(new_value);
    assert(new_value_length > 0);

    size_t key_length = strlen(key);
    assert(key_length > 0);

    // +2 for '=' and '\0'
    if ((key_length + new_value_length + 2) > TC_LINE_MAX_SIZE)
    {
        return NULL;
    }

    for (size_t i = 0; i < config->size; i += 1)
    {
        size_t offset = line_offset_get(config, i);
        char *key_start = header_read(line_get(config, i));
        if (key_compare(key, offset - 1, key_start))
        {
            char *value_start = &key_start[offset];
            string_copy_slice_null(
                new_value,
                0,
                new_value_length - 1,
                value_start
            );
            return value_start;
        }
    }

    return NULL;
}

extern bool tc_save_to_file(tc_config *config, const char *file_path)
{
    FILE* file;
    int ok = open_file(&file, file_path, "w");
    if (!ok)
        return false;

    for (size_t i = 0; i < config->size; i++)
    {
        char *key_start = header_read(line_get(config, i));
        fprintf(file, "%s\n", key_start);
    }

    fclose(file);
    return true;
}
