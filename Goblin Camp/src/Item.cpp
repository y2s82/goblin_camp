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

#include <libtcod.hpp>
#include <ticpp.h>
#include <boost/shared_ptr.hpp>
#include <boost/algorithm/string.hpp>
#ifdef DEBUG
#include <iostream>
#include <boost/format.hpp>
#endif

#include "Item.hpp"
#include "Game.hpp"
#include "Map.hpp"
#include "Logger.hpp"
#include "StockManager.hpp"

std::vector<ItemPreset> Item::Presets = std::vector<ItemPreset>();
std::vector<ItemCat> Item::Categories = std::vector<ItemCat>();
std::map<std::string, ItemType> Item::itemTypeNames = std::map<std::string, ItemType>();
std::map<std::string, ItemType> Item::itemCategoryNames = std::map<std::string, ItemType>();

Item::Item(Coordinate pos, ItemType typeval, int owner, std::vector<boost::weak_ptr<Item> > components) : Entity(),
	type(typeval),
	flammable(false),
	attemptedStore(false),
	decayCounter(-1),
	ownerFaction(owner),
	container(boost::weak_ptr<Item>())
{
	//Remember that the components are destroyed after this constructor!
	x = pos.X();
	y = pos.Y();

	name = Item::Presets[type].name;
	categories = Item::Presets[type].categories;
	graphic = Item::Presets[type].graphic;
	color = Item::Presets[type].color;
	if (Item::Presets[type].decays) decayCounter = Item::Presets[type].decaySpeed;

	for (int i = 0; i < (signed int)components.size(); ++i) {
		if (components[i].lock()) 
			color = TCODColor::lerp(color, components[i].lock()->Color(), 0.5f);
	}

	if (ownerFaction == 0) { //Player owned
		StockManager::Inst()->UpdateQuantity(type, 1);
	}
}

Item::~Item() {
#ifdef DEBUG
	std::cout<<"Item destroyed\n";
#endif
	if (ownerFaction == 0) {
		StockManager::Inst()->UpdateQuantity(type, -1);
	}
}

void Item::Draw(Coordinate upleft, TCODConsole* console) {
	int screenx = x - upleft.X();
	int screeny = y - upleft.Y();
	if (!container.lock() && screenx >= 0 && screenx < console->getWidth() && screeny >= 0 && screeny < console->getHeight()) {
		console->putCharEx(screenx, screeny, graphic, color, Map::Inst()->BackColor(x,y));
	}
}

ItemType Item::Type() {return type;}
bool Item::IsCategory(ItemCategory category) { return (categories.find(category) != categories.end());}
TCODColor Item::Color() {return color;}
void Item::Color(TCODColor col) {color = col;}

void Item::Position(Coordinate pos) {
	x = pos.X(); y = pos.Y();
}
Coordinate Item::Position() {
	if (container.lock()) return container.lock()->Position();
	return this->Entity::Position();
}

void Item::Reserve(bool value) {
	reserved = value;
	if (!reserved && !container.lock() && !attemptedStore) {
		Game::Inst()->StockpileItem(boost::static_pointer_cast<Item>(shared_from_this()));
		attemptedStore = true;
	}
}

void Item::PutInContainer(boost::weak_ptr<Item> con) {
	container = con;
	attemptedStore = false;

	if (container.lock()) Game::Inst()->ItemContained(boost::static_pointer_cast<Item>(shared_from_this()), true);
	else Game::Inst()->ItemContained(boost::static_pointer_cast<Item>(shared_from_this()), false);

	if (!container.lock() && !reserved) {
		Game::Inst()->StockpileItem(boost::static_pointer_cast<Item>(shared_from_this()));
		attemptedStore = true;
	}
}
boost::weak_ptr<Item> Item::ContainedIn() {return container;}

int Item::Graphic() {return graphic;}

std::string Item::ItemTypeToString(ItemType type) {
	return Item::Presets[type].name;
}

ItemType Item::StringToItemType(std::string str) {
	return itemTypeNames[str];
}

