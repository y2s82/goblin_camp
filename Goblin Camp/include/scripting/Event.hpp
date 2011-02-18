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

#include <string>
#include <boost/shared_ptr.hpp>

class Construction;
class Item;

namespace Script {
	namespace Event {
		void GameStart();
		void GameEnd();
		void GameSaved(const std::string&);
		void GameLoaded(const std::string&);
		void BuildingCreated(boost::weak_ptr<Construction>, int, int);
		void BuildingDestroyed(boost::weak_ptr<Construction>, int, int);
		void ItemCreated(boost::weak_ptr<Item>, int, int);
		void TierChanged(int, const std::string&);
		/*void ItemDestroyed(Item*, int, int);
		void NPCSpawned(NPC*, Construction*, int, int);
		void NPCKilled(NPC*, NPC*, int, int);*/
	}
}
