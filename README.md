# rCSSmin - A CSS Minifier For Python

TABLE OF CONTENTS
-----------------

1. Introduction
1. Copyright and License
1. System Requirements
1. Installation
1. Documentation
1. Bugs
1. Author Information


## INTRODUCTION

RCSSmin is a CSS minifier written in python.

The minifier is based on the semantics of the `YUI compressor`_\, which itself
is based on `the rule list by Isaac Schlueter`_\.

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
  - Mac-IE5 hack (``/*\*/.../**/``)
  - The boxmodelhack is supported naturally because it relies on valid CSS2
    strings
  - Between ``:first-line`` and the following comma or curly brace a space is
    inserted. (apparently it's needed for IE6)
  - Same for ``:first-letter``

rcssmin.c is a reimplementation of rcssmin.py in C and improves runtime up to
factor 100 or so (depending on the input). docs/BENCHMARKS in the source
distribution contains the details.

.. _YUI compressor: https://github.com/yui/yuicompressor/

.. _the rule list by Isaac Schlueter: https://github.com/isaacs/cssmin/

* [Change Log](CHANGES)
* [Development](docs/DEVELOPMENT.md)


## COPYRIGHT AND LICENSE

Copyright 2011 - 2024
André Malo or his licensors, as applicable.

The whole package (except for the files in the bench/ directory)
is distributed under the Apache License Version 2.0. You'll find a copy in the
root directory of the distribution or online at:
<http://www.apache.org/licenses/LICENSE-2.0>.


## SYSTEM REQUIREMENTS

Supported python versions are 2.7 and 3.6+.

You also need a build environment for python C extensions (i.e. a compiler
and the python development files).


## INSTALLATION

### Using pip

```
$ pip install rcssmin
```


### Using distutils

Download the package, unpack it, change into the directory

```
$ python setup.py install
```

The command above will install a new "rcssmin" package into python's
library path.


### Drop-in

rCSSmin effectively consists of two files: rcssmin.py and rcssmin.c, the
latter being entirely optional. So, for simple integration you can just
copy rcssmin.py into your project and use it.


## DOCUMENTATION

The module provides a simple function, called cssmin which takes the CSS as
a string and returns the minified CSS as a string.

The module additionally provides a "streamy" interface:

```
$ python -mrcssmin <css >minified
```

It takes two options:

  -b  Keep bang-comments (Comments starting with an exclamation mark)
  -p  Force using the python implementation (not the C implementation)

The latest documentation is also available online at
<http://opensource.perlig.de/rcssmin/>.


## BUGS

No bugs, of course. ;-)
But if you've found one or have an idea how to improve rcssmin, feel free
to send a pull request on [github](https://github.com/ndparker/rcssmin)
or send a mail to <rcssmin-bugs@perlig.de>.


## AUTHOR INFORMATION

André "nd" Malo <nd@perlig.de>, GPG: 0x029C942244325167


>  If God intended people to be naked, they would be born that way.
>                                                   -- Oscar Wilde
