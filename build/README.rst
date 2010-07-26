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

* `Python`_ **2.7.x**.
* `bjam`_ **03.1.19** — included with Goblin Camp.

.. _Python: http://python.org/

bjam
++++

You can build bjam using ``boost-build\jam_src\build.bat`` (Windows) or ``boost-build\jam_src\build.sh`` (\*nix) —
``boost-build`` is in the same directory as this README.

Before reporting any problems with the build, use included bjam first!

Dependencies
~~~~~~~~~~~~

Goblin Camp requires several third party libraries to build and link:

* `Boost`_ **1.43**.
* `libtcod`_ **r427** (GC uses not-yet-released SVN version — listed here is a known working revision).
* `Windows SDK`_ **7.1** (or newer; 7.0 may work as well, but older are not supported).

.. _Boost:       http://boost.org
.. _libtcod:     http://doryen.eptalys.net/libtcod
.. _Windows SDK: http://microsoft.com/downloads/details.aspx?FamilyID=6b6c21d2-2006-4afa-9702-529fa782d63b

Goblin Camp build system
------------------------

Configuring build
~~~~~~~~~~~~~~~~~

Before you build GC, you'll need to create a configuration file for the build,
so that compiler can find all needed headers and libraries.

Minimal config file (let's call it ``gc-config.jam``, put it into root directory of the source;
i.e. the one with Jamroot.jam) must have at least two ``using`` directives: for Boost and for
the compiler::

    using msvc ;
    using boost : 1.43 ;

Build system requests specific Boost version to avoid breakages. In place of ``msvc`` you can
use e.g. ``gcc`` (on Linux) or ``darwin`` (on OSX). You can further configure the compiler,
e.g. by giving it custom flags, but this is outside of this document's scope — see `Boost Build`_
documentation instead.

.. _Boost Build: http://boost.org/doc/tools/build/doc/html/bbv2/reference/tools.html

Boost
+++++

If your Boost is not on standard compiler's search path (MSVC: and you don't have LIB/INCLUDE environment
variables set properly), you will need to provide them in config, in format::

    using boost : 1.43 : properties ;

In properties field you can put:

* ``<library>path`` — path to libraries
* ``<include>path`` — path to headers
* ``<layout>layout`` — same value you build Boost with. Either ``<layout>system``
  (common if you've installed Boost via system-provided package manager) or
  ``<layout>versioned`` (default).

Example::

    using boost : 1.43 : <layout>system ;

libtcod
+++++++

Same as with Boost, but the format is::

    using build/libtcod : properties ;

Available properties:

* ``<library>path`` — path to libraries
* ``<include>path`` — path to headers
* ``<debug-name>lib-name <release-name>lib-name`` — library name for debug and release variants respectively.
  If you don't provide these, build system will attempt to guess them (it will print ``** Using libtcod: name``).

Example::

    using build/libtcod : <library>C:\dev\libs\libtcod\lib <include>C:\dev\libs\libtcod\include ;

Windows SDK
+++++++++++

Format::

    using build/winsdk : properties ;

Properties:

* ``<library>path`` — path to libraries
* ``<include>path`` — path to headers

Example::

    using build/winsdk : <library>C:\dev\libs\WinSDK\v7.1\Lib <include>C:\dev\libs\WinSDK\v7.1\Include ;

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
This will copy all data files and executables into ``build\dist\<variant>`` directory.

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

Build system can generate MSVC2008 and MSVC2010 solution and project.
To do this, use::

    bjam --user-config=gc-config.jam -sSLN_USE_CONFIG=gc-config.jam sln2008

You can use ``sln2010`` instead to generate MSVC2010 project.

Generating NSIS installer
~~~~~~~~~~~~~~~~~~~~~~~~~

There is NSIS installer template included with sources. To build it, you need
to build ``release`` variant, install it into dist, and then run::

    bjam --user-config=gc-config.jam nsis

Automatic versioning
~~~~~~~~~~~~~~~~~~~~

Build system generates _version.rc (on Windows) and _version.cpp from ``GC_VERSION``
constant defined in ``Jamroot.jam`` and Mercurial revision ID (if it can be obtained).

This is done to reduce maintenance effort in bumping the GC version.
