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

#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <libtcod.hpp>
#ifdef DEBUG
#	include <iostream>
#endif
#include <algorithm>

#include <boost/serialization/weak_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/list.hpp>

#include "Random.hpp"
#include "Construction.hpp"
#include "Announce.hpp"
#include "Logger.hpp"
#include "Game.hpp"
#include "Map.hpp"
#include "JobManager.hpp"
#include "GCamp.hpp"
#include "Camp.hpp"
#include "StockManager.hpp"
#include "UI.hpp"
#include "UI/ConstructionDialog.hpp"
#include "Item.hpp"
#include "scripting/Event.hpp"
#include "Faction.hpp"
#include "Stockpile.hpp"
#include "Stats.hpp"
#include "data/Config.hpp"

Coordinate Construction::Blueprint(ConstructionType construct) {
	return Construction::Presets[construct].blueprint;
}

Coordinate Construction::ProductionSpot(ConstructionType construct) {
	return Construction::Presets[construct].productionSpot;
}

boost::unordered_map<std::string, ConstructionType> Construction::constructionNames = boost::unordered_map<std::string, ConstructionType>();

ConstructionType Construction::StringToConstructionType(std::string name) {
	boost::to_upper(name);
	if (constructionNames.find(name) == constructionNames.end()) {
		return -1;
	}
	return constructionNames[name];
}

std::string Construction::ConstructionTypeToString(ConstructionType type) {
	return Construction::Presets[type].name;
}

std::vector<int> Construction::AllowedAmount = std::vector<int>();

Construction::Construction(ConstructionType vtype, const Coordinate& target) : Entity(),
	color(TCODColor::white),
	type(vtype),
	producer(false),
	progress(0),
	container(boost::shared_ptr<Container>(new Container(Construction::Presets[type].productionSpot + target, -1, 1000, -1))),
	materialsUsed(boost::shared_ptr<Container>(new Container(Construction::Presets[type].productionSpot + target, -1, 1000, -1))),
	dismantle(false),
	time(0),
	built(false),
	flammable(false),
	smoke(0)
{
	pos = target;
	graphic = Construction::Presets[type].graphic;
	maxCondition = Construction::Presets[type].maxCondition;
	materials = Construction::Presets[type].materials;
	name = Construction::Presets[type].name;
	walkable = Construction::Presets[type].walkable;
	producer = Construction::Presets[type].producer;
	products = Construction::Presets[type].products;
	stockpile = Construction::Presets[type].tags[STOCKPILE];
	farmplot = Construction::Presets[type].tags[FARMPLOT];
	condition = 0-maxCondition;
	if (Construction::Presets[type].color != TCODColor::black) color = Construction::Presets[type].color;
}

Construction::~Construction() {
	Coordinate end = pos + Construction::Blueprint(type);
	for (int ix = pos.X(); ix < end.X(); ++ix) {
		for (int iy = pos.Y(); iy < end.Y(); ++iy) {
			Coordinate p(ix, iy);
			map->SetBuildable(p,true);
			map->SetWalkable(p,true);
			map->SetConstruction(p,-1);
			map->SetBlocksLight(p,false);
			map->SetBlocksWater(p,false);
		}
	}
	
	for (std::set<boost::weak_ptr<Item> >::iterator itemi = materialsUsed->begin(); itemi != materialsUsed->end(); ++itemi) {
		if (itemi->lock()) {
			itemi->lock()->SetFaction(PLAYERFACTION); //Return item to player faction
			itemi->lock()->PutInContainer(boost::weak_ptr<Item>()); //Set container to none
		}
	}
	while (!materialsUsed->empty()) { materialsUsed->RemoveItem(materialsUsed->GetFirstItem()); }

	if (producer) StockManager::Inst()->UpdateWorkshops(boost::weak_ptr<Construction>(), false);

	if (Construction::Presets[type].tags[WALL]) { UpdateWallGraphic(); }
	else if (Construction::Presets[type].tags[DOOR]) { UpdateWallGraphic(true, false); }

	if (Construction::AllowedAmount[type] >= 0) {
		++Construction::AllowedAmount[type];
	}
	
	if (built) {
		if (!HasTag(CENTERSCAMP)) Camp::Inst()->UpdateCenter(Center(), false);
		else Camp::Inst()->UnlockCenter();
	}
}

void Construction::SetMap(Map* map) {
	this->map = map;
}

void Construction::Condition(int value) {condition = value;}
int Construction::Condition() const {return condition;}
int Construction::GetMaxCondition() const {return maxCondition;}


int Construction::GetGraphicsHint() const {
	return Construction::Presets[type].graphicsHint;
}

void Construction::Draw(Coordinate upleft, TCODConsole* console) {
	int screenx = (pos-upleft).X(), screeny = (pos-upleft).Y();
	int ychange = 0;
	int width = graphic[0];
	int height = (graphic.size() - 1) / width;
	if (screenx + width - 1 >= 0 && screenx < console->getWidth() && screeny + height - 1 >= 0 && screeny < console->getHeight()) {
		for (int i = 1; i < (signed int)graphic.size(); ++i) {
			if(screenx + i - 1 >= 0 && screeny >= 0 && screenx + i - 1 < console->getWidth() && screeny < console->getHeight()) {
				if (dismantle) console->setCharBackground(screenx+i-1, screeny, TCODColor::darkGrey);
				else console->setCharBackground(screenx+i-1, screeny,  TCODColor((int)(50 - cos(strobe) * 50), (int)(50 - cos(strobe) * 50), (int)(50 - cos(strobe) * 50)));

				console->setCharForeground(screenx+i-1,screeny, color);
				if (condition > i*-10) console->setChar(screenx+i-1,screeny, (graphic[i]));
				else console->setChar(screenx+i-1,screeny, TCOD_CHAR_BLOCK2);
			}
			++ychange;
			if (ychange == width) { ++screeny; screenx -= width; ychange = 0; }
		}
	}
}

