### tinyconfig cpp
The official tinyconfig C++ wrapper. It's simply a CMake library that automatically builds tinyconfig
and puts its functions and config struct inside the `tc` namespace.

Because it forces inlining of code, there is no real overhead when using this wrapper.

This can be used with any other build system, as long as you compile.