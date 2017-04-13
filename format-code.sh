#!/bin/bash

find . '(' -name '*.h' -o -name '*.c' ')' -exec clang-format-3.7 -style=file -i '{}' \;
