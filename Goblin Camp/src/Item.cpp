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
#ifdef DEBUG
#include <iostream>
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

void Item::LoadPresets(ticpp::Document doc) {
    std::string strVal;
    int intVal = 0;
    ticpp::Element* parent = doc.FirstChildElement();

    Logger::Inst()->output<<"Reading items.xml\n";
    try {
        ticpp::Iterator<ticpp::Node> node;
        for (node = node.begin(parent); node != node.end(); ++node) {
            if (node->Value() == "category") {
#ifdef DEBUG
                std::cout<<"Category\n";
#endif
                Categories.push_back(ItemCat());
                ++Game::ItemCatCount;
                ticpp::Iterator<ticpp::Node> child;
                for (child = child.begin(node->ToElement()); child != child.end(); ++child) {
                    if (child->Value() == "name") {
                        Categories.back().name = child->ToElement()->GetText();
                        itemCategoryNames.insert(std::pair<std::string, ItemCategory>(Categories.back().name, Game::ItemCatCount-1));
                    } else if (child->Value() == "flammable") {
                        Categories.back().flammable = (child->ToElement()->GetText() == "true") ? true : false;
                    }
                }
            } else if (node->Value() == "item") {
#ifdef DEBUG
				std::cout<<"Item\n";
#endif
                Presets.push_back(ItemPreset());
                ++Game::ItemTypeCount;
                ticpp::Iterator<ticpp::Node> child;
                for (child = child.begin(node->ToElement()); child != child.end(); ++child) {
                    if (child->Value() == "name") {
                        Presets.back().name = child->ToElement()->GetText();
#ifdef DEBUG
                        std::cout<<"Name: "<<Presets.back().name<<"\n";
#endif
                        itemTypeNames.insert(std::pair<std::string, ItemType>(Presets.back().name, Game::ItemTypeCount-1));
                    } else if (child->Value() == "category") {
                        Presets.back().categories.insert(Item::StringToItemCategory(child->ToElement()->GetText()));
                    } else if (child->Value() == "graphic") {
                        child->ToElement()->GetText(&intVal);
                        Presets.back().graphic = intVal;
                    } else if (child->Value() == "color") {
                        int r = -1,g = -1,b = -1;
                        ticpp::Iterator<ticpp::Node> c;
                        for (c = c.begin(child->ToElement()); c != c.end(); ++c) {
                            c->ToElement()->GetTextOrDefault(&intVal, 0);
                            if (r == -1) r = intVal;
                            else if (g == -1) g = intVal;
                            else b = intVal;
                        }
                        Presets.back().color = TCODColor(r,g,b);
                    } else if (child->Value() == "components") {
                        ticpp::Iterator<ticpp::Node> c;
                        for (c = c.begin(child->ToElement()); c != c.end(); ++c) {
							if (c->ToElement()->GetAttribute("containin") == "true") {
								Presets.back().containIn = Item::StringToItemCategory(c->ToElement()->GetText());
#ifdef DEBUG
								std::cout<<"Contained in "<<Item::ItemCategoryToString(Presets.back().containIn)<<"\n";
#endif
							} 
							//Even if it's a container for the product we still count it as a
							//component so that its brought to the workshop correctly
							Presets.back().components.push_back(Item::StringToItemCategory(c->ToElement()->GetText()));
						}
                    } else if (child->Value() == "nutrition") {
                        child->ToElement()->GetText(&intVal);
                        Presets.back().nutrition = intVal;
                        Presets.back().organic = true;
                    } else if (child->Value() == "growth") {
                        Presets.back().growth = Item::StringToItemType(child->ToElement()->GetText());
                        Presets.back().organic = true;
                    } else if (child->Value() == "fruits") {
#ifdef DEBUG
                        std::cout<<"Fruits\n";
#endif
                        ticpp::Iterator<ticpp::Node> fruits;
                        for (fruits = fruits.begin(child->ToElement()); fruits != fruits.end(); ++fruits) {
                            fruits->ToElement()->GetText(&intVal);
                            Presets.back().fruits.push_back(intVal);
                        }
                    } else if (child->Value() == "organic") {
                        Presets.back().organic = true;
                    } else if (child->Value() == "multiplier") {
                        child->ToElement()->GetText(&intVal);
                        Presets.back().multiplier = intVal;
                    } else if (child->Value() == "container") {
                        child->ToElement()->GetText(&intVal);
                        Presets.back().container = intVal;
                    } else if (child->Value() == "fitsin") {
                        Presets.back().fitsin = Item::StringToItemCategory(child->ToElement()->GetText());
                    } else if (child->Value() == "decay") {
                        Presets.back().decays = true;
                        ticpp::Iterator<ticpp::Node> decit;
                        for (decit = decit.begin(child->ToElement()); decit != decit.end(); ++decit) {
                            if (decit->Value() == "speed") {
                                decit->ToElement()->GetText(&intVal);
                                Presets.back().decaySpeed = intVal;
                            } else if (decit->Value() == "item") {
                                strVal = decit->ToElement()->GetText();
                                if (strVal != "Filth") {
                                    Presets.back().decayList.push_back(Item::StringToItemType(strVal));
                                } else {
                                    Presets.back().decayList.push_back(-1);
                                }
                            }
                        }
                    }
                }
            }
        }
    } catch (ticpp::Exception& ex) {
        Logger::Inst()->output<<"Failed reading items.xml!\n";
        Logger::Inst()->output<<ex.what()<<'\n';
        Game::Inst()->Exit();
    }

    Logger::Inst()->output<<"Finished reading items.xml\nItems: "<<Presets.size()<<'\n';
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