Coordinate Construction::Center() const {
	int width = graphic[0];
	int height = (graphic.size() - 1) / width;
	return pos + (Coordinate(width, height) - 1) / 2;
}

int Construction::Build() {
	++condition;
	if (condition > 0) {
		//Check that all the required materials are inside the building
		//It only checks that the amount of materials equals whats expected, but not what those materials are
		//Theoretically it'd be possible to build a construction out of the wrong materials, but this should
		//be impossible in practice.
		if ((signed int)materials.size() != materialsUsed->size()) return BUILD_NOMATERIAL;

		int flame = 0;
		std::list<boost::weak_ptr<Item> > itemsToRemove;
		for (std::set<boost::weak_ptr<Item> >::iterator itemi = materialsUsed->begin(); itemi != materialsUsed->end(); ++itemi) {
			color = TCODColor::lerp(color, itemi->lock()->Color(), 0.75f);
			itemi->lock()->SetFaction(-1); //Remove from player faction so it doesn't show up in stocks
			itemi->lock()->IsFlammable() ? ++flame : --flame;
			if (Random::Generate(9) < 8) { //80% of materials can't be recovered
				itemsToRemove.push_back(*itemi);
			}
		}

		if (flame > 0) flammable = true;

		for (std::list<boost::weak_ptr<Item> >::iterator itemi = itemsToRemove.begin(); itemi != itemsToRemove.end(); ++itemi) {
			materialsUsed->RemoveItem(*itemi);
			Game::Inst()->RemoveItem(*itemi);
			Game::Inst()->CreateItem(materialsUsed->Position(), Item::StringToItemType("debris"), false, -1, 
				std::vector<boost::weak_ptr<Item> >(), materialsUsed);
		}

		//TODO: constructions should have the option of having both walkable and unwalkable tiles
		condition = maxCondition;
		Coordinate end = pos + Construction::Blueprint(type).X();
		for (int ix = pos.X(); ix < end.X(); ++ix) {
			for (int iy = pos.Y(); iy < end.Y(); ++iy) {
				Coordinate p(ix,iy);
				map->SetWalkable(p, walkable);
				map->SetBlocksWater(p, !walkable);
				map->SetBlocksLight(p, Construction::Presets[type].blocksLight);
			}
		}

		if (Construction::Presets[type].tags[WALL]) { UpdateWallGraphic(); }
		else if (Construction::Presets[type].tags[DOOR]) { UpdateWallGraphic(true, false); }
		if (producer) {
			StockManager::Inst()->UpdateWorkshops(boost::static_pointer_cast<Construction>(shared_from_this()), true);
			for (unsigned int prod = 0; prod < Construction::Presets[type].products.size(); ++prod) {
				StockManager::Inst()->UpdateQuantity(Construction::Presets[type].products[prod], 0);
			}
		}
		built = true;
		if (!HasTag(CENTERSCAMP)) Camp::Inst()->UpdateCenter(Center(), true);
		else Camp::Inst()->LockCenter(Center());
		Camp::Inst()->ConstructionBuilt(type);
		Stats::Inst()->ConstructionBuilt(Construction::Presets[type].name);

		Script::Event::BuildingCreated(boost::static_pointer_cast<Construction>(shared_from_this()), pos.X(), pos.Y());
	}
	return condition;
}

ConstructionType Construction::Type() const { return type; }

std::list<ItemCategory>* Construction::MaterialList() {return &materials;}

bool Construction::Producer() const {return producer;}

std::vector<ItemType>* Construction::Products() { return &products; }
ItemType Construction::Products(int index) const { return products[index]; }

std::deque<ItemType>* Construction::JobList() { return &jobList; }
ItemType Construction::JobList(int index) const { return jobList[index]; }

void Construction::AddJob(ItemType item) {
	if (!dismantle) {
#ifdef DEBUG
		std::cout<<"Produce "<<Item::ItemTypeToString(item)<<" at "<<name<<'\n';
#endif
		jobList.push_back(item);
		if (jobList.size() == 1) {
			SpawnProductionJob();
		}
	}
}

void Construction::CancelJob(int index) {
	smoke = 0;
	if (index == 0 && index < (signed int)jobList.size()) {
		jobList.erase(jobList.begin());
		//Empty container in case some pickup jobs got done
		while (!container->empty()) {
			boost::weak_ptr<Item> item = container->GetFirstItem();
			container->RemoveItem(item);
			if (item.lock()) item.lock()->PutInContainer();
		}
		while (!jobList.empty() && !reserved && !SpawnProductionJob());
	} else if (index > 0 && index < (signed int)jobList.size()) { 
		jobList.erase(jobList.begin() + index); 
	} else if (condition <= 0) {
		Game::Inst()->RemoveConstruction(boost::static_pointer_cast<Construction>(shared_from_this()));
	}
	else if (dismantle) dismantle = false; //Stop trying to dismantle
}

