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

#include <boost/python/detail/wrap_python.hpp>
#include <boost/python.hpp>
namespace py = boost::python;

#include "scripting/_gcampapi/Functions.hpp"
#include "Announce.hpp"
#include "scripting/API.hpp"
#include "Version.hpp"
#include "UI/MessageBox.hpp"
#include "Game.hpp"
#include "Item.hpp"
#include "Construction.hpp"
#include "NatureObject.hpp"
#include "Logger.hpp"

namespace Script { namespace API {
	void Announce(const std::string& str) {
		::Announce::Inst()->AddMsg(str);
	}
	
	bool IsDebugBuild() {
	#ifdef DEBUG
		return true;
	#else
		return false;
	#endif
	}
	
	bool IsDevMode() {
		return Game::Inst()->DevMode();
	}
	
	const char *GetVersionString() {
		return Globals::gameVersion;
	}
	
	void MessageBox(const std::string& str) {
		MessageBox::ShowMessageBox(str);
	}
	
	void Delay(int delay, PyObject* callback) {
		if (!PyCallable_Check(callback)) {
			LOG("WARNING: Attempted to add a delay to an uncallable object");
			return;
		}
		py::object function(py::handle<>(py::borrowed(callback)));
		Game::Inst()->AddDelay(delay, function);
	}
	
	enum EntityType {
		EConstr, EItem, ENPC, EPlant
	};
	
	int _SpawnItem(Coordinate coords, int type) {
		return Game::Inst()->CreateItem(coords, type);
	}
	
	// XXX:  it doesn't 'spawn' constructions, it builds them (as in will fail and return -1 when there are no resources)
	// TODO: make it spawn, and reserve building for something else
	int SpawnEntity(EntityType type, const std::string& name, int x, int y) {
		boost::function<int(Coordinate, int)> spawn;
		boost::function<int(std::string)> getID;
		Coordinate coords(x, y);
		
		switch (type) {
			case EConstr:
				spawn = &Game::PlaceConstruction;
				getID = &Construction::StringToConstructionType;
			break;
			case EItem:
				//spawn = boost::bind(&Game::CreateItem, Game::Inst(), _1, _2); // this makes the compiler cry for some reason
				spawn = &_SpawnItem;
				getID = &Item::StringToItemType;
			break;
			case ENPC:
				spawn = boost::bind(&Game::CreateNPC, Game::Inst(), _1, _2);
				getID = &NPC::StringToNPCType;
			break;
			case EPlant:
				Game::Inst()->CreateNatureObject(coords, name);
				return -1;
			default:
				PyErr_SetString(PyExc_ValueError, "Invalid type");
				py::throw_error_already_set();
				return -1;
		}
		
		int id = getID(name);
		
		if (id == -1) {
			PyErr_SetString(PyExc_ValueError, "Invalid name");
			py::throw_error_already_set();
		}
		
		return spawn(coords, id);
	}
	
	void ExposeFunctions() {
		py::def("announce",         &Announce);
		py::def("appendListener",   &Script::AppendListener);
		py::def("getVersionString", &GetVersionString);
		py::def("isDebugBuild",     &IsDebugBuild);
		py::def("isDevMode",        &IsDevMode);
		py::def("messageBox",       &MessageBox);
		py::def("delay",            &Delay);
		py::def("spawnEntity",      &SpawnEntity);
		
		py::enum_<EntityType>("EntityType").
			value("ENTITY_BUILDING", EConstr).
			value("ENTITY_ITEM",     EItem).
			value("ENTITY_NPC",      ENPC).
			value("ENTITY_PLANT",    EPlant).
		export_values();
	}
}}