std::string Item::ItemCategoryToString(ItemCategory category) {
	return Item::Categories[category].name;
}

ItemCategory Item::StringToItemCategory(std::string str) {
	return Item::itemCategoryNames[str];
}

std::vector<ItemCategory> Item::Components(ItemType type) {
	return Item::Presets[type].components;
}

ItemCategory Item::Components(ItemType type, int index) {
	return Item::Presets[type].components[index];
}

enum ItemListenerMode {
	ITEMMODE,
	CATEGORYMODE,
	COMPONENTMODE
};

class ItemListener : public ITCODParserListener {
	ItemListenerMode mode;
	/*preset[x] holds item names as strings untill all items have been
	read, and then they are converted into ItemTypes */
	std::vector<std::string> presetGrowth;
	std::vector<std::vector<std::string> > presetFruits;
	std::vector<std::vector<std::string> > presetDecay;

public:
	ItemListener() : ITCODParserListener(),
		mode(CATEGORYMODE) {
	}

	void translateNames() {
		for (unsigned int i = 0; i < Item::Presets.size(); ++i) {
			if (presetGrowth[i] != "") Item::Presets[i].growth = Item::StringToItemType(presetGrowth[i]);
			for (unsigned int fruit = 0; fruit < presetFruits[i].size(); ++fruit) {
				Item::Presets[i].fruits.push_back(Item::StringToItemType(presetFruits[i][fruit]));
			}
			for (unsigned int decay = 0; decay < presetDecay[i].size(); ++decay) {
				if (boost::iequals(presetDecay[i][decay], "Filth"))
					Item::Presets[i].decayList.push_back(-1);
				else
					Item::Presets[i].decayList.push_back(Item::StringToItemType(presetDecay[i][decay]));
			}
		}
	}

private:
	bool parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
#ifdef DEBUG
		std::cout<<(boost::format("new %s structure: '%s'\n") % str->getName() % name).str();
#endif
		if (boost::iequals(str->getName(), "category_type")) {
			mode = CATEGORYMODE;
			Item::Categories.push_back(ItemCat());
			++Game::ItemCatCount;
			Item::Categories.back().name = name;
			Item::itemCategoryNames.insert(std::pair<std::string, ItemCategory>(name, Game::ItemCatCount-1));
		} else if (boost::iequals(str->getName(), "item_type")) {
			mode = ITEMMODE;
			Item::Presets.push_back(ItemPreset());
			presetGrowth.push_back("");
			presetFruits.push_back(std::vector<std::string>());
			presetDecay.push_back(std::vector<std::string>());
			++Game::ItemTypeCount;
			Item::Presets.back().name = name;
			Item::itemTypeNames.insert(std::pair<std::string, ItemType>(name, Game::ItemTypeCount-1));
		} 

		return true;
	}

	bool parserFlag(TCODParser *parser,const char *name) {
#ifdef DEBUG
		std::cout<<(boost::format("%s\n") % name).str();
#endif
		return true;
	}

	bool parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value) {
#ifdef DEBUG
		std::cout<<(boost::format("%s\n") % name).str();
#endif
		if (boost::iequals(name, "category")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				Item::Presets.back().categories.insert(Item::StringToItemCategory((char*)TCOD_list_get(value.list,i)));
			}
		} else if (boost::iequals(name, "graphic")) {
			Item::Presets.back().graphic = value.i;
		} else if (boost::iequals(name, "color")) {
			Item::Presets.back().color = value.col;
		} else if (boost::iequals(name, "components")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				Item::Presets.back().components.push_back(Item::StringToItemCategory((char*)TCOD_list_get(value.list, i)));
			}
		} else if (boost::iequals(name, "containin")) {
			Item::Presets.back().containIn = Item::StringToItemCategory(value.s);
		} else if (boost::iequals(name, "nutrition")) {
			Item::Presets.back().nutrition = value.i;
			Item::Presets.back().organic = true;
		} else if (boost::iequals(name, "growth")) {
			presetGrowth.back() = value.s;
			Item::Presets.back().organic = true;
		} else if (boost::iequals(name, "fruits")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				presetFruits.back().push_back((char*)TCOD_list_get(value.list,i));
			}
		} else if (boost::iequals(name, "multiplier")) {
			Item::Presets.back().multiplier = value.i;
		} else if (boost::iequals(name, "containerSize")) {
			Item::Presets.back().container = value.i;
		} else if (boost::iequals(name, "fitsin")) {
			Item::Presets.back().fitsin = Item::StringToItemCategory(value.s);
		} else if (boost::iequals(name, "decay")) {
			Item::Presets.back().decays = true;
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				presetDecay.back().push_back((char*)TCOD_list_get(value.list,i));
			}
		} else if (boost::iequals(name, "decaySpeed")) {
			Item::Presets.back().decaySpeed = value.i;
		}		
		return true;
	}

	bool parserEndStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