int Construction::Use() {
	if (jobList.size() > 0) {
		++progress;

		if (smoke == 0) {
			smoke = 1;
			for (std::set<boost::weak_ptr<Item> >::iterator itemi = container->begin(); itemi != container->end(); ++itemi) {
				if (itemi->lock()->IsCategory(Item::StringToItemCategory("Fuel"))) {
					smoke = 2;
					break;
				}
			}
			if (Item::Presets[jobList[0]].categories.find(Item::StringToItemCategory("charcoal")) != Item::Presets[jobList[0]].categories.end())
				smoke = 2;
		}

		if (smoke == 2 && Construction::Presets[type].chimney != undefined) {
			if (Random::Generate(9) == 0) {
				boost::shared_ptr<Spell> smoke = Game::Inst()->CreateSpell(Position()+Construction::Presets[type].chimney, Spell::StringToSpellType("smoke"));
				Coordinate direction;
				Direction wind = map->GetWindDirection();
				if (wind == NORTH || wind == NORTHEAST || wind == NORTHWEST) direction.Y(Random::Generate(25, 75));
				if (wind == SOUTH || wind == SOUTHEAST || wind == SOUTHWEST) direction.Y(Random::Generate(-75, -25));
				if (wind == EAST || wind == NORTHEAST || wind == SOUTHEAST) direction.X(Random::Generate(-75, -25));
				if (wind == WEST || wind == SOUTHWEST || wind == NORTHWEST) direction.X(Random::Generate(25, 75));
				direction += Random::ChooseInRadius(3);
				smoke->CalculateFlightPath(Position()+ Construction::Presets[type].chimney + direction, 5, 1);
				if (Random::Generate(50000) == 0) {
					boost::shared_ptr<Spell> spark = Game::Inst()->CreateSpell(pos, Spell::StringToSpellType("spark"));
					int distance = Random::Generate(0, 15);
					if (distance < 12) {
						distance = 1;
					} else if (distance < 14) {
						distance = 2;
					} else {
						distance = 3;
					}
					direction = Coordinate(0,0);
					if (wind == NORTH || wind == NORTHEAST || wind == NORTHWEST) direction.Y(distance);
					if (wind == SOUTH || wind == SOUTHEAST || wind == SOUTHWEST) direction.Y(-distance);
					if (wind == EAST || wind == NORTHEAST || wind == SOUTHEAST) direction.X(-distance);
					if (wind == WEST || wind == SOUTHWEST || wind == NORTHWEST) direction.X(distance);
					if (Random::Generate(9) < 8) direction += Random::ChooseInRadius(1);
					else direction += Random::ChooseInRadius(3);
					spark->CalculateFlightPath(pos + direction, 50, 1);
				}
			}
		}

		if (progress >= 100) {

			bool allComponentsFound = true;

			for (int compi = 0; compi < (signed int)Item::Components(jobList[0]).size(); ++compi) {
				allComponentsFound = false;
				for (std::set<boost::weak_ptr<Item> >::iterator itemi = container->begin(); itemi != container->end(); ++itemi) {
					if (itemi->lock()->IsCategory(Item::Components(jobList[0], compi))) {
						allComponentsFound = true;
						break;
					}
				}
			}
			if (!allComponentsFound) return -1;

			std::vector<boost::weak_ptr<Item> > components;
			boost::shared_ptr<Container> itemContainer;

			for (int compi = 0; compi < (signed int)Item::Components(jobList[0]).size(); ++compi) {
				for (std::set<boost::weak_ptr<Item> >::iterator itemi = container->begin(); itemi != container->end(); ++itemi) {
					if (itemi->lock()->IsCategory(Item::Components(jobList[0], compi))) {
						if (itemi->lock()->IsCategory(Item::Presets[jobList[0]].containIn)) {
							//This component is the container our product should be placed in
							itemContainer = boost::static_pointer_cast<Container>(itemi->lock());
						} else {
							//Just a component of the product
							components.push_back(*itemi);
						}
						container->RemoveItem(*itemi);
						break;
					}
				}
			}

			//Create the "fruit" of the components. Basically fruits have their seeds as their fruits,
			//this makes berries give their seeds when made into wine, for example.
			for (unsigned int i = 0; i < components.size(); ++i) {
				if (components[i].lock()) {
					for (std::list<ItemType>::iterator fruiti = Item::Presets[components[i].lock()->Type()].fruits.begin(); fruiti != Item::Presets[components[i].lock()->Type()].fruits.end(); ++fruiti) {
						Game::Inst()->CreateItem(Position(), *fruiti, true);
					}
				}
			}

			//Create the items
			if (itemContainer) itemContainer->PutInContainer(); 
			for (int i = 0; i < Item::Presets[jobList[0]].multiplier; ++i) {
				if (itemContainer) Game::Inst()->CreateItem(Position()+Construction::Presets[type].productionSpot, jobList[0], false, 0, components, itemContainer);
				else Game::Inst()->CreateItem(Position()+Construction::Presets[type].productionSpot, jobList[0], true, 0, components);
				Stats::Inst()->ItemBuilt(Item::Presets[jobList[0]].name);
			}

			//Destroy the components
			for (unsigned int i = 0; i < components.size(); ++i) {
				if (components[i].lock()) {
					Game::Inst()->RemoveItem(components[i]);
				}
			}

			progress = 0;
			//~Job removes the job from the jobList
			return 100;
		}
		return progress;
	}
	return -1;
}

std::set<std::string> Construction::Categories = std::set<std::string>();
std::vector<ConstructionPreset> Construction::Presets = std::vector<ConstructionPreset>();

class ConstructionListener : public ITCODParserListener {

	int constructionIndex;

