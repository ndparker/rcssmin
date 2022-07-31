#!/usr/bin/env python
u"""
=========================================
 Character table generator for rcssmin.c
=========================================

Character table generator for rcssmin.c

:Copyright:

 Copyright 2011 - 2022
 Andr\xe9 Malo or his licensors, as applicable

:License:

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
"""
from __future__ import print_function
__author__ = u"Andr\xe9 Malo"

import re as _re

TPL = r"""
static const unsigned short rcssmin_charmask[128] = {
    @@mask@@
};
""".strip() + "\n"


def _make_charmask():
    """ Generate character mask table """
    # pylint: disable = too-many-branches

    dull = r'[^\\"\047u>@\r\n\f\040\t/;:{}+]'
    hexchar = r'[0-9a-fA-F]'
    escaped = r'[^\n\r\f0-9a-fA-F]'
    space = r'[\040\t\r\n\f]'
    string_dull = r'[^"\047\\\r\n\f]'
    nmchar = r'[^\000-\054\056\057\072-\100\133-\136\140\173-\177]'
    uri_dull = r'[^\000-\040"\047()\\\177]'
    pre_char = r'[{}(=:>[,!]'
    post_char = r'[:{});=>\],!]'

    charmask = []
    for x in range(8):  # pylint: disable = invalid-name
        maskline = []
        for y in range(16):  # pylint: disable = invalid-name
            c, mask = chr(x*16 + y), 0
            if _re.match(dull, c):
                mask |= 1
            if _re.match(hexchar, c):
                mask |= 2
            if _re.match(escaped, c):
                mask |= 4
            if _re.match(space, c):
                mask |= 8
            if _re.match(string_dull, c):
                mask |= 16
            if _re.match(nmchar, c):
                mask |= 32
            if _re.match(uri_dull, c):
                mask |= 64
            if _re.match(pre_char, c):
                mask |= 128
            if _re.match(post_char, c):
                mask |= 256

            if mask < 10:
                mask = '  ' + str(mask)
            elif mask < 100:
                mask = ' ' + str(mask)
            maskline.append(str(mask))
            if y == 7:
                charmask.append(', '.join(maskline))
                maskline = []
        charmask.append(', '.join(maskline))
    return TPL.replace('@@mask@@', ',\n    '.join(charmask))

print(_make_charmask())
