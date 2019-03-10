# Development Setup

The easiest way is to setup a virtualenv container and install the code using
the "editable" install feature of pip. Note that the package uses
[invoke](http://www.pyinvoke.org/) for management tasks, which itself will
execute shell commands, which are most likely only runnable on unix/linux
systems.

1. Create a virtual env, either using [virtualenvwrapper](https://virtualenvwrapper.readthedocs.io/en/latest/) or [virtualenv](https://pypi.python.org/pypi/virtualenv) directly and make sure to enter it:

```
    $ mkvirtualenv rcssmin
    $ workon rcssmin
```

1. Checkout the repository and install the dependencies

```
    $ git clone git@github.com:ndparker/rcssmin.git src/rcssmin
    $ cd src/rcssmin
    $ pip install -r development.txt
```

1. For convenience reasons you might want to change into the source directory
   automatically whenever you enter the venv again. Do it like that:

```
    $ echo 'cd "$VIRTUAL_ENV"/src/rcssmin' >> "$VIRTUAL_ENV"/bin/postactivate
```