	bool parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
		if (name && boost::iequals(str->getName(), "construction_type")) {

			//Figure out the index, whether this is a new construction or a redefinition
			std::string strName(name);
			boost::to_upper(strName);
			if (Construction::constructionNames.find(strName) != Construction::constructionNames.end()) {
				constructionIndex = Construction::constructionNames[strName];
				//A redefinition, so wipe out the earlier one
				Construction::Presets[constructionIndex] = ConstructionPreset();
				Construction::Presets[constructionIndex].name = name;
				Construction::AllowedAmount[constructionIndex] = -1;
			} else { //New construction
				Construction::Presets.push_back(ConstructionPreset());
				Construction::Presets.back().name = name;
				Construction::constructionNames.insert(std::make_pair(strName, static_cast<ConstructionType>(Construction::Presets.size() - 1)));
				Construction::AllowedAmount.push_back(-1);
				constructionIndex = Construction::Presets.size() - 1;
			}
		}
		return true;
	}

	bool parserFlag(TCODParser *parser,const char *name) {
		if (boost::iequals(name, "walkable")) {
			Construction::Presets[constructionIndex].walkable = true;
			Construction::Presets[constructionIndex].blocksLight = false;
		} else if (boost::iequals(name, "wall")) {
			Construction::Presets[constructionIndex].graphic.push_back(1);
			Construction::Presets[constructionIndex].graphic.push_back('W');
			Construction::Presets[constructionIndex].tags[WALL] = true;
		} else if (boost::iequals(name, "stockpile")) {
			Construction::Presets[constructionIndex].tags[STOCKPILE] = true;
		} else if (boost::iequals(name, "farmplot")) {
			Construction::Presets[constructionIndex].tags[FARMPLOT] = true;
			Construction::Presets[constructionIndex].dynamic = true;
		} else if (boost::iequals(name, "door")) {
			Construction::Presets[constructionIndex].tags[DOOR] = true;
			Construction::Presets[constructionIndex].tags[FURNITURE] = true;
			Construction::Presets[constructionIndex].dynamic = true;
		} else if (boost::iequals(name, "bed")) {
			Construction::Presets[constructionIndex].tags[BED] = true;
			Construction::Presets[constructionIndex].tags[FURNITURE] = true;
		} else if (boost::iequals(name, "furniture")) {
			Construction::Presets[constructionIndex].tags[FURNITURE] = true;
		} else if (boost::iequals(name, "permanent")) {
			Construction::Presets[constructionIndex].permanent = true;
			Construction::Presets[constructionIndex].tags[PERMANENT] = true;
		} else if (boost::iequals(name, "blocksLight")) {
			Construction::Presets[constructionIndex].blocksLight = true;
		} else if (boost::iequals(name, "unique")) {
			Construction::AllowedAmount[constructionIndex] = 1;
		} else if (boost::iequals(name, "centersCamp")) {
			Construction::Presets[constructionIndex].tags[CENTERSCAMP] = true;
		} else if (boost::iequals(name, "spawningPool")) {
			Construction::Presets[constructionIndex].tags[SPAWNINGPOOL] = true;
			Construction::Presets[constructionIndex].dynamic = true;
		} else if (boost::iequals(name, "bridge")) {
			Construction::Presets[constructionIndex].tags[BRIDGE] = true;
			Construction::Presets[constructionIndex].moveSpeedModifier = 0;
		}
		return true;
	}

	bool parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value) {
		if (boost::iequals(name, "graphicLength")) {
			if (Construction::Presets[constructionIndex].graphic.size() == 0)
				Construction::Presets[constructionIndex].graphic.push_back(value.i);
			else
				Construction::Presets[constructionIndex].graphic[0] = value.i;
		} else if (boost::iequals(name, "graphic")) {
			if (Construction::Presets[constructionIndex].graphic.size() == 0) //In case graphicLength hasn't been parsed yet
				Construction::Presets[constructionIndex].graphic.push_back(1);
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				Construction::Presets[constructionIndex].graphic.push_back((intptr_t)TCOD_list_get(value.list,i));
			}
		} else if (boost::iequals(name, "fallbackGraphicsSet")) {
			Construction::Presets[constructionIndex].fallbackGraphicsSet = value.s;
		} else if (boost::iequals(name, "category")) {
			Construction::Presets[constructionIndex].category = value.s;
			Construction::Categories.insert(value.s);
		} else if (boost::iequals(name, "placementType")) {
			Construction::Presets[constructionIndex].placementType = value.i;
		} else if (boost::iequals(name, "materials")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				Construction::Presets[constructionIndex].materials.push_back(Item::StringToItemCategory((char*)TCOD_list_get(value.list,i)));
			}
		} else if (boost::iequals(name, "maxCondition")) {
			Construction::Presets[constructionIndex].maxCondition = value.i;
		} else if (boost::iequals(name, "productionx")) {
			Construction::Presets[constructionIndex].productionSpot.X(value.i);
		} else if (boost::iequals(name, "productiony")) {
			Construction::Presets[constructionIndex].productionSpot.Y(value.i);
		} else if (boost::iequals(name, "spawnsCreatures")) {
			Construction::Presets[constructionIndex].spawnCreaturesTag = value.s;
			Construction::Presets[constructionIndex].dynamic = true;
		} else if (boost::iequals(name, "spawnFrequency")) {
			Construction::Presets[constructionIndex].spawnFrequency = value.i * UPDATES_PER_SECOND;
		} else if (boost::iequals(name, "col")) {
			Construction::Presets[constructionIndex].color = value.col;
		} else if (boost::iequals(name, "tileReqs")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				Construction::Presets[constructionIndex].tileReqs.insert(Tile::StringToTileType((char*)TCOD_list_get(value.list,i)));
				//TILEGRASS changes to TILESNOW in winter
				if (Tile::StringToTileType((char*)TCOD_list_get(value.list,i)) == TILEGRASS) {
					Construction::Presets[constructionIndex].tileReqs.insert(TILESNOW);
				}
			}
		} else if (boost::iequals(name, "tier")) {
			Construction::Presets[constructionIndex].tier = value.i;
		} else if (boost::iequals(name, "description")) {
			/*Tokenize the description string and add/remove spaces to make it fit nicely
			into the 25-width tooltip*/
			/*I was going to use boost::tokenizer but hey it starts giving me assertion failures
			in debug. So let's just do it ourselves then, wouldn't want to use a readymade wheel
			or anything, that'd be dumb*/
			std::string desc(value.s);
			std::vector<std::string> tokens;
			while (desc.length() > 0) {
				tokens.push_back(desc.substr(0, desc.find_first_of(" ")));
				if (desc.find_first_of(" ") != std::string::npos) desc = desc.substr(desc.find_first_of(" ")+1);
				else desc.clear();
			}

			int width = 0;
			for (std::vector<std::string>::iterator it = tokens.begin(); it != tokens.end(); ++it) {
				if (width > 0 && width < 25 && width + it->length() >= 25) {
					Construction::Presets[constructionIndex].description += std::string(25 - width, ' ');
					width = 0;
				} 

				while (width >= 25) width -= 25;
				if (width > 0) {
					Construction::Presets[constructionIndex].description += " ";
					++width;
				}

				Construction::Presets[constructionIndex].description += *it;
				width += it->length();
			}

		} else if (boost::iequals(name, "chimneyx")) {
			Construction::Presets[constructionIndex].chimney.X(value.i);
		} else if (boost::iequals(name, "chimneyy")) {
			Construction::Presets[constructionIndex].chimney.Y(value.i);
		} else if (boost::iequals(name,"type")) {
			Construction::Presets[constructionIndex].trapAttack.Type(Attack::StringToDamageType(value.s));
			Construction::Presets[constructionIndex].dynamic = true;
			Construction::Presets[constructionIndex].tags[TRAP] = true;
		} else if (boost::iequals(name,"damage")) {
			Construction::Presets[constructionIndex].trapAttack.Amount(value.dice);
		} else if (boost::iequals(name,"statusEffects")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				StatusEffectType type = StatusEffect::StringToStatusEffectType((char*)TCOD_list_get(value.list,i));
				if (StatusEffect::IsApplyableStatusEffect(type))
					Construction::Presets[constructionIndex].trapAttack.StatusEffects()->push_back(std::pair<StatusEffectType, int>(type, 100));
			}
		} else if (boost::iequals(name,"effectChances")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				Construction::Presets[constructionIndex].trapAttack.StatusEffects()->at(i).second = (intptr_t)TCOD_list_get(value.list,i);
			}
		} else if (boost::iequals(name,"reloadItem")) {
			Construction::Presets[constructionIndex].trapReloadItem = Item::StringToItemCategory(value.s);
		} else if (boost::iequals(name,"slowMovement")) {
			Construction::Presets[constructionIndex].moveSpeedModifier = value.i;
			Construction::Presets[constructionIndex].walkable = true;
			Construction::Presets[constructionIndex].blocksLight = false;
		} else if (boost::iequals(name,"passiveStatusEffects")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				Construction::Presets[constructionIndex].passiveStatusEffects.push_back(StatusEffect::StringToStatusEffectType((char*)TCOD_list_get(value.list,i)));
			}
			Construction::Presets[constructionIndex].dynamic = true;
			if (Construction::Presets[constructionIndex].passiveStatusEffects.back() == HIGHGROUND)
				Construction::Presets[constructionIndex].tags[RANGEDADVANTAGE] = true;
		}

		return true;
	}

	bool parserEndStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
		if (boost::iequals(str->getName(), "construction_type")) {
			Construction::Presets[constructionIndex].blueprint = Coordinate(Construction::Presets[constructionIndex].graphic[0],
				(Construction::Presets[constructionIndex].graphic.size()-1)/Construction::Presets[constructionIndex].graphic[0]);

			if (Construction::Presets[constructionIndex].tileReqs.empty()) {
				Construction::Presets[constructionIndex].tileReqs.insert(TILEGRASS);
				Construction::Presets[constructionIndex].tileReqs.insert(TILEMUD);
				Construction::Presets[constructionIndex].tileReqs.insert(TILEROCK);
				Construction::Presets[constructionIndex].tileReqs.insert(TILESNOW);
			}

			//Add material information to the description
			if (Construction::Presets[constructionIndex].materials.size() > 0) {
				if (Construction::Presets[constructionIndex].description.length() > 0 && Construction::Presets[constructionIndex].description.length() % 25 != 0)
					Construction::Presets[constructionIndex].description += std::string(25 - Construction::Presets[constructionIndex].description.length() % 25, ' ');
				ItemCategory item = -1;
				int multiplier = 0;

				for (std::list<ItemCategory>::iterator mati = Construction::Presets[constructionIndex].materials.begin(); 
					mati != Construction::Presets[constructionIndex].materials.end(); ++mati) {
						if (item == *mati) ++multiplier;
						else { 
							if (multiplier > 0) {
								Construction::Presets[constructionIndex].description += 
									(boost::format("%s x%d") % Item::ItemCategoryToString(item) % multiplier).str();
								if (Construction::Presets[constructionIndex].description.length() % 25 != 0)
									Construction::Presets[constructionIndex].description += std::string(25 - Construction::Presets[constructionIndex].description.length() % 25, ' ');
							}
							item = *mati;
							multiplier = 1;
						}
				}
				Construction::Presets[constructionIndex].description += 
					(boost::format("%s x%d") % Item::ItemCategoryToString(item) % multiplier).str();

			}
		}
		return true;
	}
	void error(const char *msg) {
		throw std::runtime_error(msg);
	}
};

