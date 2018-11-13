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
#include "stdafx.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/python/detail/wrap_python.hpp>
#include <boost/python.hpp>
namespace py = boost::python;

#include "scripting/_gcampapi/PyConstruction.hpp"
#include "Construction.hpp"
#include "Coordinate.hpp"
#include "Logger.hpp"

namespace Script { namespace API {
	#define CONSTRUCTION_ALIVE(var) boost::shared_ptr<Construction> var = construction.lock()
	
	PyConstruction::PyConstruction(boost::weak_ptr<Construction> construction) : construction(construction) {
	}
	
	py::tuple PyConstruction::GetPosition() {
		if (CONSTRUCTION_ALIVE(ptr)) {
			Coordinate coords = ptr->Position();
			return py::make_tuple(coords.X(), coords.Y());
		} else {
			LOG("WARNING: CONSTRUCTION POINTER LOST");
			return py::make_tuple(py::object(), py::object());
		}
	}
	
	std::string PyConstruction::GetTypeString() {
		if (CONSTRUCTION_ALIVE(ptr)) {
			if (GetType() >= 0) {
				return Construction::Presets[GetType()].name;
			} else {
				LOG("WARNING: CONSTRUCTION TYPE == -1");
				return "<invalid type>";
			}
		} else {
			LOG("WARNING: CONSTRUCTION POINTER LOST");
			return "<construction pointer lost>";
		}
	}
	
	int PyConstruction::GetType() {
		if (CONSTRUCTION_ALIVE(ptr)) {
			return ptr->Type();
		} else {
			LOG("WARNING: CONSTRUCTION POINTER LOST");
			return -1;
		}
	}
	
	void PyConstruction::Expose() {
		py::class_<PyConstruction>("PyConstruction", py::no_init)
			.def("getPosition",   &PyConstruction::GetPosition)
			.def("getType",       &PyConstruction::GetType)
			.def("getTypeString", &PyConstruction::GetTypeString)
		;
	}
}}
