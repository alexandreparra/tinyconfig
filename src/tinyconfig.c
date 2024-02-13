// Copyright 2023-2024 Alexandre Parra
// MIT License
// tinyconfig.

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tinyconfig.h>

// Modified #define names, originally from tinyxml2
// https://github.com/leethomason/tinyxml2
#if defined(_WIN64)
    #define TC_FSEEK _fseeki64
    #define TC_FTELL _ftelli64
#elif defined(__unix__) || defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__DragonFly__) || (__CYGWIN__)
    #define TC_FSEEK fseeko
	#define TC_FTELL ftello
#elif defined(__ANDROID__)
    #if __ANDROID_API__ > 24
        #define TC_FSEEK fseeko64
        #define TC_FTELL ftello64
    #else
        #define TC_FSEEK fseeko
        #define TC_FTELL ftello
    #endif
#else
	#define TC_FSEEK fseek
	#define TC_FTELL ftell
#endif

#ifndef TC_CONFIG_DEFAULT_SIZE
    #define TC_CONFIG_DEFAULT_SIZE 20
#endif

#ifndef TC_CONFIG_DEFAULT_LINE_SIZE
    #define TC_CONFIG_DEFAULT_LINE_SIZE 50
#endif

#ifdef TC_DEBUG_LOGS
    #include <time.h>
    #define DEBUG
    #define DEBUG_PRINTLN(str, ...) printf("| %d %s | " str " |\n", __LINE__, __func__, ##__VA_ARGS__)

    #define DEBUG_PRINT_CONFIG_STATUS(message, config) DEBUG_PRINTLN( \
            message " | size %zi, capacity %zi", \
            config->size,                        \
            config->capacity)
    #define DEBUG_PRINT_MESSAGE_LINE(message, config, at) DEBUG_PRINTLN( \
            message " | size %zi, capacity %zi, line[%zi] %s", \
            config->size,                                             \
            config->capacity,                                         \
            at,                                                       \
            config->lines[at])
    #define DEBUG_PRINT_ADD_LINE(config, at) DEBUG_PRINTLN( \
            "Config add line | size %zi, capacity %zi, line[%zi] %s", \
            config->size,                                             \
            config->capacity,                                         \
            at,                                                       \
            config->lines[at])
#endif

//---------------------------------------------------------------------------
// Static
//---------------------------------------------------------------------------

/// Compare a slice from start to end from the compared string with the base string.
static bool tc_str_compare(const char *key, size_t key_end, const char *compared)
{
    size_t i;
    for (i = 0; i < key_end; i++)
    {
        if (key[i] != compared[i]) return false;
    }

    if (i < key_end) return false;

    return true;
}

static void tc_str_copy(const char *source, size_t source_size, char *dest)
{
   size_t i;
   for (i = 0; i < source_size; i++)
       dest[i] = source[i];
   dest[i] = '\0';
}

/// Copy a slice from start to end from source into dest 
/// starting from start_from including a terminator character '\0'.
static void tc_str_copy_slice_null(
        const char *source,
        size_t start,
        size_t end,
        char *dest,
        size_t start_from
) {
    size_t i;
    for (i = 0; i <= (end - start); i++)
        dest[i+start_from] = source[i+start];
    dest[i+start_from] = '\0';
}


/// Copy a slice from start to end from the source string into the dest string.
static void tc_str_copy_slice(const char *source, size_t start, size_t end, char *dest)
{
    for (size_t i = 0; i <= (end - start); i++)
        dest[i] = source[i+start];
}

static bool open_file(FILE** fp, const char* file_path, const char* mode)
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

/// Finds the config value and return its position inside the config lines.
/// Returns -1 if the value wasn't found.
static int tc_get_value_position(tc_config *config, const char *key)
{
    for (size_t i = 0; i < config->size; i += 1)
    {
        if (tc_str_compare(key, config->offsets[i] - 1, config->lines[i]))
        {
            return (int) i;
        }
    }

    return -1;
}