void Construction::LoadPresets(std::string filename) {
	TCODParser parser = TCODParser();
	TCODParserStruct* constructionTypeStruct = parser.newStructure("construction_type");
	constructionTypeStruct->addProperty("graphicLength", TCOD_TYPE_INT, true);
	constructionTypeStruct->addListProperty("graphic", TCOD_TYPE_INT, true);
	constructionTypeStruct->addFlag("walkable");
	constructionTypeStruct->addListProperty("materials", TCOD_TYPE_STRING, true);
	constructionTypeStruct->addProperty("productionx", TCOD_TYPE_INT, false);
	constructionTypeStruct->addProperty("productiony", TCOD_TYPE_INT, false);
	constructionTypeStruct->addProperty("maxCondition", TCOD_TYPE_INT, true);
	constructionTypeStruct->addFlag("stockpile");
	constructionTypeStruct->addFlag("farmplot");
	constructionTypeStruct->addFlag("wall");
	constructionTypeStruct->addFlag("door");
	constructionTypeStruct->addFlag("bed");
	constructionTypeStruct->addFlag("permanent");
	constructionTypeStruct->addFlag("furniture");
	constructionTypeStruct->addProperty("spawnsCreatures", TCOD_TYPE_STRING, false);
	constructionTypeStruct->addProperty("spawnFrequency", TCOD_TYPE_INT, false);
	constructionTypeStruct->addProperty("category", TCOD_TYPE_STRING, true);
	constructionTypeStruct->addProperty("placementType", TCOD_TYPE_INT, false);
	constructionTypeStruct->addFlag("blocksLight");
	constructionTypeStruct->addProperty("col", TCOD_TYPE_COLOR, false);
	constructionTypeStruct->addFlag("unique");
	constructionTypeStruct->addFlag("centersCamp");
	constructionTypeStruct->addFlag("spawningPool");
	constructionTypeStruct->addListProperty("tileReqs", TCOD_TYPE_STRING, false);
	constructionTypeStruct->addFlag("bridge");
	constructionTypeStruct->addProperty("tier", TCOD_TYPE_INT, false);
	constructionTypeStruct->addProperty("description", TCOD_TYPE_STRING, false);
	constructionTypeStruct->addProperty("fallbackGraphicsSet", TCOD_TYPE_STRING, false);
	constructionTypeStruct->addProperty("chimneyx", TCOD_TYPE_INT, false);
	constructionTypeStruct->addProperty("chimneyy", TCOD_TYPE_INT, false);
	constructionTypeStruct->addProperty("slowMovement", TCOD_TYPE_INT, false);
	constructionTypeStruct->addListProperty("passiveStatusEffects", TCOD_TYPE_STRING, false);

	TCODParserStruct *attackTypeStruct = parser.newStructure("attack");
	const char* damageTypes[] = { "slashing", "piercing", "blunt", "magic", "fire", "cold", "poison", NULL };
	attackTypeStruct->addValueList("type", damageTypes, true);
	attackTypeStruct->addProperty("damage", TCOD_TYPE_DICE, false);
	attackTypeStruct->addListProperty("statusEffects", TCOD_TYPE_STRING, false);
	attackTypeStruct->addListProperty("effectChances", TCOD_TYPE_INT, false);	
	attackTypeStruct->addProperty("reloadItem", TCOD_TYPE_STRING, false);

	constructionTypeStruct->addStructure(attackTypeStruct);

	ConstructionListener listener = ConstructionListener();
	parser.run(filename.c_str(), &listener);
}

