Goblin Camp building guide
==========================

This document describes the build system of `Goblin Camp`_.
If you haven't used new build system yet (introduced in `changeset 5aa18bbb7b4d`_),
you should read the entire document.

If you need more help, visit `our forums`_.

.. _Goblin Camp:            http://goblincamp.com
.. _our forums:             http://goblincamp.com/forum
.. _changeset 5aa18bbb7b4d: http://bitbucket.org/genericcontainer/goblin-camp/changeset/5aa18bbb7b4d

.. contents:: Table of Contents

Prerequisites
-------------

Tools
~~~~~

Except for C++ compiler, you will need several tools to be able to build Goblin Camp.
Here's list with known working versions:

* `Python`_ **2.6.6** or newer (**not** 3.x).
* `Boost.Jam`_ **03.1.18**.

.. _Python: http://python.org/
.. _Boost.Jam:   http://sourceforge.net/projects/boost/files/boost-jam/3.1.18/

Dependencies
~~~~~~~~~~~~

Goblin Camp requires several third party libraries to build and link:

* `Boost`_ **1.44** (or newer, but not earlier).
* `libtcod`_ **r494** [must be later than **r477**: new API]
  (the game uses not-yet-released SVN version, which tends to break the API a lot — listed here is a known working revision, stick to it if you're not experienced in handling build errors).
* `Windows SDK`_ **7.1** (or newer; 7.0 may work as well, but older are not supported).
* `Python`_ **2.6.6** or **2.7.x** (3.x will not work).
* `zlib`_.

.. _Boost:       http://boost.org
.. _libtcod:     http://doryen.eptalys.net/libtcod
.. _Windows SDK: http://microsoft.com/downloads/details.aspx?FamilyID=6b6c21d2-2006-4afa-9702-529fa782d63b
.. _zlib:        http://zlib.net

Goblin Camp build system
------------------------

Configuring build
~~~~~~~~~~~~~~~~~

Before you build GC, you'll need to create a configuration file for the build,
so that compiler can find all needed headers and libraries.

Minimal config file (let's call it ``gc-config.jam``, put it into root directory of the source;
i.e. the one with Jamroot.jam) must have at least one ``using`` directive: for the compiler::

    using msvc ;

Build system requests specific Boost version to avoid breakages. In place of ``msvc`` you can
use e.g. ``gcc`` (on Linux) or ``darwin`` (on OSX). You can further configure the compiler,
e.g. by giving it custom flags, but this is outside of this document's scope — see `Boost Build`_
documentation instead.

.. _Boost Build: http://boost.org/doc/tools/build/doc/html/bbv2/reference/tools.html

Boost
+++++

If your Boost is not on standard compiler's search path (MSVC: and you don't have LIB/INCLUDE environment
variables set properly), you will need to provide them in config, in format::

    using boost : 1.44 : properties ;

In properties field you can put:

* ``<library>path`` — path to libraries
* ``<include>path`` — path to headers
* ``<layout>layout`` — same value you build Boost with:
    * ``<layout>system`` — if your Boost libraries **don't have any** tags in their name
      (e.g. they all look like ``libboost_thread.lib``, or similar) — they need
      to be built as multithreaded, with static linkage and dynamic runtime linkage!
    * ``<layout>tagged`` — if your Boost libraries **have** tags in their name, but
      without compiler and version information (e.g. ``libboost_thread-mt-gd.lib``).
    * ``<layout>versioned`` — if your Boost libraries have all tags in their names
      (e.g. ``libboost_thread-vc90-mt-gd-1_43.lib``).

See `Boost documentation`_ for more information about tags.

Example::

    using boost : 1.44 : <layout>tagged ;

.. _Boost documentation: http://boost.org/doc/libs/1_43_0/more/getting_started/unix-variants.html#library-naming

libtcod
+++++++

Same as with Boost, but the format is::

    using libtcod : properties ;

Available properties:

* ``<library>path`` — path to libraries
* ``<include>path`` — path to headers
* ``<debug-name>lib-name <release-name>lib-name`` — library name for debug and release variants respectively.
  If you don't provide these, build system will attempt to guess them (it will print ``** Using libtcod: name``).

Example::

    using libtcod : <library>C:\dev\libs\libtcod\lib <include>C:\dev\libs\libtcod\include ;

Windows SDK
+++++++++++

Format::

    using winsdk : properties ;

Properties:

* ``<library>path`` — path to libraries
* ``<include>path`` — path to headers

Example::

    using winsdk : <library>C:\dev\libs\WinSDK\v7.1\Lib <include>C:\dev\libs\WinSDK\v7.1\Include ;

Python
++++++

Format::

    using python : properties ;

Properties:

* ``<library>path`` — path to libraries
* ``<include>path`` — path to headers
* ``<version>version`` — Python version (major.minor; by default 2.7)
* ``<pydebug>on`` — enable Python debugging (link to debug version of Python)
* ``<suffix>suffix`` — library name suffix (_d is default for debug libs)
* ``<interpreter>python`` — interpreter executable name (default: python)

Example::

    using python : <version>2.7 <library>C:\dev\apps\Python27\libs <include>C:\dev\apps\Python27\include ;

zlib
++++

Format::

    using zlib : properties ;

Properties:

* ``<library>path`` — path to libraries
* ``<include>path`` — path to headers
* ``<name>name`` — name of the library

Example::

    using zlib : <name>zlib1 <library>C:\dev\libs\zlib-1.2.5\lib <include>C:\dev\libs\zlib-1.2.5\include ;

Building
~~~~~~~~

To build you need to run bjam::

    bjam --user-config=gc-config.jam

This will build the default variant (debug), and **not** copy files into ``dist``.

Variants
~~~~~~~~

Goblin Camp can be built in two variants:

* ``debug`` — much slower, but more suitable for testing newest revisions.
* ``release`` — intended for released versions, optimised and without debugging routines compiled in.

To build selected variant append ``variant=<name>`` to the bjam invocation. You can build both at the
same time with ``variant=release,debug``.

Parallel builds
~~~~~~~~~~~~~~~

If you have multicore CPU, you can run more than one compiler instance at the same time, to speed
the compilation up. To use this append ``-jN`` to bjam invocation, where N is number of parallel
processes.

Installing to dist
~~~~~~~~~~~~~~~~~~

To run compiled game, you should install to dist first. To do this, append ``dist`` to bjam invocation.
This will copy all data files and executables into ``build\dist\<variant>-<x86/x64>`` directory.

.. warning::
    Windows:
    
    If your PATH environment variable is not set correctly, you will need to manually copy
    DLLs of libtcod and Boost into dist directory, or the game won't run. Currently
    build system can only find and copy bundled dbghelp.dll (and it was a priority, because
    crash handler depends on new version, and cannot run with OS-provided one).

Additional build system capabilities
------------------------------------

Generating MSVC project files
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Build system can generate MSVC2010 solution and project files.
To do this, use::

    bjam --user-config=gc-config.jam msvc10

Generating NSIS installer
~~~~~~~~~~~~~~~~~~~~~~~~~

There is NSIS installer template included with sources. To build it, you need
to run::

    bjam --user-config=gc-config.jam nsis variant=release

Installer will be saved into ``build\dist\installer``.

.. note::
    You cannot build NSIS installer with ``variant=debug``.

Automatic versioning
~~~~~~~~~~~~~~~~~~~~

Build system generates _version.rc (on Windows) and _version.cpp from ``GC_VERSION``
constant defined in ``Jamroot.jam`` and Mercurial revision ID (if it can be obtained).

This is done to reduce maintenance effort in bumping the GC version.
