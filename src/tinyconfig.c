// Copyright 2023 Alexandre Parra
// MIT License
// tinyconfig.

#include "../include/tinyconfig.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#ifndef TC_CONFIG_DEFAULT_GROW_SIZE
    #define TC_CONFIG_DEFAULT_GROW_SIZE 10
#endif

#ifndef TC_CONFIG_DEFAULT_LINE_SIZE
    #define TC_CONFIG_DEFAULT_LINE_SIZE 50
#endif 

//---------------------------------------------------------------------------
// Static
//---------------------------------------------------------------------------

/// Compare a slice from start to end from the compared string with the base string.
static bool tc_compare(const char *key, size_t key_end, const char *compared)
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

static int open_file(FILE** fp, const char* file_path, const char* mode) 
{
#ifdef _MSC_VER
    int err = fopen_s(fp, file_path, mode);
    if (err) return 1;
#else
    *fp = fopen(file_path, mode);
#endif

    if (fgetc(*fp) == EOF || ferror(*fp))
        return 1;

    return 0;
}

//---------------------------------------------------------------------------
// src.h
//---------------------------------------------------------------------------

extern int tc_load_config(tc_config **config, const char *file_path)
{
    FILE *fp;
    int err = open_file(&fp, file_path, "rb");
    if (err)
        return -1;

    TC_FSEEK(fp, 0L, SEEK_END);
    size_t file_size = TC_FTELL(fp);
    rewind(fp);

    char *file_buffer = malloc(file_size + 1);
    if (!file_buffer)
        return -1;

    size_t bytes_read = fread(file_buffer, sizeof(char), file_size, fp);
    if (bytes_read == 0)
        return -1;

    file_buffer[bytes_read] = '\0';

    fclose(fp);

    // TODO check for empty file

    tc_config *tmp_config = malloc(sizeof(tc_config));

#ifndef DEBUG
    tmp_config->lines    = calloc(TC_CONFIG_DEFAULT_SIZE, sizeof(char *));
    tmp_config->offsets  = calloc(TC_CONFIG_DEFAULT_SIZE, sizeof(size_t));
#else
    tmp_config->lines   = malloc(sizeof(char *) * TC_CONFIG_DEFAULT_SIZE);
    tmp_config->offsets = malloc(sizeof(size_t) * TC_CONFIG_DEFAULT_SIZE);
#endif
    tmp_config->capacity = TC_CONFIG_DEFAULT_SIZE;

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
                        if (new_line == NULL) 
                            // TODO memory allocation error;
                            return -1;
                    }

                    tc_str_copy_slice_null(
                            file_buffer,
                            start_pos,
                            pos,
                            tmp_config->lines[line],
                            key_size+2
                    );

                    char *tt = tmp_config->lines[line];

                    line++;
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
                        tmp_config->capacity += TC_CONFIG_DEFAULT_GROW_SIZE;
                        char *new_str_array = realloc(tmp_config->lines, tmp_config->capacity);
                        if (new_str_array != NULL) *tmp_config->lines = new_str_array;

                        size_t *new_offsets = realloc(tmp_config->offsets, tmp_config->capacity);
                        if (new_offsets != NULL) tmp_config->offsets = new_offsets;
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
    tmp_config->size = line;
    *config = tmp_config;

    return 0;
}

extern char *tc_get_value(tc_config *config, const char *key)
{
    for (size_t i = 0; i < config->size; i += 1)
    {
        if (tc_compare(key, config->offsets[i] - 1, config->lines[i]))
        {
            char *str = config->lines[i];
            return &str[config->offsets[i]];
        }
    }

    return NULL;
}

extern char *tc_set_value(tc_config *config, const char *key_name, const char *new_value)
{
    char *line = tc_get_value(config, key_name);
    if (line == NULL)
    {
        if (config->size == config->capacity)
        {
            config->capacity += TC_CONFIG_DEFAULT_GROW_SIZE;
            char *new_str_array = realloc(config->lines, config->capacity);
            if (new_str_array != NULL) *config->lines = new_str_array;

            size_t *new_offsets = realloc(config->offsets, config->capacity);
            if (new_offsets != NULL) config->offsets = new_offsets;
        }

        size_t key_size   = strlen(key_name);
        size_t value_size = strlen(new_value);

        char *new_line                = malloc(sizeof(char) * TC_CONFIG_DEFAULT_LINE_SIZE);
        config->offsets[config->size] = key_size + 1;

        tc_str_copy_slice(key_name, 0, key_size - 1, new_line);
        new_line[key_size] = '=';
        tc_str_copy_slice_null(new_value, 0, value_size, new_line, key_size + 1);

        config->lines[config->size] = new_line;
        config->size += 1;
        return &new_line[key_size + 1];
    }

    size_t line_size  = strlen(line);
    size_t value_size = strlen(new_value);

    // + 20 is an arbitrary number so that we can force a reallocation instead of having out of bounds access.
    if ((line_size + value_size + 20) >= TC_CONFIG_DEFAULT_LINE_SIZE)
    {
        char *new_line = realloc(line, TC_CONFIG_DEFAULT_LINE_SIZE * 2);
        if (new_line != NULL) line = new_line;
    }

    tc_str_copy(new_value, value_size, line);
    return line;
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