bool _ConstructionNameEquals(const ConstructionPreset& preset, const std::string& name) {
	return boost::iequals(preset.name, name);
}

void Construction::ResolveProducts() {
	typedef std::vector<ConstructionPreset>::iterator conIterator;
	typedef std::vector<ItemPreset>::iterator itmIterator;
	using boost::lambda::_1;
	
	for (itmIterator it = Item::Presets.begin(); it != Item::Presets.end(); ++it) {
		ItemPreset& itemPreset = *it;
		
		if (!itemPreset.constructedInRaw.empty()) {
			conIterator conIt = std::find_if(
				Construction::Presets.begin(), Construction::Presets.end(),
				// Could use bit more complicated lambda expression to eliminate
				// separate predicate function entirely, but I think this is more
				// clear to people not used to Boost.Lambda
				boost::bind(_ConstructionNameEquals, _1, itemPreset.constructedInRaw)
			);
			
			if (conIt != Construction::Presets.end()) {
				ConstructionPreset& conPreset = *conIt;
				
				conPreset.producer = conPreset.tags[WORKSHOP] = true;
				conPreset.products.push_back(Item::StringToItemType(itemPreset.name));
				
				itemPreset.constructedInRaw.clear();
			} else {
				LOG(
					"Item " << itemPreset.name <<
					" refers to nonexistant construction " << itemPreset.constructedInRaw << "."
				);
			}
		}
	}
}

bool Construction::SpawnProductionJob() {
	//Only spawn a job if the construction isn't already reserved
	if (!reserved) {
		//First check that the requisite items actually exist
		std::list<boost::weak_ptr<Item> > componentList;
		for (int compi = 0; compi < (signed int)Item::Components(jobList.front()).size(); ++compi) {
			boost::weak_ptr<Item> item = Game::Inst()->FindItemByCategoryFromStockpiles(Item::Components(jobList.front(), compi), Center(), APPLYMINIMUMS);
			if (item.lock()) {
				componentList.push_back(item);
				item.lock()->Reserve(true);
			} else {
				//Not all items available, cancel job and unreserve the reserved items.
				for (std::list<boost::weak_ptr<Item> >::iterator resi = componentList.begin(); resi != componentList.end(); ++resi) {
					resi->lock()->Reserve(false);
				}
				jobList.pop_front();
#ifdef DEBUG
				std::cout<<"Couldn't spawn production job at "<<name<<": components missing\n";
#endif
				return false;
			}
		}

		//Unreserve the items now, because the individual jobs will reserve them for themselves
		for (std::list<boost::weak_ptr<Item> >::iterator resi = componentList.begin(); resi != componentList.end(); ++resi) {
			resi->lock()->Reserve(false);
		}


		boost::shared_ptr<Job> newProductionJob(new Job("Produce "+Item::ItemTypeToString(jobList.front()), MED, 0, false));
		newProductionJob->ConnectToEntity(shared_from_this());
		newProductionJob->ReserveEntity(shared_from_this());

		for (int compi = 0; compi < (signed int)Item::Components(jobList.front()).size(); ++compi) {
			boost::shared_ptr<Job> newPickupJob(new Job("Pickup " + Item::ItemCategoryToString(Item::Components(jobList.front(), compi)) + " for " + Presets[Type()].name));
			newPickupJob->tasks.push_back(Task(FIND, Center(), boost::shared_ptr<Entity>(), Item::Components(jobList.front(), compi), APPLYMINIMUMS | EMPTY));
			newPickupJob->tasks.push_back(Task(MOVE));
			newPickupJob->tasks.push_back(Task(TAKE));
			newPickupJob->tasks.push_back(Task(MOVE, container->Position(), container));
			newPickupJob->tasks.push_back(Task(PUTIN, container->Position(), container));

			newProductionJob->PreReqs()->push_back(newPickupJob);
			newPickupJob->Parent(newProductionJob);

			JobManager::Inst()->AddJob(newPickupJob);
		}
		newProductionJob->tasks.push_back(Task(MOVE, Position()+Construction::ProductionSpot(type)));
		newProductionJob->tasks.push_back(Task(USE, Position()+Construction::ProductionSpot(type), shared_from_this()));
		JobManager::Inst()->AddJob(newProductionJob);
		return true;
	}
#ifdef DEBUG
std::cout<<"Couldn't spawn a production job at "<<name<<": Reserved\n";
#endif
	return false;
}

