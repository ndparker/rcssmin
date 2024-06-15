# -*- coding: ascii -*-
u"""
:Copyright:

 Copyright 2019 - 2024
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

=====
 YUI
=====

YUI tests.
"""
__author__ = u"Andr\xe9 Malo"

import os as _os
import re as _re

import rcssmin as _rcssmin

# pylint: disable = protected-access
py_cssmin = _rcssmin._make_cssmin(python_only=True)

import _rcssmin

c_cssmin = _rcssmin.cssmin

_TESTS = _os.path.dirname(__file__)


def find(base, ext, out):
    """Find test files"""
    for name in _os.listdir(_os.path.join(_TESTS, base)):
        item = "%s/%s" % (base, name)
        if _os.path.isfile(_os.path.join(_TESTS, item)) and item.endswith(
            '.' + ext
        ):
            outitem = "%s/out/%s.%s" % (base, name[: -len(ext) - 1], out)
            yield item, outitem


def load(name):
    """Load a file"""
    with open(_os.path.join(_TESTS, name), 'rb') as fp:
        return fp.read()


def save(name, value):
    """Load a file"""
    with open(_os.path.join(_TESTS, name), 'wb') as fp:
        fp.write(value)


def test_regular():
    """Test yui/*.css"""
    bscheck = _re.compile(
        r'(?<!\\)(?:\\\\)*\\[0-9a-zA-Z]{1,6}$'.encode('ascii')
    ).search
    for name, out in find('yui', 'css', 'out'):
        # print(name)
        inp = load(name)
        exp = load(out).strip()
        if bscheck(exp):
            exp += b' '

        # save(out, py_cssmin(inp))
        assert py_cssmin(inp) == exp
        assert c_cssmin(inp) == exp

        inp = inp.decode('latin-1')
        exp = exp.decode('latin-1')
        assert py_cssmin(inp) == exp
        assert c_cssmin(inp) == exp


def test_banged():
    """Test yui/*.css"""
    bscheck = _re.compile(
        r'(?<!\\)(?:\\\\)*\\[0-9a-zA-Z]{1,6}$'.encode('ascii')
    ).search
    for name, out in find('yui', 'css', 'out.b'):
        # print(name)
        inp = load(name)
        exp = load(out).strip()
        if bscheck(exp):
            exp += b' '

        # save(out, py_cssmin(inp, keep_bang_comments=True))
        assert py_cssmin(inp, keep_bang_comments=True) == exp
        assert c_cssmin(inp, keep_bang_comments=True) == exp

        inp = inp.decode('latin-1')
        exp = exp.decode('latin-1')
        assert py_cssmin(inp, keep_bang_comments=True) == exp
        assert c_cssmin(inp, keep_bang_comments=True) == exp