inline static void realloc_config(tc_config * config) {
    config->capacity += TC_CONFIG_DEFAULT_SIZE;
    char ** new_str_array = realloc(config->lines, sizeof(char *) * config->capacity);
    config->lines = new_str_array;

    size_t * new_offsets = realloc(config->offsets, sizeof(size_t) * config->capacity);
    config->offsets = new_offsets;

#ifdef DEBUG
    DEBUG_PRINT_CONFIG_STATUS("REALLOC config", config);
#endif
}

//---------------------------------------------------------------------------
// tinyconfig.h
//---------------------------------------------------------------------------

extern bool tc_load_config(tc_config **config, const char *file_path)
{
#ifdef DEBUG
    double startTime = (double) clock() / CLOCKS_PER_SEC;
#endif

    FILE *fp;
    bool ok = open_file(&fp, file_path, "rb");
    if (!ok)
        return false;

    TC_FSEEK(fp, 0L, SEEK_END);
    size_t file_size = TC_FTELL(fp);
    rewind(fp);

    char *file_buffer = malloc(file_size + 1);
    if (!file_buffer)
        return false;

    size_t bytes_read = fread(file_buffer, sizeof(char), file_size, fp);
    if (bytes_read == 0)
        return false;

    file_buffer[bytes_read] = '\0';

    fclose(fp);

    tc_config *tmp_config = malloc(sizeof(tc_config));

#ifndef DEBUG
    tmp_config->lines    = calloc(TC_CONFIG_DEFAULT_SIZE, sizeof(char *));
    tmp_config->offsets  = calloc(TC_CONFIG_DEFAULT_SIZE, sizeof(size_t));
#else
    tmp_config->lines   = malloc(sizeof(char *) * TC_CONFIG_DEFAULT_SIZE);
    tmp_config->offsets = malloc(sizeof(size_t) * TC_CONFIG_DEFAULT_SIZE);
#endif
    tmp_config->capacity = TC_CONFIG_DEFAULT_SIZE;
    tmp_config->size     = 0;

    char c             = 0;
    size_t line        = 0;
    size_t start_pos   = 0;
    size_t key_size    = 0;
    bool reading_value = false;

    for (size_t pos = 0; pos < bytes_read; pos++)
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
                    if (isalpha(c))
                    {
                        while (isalpha((c = file_buffer[pos+1])) || c == '_') pos++;
                    }
                    else if (c == '"')
                    {
                        while ((c = file_buffer[pos+1]) != '"') pos++;

                        // Skip the first "
                        start_pos += 1;
                    }
                    else if (isdigit(c) || c == '-' ||c == '.')
                    {
                        while ((c = file_buffer[pos+1]) == '.' || isdigit(c)) pos++;
                    }

                    size_t value_size = pos - start_pos;

                    // +2 for '=' and '\0'
                    if ((value_size + key_size + 2) >= TC_CONFIG_DEFAULT_LINE_SIZE)
                    {
                        char *new_line = realloc(tmp_config->lines[line], TC_CONFIG_DEFAULT_LINE_SIZE * 2);
                        if (new_line == NULL) goto file_buffer_error;
                        tmp_config->lines[line] = new_line;
                    }

                    tc_str_copy_slice_null(
                            file_buffer,
                            start_pos,
                            pos,
                            tmp_config->lines[line],
                            key_size+2
                    );

                    line++;
#ifdef DEBUG
                    DEBUG_PRINT_ADD_LINE(tmp_config, tmp_config->size);
#endif
                    tmp_config->size += 1;
                    reading_value = false;
                }
                else
                {
                    if (isalpha(c))
                    {
                        while ((c = file_buffer[pos+1]) == '_' || isalpha(c)) pos++;
                    }
                    else if (isdigit(c))
                    {
                        while (isdigit(c = file_buffer[pos+1])) pos++;
                    }
                    else
                    {
                        // Invalid key, skip line.
                        while ((c = file_buffer[pos+1]) != '\n') pos++;
                        continue;
                    }

                    key_size = (pos - start_pos);

                    if (tmp_config->size == tmp_config->capacity)
                    {
#ifdef DEBUG
                        DEBUG_PRINTLN("REALLOC config size");
#endif
                        realloc_config(tmp_config);
                    }

                    if (key_size >= TC_CONFIG_DEFAULT_LINE_SIZE) 
                        tmp_config->lines[line] = malloc(sizeof(char) * (TC_CONFIG_DEFAULT_LINE_SIZE + key_size));
                    else
                        tmp_config->lines[line] = malloc(sizeof(char) * TC_CONFIG_DEFAULT_LINE_SIZE);

                    // Indicate the index of the first letter from the value.
                    tmp_config->offsets[line] = key_size + 2;

                    tc_str_copy_slice(file_buffer, start_pos, pos, tmp_config->lines[line]);
                    char *key = tmp_config->lines[line];
                    key[key_size + 1] = '=';
                }

                break;
            }
        }
    }

    free(file_buffer);
    *config = tmp_config;

