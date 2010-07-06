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
#pragma once

#include "Construction.hpp"

class Stockpile : public Construction {
    friend class Game;

	protected:
		Stockpile(ConstructionType, int symbol, Coordinate);

		int symbol;
		Coordinate a, b; //Opposite corners so we know which tiles the stockpile
						 //approximately encompasses
		 int capacity;
		 std::map<ItemCategory, int> amount;
		 std::map<ItemCategory, bool> allowed;
		 std::map<Coordinate, bool> used;
		 std::map<Coordinate, bool> reserved;
		 std::map<Coordinate, boost::shared_ptr<Container> > containers;
	public:
		int Build();
		virtual void Draw(Coordinate, TCODConsole*);
		boost::weak_ptr<Item> FindItemByCategory(ItemCategory, int flags=0);
		boost::weak_ptr<Item> FindItemByType(ItemType, int flags=0);
		int Symbol();
		void Symbol(int);
		void Expand(Coordinate,Coordinate);
		bool Allowed(ItemCategory);
		bool Allowed(std::set<ItemCategory>);
		bool Full();
		Coordinate FreePosition();
		void ReserveSpot(Coordinate, bool);
		boost::weak_ptr<Container> Storage(Coordinate);
		void SwitchAllowed(ItemCategory);
};

