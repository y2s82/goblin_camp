/* Copyright 2010 Ilkka Halila
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

#include "scripting/_python.hpp"

#include "scripting/_gcampapi/APIItem.hpp"
#include "Item.hpp"
#include "Coordinate.hpp"

namespace {
	py::tuple GetPosition(Item *item) {
		Coordinate coords = item->Position();
		return py::make_tuple(coords.X(), coords.Y());
	}
	
	void SetPosition(Item *item, int x, int y) {
		Coordinate coords(x, y);
		item->Position(coords);
	}
	
	py::tuple GetColor(Item *item) {
		TCODColor color = item->Color();
		return py::make_tuple(color.getHue(), color.getSaturation(), color.getValue());
	}
	
	void SetColor(Item *item, float h, float s, float v) {
		TCODColor color(h, s, v);
		item->Color(color);
	}
}

namespace Script { namespace API {
	void ExposeItem() {
		py::class_<Item>("Item", py::no_init)
			//.def("setContainedIn", &Item::PutInContainer)
			//.def("getContainedIn", &Item::ContainedIn)
			.def("getPosition", &GetPosition)
			.def("setPosition", &SetPosition)
			.def("getGraphic",  &Item::Graphic)
			.def("getType",     &Item::Type)
			.def("getColor",    &GetColor)
		;
	}
}}
