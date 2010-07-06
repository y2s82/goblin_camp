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

#include <boost/shared_ptr.hpp>
#include <libtcod.hpp>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <ticpp.h>

#include "Entity.hpp"
#include "Coordinate.hpp"

typedef int ItemCategory;
#define	WOOD 0
#define STONE 1
#define	BEVERAGE 3
#define	FOOD 2
#define	DOOR 4
#define SEED 8

//only an example, should come from an XML file really
typedef int ItemType;
#define CEDARWOOD 0
#define PINEWOOD 0
#define MAPLEWOOD 0
#define GRANITE 2
#define CHALK 3
#define SANDSTONE 4
#define WINE 5
#define RATMEAT 6
#define WOODDOOR 7
#define BERRYSEED 12

typedef unsigned int SeasonType;
#define SPRING (1 << 0)
#define SUMMER (1 << 1)
#define FALL (1 << 2)
#define WINTER (1 << 3)

class ItemCat {
    public:
        ItemCat();
        bool flammable;
        std::string name;
};

struct ItemPreset {
	ItemPreset();
	int graphic;
	TCODColor color;
	std::string name;
	std::set<ItemCategory> categories;
	std::vector<ItemCategory> components;
	SeasonType season;
	int nutrition;
	ItemType growth;
	std::list<ItemType> fruits;
	bool organic;
	int container;
	int multiplier;
	ItemCategory fitsin;
	ItemCategory containIn;
	bool decays;
	int decaySpeed;
	std::vector<ItemType> decayList;
};

class Item : public Entity {
    friend class Game;

	private:
		int graphic;
		ItemType type;
		TCODColor color;
        std::set<ItemCategory> categories;
        bool flammable;
        bool attemptedStore;
        int decayCounter;

        static std::map<std::string, ItemType> itemTypeNames;
        static std::map<std::string, ItemCategory> itemCategoryNames;

    protected:
		int ownerFaction;
		Item(Coordinate, ItemType, int owner = 0,
			std::vector<boost::weak_ptr<Item> > = std::vector<boost::weak_ptr<Item> >());
        boost::weak_ptr<Item> container;

	public:

		static std::string ItemTypeToString(ItemType);
		static ItemType StringToItemType(std::string);
		static std::string ItemCategoryToString(ItemCategory);
		static ItemCategory StringToItemCategory(std::string);

		static std::vector<ItemCategory> Components(ItemType);
		static ItemCategory Components(ItemType, int);

		static void LoadPresets(ticpp::Document);

		static std::vector<ItemCat> Categories;
		static std::vector<ItemPreset> Presets;

        virtual ~Item();

		void Draw(Coordinate, TCODConsole*);
        void PutInContainer(boost::weak_ptr<Item> = boost::weak_ptr<Item>());
        boost::weak_ptr<Item> ContainedIn();
		virtual void Position(Coordinate);
		virtual Coordinate Position();
		ItemType Type();
		int Graphic();
		TCODColor Color();
		void Color(TCODColor);
		bool IsCategory(ItemCategory);
		virtual void Reserve(bool);
		virtual void Faction(int);
		virtual int Faction() const;
};

class OrganicItem : public Item {
    friend class game;

    private:
        SeasonType season;
        int nutrition;
        ItemType growth;

    public:
        OrganicItem(Coordinate, ItemType);
        SeasonType Season();
        void Season(SeasonType);
        int Nutrition();
        void Nutrition(int);
        ItemType Growth();
        void Growth(ItemType);
};