boost::weak_ptr<Container> Construction::Storage() const {
	if (condition > 0) return container;
	else return materialsUsed;
}

void Construction::UpdateWallGraphic(bool recurse, bool self) {
	Direction dirs[4] = { WEST, EAST, NORTH, SOUTH };
	Coordinate posDir[4];
	int consId[4];
	bool wod[4];

	for (int i = 0; i < 4; ++i) {
		posDir[i] = pos + Coordinate::DirectionToCoordinate(dirs[i]);
		consId[i] = map->GetConstruction(posDir[i]);
		wod[i] = false;
		if (consId[i] > -1) {
			boost::shared_ptr<Construction> cons = Game::Inst()->GetConstruction(consId[i]).lock();
			if (cons && cons->Condition() > 0 && !cons->dismantle
				&& (Construction::Presets[cons->Type()].tags[WALL] || Construction::Presets[cons->Type()].tags[DOOR]))
			{
				wod[i] = true;
			}
		}
	}

	if (self && Construction::Presets[type].tags[WALL]) {
		bool w = wod[0], e = wod[1], n = wod[2], s = wod[3];
		if (n&&s&&e&&w) graphic[1] = 197;
		else if (n&&s&&e) graphic[1] = 195;
		else if (n&&s&&w) graphic[1] = 180;
		else if (n&&e&&w) graphic[1] = 193;
		else if (s&&e&&w) graphic[1] = 194;
		else if (n&&s) graphic[1] = 179;
		else if (e&&w) graphic[1] = 196;
		else if (n&&e) graphic[1] = 192;
		else if (n&&w) graphic[1] = 217;
		else if (s&&e) graphic[1] = 218;
		else if (s&&w) graphic[1] = 191;
		else if (e||w) graphic[1] = 196;
		else if (n||s) graphic[1] = 179;
		else graphic[1] = 197;
	}

	if (recurse)
		for (int i = 0; i < 4; ++i)
			if (wod[i])
				Game::Inst()->GetConstruction(consId[i]).lock()->UpdateWallGraphic(false);
}

bool Construction::HasTag(ConstructionTag tag) const { return Construction::Presets[type].tags[tag]; }

void Construction::Update() {
	if (Construction::Presets[type].spawnCreaturesTag != "" && condition > 0) {
		if (Random::Generate(Construction::Presets[type].spawnFrequency - 1) == 0) {
			NPCType monsterType = Game::Inst()->GetRandomNPCTypeByTag(Construction::Presets[type].spawnCreaturesTag);
			TCODColor announceColor = NPC::Presets[monsterType].tags.find("friendly") != 
				NPC::Presets[monsterType].tags.end() ? TCODColor::green : TCODColor::red;

			if (announceColor == TCODColor::red && Config::GetCVar<bool>("pauseOnDanger")) 
				Game::Inst()->AddDelay(UPDATES_PER_SECOND, boost::bind(&Game::Pause, Game::Inst()));

			int amount = Game::DiceToInt(NPC::Presets[monsterType].group);
			if (amount == 1) {
				Announce::Inst()->AddMsg("A "+NPC::NPCTypeToString(monsterType)+" emerges from the "+name+"!", announceColor, Position());
			} else {
				Announce::Inst()->AddMsg(NPC::Presets[monsterType].plural+" emerge from the "+name+"!", announceColor, Position());
			}
			for (int i = 0; i < amount; ++i) {
				Game::Inst()->CreateNPC(Position() + ProductionSpot(type), monsterType);
			}
		}
	}

	if (!Construction::Presets[type].passiveStatusEffects.empty() && !map->NPCList(pos)->empty()) {
		boost::shared_ptr<NPC> npc = Game::Inst()->GetNPC(*map->NPCList(pos)->begin());
		if (!npc->HasEffect(FLYING)) {
			for (size_t i = 0; i < Construction::Presets[type].passiveStatusEffects.size(); ++i) {
				npc->AddEffect(Construction::Presets[type].passiveStatusEffects[i]);
			}
		}
	}
}

void Construction::Dismantle(const Coordinate&) {
	if (!Construction::Presets[type].permanent && !dismantle) {
		dismantle = true;
		if (producer) {
			jobList.clear();
		}

		if (built) {
			boost::shared_ptr<Job> dismantleJob(new Job((boost::format("Dismantle %s") % name).str(), HIGH, 0, false));
			dismantleJob->ConnectToEntity(shared_from_this());
			dismantleJob->Attempts(3);
			dismantleJob->tasks.push_back(Task(MOVEADJACENT, Position(), shared_from_this()));
			dismantleJob->tasks.push_back(Task(DISMANTLE, Position(), shared_from_this()));
			JobManager::Inst()->AddJob(dismantleJob);
		} else {
			Game::Inst()->RemoveConstruction(boost::static_pointer_cast<Construction>(shared_from_this()));
		}
	}
}

Panel *Construction::GetContextMenu() {
	return ConstructionDialog::ConstructionInfoDialog(boost::static_pointer_cast<Construction>(shared_from_this()));
}
	
void Construction::Damage(Attack* attack) {
	double damageModifier = 1.0;

	switch (attack->Type()) {
	case DAMAGE_SLASH: damageModifier = 0.5; break;

	case DAMAGE_PIERCE: damageModifier = 0.25; break;

	case DAMAGE_BLUNT:
	case DAMAGE_MAGIC:
	case DAMAGE_FIRE:
	case DAMAGE_COLD: damageModifier = 1.0; break;

	case DAMAGE_POISON: damageModifier = 0.1; break;

	default: damageModifier = 1.0; break;
	}

	int damage = (int)(Game::DiceToInt(attack->Amount()) * damageModifier);
	condition -= damage;

	if (attack->Type() == DAMAGE_FIRE && Random::Generate(5) == 0) {
		Game::Inst()->CreateFire(Center());
	}

	if (condition <= 0) {
		if (attack->Type() != DAMAGE_FIRE) Explode();
		else BurnToTheGround();
		Game::Inst()->RemoveConstruction(boost::static_pointer_cast<Construction>(shared_from_this()));
	}
}

