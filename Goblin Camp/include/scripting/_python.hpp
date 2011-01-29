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

// There's a know issue with Python using the _DEBUG symbol to switch to
// using a debug version of every lib, when it should probably use a different
// symbol. For the main body of classes this is fixed using boost's wrapped include in
// stdafx.hpp (needs to be in the precompiled header to prevent multiple linking which
// causes random failures in debug builds)
// For the python modules we just undefine the _DEBUG symbol before the
// include and then restore it afterwards.
//
// At any rate don't include Python.h directly, unless you want magic to break everything

#ifdef _DEBUG
#	define _SAVE_DEBUG
#	undef _DEBUG
#endif

#include <Python.h>

#ifdef __cplusplus
#	include <boost/python.hpp>
namespace py = boost::python;
#endif
