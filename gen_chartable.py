#!/usr/bin/env python

import re as _re

TPL = r"""
static const unsigned short rcssmin_charmask[128] = {
    @@mask@@
};
""".strip() + "\n"

def _make_charmask():
    dull = r'[^\\"\047u>@\r\n\f\040\t/;:{}]'
    hexchar = r'[0-9a-fA-F]'
    escaped = r'[^\n\r\f0-9a-fA-F]'
    space = r'[\040\t\r\n\f]'
    string_dull = r'[^"\047\\\r\n\f]'
    nmchar = r'[^\000-\054\056\057\072-\100\133-\136\140\173-\177]'
    uri_dull = r'[^\000-\040"\047()\\\177]'
    pre_char = r'[{}(=:>+[,!]'
    post_char = r'[:{});=>+\],!]'

    charmask = []
    for x in range(8):
        maskline = []
        for y in range(16):
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

print _make_charmask()