void Construction::Explode() {
	for (std::set<boost::weak_ptr<Item> >::iterator itemi = materialsUsed->begin(); itemi != materialsUsed->end(); ++itemi) {
		if (boost::shared_ptr<Item> item = itemi->lock()) {
			item->PutInContainer(); //Set container to none
			Coordinate randomTarget = Random::ChooseInRadius(Position(), 5);
			item->CalculateFlightPath(randomTarget, 50, GetHeight());
			if (item->Type() != Item::StringToItemType("debris")) item->SetFaction(PLAYERFACTION); //Return item to player faction
		}
	}
	while (!materialsUsed->empty()) { materialsUsed->RemoveItem(materialsUsed->GetFirstItem()); }
}

bool Construction::CheckMaterialsPresent() { 
	if ((signed int)materials.size() != materialsUsed->size()) { return false; }
	return true;
}

bool Construction::DismantlingOrdered() { return dismantle; }

bool Construction::Built() { return built; }

ConstructionPreset::ConstructionPreset() :
	maxCondition(0),
	graphic(std::vector<int>()),
	walkable(false),
	materials(std::list<ItemCategory>()),
	producer(false),
	products(std::vector<ItemType>()),
	name("???"),
	blueprint(Coordinate(1,1)),
	productionSpot(Coordinate(0,0)),
	dynamic(false),
	spawnCreaturesTag(""),
	spawnFrequency(10),
	placementType(UIPLACEMENT),
	blocksLight(true),
	permanent(false),
	color(TCODColor::black),
	tileReqs(std::set<TileType>()),
	tier(0),
	description(""),
	fallbackGraphicsSet(""),
	graphicsHint(-1),
	chimney(Coordinate(-1,-1)),
	trapAttack(Attack()),
	trapReloadItem(-1),
	moveSpeedModifier(2)
{
	for (int i = 0; i < TAGCOUNT; ++i) { tags[i] = false; }
}

void Construction::AcceptVisitor(ConstructionVisitor& visitor) {
	visitor.Visit(this);
}

bool Construction::IsFlammable() const { return flammable; }

int Construction::Repair() {
	if (condition < maxCondition) ++condition;
	return condition < maxCondition ? 1 : 100;
}

void Construction::SpawnRepairJob() {
	if (built && condition < maxCondition && !repairJob.lock()) {
		boost::shared_ptr<Item> repairItem = Game::Inst()->FindItemByCategoryFromStockpiles(*boost::next(Construction::Presets[type].materials.begin(), Random::ChooseIndex(Construction::Presets[type].materials)),
			Position()).lock();
		if (repairItem) {
			boost::shared_ptr<Job> repJob(new Job("Repair " + name));
			repJob->ReserveEntity(repairItem);
			repJob->tasks.push_back(Task(MOVE, repairItem->Position()));
			repJob->tasks.push_back(Task(TAKE, repairItem->Position(), repairItem));
			repJob->tasks.push_back(Task(MOVEADJACENT, Position(), shared_from_this()));
			repJob->tasks.push_back(Task(REPAIR, Position(), shared_from_this()));
			repairJob = repJob;
			JobManager::Inst()->AddJob(repJob);
		}
	}
}

void Construction::BurnToTheGround() {
	for (std::set<boost::weak_ptr<Item> >::iterator itemi = materialsUsed->begin(); itemi != materialsUsed->end(); ++itemi) {
		if (boost::shared_ptr<Item> item = itemi->lock()) {
			item->PutInContainer(); //Set container to none
			Coordinate randomTarget = Random::ChooseInRadius(Position(), 2);
			item->Position(randomTarget);
			if (item->Type() != Item::StringToItemType("debris")) item->SetFaction(PLAYERFACTION); //Return item to player faction
			Game::Inst()->CreateFire(randomTarget);
		}
	}
	while (!materialsUsed->empty()) { materialsUsed->RemoveItem(materialsUsed->GetFirstItem()); }
}

int Construction::GetMoveSpeedModifier() { return Construction::Presets[type].moveSpeedModifier; }

bool Construction::CanStrobe() { return true; }

void Construction::save(OutputArchive& ar, const unsigned int version) const {
	ar & boost::serialization::base_object<Entity>(*this);
	ar & condition;
	ar & maxCondition;
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	std::string constructionType(Construction::ConstructionTypeToString(type));
	ar & constructionType;
	ar & walkable;
	ar & materials;
	ar & producer;
	ar & products;
	ar & jobList;
	ar & progress;
	ar & container;
	ar & materialsUsed;
	ar & stockpile;
	ar & farmplot;
	ar & dismantle;
	ar & time;
	ar & AllowedAmount;
	ar & built;
	ar & flammable;
	ar & repairJob;
}

void Construction::load(InputArchive& ar, const unsigned int version) {
	ar & boost::serialization::base_object<Entity>(*this);
	ar & condition;
	ar & maxCondition;
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	bool failedToFindType = false;
	std::string typeName;
	ar & typeName;
	type = Construction::StringToConstructionType(typeName);
	if (type == -1) {
		type = Construction::StringToConstructionType("Saw pit");
		failedToFindType = true;
	}
	ar & walkable;
	ar & materials;
	ar & producer;
	ar & products;
	ar & jobList;
	ar & progress;
	ar & container;
	ar & materialsUsed;
	ar & stockpile;
	ar & farmplot;
	ar & dismantle;
	ar & time;
	ar & AllowedAmount;
	ar & built;
	ar & flammable;
	if (failedToFindType) flammable = true; //So you can burn these constructions
	ar & repairJob;
}
