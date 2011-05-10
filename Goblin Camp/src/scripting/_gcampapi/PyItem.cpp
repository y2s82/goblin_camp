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

#include "scripting/_gcampapi/PyItem.hpp"
#include "Item.hpp"
#include "Coordinate.hpp"
#include "Logger.hpp"

namespace Script { namespace API {
	#define ITEM_ALIVE(var) boost::shared_ptr<Item> var = item.lock()
	
	PyItem::PyItem(boost::weak_ptr<Item> item) : item(item) {
	}
	
	py::tuple PyItem::GetPosition() {
		if (ITEM_ALIVE(ptr)) {
			Coordinate coords = ptr->Position();
			return py::make_tuple(coords.X(), coords.Y());
		} else {
			LOG("WARNING: ITEM POINTER LOST");
			return py::make_tuple(py::object(), py::object());
		}
	}
	
	bool PyItem::SetPosition(int x, int y) {
		if (ITEM_ALIVE(ptr)) {
			Coordinate coords(x, y);
			ptr->Position(coords);
			return true;
		} else {
			LOG("WARNING: ITEM POINTER LOST");
			return false;
		}
	}
	
	py::tuple PyItem::GetColor() {
		if (ITEM_ALIVE(ptr)) {
			TCODColor color = ptr->Color();
			return py::make_tuple(color.getHue(), color.getSaturation(), color.getValue());
		} else {
			LOG("WARNING: ITEM POINTER LOST");
			return py::make_tuple(py::object(), py::object(), py::object());
		}
	}
	
	bool PyItem::SetColor(float h, float s, float v) {
		if (ITEM_ALIVE(ptr)) {
			TCODColor color(h, s, v);
			ptr->Color(color);
			return true;
		} else {
			LOG("WARNING: ITEM POINTER LOST");
			return false;
		}
	}
	
	std::string PyItem::GetTypeString() {
		if (ITEM_ALIVE(ptr)) {
			return Item::ItemTypeToString(GetType());
		} else {
			LOG("WARNING: ITEM POINTER LOST");
			return "<item pointer lost>";
		}
	}
	
	int PyItem::GetGraphic() {
		if (ITEM_ALIVE(ptr)) {
			return ptr->GetGraphic();
		} else {
			LOG("WARNING: ITEM POINTER LOST");
			return -1;
		}
	}
	
	int PyItem::GetType() {
		if (ITEM_ALIVE(ptr)) {
			return ptr->Type();
		} else {
			LOG("WARNING: ITEM POINTER LOST");
			return -1;
		}
	}
	
	void PyItem::Expose() {
		py::class_<PyItem>("PyItem", py::no_init)
			//.def("setContainedIn", &PyItem::PutInContainer)
			//.def("getContainedIn", &PyItem::ContainedIn)
			.def("getPosition",   &PyItem::GetPosition)
			.def("setPosition",   &PyItem::SetPosition)
			.def("getGraphic",    &PyItem::GetGraphic)
			.def("getType",       &PyItem::GetType)
			.def("getColor",      &PyItem::GetColor)
			.def("getTypeString", &PyItem::GetTypeString)
		;
		
		//def("getCategory", &GetCategory);
		//def("getPreset",   &GetPreset);
	}
}}
