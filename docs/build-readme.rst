==================================
Goblin Camp build system reference
==================================

This document describes third iteration of the Goblin Camp build system, and replaces
earlier ``build/README.rst``.

.. This is reStructuredText document. You can build HTML version using rst2html utility
.. bundled with docutils (http://docutils.sourceforge.net).

.. contents:: Table of Contents

-------------------------------
Frequently encountered problems
-------------------------------

* Paths used in configuration file should always use slashes, not backslashes (even on Windows).
* Library names should not include ``lib`` prefix when using GCC.

----------------------------
Required tools and libraries
----------------------------

Goblin Camp, as any non-trivial piece of software, depends on several tools and external libraries.
To build it, you will need:

* A **working C++ compiler**. We support MSVC2010 on Windows, and GCC on Linux and OSX. Codebase is
  guaranteed to build at least on Windows, and tested quite often on Linux. Other compilers and
  operating systems are not supported (and also configurations like building on Linux using winelib).
  Clang compiler should be able to build the codebase, but has not been tested yet. Cross-compiling is
  not yet supported reliably.
* `BJam`_ **03.1.18** or newer. Build system is based on Boost.Build, which in turn is built on top
  of the BJam.
* `Python`_ **2.7** — there are few Python scripts used in the process of building Goblin Camp, so you
  will need a working installation. Python is also used as a scripting engine in the game, so you
  will also need a linkable library (either debug or release version). Neither Python 3.x nor older
  versions of 2.x line are suitable to be used with Goblin Camp at the moment.
* `SDL`_ **1.2** and `SDL_image`_ **1.2**.
* `libpng`_ — both 1.2 and 1.4 should work, but **1.4** is proven to work and recommended.
* `zlib`_ — you should try to always use the latest version (**1.2.5** at the time of writing this document).
* If you're building the Windows version — `Windows SDK`_ **7** (either 7.0 bundled with MSVC2010 or 7.1),
  and while older are not tested against, they should work as well (Goblin Camp is targetting WinXP SP2 and newer).
* If you're building the Linux version — GLX (libGL; or another compatible OpenGL implementation).

Goblin Camp also depends on Boost and libtcod, but those are bundled with the sources,
and built automatically along with the game (you can find them in the ``vendor`` subdirectory).

.. _BJam:        http://sourceforge.net/projects/boost/files/boost-jam/3.1.18/
.. _Windows SDK: http://microsoft.com/downloads/details.aspx?FamilyID=6b6c21d2-2006-4afa-9702-529fa782d63b
.. _zlib:        http://zlib.net
.. _SDL:         http://libsdl.org
.. _SDL_image:   http://www.libsdl.org/projects/SDL_image/
.. _Python:      http://python.org
.. _libpng:      http://libpng.org/pub/png/libpng.html

-------------------------------------------------------
Configuring the build — configuration file and features
-------------------------------------------------------

If your libraries live in non-standard location (e.g. outside paths specified in INCLUDE/LIB
environment variables for MSVC2010), you will need to create a configuration file for the build.
In this file you can also fine-tune compiler settings (e.g. tell the build system to use specific
executable, or specific flags). Configuration file is typically named ``user-config.jam`` (Boost.Build
will look for one in your home directory), but I recommend naming it differently, e.g. ``gc-config.jam``.

Minimal configuration file should at least specify which compiler is to be used, one of::

    using msvc ;
    using gcc ;
    using darwin ;
    using clang ;

To learn what additional options can be specified for the compiler, see `this document`_.

To configure a library you use::

    using library : properties ;

The order of the ``using`` directives does not matter.

.. _this document: http://boost.org/doc/tools/build/doc/html/bbv2/reference/tools.html

~~~~~~~~~~~~~~~~~~
Configuring libpng
~~~~~~~~~~~~~~~~~~

Tool name: ``libpng``.

Available properties:

* ``<library>path`` — path to the library
* ``<include>path`` — path to the headers
* ``<name>library`` — name of the library (default is ``libpng``)

Example::

    using libpng : <library>C:/dev/libs/msvc10-x86/lib <include>C:/dev/libs/msvc10-x86/include <name>lpng14 ;

~~~~~~~~~~~~~~~~~~
Configuring OpenGL
~~~~~~~~~~~~~~~~~~

.. note::
    This tool is relevant only on \*nix systems, and ignored on Windows (as OpenGL libraries are part of the Windows SDK).

Tool name:: ``opengl``.

Available properties:

* ``<library>path`` — path to the library
* ``<include>path`` — path to the headers
* ``<name>library`` — name of the library (default is ``GL``)

Example::

    using opengl : <name>GL <include>/usr/local/include <library>/usr/local/lib ;

~~~~~~~~~~~~~~~~~~
Configuring Python
~~~~~~~~~~~~~~~~~~

Tool name: ``python``.

Available properties:

* ``<library>path`` — path to the library
* ``<include>path`` — path to the headers
* ``<name>library`` — name of the library (NB: ``<debug>yes`` will not modify the name, you need to specify it explicitly) — default is ``python27``
* ``<exec>path``    — path to the interpreter executable
* ``<debug>yes``    — whether debug version of the library is being used (typically named pythonXX_d)

Example::

    using python : <library>C:/dev/apps/Python27/libs <include>C:/dev/apps/Python27/include <exec>C:/dev/apps/Python27/python.exe <name>python27 ;

~~~~~~~~~~~~~~~
Configuring SDL
~~~~~~~~~~~~~~~

.. note::
    This tool handles both SDL and SDL_image.

.. note::
    We're currently assuming SDL.h file is directly in the search path (i.e. not in
    the SDL subdirectory).

