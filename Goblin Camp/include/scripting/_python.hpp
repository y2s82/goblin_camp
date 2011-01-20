/* Copyright 2010-2011 Ilkka Halila
This file is part of Goblin Camp.

Goblin Camp is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Goblin Camp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Goblin Camp. If not, see <http://www.gnu.org/licenses/>.*/
#pragma once

// This is ridiculous. *Something* is defining _DEBUG symbol, which
// causes win32's pyconfig.h to issue #pragma comment that causes linker
// to search for python27_d.lib even when I *explicitly* tell it that I
// *don't want* pydebug.
//
// So, stupid workaround. To use debug version of Python,
// define Py_DEBUG directly (<pydebug>on with Boost.Build).
//
// Don't include Python.h directly, unless you want magic to take
// control over what's linked in.

#ifdef _DEBUG
#	define _SAVE_DEBUG
#	undef _DEBUG
#endif

#include <Python.h>

#ifdef __cplusplus
#	include <boost/python.hpp>
namespace py = boost::python;
#endif

#ifdef _SAVE_DEBUG
#	define _DEBUG
#	undef _SAVE_DEBUG
#endif
