cmake_minimum_required(VERSION 3.23)
project(pl C)

# The latest released standard is used to get those modern language features
set(CMAKE_C_STANDARD 17)

# Enable some CMake messages
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

# Now is using -Weverything to give as many warnings as possible. In the final build, -Wall -Wextra -Wconversion -Wpedantic -Werror should be used.
# -Wno-extra-semi: Stop warning ";" out of no where. Usually caused by macro.
# -Wno-gnu-zero-variadic-macro-arguments: A GNU feature (supported by gcc and clang) that needed to be used for this project.
# -Wno-documentation-unknown-command: A weird warning about the documentation.
# -Wno-documentation: Another weird warning about the documentation.
# -Wno-declaration-after-statement: A very old warning related to the C90 standard that could be omitted under C17.
# -Wno-gnu-statement-expression: A GNU feature that needed to to used to implement clean macros for modules like backtrace, Exception handling, etc.
# -Wno-shadow: Some constructors provided in this library need to nest statement expression together.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-extra-semi -Wno-gnu-zero-variadic-macro-arguments -Wno-documentation-unknown-command -Wno-documentation -Wno-declaration-after-statement -Wno-gnu-statement-expression -Wno-shadow -Weverything")

add_executable(pl main.c pl_misc.c pl_misc.h pl.c pl.h pl_error.c pl_error.h pl_optional.h pl_bt.c pl_bt.h pl_mem.c pl_mem.h)