Tool name: ``sdl``.

Available properties:

* ``<library>path`` — path to the libraries
* ``<include>path`` — path to the headers

Example::

    using sdl : <library>C:/dev/libs/msvc10-x86/lib <include>C:/dev/libs/msvc10-x86/include/SDL ;

~~~~~~~~~~~~~~~~~~~~~~~
Configuring Windows SDK
~~~~~~~~~~~~~~~~~~~~~~~

.. note::
    This tool is (obviously) used only on Windows.

Tool name: ``winsdk``.

Available properties:

* ``<library>path`` — path to the libraries
* ``<include>path`` — path to the headers

Example::

    using winsdk : <library>C:/dev/libs/WinSDK/v7.1/Lib <include>C:/dev/libs/WinSDK/v7.1/Include ;

~~~~~~~~~~~~~~~~
Configuring zlib
~~~~~~~~~~~~~~~~

Tool name: ``zlib``.

Available properties:

* ``<library>path`` — path to the library
* ``<include>path`` — path to the headers
* ``<name>library`` — name of the library (default is ``zdll`` on Windows and ``z`` on other OSes)

Example::

    using zlib : <library>C:/dev/libs/msvc10-x86/lib <include>C:/dev/libs/msvc10-x86/include <name>zdll ;

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Configuring Goblin Camp — features
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

There are several aspects of the build that are tunable through so-called features,
which are passed on the command line as ``feature=value``.

Available features:

* ``variant`` — possible values are either ``debug`` or ``release``. Debug builds are slow
  (no optimisations), but fast to build and debugging-friendly. Release builds are heavily optimised,
  but may be much slower to build and harder to debug. You can use ``variant=debug,release`` to build
  both variants simultaneously (although that's not very useful).
* ``address-model`` — possible values are either ``32`` or ``64``. Specifies whether compiler
  should generate 32- or 64-bit code. Remember that architecture of the dependencies must match this
  setting (you **cannot** link 32-bit code with 64-bit libs or 64-bit code with 32-bit libs)!
  You can use ``address-model=32,64`` to build both versions simultaneously, but currently it'll
  probably fail at linking (since you can only configure only one search path for libraries).
* ``link`` — possible values are either ``shared`` or ``static``. Tells the build system whether
  it should try to link the resulting binary dynamically or statically. This is more of a hint,
  and may not change much.
* ``runtime-link`` — possible values are either ``shared`` or ``static``. Tells the build system
  whether it should link the C/C++ runtime libraries statically or dynamically. Note that setting
  this to ``static`` is generally a bad idea, and is strongly discouraged.

----------------------
Using the build system
----------------------

To trigger the build, run ``bjam`` with appropriate parameters:

* ``--user-config=file`` — specifies user config to use. See `Configuring the build — configuration file and features`_
* ``-jN`` — specifies the number of parallel compiler processes to use. Recommended setting is at least 2, and more on multicore machines
* ``-a`` — tells bjam to rebuild entire project and all of its children
* ``-tX`` — rebuilds target ``X``
* ``--clean`` — purges build cache (``build/tmp`` directory)

To (re-)build only specific part of the codebase (e.g. to rebuild Goblin Camp without rebuilding vendor projects), change directory
to project's (i.e. from the one containing Jamroot to the one containing Jamfile) and then run invoke bjam. Note that dependencies
will be built automatically, even if you trigger the build of only one project.

After the build, you will find Goblin Camp executables in the ``build/bin-*-*`` directory (actual name depends
on the variant and target address model of the build).

----------------------
Using additional tools
----------------------

There are several tools bundled with the Goblin Camp, all in ``tools`` subdirectory:

* ``bundle-boost.py`` copies Boost headers used by the game into vendor directory
* ``gather-dlls.py`` analyses the executable and looks for DLLs it depends on
* ``generate-solution.py`` generates MS Visual Studio 2010 solution and project for the Goblin Camp
  (it will be put into ``Goblin Camp`` subdirectory, and will run bjam to do the build)
* ``make-installer.py`` creates NSIS installer out of release build
