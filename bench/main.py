#!/usr/bin/env python
# -*- coding: ascii -*-
r"""
=================================
 Benchmark cssmin implementations
=================================

Benchmark cssmin implementations.

:Copyright:

 Copyright 2011 - 2014
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

Usage::

    python -mbench.main [-c COUNT] cssfile ...

    -c COUNT  number of runs per cssfile and minifier. Defaults to 10.

"""
if __doc__:
    __doc__ = __doc__.encode('ascii').decode('unicode_escape')
__author__ = r"Andr\xe9 Malo".encode('ascii').decode('unicode_escape')
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
    flush = _sys.stdout.flush

    struct = []
    inputs = [(filename, slurp(filename)) for filename in filenames]
    for filename, style in inputs:
        print_("Benchmarking %r..." % filename, end=" ")
        flush()
        outputs = [cssmin(style) for _, cssmin in ports]
        struct.append(dict(filename=filename, messages=[], times=[]))
        failed = []
        for idx in xrange(1, len(outputs)):
            if outputs[idx] != outputs[0]:
                failed.append(ports[idx][0])
        struct[-1]['messages'].append("(%.1f KiB -> %.1f KiB)" % (
            len(style) / 1024.0, len(outputs[0]) / 1024.0,
        ))
        print_(struct[-1]['messages'][-1])
        if failed:
            for item in failed:
                struct[-1]['messages'].append(
                    "  NOTE - Output of %r differs" % (item,)
                )
                print_(struct[-1]['messages'][-1])
        else:
            struct[-1]['messages'].append("  ok   - Output identical")
            print_(struct[-1]['messages'][-1])
        flush()
        times = []
        for name, cssmin in ports:
            print_("  Timing %s..." % name, end=" ")
            flush()

            xcount = count
            while True:
                counted = [None for _ in xrange(xcount)]
                start = _time.time()
                for _ in counted:
                    cssmin(style)
                end = _time.time()
                result = (end - start) * 1000
                if result < 10: # avoid measuring within the error range
                    xcount *= 10
                    continue
                times.append(result / xcount)
                break

            print_("%.2f ms" % times[-1], end=" ")
            flush()
            if len(times) <= 1:
                print_()
            else:
                print_("(factor: %s)" % (', '.join([
                    '%.2f' % (timed / times[-1]) for timed in times[:-1]
                ])))
            flush()
            struct[-1]['times'].append((name, times[-1]))

        print_()

    return struct


def main(argv=None):
    """ Main """
    import getopt as _getopt
    import os as _os
    import pickle as _pickle

    if argv is None:
        argv = _sys.argv[1:]
    try:
        opts, args = _getopt.getopt(argv, "hc:p:", ["help"])
    except getopt.GetoptError:
        e = _sys.exc_info()[0](_sys.exc_info()[1])
        print >> _sys.stderr, "%s\nTry %s -mbench.main --help" % (
            e,
            _os.path.basename(_sys.executable),
        )
        _sys.exit(2)

    count, pickle = 10, None
    for key, value in opts:
        if key in ("-h", "--help"):
            print >> _sys.stderr, (
                "%s -mbench.main [-c count] [-p file] cssfile ..." % (
                    _os.path.basename(_sys.executable),
                )
            )
            _sys.exit(0)
        elif key == '-c':
            count = int(value)
        elif key == '-p':
            pickle = str(value)

    struct = bench(args, count)
    if pickle:
        fp = open(pickle, 'wb')
        try:
            fp.write(_pickle.dumps((
                ".".join(map(str, _sys.version_info[:3])), struct
            ), 0))
        finally:
            fp.close()


if __name__ == '__main__':
    main()
