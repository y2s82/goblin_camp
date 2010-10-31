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

#include <list>
#include <string>
#include <boost/shared_ptr.hpp>

#include "NPC.hpp"
#include "Construction.hpp"
#include "Item.hpp"
#include "scripting/Engine.hpp"
#include "scripting/Event.hpp"
#include "scripting/API.hpp"
#include "scripting/_gcampapi/PyItem.hpp"

namespace Script { namespace Event {
	void GameStart() {
		Script::InvokeListeners("onGameStart");
	}
	
	void GameEnd() {
		Script::InvokeListeners("onGameEnd");
	}
	
	void GameSaved(const std::string& filename) {
		Script::InvokeListeners("onGameSaved", "(s)", filename.c_str());
	}
	
	void GameLoaded(const std::string& filename) {
		Script::InvokeListeners("onGameLoaded", "(s)", filename.c_str());
	}
	
	/*void BuildingCreated(Construction*, int, int) {
	
	}
	
	void BuildingDestroyed(Construction*, int, int) {
	
	}*/
	
	void ItemCreated(boost::weak_ptr<Item> item, int x, int y) {
		Script::API::PyItem pyitem(item);
		
		py::object obj(boost::ref(pyitem));
		Script::InvokeListeners("onItemCreated", "(Oii)", obj.ptr(), x, y);
	}
	
	/*void ItemDestroyed(Item*, int, int) {
	
	}
	
	void NPCSpawned(NPC*, Construction*, int, int) {
	
	}
	
	void NPCKilled(NPC*, NPC*, int, int) {
	
	}*/
}}
