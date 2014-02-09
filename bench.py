#!/usr/bin/env python
# -*- coding: ascii -*-
#
# Copyright 2011
# Andr\xe9 Malo or his licensors, as applicable
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""
=================================
 Benchmark cssmin implementations
=================================

Benchmark cssmin implementations.

Usage::

    bench.py [-c COUNT] cssfile ...

    -c COUNT  number of runs per cssfile and minifier. Defaults to 10.

"""
__author__ = "Andr\xe9 Malo"
__author__ = getattr(__author__, 'decode', lambda x: __author__)('latin-1')
__docformat__ = "restructuredtext en"
__license__ = "Apache License, Version 2.0"
__version__ = "1.0.0"

import sys as _sys
import time as _time

class _p_02__rcssmin(object):
    def __init__(self):
        import rcssmin
        cssmin = rcssmin._make_cssmin(python_only=True)
        self.cssmin = lambda x: cssmin(x, keep_bang_comments=True)

class _p_03__rcssmin(object):
    def __init__(self):
        import _rcssmin
        cssmin = _rcssmin.cssmin
        self.cssmin = lambda x: cssmin(x, keep_bang_comments=True)

class cssmins(object):
    from bench import cssmin as p_01_cssmin
    p_02_rcssmin = _p_02__rcssmin()
    try:
        p_03__rcssmin = _p_03__rcssmin()
    except ImportError:
        print("C-Port not available")

print("Python Release: %s" % ".".join(map(str, _sys.version_info[:3])))
print("")


def slurp(filename):
    """ Load a file """
    fp = open(filename)
    try:
        return fp.read()
    finally:
        fp.close()


def print_(*value, **kwargs):
    """ Print stuff """
    (kwargs.get('file') or _sys.stdout).write(
        ''.join(value) + kwargs.get('end', '\n')
    )


def bench(filenames, count):
    """
    Benchmark the minifiers with given css samples

    :Parameters:
      `filenames` : sequence
        List of filenames

      `count` : ``int``
        Number of runs per css file and minifier

    :Exceptions:
      - `RuntimeError` : empty filenames sequence
    """
    if not filenames:
        raise RuntimeError("Missing files to benchmark")
    try:
        xrange
    except NameError:
        xrange = range

    ports = [item for item in dir(cssmins) if item.startswith('p_')]
    ports.sort()
    ports = [(item[5:], getattr(cssmins, item).cssmin) for item in ports]
    counted = [None for _ in xrange(count)]
    flush = _sys.stdout.flush

    inputs = [(filename, slurp(filename)) for filename in filenames]
    for filename, script in inputs:
        print_("Benchmarking %r..." % filename, end=" ")
        flush()
        outputs = [cssmin(script) for _, cssmin in ports]
        failed = []
        for idx in xrange(1, len(outputs)):
            if outputs[idx] != outputs[0]:
                failed.append(ports[idx][0])
        print_("(%.1f KiB -> %.1f KiB)" % (
            len(script) / 1024.0, len(outputs[0]) / 1024.0,
        ))
        if failed:
            for item in failed:
                print_("  NOTE - Output of %r differs" % (item,))
        else:
            print_("  ok   - Output identical")
        flush()
        times = []
        for name, cssmin in ports:
            print_("  Timing %s..." % name, end=" ")
            flush()
            start = _time.time()
            for _ in counted:
                cssmin(script)
            end = _time.time()
            times.append((end - start) * 1000 / count)
            print_("%.2f ms" % times[-1], end=" ")
            flush()
            if len(times) <= 1:
                print_()
            else:
                print_("(factor: %s)" % (', '.join([
                    '%.2f' % (timed / times[-1]) for timed in times[:-1]
                ])))
            flush()
        print_()


def main(argv):
    """ Main """
    count, idx = 10, 0
    if argv and argv[0] == '-c':
        count, idx = int(argv[1]), 2
    elif argv and argv[0].startswith('-c'):
        count, idx = int(argv[0][2:]), 1
    bench(argv[idx:], count)


if __name__ == '__main__':
    main(_sys.argv[1:])
