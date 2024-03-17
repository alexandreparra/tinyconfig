// Copyright 2023-2024 Alexandre Parra
// MIT License
// tinyconfig

#pragma once

#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TC_CONFIG_MAX_SIZE
#define TC_CONFIG_MAX_SIZE 20
#endif

#ifndef TC_LINE_MAX_SIZE
#define TC_LINE_MAX_SIZE 64
#endif

#define TC_HEADER_SIZE sizeof(size_t)
#define TC_LINE_TOTAL_SIZE (TC_LINE_MAX_SIZE + TC_HEADER_SIZE)

/// tc_config works as a Pool allocator but without a free list. We don't
/// need to know the
typedef struct {
    void      *buffer;
    size_t     size;
} tc_config;
extern bool tc_load_config(tc_config *config, const char *file_path);
extern char *tc_get_value(tc_config *config, const char *key_name);
extern char *tc_set_value(tc_config *config, char *key, char *new_value);
extern bool tc_save_to_file(tc_config *config, const char *file_path);

#ifdef __cplusplus
}
#endif
