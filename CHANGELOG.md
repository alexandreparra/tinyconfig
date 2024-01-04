## 2.0.2
- Changed reallocation strategy for `tc_set_value` if the new line size is bigger than the default line allocation size.
- Fixed bug where `tc_save_to_file` would simply not write the configuration back to an empty file.

## 2.0.1
- Address possible memory leak when memory allocations occur while parsing the file, if any reallocation fails the file_buffer is freed.
- Change open_file return type to bool to be more cohesive.
- Fix segfault on open_file function where `fgetc` would be called on a null pointer.
- **BREAKING CHANGE** `tc_load_config` return type to bool to better follow open_file function and to not be so confusing (false is always error, true is always valid).
- Change example to handle errors correctly to do an early return and avoid segfault.

## 2.0.0
- Rewritten all internals of tinyconfig, now every line is saved inside the same buffer, making only one allocation per key-value.
- Almost untouched interface (`tc_save_to_file` now returns an int instead of void).
- Moved `tinyconfig.c` file to `src` directory.
- Added 4 official types: raw strings, strings, integers and floats.
- Clear distinction between what can be a key or value (because the lexer was way too simple on 1.0, even floats could be keys)
- Keys can now only be raw strings or integers.
- Improved coding consistency and style.

## 1.1.2
- Fixed freeing the new config buffer on `tc_save_to_file` to work on every compiler.

## 1.1.1
- Fixed file opening on systems other than Windows (use plain fopen)
- Fixed `tc_set_value` not incrementing the config struct `buffer_size` if the setted value was associated with a new key.
- Always malloc the buffer to save the new config to a file on `tc_save_to_file`.
- Stop using fseeko64 and fteelo64 on x64 \_\_unix\_\_

## 1.1.0
- Add the possibility to override the #define values: `TC_CONFIG_DEFAULT_SIZE` and `TC_CONFIG_DEFAULT_GROW_SIZE` using the build system.
- Add negative number parsing.

## 1.0.0
- Changed how to get values from the config file, removed get string, int and float.
- Added `tc_get_value` to get the string representation of the value you want, you'll need to provide the means to convert the variable.
- Added `tc_set_value` so that you can change the config on runtime and later save it to some file.
- Added `tc_save_to_file` to make any changes for the config during runtime persist on disk.
- `tc_pair` struct is now private.
- Updated README and example to reflect the new changes.

## 0.1.2
- Added tab character check on parsing.

## 0.1.1
- Fixed MSVC build.

## 0.1.0
- Initial release
- Simple key-value parsing
- Support to get floats, ints and characters
