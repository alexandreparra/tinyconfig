// Copyright 2023 Alexandre Parra
// MIT License
// tinyconfig cpp wrapper.

#include <tinyconfig.h>

#ifdef _MSC_VER
    #define force_inline __forceinline
#else
    #define force_inline __attribute__((always_inline))
#endif

namespace tc
{

using config = tc_config;

force_inline int load_config(tc_config **config, const char *file_path)
{
    return tc_load_config(config, file_path);
}

force_inline char *get_value(tc_config *config, const char *key_name)
{
    return tc_get_value(config, key_name);
}

force_inline char *set_value(tc_config *config, const char *file_path, const char *new_value)
{
    return tc_set_value(config, file_path, new_value);
}

force_inline int save_to_file(tc_config *config, const char *file_path)
{
    return tc_save_to_file(config, file_path);
}

force_inline void free(tc_config *config)
{
    tc_free(config);
}

} // namespace tc
