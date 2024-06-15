#!/usr/bin/env python
# -*- coding: ascii -*-
u"""
==============
 CSS Minifier
==============

CSS Minifier.

The minifier is based on the semantics of the `YUI compressor`_\\, which
itself is based on `the rule list by Isaac Schlueter`_\\.

:Copyright:

 Copyright 2011 - 2024
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

This module is a re-implementation aiming for speed instead of maximum
compression, so it can be used at runtime (rather than during a preprocessing
step). RCSSmin does syntactical compression only (removing spaces, comments
and possibly semicolons). It does not provide semantic compression (like
removing empty blocks, collapsing redundant properties etc). It does, however,
support various CSS hacks (by keeping them working as intended).

Here's a feature list:

- Strings are kept, except that escaped newlines are stripped
- Space/Comments before the very end or before various characters are
  stripped: ``:{});=>],!`` (The colon (``:``) is a special case, a single
  space is kept if it's outside a ruleset.)
- Space/Comments at the very beginning or after various characters are
  stripped: ``{}(=:>[,!``
- Optional space after unicode escapes is kept, resp. replaced by a simple
  space
- whitespaces inside ``url()`` definitions are stripped, except if it's a
  quoted non-base64 data url
- Comments starting with an exclamation mark (``!``) can be kept optionally.
- All other comments and/or whitespace characters are replaced by a single
  space.
- Multiple consecutive semicolons are reduced to one
- The last semicolon within a ruleset is stripped
- CSS Hacks supported:

  - IE7 hack (``>/**/``)
  - Mac-IE5 hack (``/*\\*/.../**/``)
  - The boxmodelhack is supported naturally because it relies on valid CSS2
    strings
  - Between ``:first-line`` and the following comma or curly brace a space is
    inserted. (apparently it's needed for IE6)
  - Same for ``:first-letter``

rcssmin.c is a reimplementation of rcssmin.py in C and improves runtime up to
factor 100 or so (depending on the input). docs/BENCHMARKS in the source
distribution contains the details.

Supported python versions are 2.7 and 3.4+.

.. _YUI compressor: https://github.com/yui/yuicompressor/

.. _the rule list by Isaac Schlueter: https://github.com/isaacs/cssmin/
"""
__author__ = u"Andr\xe9 Malo"
__license__ = "Apache License, Version 2.0"
__version__ = '1.1.2'
__all__ = ['cssmin']

import re as _re


