/* -*- mode: c; c-file-style: "k&r" -*-

  strnatcmp.c -- Perform 'natural order' comparisons of strings in C.
  Copyright (C) 2000, 2004 by Martin Pool <mbp sourcefrog net>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/* partial change history:
 *
 * 2004-10-10 mbp: Lift out character type dependencies into macros.
 *
 * Eric Sosman pointed out that ctype functions take a parameter whose
 * value must be that of an unsigned int, even on platforms that have
 * negative chars in their default char type.
 */

#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "strnatcmp.h"

// Compare two right-aligned numbers:
// The longest run of digits wins.  That aside, the greatest
// value wins, but we can't know that it will until we've scanned
// both numbers to know that they have the same magnitude, so we
// remember it in BIAS.
static int compare_right(char const *a, char const *b)
{
    int bias = 0;

    for (;; a++, b++) {
        if (!isdigit(*a) && !isdigit(*b))
            return bias;
        else if (!isdigit(*a))
            return -1;
        else if (!isdigit(*b))
            return +1;
        else if (*a < *b) {
            if (!bias)
                bias = -1;
        } else if (*a > *b) {
            if (!bias)
                bias = +1;
        } else if (!*a && !*b)
            return bias;
    }

    return 0;
}

// Compare two left-aligned numbers:
// The first to have a different value wins.
static int compare_left(char const *a, char const *b)
{
    for (;; a++, b++) {
        if (!isdigit(*a) && !isdigit(*b))
            return 0;
        else if (!isdigit(*a))
            return -1;
        else if (!isdigit(*b))
            return +1;
        else if (*a < *b)
            return -1;
        else if (*a > *b)
            return +1;
    }

    return 0;
}

static int strnatcmp0(char const *a, char const *b, int ignore_case)
{
    assert(a && b);

    int ai, bi;
    ai = bi = 0;
    while (1) {
        char ca = a[ai];
        char cb = b[bi];

        // Skip over leading spaces
        while (isspace(ca)) {
            ai++;
            ca = a[ai];
        }

        while (isspace(cb)) {
            bi++;
            cb = b[bi];
        }

        // Process run of digits
        if (isdigit(ca) && isdigit(cb)) {
            int fractional = (ca == '0' || cb == '0');

            if (fractional) {
                int result = compare_left(a + ai, b + bi);
                if (result)
                    return result;
            } else {
                int result = compare_right(a + ai, b + bi);
                if (result)
                    return result;
            }
        }

        if (!ca && !cb) {
            // The strings compare the same.  Perhaps the caller will want to call strcmp to break the tie.
            return 0;
        }

        if (ignore_case) {
            ca = toupper(ca);
            cb = toupper(cb);
        }

        if (ca < cb)
            return -1;
        else if (ca > cb)
            return +1;

        ai++;
        bi++;
    }
}

int strnatcmp(char const *a, char const *b)
{
    return strnatcmp0(a, b, 0);
}

int strnatcasecmp(char const *a, char const *b)
{
    return strnatcmp0(a, b, 1);
}
