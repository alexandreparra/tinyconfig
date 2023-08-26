// Copyright 2023 Alexandre Parra
// MIT License
// tinyconfig

#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char **lines;
    size_t *offsets;
    size_t size;
    size_t capacity;
} tc_config;

extern int tc_load_config(tc_config **config, const char *file_path);

extern char *tc_get_value(tc_config *config, const char *key_name);

extern char *tc_set_value(tc_config *config, const char *key_name, const char *new_value);

extern int tc_save_to_file(tc_config *config, const char *file_path);

extern void tc_free(tc_config *config);

#ifdef __cplusplus
}
#endif