def _make_cssmin(python_only=False):
    """
    Generate CSS minifier.

    Parameters:
      python_only (bool):
        Use only the python variant. If true, the c extension is not even
        tried to be loaded.

    Returns:
      callable: Minifier
    """
    # pylint: disable = unused-variable, possibly-unused-variable
    # pylint: disable = too-many-locals, too-many-statements

    if not python_only:
        try:
            import _rcssmin  # pylint: disable = import-outside-toplevel
        except ImportError:
            pass
        else:
            # Ensure that the C version is in sync
            if getattr(_rcssmin, '__version__', None) == __version__:
                return _rcssmin.cssmin

    nl = r'(?:[\n\f]|\r\n?)'  # pylint: disable = invalid-name
    spacechar = r'[\r\n\f\040\t]'

    unicoded = r'[0-9a-fA-F]{1,6}(?:[\040\n\t\f]|\r\n?)?'
    escaped = r'[^\n\r\f0-9a-fA-F]'
    escape = r'(?:\\(?:%(unicoded)s|%(escaped)s))' % locals()

    nmchar = r'[^\000-\054\056\057\072-\100\133-\136\140\173-\177]'
    # nmstart = r'[^\000-\100\133-\136\140\173-\177]'
    # ident = (r'(?:'
    #     r'-?(?:%(nmstart)s|%(escape)s)%(nmchar)s*(?:%(escape)s%(nmchar)s*)*'
    # r')') % locals()

    comment = r'(?:/\*[^*]*\*+(?:[^/*][^*]*\*+)*/)'

    # only for specific purposes. The bang is grouped:
    _bang_comment = r'(?:/\*(!?)[^*]*\*+(?:[^/*][^*]*\*+)*/)'

    string1 = \
        r'(?:\047[^\047\\\r\n\f]*(?:\\[^\r\n\f][^\047\\\r\n\f]*)*\047)'
    string2 = r'(?:"[^"\\\r\n\f]*(?:\\[^\r\n\f][^"\\\r\n\f]*)*")'
    strings = r'(?:%s|%s)' % (string1, string2)

    nl_string1 = \
        r'(?:\047[^\047\\\r\n\f]*(?:\\(?:[^\r]|\r\n?)[^\047\\\r\n\f]*)*\047)'
    nl_string2 = r'(?:"[^"\\\r\n\f]*(?:\\(?:[^\r]|\r\n?)[^"\\\r\n\f]*)*")'
    nl_strings = r'(?:%s|%s)' % (nl_string1, nl_string2)

    uri_nl_string1 = r'(?:\047[^\047\\]*(?:\\(?:[^\r]|\r\n?)[^\047\\]*)*\047)'
    uri_nl_string2 = r'(?:"[^"\\]*(?:\\(?:[^\r]|\r\n?)[^"\\]*)*")'
    uri_nl_strings = r'(?:%s|%s)' % (uri_nl_string1, uri_nl_string2)

    nl_escaped = r'(?:\\%(nl)s)' % locals()

    space = r'(?:%(spacechar)s|%(comment)s)' % locals()

    ie7hack = r'(?:>/\*\*/)'

    uri = (
        # noqa pylint: disable = bad-option-value, bad-continuation

        r'(?:'

        r'(?:[^\000-\040"\047()\\\177]*'
            r'(?:%(escape)s[^\000-\040"\047()\\\177]*)*)'
        r'(?:'
            r'(?:%(spacechar)s+|%(nl_escaped)s+)'
            r'(?:'
                r'(?:[^\000-\040"\047()\\\177]|%(escape)s|%(nl_escaped)s)'
                r'[^\000-\040"\047()\\\177]*'
                r'(?:%(escape)s[^\000-\040"\047()\\\177]*)*'
            r')+'
        r')*'

        r')'
    ) % locals()

    nl_unesc_sub = _re.compile(nl_escaped).sub

    uri_data_plain = _re.compile((
        r'[\047"][dD][aA][tT][aA]:([^\000-\040\\"\047,]*),'
    )).match

    uri_space_sub = _re.compile((
        r'(%(escape)s+)|%(spacechar)s+|%(nl_escaped)s+'
    ) % locals()).sub
    uri_space_subber = lambda m: m.groups()[0] or ''

    space_sub_simple = _re.compile((
        r'[\r\n\f\040\t;]+|(%(comment)s+)'
    ) % locals()).sub
    space_sub_banged = _re.compile((
        r'[\r\n\f\040\t;]+|(%(_bang_comment)s+)'
    ) % locals()).sub

    post_esc_sub = _re.compile(r'[\r\n\f\t]+').sub

    main_sub = _re.compile((
        # noqa pylint: disable = bad-option-value, bad-continuation

        r'([^\\"\047u>@\r\n\f\040\t/;:{}+]+)'             # 1
        r'|(?<=[{}(=:>[,!])(%(space)s+)'                  # 2
        r'|^(%(space)s+)'                                 # 3
        r'|(%(space)s+)(?=(([:{});=>\],!])|$)?)'          # 4, 5, 6
        r'|;(%(space)s*(?:;%(space)s*)*)(?=(\})?)'        # 7, 8
        r'|(\{)'                                          # 9
        r'|(\})'                                          # 10
        r'|(%(strings)s)'                                 # 11
        r'|(?<!%(nmchar)s)url\(%(spacechar)s*('           # 12
                r'%(uri_nl_strings)s'
                r'|%(uri)s'
            r')%(spacechar)s*\)'
        r'|(@(?:'                                         # 13
              r'[mM][eE][dD][iI][aA]'
              r'|[sS][uU][pP][pP][oO][rR][tT][sS]'
              r'|[dD][oO][cC][uU][mM][eE][nN][tT]'
              r'|(?:-(?:'
                  r'[wW][eE][bB][kK][iI][tT]|[mM][oO][zZ]|[oO]|[mM][sS]'
                r')-)?'
                r'[kK][eE][yY][fF][rR][aA][mM][eE][sS]'
            r'))(?!%(nmchar)s)'
        r'|(%(ie7hack)s)(%(space)s*)'                     # 14, 15
        r'|(:[fF][iI][rR][sS][tT]-[lL]'                   # 16
            r'(?:[iI][nN][eE]|[eE][tT][tT][eE][rR]))'
            r'(%(space)s*)(?=[{,])'                       # 17
        r'|(%(nl_strings)s)'                              # 18
        r'|(%(escape)s[^\\"\047u>@\r\n\f\040\t/;:{}+]*)'  # 19
    ) % locals()).sub

    # print(main_sub.__self__.pattern)

    def main_subber(keep_bang_comments):
        """ Make main subber """
        in_macie5, in_rule, at_group = [0], [0], [0]

        if keep_bang_comments:
            space_sub = space_sub_banged

            def space_subber(match):
                """ Space|Comment subber """
                if match.lastindex:
                    group1, group2 = match.group(1, 2)
                    if group2:
                        if group1.endswith(r'\*/'):
                            in_macie5[0] = 1
                        else:
                            in_macie5[0] = 0
                        return group1

                    if group1.endswith(r'\*/'):
                        if in_macie5[0]:
                            return ''
                        in_macie5[0] = 1
                        return r'/*\*/'
                    elif in_macie5[0]:
                        in_macie5[0] = 0
                        return '/**/'
                return ''
        else:
            space_sub = space_sub_simple

            def space_subber(match):
                """ Space|Comment subber """
                if match.lastindex:
                    if match.group(1).endswith(r'\*/'):
                        if in_macie5[0]:
                            return ''
                        in_macie5[0] = 1
                        return r'/*\*/'
                    elif in_macie5[0]:
                        in_macie5[0] = 0
                        return '/**/'
                return ''

        def fn_space_post(group):
            """ space with token after """
            if group(5) is None or (
                    group(6) == ':' and not in_rule[0] and not at_group[0]):
                return ' ' + space_sub(space_subber, group(4))
            return space_sub(space_subber, group(4))

        def fn_semicolon(group):
            """ ; handler """
            return ';' + space_sub(space_subber, group(7))

        def fn_semicolon2(group):
            """ ; handler """
            if in_rule[0]:
                return space_sub(space_subber, group(7))
            return ';' + space_sub(space_subber, group(7))

        def fn_open(_):
            """ { handler """
            if at_group[0]:
                at_group[0] -= 1
            else:
                in_rule[0] = 1
            return '{'

        def fn_close(_):
            """ } handler """
            in_rule[0] = 0
            return '}'

        def fn_url(group):
            """ url() handler """
            uri = group(12)
            data = uri_data_plain(uri)
            if not data or data.group(1).lower().endswith(';base64'):
                uri = uri_space_sub(uri_space_subber, uri)
            return 'url(%s)' % (uri,)

        def fn_at_group(group):
            """ @xxx group handler """
            at_group[0] += 1
            return group(13)

        def fn_ie7hack(group):
            """ IE7 Hack handler """
            if not in_rule[0] and not at_group[0]:
                in_macie5[0] = 0
                return group(14) + space_sub(space_subber, group(15))
            return '>' + space_sub(space_subber, group(15))

        table = (
            # noqa pylint: disable = bad-option-value, bad-continuation

            None,
            None,
            None,
            None,
            fn_space_post,                       # space with token after
            fn_space_post,                       # space with token after
            fn_space_post,                       # space with token after
            fn_semicolon,                        # semicolon
            fn_semicolon2,                       # semicolon
            fn_open,                             # {
            fn_close,                            # }
            lambda g: g(11),                     # string
            fn_url,                              # url(...)
            fn_at_group,                         # @xxx expecting {...}
            None,
            fn_ie7hack,                          # ie7hack
            None,
            lambda g: g(16) + ' ' + space_sub(space_subber, g(17)),
                                                 # :first-line|letter followed
                                                 # by [{,] (apparently space
                                                 # needed for IE6)
            lambda g: nl_unesc_sub('', g(18)),   # nl_string
            lambda g: post_esc_sub(' ', g(19)),  # escape
        )

        def func(match):
            """ Main subber """
            idx, group = match.lastindex, match.group
            if idx > 3:
                return table[idx](group)

            # shortcuts for frequent operations below:
            elif idx == 1:     # not interesting
                return group(1)
            # else: # space with token before or at the beginning
            return space_sub(space_subber, group(idx))

        return func

    def cssmin(style, keep_bang_comments=False):
        """
        Minify CSS.

        Parameters:
          style (str):
            CSS to minify

          keep_bang_comments (bool):
            Keep comments starting with an exclamation mark? (``/*!...*/``)

        Returns:
          str: Minified style
        """
        # pylint: disable = redefined-outer-name

        is_bytes, style = _as_str(style)
        style = main_sub(main_subber(keep_bang_comments), style)
        if is_bytes:
            style = style.encode('latin-1')
            if is_bytes == 2:
                style = bytearray(style)
        return style

    return cssmin

cssmin = _make_cssmin()


def _as_str(script):
    """ Make sure the style is a text string """
    is_bytes = False
    if str is bytes:
        if not isinstance(script, basestring):  # noqa pylint: disable = undefined-variable
            raise TypeError("Unexpected type")
    elif isinstance(script, bytes):
        is_bytes = True
        script = script.decode('latin-1')
    elif isinstance(script, bytearray):
        is_bytes = 2
        script = script.decode('latin-1')
    elif not isinstance(script, str):
        raise TypeError("Unexpected type")

    return is_bytes, script


if __name__ == '__main__':
    def main():
        """ Main """
        import sys as _sys  # pylint: disable = import-outside-toplevel

        keep_bang_comments = (
            '-b' in _sys.argv[1:]
            or '-bp' in _sys.argv[1:]
            or '-pb' in _sys.argv[1:]
        )
        if '-p' in _sys.argv[1:] or '-bp' in _sys.argv[1:] \
                or '-pb' in _sys.argv[1:]:
            xcssmin = _make_cssmin(python_only=True)
        else:
            xcssmin = cssmin
        _sys.stdout.write(xcssmin(
            _sys.stdin.read(), keep_bang_comments=keep_bang_comments
        ))

    main()
