===========================
Goblin Camp Developer Guide
===========================

.. vim: set filetype=rst fileencoding=utf-8 expandtab tabstop=4 shiftwidth=4 :

.. note::
    This document is a working draft.
    All comments are welcome.

This document describes the Goblin Camp codebase, how it works internally, quirks and gotchas related to modifying the code,
and general guidelines for developers who'd like to contribute.

.. This is reStructuredText document. You can build HTML version using rst2html utility
.. bundled with docutils (http://docutils.sourceforge.net).

.. contents:: Table of Contents

---------------------------
Overview of the code layout
---------------------------

TODO what all those files and directories are.

------------------
Coding conventions
------------------

TODO indent, naming conventions.

------------------------------
Handling external dependencies
------------------------------

TODO what vendor/ is, how to use new stuff, ``VERSION`` and documenting changes, precompiled header.

------------------------
Control flow in the game
------------------------

TODO where do the process start, how to start new game procedurally.

---------
Scripting
---------

TODO how to use and extend the Python bridge.

------------
Common tools
------------

TODO macros set by the build system, ``GC_ASSERT``, ``GC_DEPRECATED``, ``GC_DEBUG_*``.

---------------------------------------------------
Saves, class versioning and backwards compatibility
---------------------------------------------------

TODO how to change save/load members safely.

-------------------------------------
Contributing — forums, IRC, Bitbucket
-------------------------------------

TODO creating forks, forum threads for forks, IRC, asking for reviews and sending changes.

------------------------
Applying for CI builders
------------------------

TODO what to do to get Buildbot builder for your fork.