#ifdef DEBUG
    double elapsed = (double)clock() / CLOCKS_PER_SEC - startTime;
    printf("Load config %s finished in: %f\n", file_path, elapsed);
#endif

    return true;

file_buffer_error:
    free(file_buffer);
    return false;
}

extern char *tc_get_value(tc_config *config, const char *key)
{
    for (size_t i = 0; i < config->size; i += 1)
    {
        if (tc_str_compare(key, config->offsets[i] - 1, config->lines[i]))
        {
            char *str = config->lines[i];
            return &str[config->offsets[i]];
        }
    }

    return NULL;
}

extern char *tc_set_value(tc_config *config, const char *key_name, const char *new_value)
{
    int line_position = tc_get_value_position(config, key_name);
    if (line_position < 0) // Value wasn't found
    {
        if (config->size == config->capacity)
        {
#ifdef DEBUG
            DEBUG_PRINTLN("REALLOC config");
#endif
            realloc_config(config);
        }

        size_t key_size   = strlen(key_name);
        size_t value_size = strlen(new_value);

        char * new_line = NULL;
        if ((key_size + value_size) >= TC_CONFIG_DEFAULT_LINE_SIZE)
            new_line = malloc(sizeof(char) * (TC_CONFIG_DEFAULT_LINE_SIZE + key_size + value_size));
        else
            new_line = malloc(sizeof(char) * TC_CONFIG_DEFAULT_LINE_SIZE);

        config->offsets[config->size] = key_size + 1;

        tc_str_copy_slice(key_name, 0, key_size - 1, new_line);
        new_line[key_size] = '=';
        tc_str_copy_slice_null(new_value, 0, value_size, new_line, key_size + 1);
        config->lines[config->size] = new_line;

#ifdef DEBUG
        DEBUG_PRINT_ADD_LINE(config, config->size);
#endif
        config->size += 1;
        return &new_line[key_size + 1];
    }

    size_t line_size      = strlen(config->lines[line_position]);
    size_t old_value_size = line_size - config->offsets[line_position];
    size_t new_value_size = strlen(new_value);
    size_t total_size     = (line_size - old_value_size) + new_value_size;

    if (total_size > TC_CONFIG_DEFAULT_LINE_SIZE)
    {
        // +1 for the null terminator character \0
        char * new_line = realloc(config->lines[line_position], total_size + 1);
        if (new_line != NULL) config->lines[line_position] = new_line;
    }

    char * value_location = &config->lines[line_position][config->offsets[line_position]];
    tc_str_copy(new_value, new_value_size, value_location);

#ifdef DEBUG
    size_t line_pos = (size_t) line_position;
    DEBUG_PRINT_MESSAGE_LINE("Changed line value", config, line_pos);
#endif

    return value_location;
}

extern int tc_save_to_file(tc_config *config, const char *file_path)
{
    FILE* fp;
    int ok = open_file(&fp, file_path, "w");
    if (!ok)
        return -1;

    for (size_t i = 0; i < config->size; i++)
    {
        fprintf(fp, "%s\n", config->lines[i]);
    }

    fclose(fp);

    return 0;
}

extern void tc_free(tc_config *config)
{
    for (size_t i = 0; i < config->size; i++)
    {
        free(config->lines[i]);
    }

    free(config->lines);
    free(config->offsets);
    free(config);
}