#ifdef DEBUG
		std::cout<<(boost::format("end of %s structure\n") % name).str();
#endif
		return true;
	}
	void error(const char *msg) {
		Logger::Inst()->output<<"ItemListener: "<<msg<<"\n";
		Game::Inst()->Exit();
	}
};

void Item::LoadPresets(std::string filename) {
	TCODParser parser = TCODParser();
	TCODParserStruct* categoryTypeStruct = parser.newStructure("category_type");
	TCODParserStruct* itemTypeStruct = parser.newStructure("item_type");
	itemTypeStruct->addListProperty("category", TCOD_TYPE_STRING, true);
	itemTypeStruct->addProperty("graphic", TCOD_TYPE_INT, true);
	itemTypeStruct->addProperty("color", TCOD_TYPE_COLOR, true);
	itemTypeStruct->addListProperty("components", TCOD_TYPE_STRING, false);
	itemTypeStruct->addProperty("containIn", TCOD_TYPE_STRING, false);
	itemTypeStruct->addProperty("nutrition", TCOD_TYPE_INT, false);
	itemTypeStruct->addProperty("growth", TCOD_TYPE_STRING, false);
	itemTypeStruct->addListProperty("fruits", TCOD_TYPE_STRING, false);
	itemTypeStruct->addProperty("multiplier", TCOD_TYPE_INT, false);
	itemTypeStruct->addProperty("containerSize", TCOD_TYPE_INT, false);
	itemTypeStruct->addProperty("fitsin", TCOD_TYPE_STRING, false);
	itemTypeStruct->addListProperty("decay", TCOD_TYPE_STRING, false);
	itemTypeStruct->addProperty("decaySpeed", TCOD_TYPE_INT, false);
	ItemListener* itemListener = new ItemListener();
	parser.run(filename.c_str(), itemListener);
	itemListener->translateNames();
}

void Item::Faction(int val) {
	if (val == 0 && ownerFaction != 0) { //Transferred to player
		StockManager::Inst()->UpdateQuantity(type, 1);
	} else if (val != 0 && ownerFaction == 0) { //Transferred from player
		StockManager::Inst()->UpdateQuantity(type, -1);
	}
	ownerFaction = val;
}

int Item::Faction() const { return ownerFaction; }

ItemCat::ItemCat() {}

ItemPreset::ItemPreset() :
graphic('?'),
	color(TCODColor::pink),
	name("Preset default"),
	categories(std::set<ItemCategory>()),
	nutrition(-1),
	growth(-1),
	fruits(std::list<ItemType>()),
	organic(false),
	container(0),
	multiplier(1),
	fitsin(-1),
	containIn(-1),
	decays(false),
	decaySpeed(0),
	decayList(std::vector<ItemType>())
{}

OrganicItem::OrganicItem(Coordinate pos, ItemType typeVal) : Item(pos, typeVal),
	nutrition(-1),
	growth(-1)
{}

int OrganicItem::Nutrition() { return nutrition; }
void OrganicItem::Nutrition(int val) { nutrition = val; }

ItemType OrganicItem::Growth() { return growth; }
void OrganicItem::Growth(ItemType val) { growth = val; }
