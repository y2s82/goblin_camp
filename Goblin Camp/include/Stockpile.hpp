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

