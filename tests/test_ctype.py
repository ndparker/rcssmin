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

=====================
 C ext special tests
=====================

C ext special tests.
"""
__author__ = u"Andr\xe9 Malo"

from pytest import raises

import rcssmin as _rcssmin

# pylint: disable = protected-access
py_cssmin = _rcssmin._make_cssmin(python_only=True)

import _rcssmin

c_cssmin = _rcssmin.cssmin

from . import _util as _test


def test_keep_bang_comments():
    """keep_bang_comments argument error"""
    with raises(RuntimeError) as e:
        c_cssmin('', keep_bang_comments=_test.badbool)
    assert e.value.args == ('yoyo',)


def test_input_type():
    """input type must be a string or bytes"""
    with raises(TypeError):
        c_cssmin(None)

    with raises(TypeError):
        py_cssmin(None)

    if str is not bytes:
        assert py_cssmin(bytearray(b'x')) == b'x'
        assert isinstance(py_cssmin(bytearray(b'x')), bytearray)
        assert c_cssmin(bytearray(b'x')) == b'x'
        assert isinstance(c_cssmin(bytearray(b'x')), bytearray)
