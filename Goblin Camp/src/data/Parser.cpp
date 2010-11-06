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

#ifdef DEBUG
#	include <iostream>
#endif
#include <algorithm>

#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <libtcod.hpp>

#include "Construction.hpp"
#include "Item.hpp"
#include "NPC.hpp"
#include "NatureObject.hpp"
#include "GCamp.hpp"
#include "Logger.hpp"
#include "Game.hpp"

//
// Contructions parser
//
class ConstructionListener : public ITCODParserListener {

	bool parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
#ifdef DEBUG
		std::cout<<(boost::format("new %s structure: '%s'\n") % str->getName() % name).str();
#endif
		Construction::Presets.push_back(ConstructionPreset());
		Construction::Presets.back().name = name;
		Construction::AllowedAmount.push_back(-1);
		return true;
	}

	bool parserFlag(TCODParser *parser,const char *name) {
#ifdef DEBUG
		std::cout<<(boost::format("%s\n") % name).str();
#endif
		if (boost::iequals(name, "walkable")) {
			Construction::Presets.back().walkable = true;
			Construction::Presets.back().blocksLight = false;
		} else if (boost::iequals(name, "wall")) {
			Construction::Presets.back().graphic.push_back(1);
			Construction::Presets.back().graphic.push_back('W');
			Construction::Presets.back().tags[WALL] = true;
		} else if (boost::iequals(name, "stockpile")) {
			Construction::Presets.back().tags[STOCKPILE] = true;
		} else if (boost::iequals(name, "farmplot")) {
			Construction::Presets.back().tags[FARMPLOT] = true;
			Construction::Presets.back().dynamic = true;
		} else if (boost::iequals(name, "door")) {
			Construction::Presets.back().tags[DOOR] = true;
			Construction::Presets.back().tags[FURNITURE] = true;
			Construction::Presets.back().dynamic = true;
		} else if (boost::iequals(name, "bed")) {
			Construction::Presets.back().tags[BED] = true;
			Construction::Presets.back().tags[FURNITURE] = true;
		} else if (boost::iequals(name, "furniture")) {
			Construction::Presets.back().tags[FURNITURE] = true;
		} else if (boost::iequals(name, "permanent")) {
			Construction::Presets.back().permanent = true;
		} else if (boost::iequals(name, "blocksLight")) {
			Construction::Presets.back().blocksLight = true;
		} else if (boost::iequals(name, "unique")) {
			Construction::AllowedAmount.back() = 1;
		} else if (boost::iequals(name, "centersCamp")) {
			Construction::Presets.back().tags[CENTERSCAMP] = true;
		} else if (boost::iequals(name, "spawningPool")) {
			Construction::Presets.back().tags[SPAWNINGPOOL] = true;
			Construction::Presets.back().dynamic = true;
		} else if (boost::iequals(name, "bridge")) {
			Construction::Presets.back().tags[BRIDGE] = true;
		}
		return true;
	}

	bool parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value) {
#ifdef DEBUG
		std::cout<<(boost::format("%s\n") % name).str();
#endif
		if (boost::iequals(name, "graphicLength")) {
			if (Construction::Presets.back().graphic.size() == 0)
				Construction::Presets.back().graphic.push_back(value.i);
			else
				Construction::Presets.back().graphic[0] = value.i;
		} else if (boost::iequals(name, "graphic")) {
			if (Construction::Presets.back().graphic.size() == 0) //In case graphicLength hasn't been parsed yet
				Construction::Presets.back().graphic.push_back(1);
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				Construction::Presets.back().graphic.push_back((intptr_t)TCOD_list_get(value.list,i));
			}
		} else if (boost::iequals(name, "category")) {
			Construction::Presets.back().category = value.s;
			Construction::Categories.insert(value.s);
		} else if (boost::iequals(name, "placementType")) {
			Construction::Presets.back().placementType = value.i;
		} else if (boost::iequals(name, "materials")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				Construction::Presets.back().materials.push_back(Item::StringToItemCategory((char*)TCOD_list_get(value.list,i)));
			}
		} else if (boost::iequals(name, "maxCondition")) {
			Construction::Presets.back().maxCondition = value.i;
		} else if (boost::iequals(name, "productionx")) {
			Construction::Presets.back().productionSpot.X(value.i);
		} else if (boost::iequals(name, "productiony")) {
			Construction::Presets.back().productionSpot.Y(value.i);
		} else if (boost::iequals(name, "spawnsCreatures")) {
			Construction::Presets.back().spawnCreaturesTag = value.s;
			Construction::Presets.back().dynamic = true;
		} else if (boost::iequals(name, "spawnFrequency")) {
			Construction::Presets.back().spawnFrequency = value.i * UPDATES_PER_SECOND;
		} else if (boost::iequals(name, "color")) {
			Construction::Presets.back().color = value.col;
		} else if (boost::iequals(name, "tileReqs")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				Construction::Presets.back().tileReqs.insert(Tile::StringToTileType((char*)TCOD_list_get(value.list,i)));
#ifdef DEBUG
				std::cout<<"("<<Construction::Presets.back().name<<") Adding tile req "<<(char*)TCOD_list_get(value.list,i)<<"\n";
#endif
			}
		}

		return true;
	}

	bool parserEndStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
#ifdef DEBUG
		std::cout<<(boost::format("end of %s structure\n") % name).str();
#endif
		Construction::Presets.back().blueprint = Coordinate(Construction::Presets.back().graphic[0],
			(Construction::Presets.back().graphic.size()-1)/Construction::Presets.back().graphic[0]);
		if (Construction::Presets.back().tileReqs.empty()) {
			Construction::Presets.back().tileReqs.insert(TILEGRASS);
			Construction::Presets.back().tileReqs.insert(TILEROCK);
		}
		return true;
	}
	void error(const char *msg) {
		LOG("ItemListener: " << msg);
		Game::Inst()->ErrorScreen();
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
	constructionTypeStruct->addProperty("color", TCOD_TYPE_COLOR, false);
	constructionTypeStruct->addFlag("unique");
	constructionTypeStruct->addFlag("centersCamp");
	constructionTypeStruct->addFlag("spawningPool");
	constructionTypeStruct->addListProperty("tileReqs", TCOD_TYPE_STRING, false);
	constructionTypeStruct->addFlag("bridge");
	
	parser.run(filename.c_str(), new ConstructionListener());
}

bool _ResolveProductsPredicate(const ConstructionPreset& preset, const std::string& name) {
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
				boost::bind(_ResolveProductsPredicate, _1, itemPreset.constructedInRaw)
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

//
// NPCs parser
//
class NPCListener : public ITCODParserListener {
	bool parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
#ifdef DEBUG
		std::cout<<boost::format("new %s structure: ") % str->getName();
#endif
		if (boost::iequals(str->getName(), "npc_type")) {
			NPC::Presets.push_back(NPCPreset(name));
			NPC::NPCTypeNames[name] = NPC::Presets.size()-1;
#ifdef DEBUG
			std::cout<<name<<"\n";
#endif
		} else if (boost::iequals(str->getName(), "attack")) {
			NPC::Presets.back().attacks.push_back(Attack());
#ifdef DEBUG
			std::cout<<name<<"\n";
#endif
		} else if (boost::iequals(str->getName(), "resistances")) {
#ifdef DEBUG
			std::cout<<"\n";
#endif
		}
		return true;
	}
	bool parserFlag(TCODParser *parser,const char *name) {
#ifdef DEBUG
		std::cout<<(boost::format("%s\n") % name).str();
#endif
		if (boost::iequals(name,"generateName")) { NPC::Presets.back().generateName = true; }
		else if (boost::iequals(name,"needsNutrition")) { NPC::Presets.back().needsNutrition = true; }
		else if (boost::iequals(name,"needsSleep")) { NPC::Presets.back().needsSleep = true; }
		else if (boost::iequals(name,"expert")) { NPC::Presets.back().expert = true; }
		return true;
	}
	bool parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value) {
#ifdef DEBUG
		std::cout<<(boost::format("%s\n") % name).str();
#endif
		if (boost::iequals(name,"name")) { NPC::Presets.back().name = value.s; }
		else if (boost::iequals(name,"plural")) { NPC::Presets.back().plural = value.s; }
		else if (boost::iequals(name,"speed")) { NPC::Presets.back().stats[MOVESPEED] = value.i; }
		else if (boost::iequals(name,"color")) { NPC::Presets.back().color = value.col; }
		else if (boost::iequals(name,"graphic")) { NPC::Presets.back().graphic = value.c; }
		else if (boost::iequals(name,"health")) { NPC::Presets.back().health = value.i; }
		else if (boost::iequals(name,"AI")) { NPC::Presets.back().ai = value.s; }
		else if (boost::iequals(name,"dodge")) { NPC::Presets.back().stats[DODGE] = value.i; }
		else if (boost::iequals(name,"spawnAsGroup")) { 
			NPC::Presets.back().spawnAsGroup = true;
			NPC::Presets.back().group = value.dice;
		} else if (boost::iequals(name,"type")) {
			NPC::Presets.back().attacks.back().Type(Attack::StringToDamageType(value.s));
		} else if (boost::iequals(name,"damage")) {
			NPC::Presets.back().attacks.back().Amount(value.dice);
		} else if (boost::iequals(name,"cooldown")) {
			NPC::Presets.back().attacks.back().CooldownMax(value.i);
		} else if (boost::iequals(name,"statusEffects")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				NPC::Presets.back().attacks.back().StatusEffects()->push_back(std::pair<StatusEffectType, int>(StatusEffect::StringToStatusEffectType((char*)TCOD_list_get(value.list,i)), 100));
			}
		} else if (boost::iequals(name,"effectChances")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				NPC::Presets.back().attacks.back().StatusEffects()->at(i).second = (intptr_t)TCOD_list_get(value.list,i);
			}
		} else if (boost::iequals(name,"projectile")) {
			NPC::Presets.back().attacks.back().Projectile(Item::StringToItemType(value.s));
		} else if (boost::iequals(name,"physical")) {
			NPC::Presets.back().resistances[PHYSICAL_RES] = value.i;
		} else if (boost::iequals(name,"magic")) {
			NPC::Presets.back().resistances[MAGIC_RES] = value.i;
		} else if (boost::iequals(name,"cold")) {
			NPC::Presets.back().resistances[COLD_RES] = value.i;
		} else if (boost::iequals(name,"fire")) {
			NPC::Presets.back().resistances[FIRE_RES] = value.i;
		} else if (boost::iequals(name,"poison")) {
			NPC::Presets.back().resistances[POISON_RES] = value.i;
		} else if (boost::iequals(name,"tags")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				std::string tag = (char*)TCOD_list_get(value.list,i);
				NPC::Presets.back().tags.insert(boost::to_lower_copy(tag));
			}
		} else if (boost::iequals(name,"strength")) {
			NPC::Presets.back().stats[STRENGTH] = value.i;
		} else if (boost::iequals(name,"size")) {
			NPC::Presets.back().stats[SIZE] = value.i;
			if (NPC::Presets.back().stats[STRENGTH] == 1) NPC::Presets.back().stats[STRENGTH] = value.i;
		}
		return true;
	}
	bool parserEndStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
#ifdef DEBUG
		std::cout<<boost::format("end of %s\n") % str->getName();
#endif
		if (NPC::Presets.back().plural == "") NPC::Presets.back().plural = NPC::Presets.back().name + "s";
		return true;
	}
	void error(const char *msg) {
		LOG("NPCListener: " << msg);
		Game::Inst()->ErrorScreen();
	}
};

void NPC::LoadPresets(std::string filename) {
	TCODParser parser = TCODParser();
	TCODParserStruct *npcTypeStruct = parser.newStructure("npc_type");
	npcTypeStruct->addProperty("name", TCOD_TYPE_STRING, true);
	npcTypeStruct->addProperty("plural", TCOD_TYPE_STRING, false);
	npcTypeStruct->addProperty("color", TCOD_TYPE_COLOR, true);
	npcTypeStruct->addProperty("graphic", TCOD_TYPE_CHAR, true);
	npcTypeStruct->addFlag("expert");
	const char* aiTypes[] = { "PlayerNPC", "PeacefulAnimal", "HungryAnimal", "HostileAnimal", NULL }; 
	npcTypeStruct->addValueList("AI", aiTypes, true);
	npcTypeStruct->addFlag("needsNutrition");
	npcTypeStruct->addFlag("needsSleep");
	npcTypeStruct->addFlag("generateName");
	npcTypeStruct->addProperty("spawnAsGroup", TCOD_TYPE_DICE, false);
	npcTypeStruct->addListProperty("tags", TCOD_TYPE_STRING, false);
	
	TCODParserStruct *attackTypeStruct = parser.newStructure("attack");
	const char* damageTypes[] = { "slashing", "piercing", "blunt", "magic", "fire", "cold", "poison", "wielded", NULL };
	attackTypeStruct->addValueList("type", damageTypes, true);
	attackTypeStruct->addProperty("damage", TCOD_TYPE_DICE, false);
	attackTypeStruct->addProperty("cooldown", TCOD_TYPE_INT, false);
	attackTypeStruct->addListProperty("statusEffects", TCOD_TYPE_STRING, false);
	attackTypeStruct->addListProperty("effectChances", TCOD_TYPE_INT, false);
	attackTypeStruct->addFlag("ranged");
	attackTypeStruct->addProperty("projectile", TCOD_TYPE_STRING, false);
	
	TCODParserStruct *resistancesStruct = parser.newStructure("resistances");
	resistancesStruct->addProperty("physical", TCOD_TYPE_INT, false);
	resistancesStruct->addProperty("magic", TCOD_TYPE_INT, false);
	resistancesStruct->addProperty("cold", TCOD_TYPE_INT, false);
	resistancesStruct->addProperty("fire", TCOD_TYPE_INT, false);
	resistancesStruct->addProperty("poison", TCOD_TYPE_INT, false);
	
	TCODParserStruct *statsStruct = parser.newStructure("stats");
	statsStruct->addProperty("health", TCOD_TYPE_INT, true);
	statsStruct->addProperty("speed", TCOD_TYPE_INT, true);
	statsStruct->addProperty("dodge", TCOD_TYPE_INT, true);
	statsStruct->addProperty("size", TCOD_TYPE_INT, true);
	statsStruct->addProperty("strength", TCOD_TYPE_INT, false);
	
	npcTypeStruct->addStructure(attackTypeStruct);
	npcTypeStruct->addStructure(resistancesStruct);
	npcTypeStruct->addStructure(statsStruct);
	
	parser.run(filename.c_str(), new NPCListener());
}

//
// NatureObjects parser
//
class NatureObjectListener : public ITCODParserListener {

	bool parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
#ifdef DEBUG
		std::cout<<(boost::format("new %s structure: '%s'\n") % str->getName() % name).str();
#endif
		NatureObject::Presets.push_back(NatureObjectPreset());
		NatureObject::Presets.back().name = name;
		return true;
	}

	bool parserFlag(TCODParser *parser,const char *name) {
#ifdef DEBUG
		std::cout<<(boost::format("%s\n") % name).str();
#endif
		if (boost::iequals(name, "walkable")) {
			NatureObject::Presets.back().walkable = true;
		} else if (boost::iequals(name, "harvestable")) {
			NatureObject::Presets.back().harvestable = true;
		} else if (boost::iequals(name, "tree")) {
			NatureObject::Presets.back().tree = true;
		} else if (boost::iequals(name, "evil")) {
			NatureObject::Presets.back().evil = true;
		}
		return true;
	}

	bool parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t type, TCOD_value_t value) {
#ifdef DEBUG
		std::cout<<(boost::format("%s\n") % name).str();
#endif
		if (boost::iequals(name, "graphic")) {
			NatureObject::Presets.back().graphic = value.i;
		} else if (boost::iequals(name, "components")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				NatureObject::Presets.back().components.push_back(Item::StringToItemType((char*)TCOD_list_get(value.list,i)));
			}
		} else if (boost::iequals(name, "color")) {
			NatureObject::Presets.back().color = value.col;
		} else if (boost::iequals(name, "rarity")) {
			NatureObject::Presets.back().rarity = value.i;
		} else if (boost::iequals(name, "cluster")) {
			NatureObject::Presets.back().cluster = value.i;
		} else if (boost::iequals(name, "condition")) {
			NatureObject::Presets.back().condition = value.i;
		} else if (boost::iequals(name, "minheight")) {
			NatureObject::Presets.back().minHeight = value.f;
		} else if (boost::iequals(name, "maxheight")) {
			NatureObject::Presets.back().maxHeight = value.f;
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
		LOG("NatureObjectListener: " << msg);
		Game::Inst()->ErrorScreen();
	}
};

void NatureObject::LoadPresets(std::string filename) {
	TCODParser parser = TCODParser();
	TCODParserStruct* natureObjectTypeStruct = parser.newStructure("plant_type");
	natureObjectTypeStruct->addProperty("graphic", TCOD_TYPE_INT, true);
	natureObjectTypeStruct->addProperty("color", TCOD_TYPE_COLOR, true);
	natureObjectTypeStruct->addListProperty("components", TCOD_TYPE_STRING, false);
	natureObjectTypeStruct->addProperty("rarity", TCOD_TYPE_INT, false);
	natureObjectTypeStruct->addProperty("condition", TCOD_TYPE_INT, false);
	natureObjectTypeStruct->addProperty("cluster", TCOD_TYPE_INT, false);
	natureObjectTypeStruct->addFlag("tree");
	natureObjectTypeStruct->addFlag("harvestable");
	natureObjectTypeStruct->addFlag("walkable");
	natureObjectTypeStruct->addProperty("minheight", TCOD_TYPE_FLOAT, false);
	natureObjectTypeStruct->addProperty("maxheight", TCOD_TYPE_FLOAT, false);
	natureObjectTypeStruct->addFlag("evil");

	parser.run(filename.c_str(), new NatureObjectListener());
}

//
// Items parser
//
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
	std::vector<std::string> presetProjectile;
	std::vector<std::string> presetCategoryParent;
	int firstCategoryIndex;
	int firstItemIndex;
public:
	ItemListener() : ITCODParserListener(),
		mode(CATEGORYMODE),
		firstCategoryIndex(Item::Categories.size()),
		firstItemIndex(Item::Presets.size())
	{
	}
	
	void translateNames() {
		for (unsigned int i = 0; i < presetCategoryParent.size(); ++i) {
			if (presetCategoryParent[i] != "") {
				Item::Categories[firstCategoryIndex+i].parent = &Item::Categories[Item::StringToItemCategory(presetCategoryParent[i])];
			}
		}
		
		/*Misleading to iterate through presetGrowth because we're handling everything else here,
		but presetGrowth.size() = the amount of items read in. Just remember to add firstItemIndex,
		because this items.dat may not be the first one read in, and we don't want to overwrite
		existing items*/
		for (unsigned int i = 0; i < presetGrowth.size(); ++i) {
			
			for (std::set<ItemCategory>::iterator cati = Item::Presets[firstItemIndex+i].categories.begin();
				cati != Item::Presets[firstItemIndex+i].categories.end(); ++cati) {
					if (Item::Categories[*cati].parent) {
#ifdef DEBUG
						std::cout<<"Item has a parent ->"<<Item::Categories[*cati].parent->name<<" = ("<<Item::StringToItemCategory(Item::Categories[*cati].parent->name)<<")\n";
#endif
						Item::Presets[firstItemIndex+i].categories.insert(Item::StringToItemCategory(Item::Categories[*cati].parent->name));
					}
			}

#ifdef DEBUG
			if (presetGrowth[i] != "") {
				Item::Presets[firstItemIndex+i].growth = Item::StringToItemType(presetGrowth[i]);
				std::cout<<"Translating "<<presetGrowth[i]<<" into growth id "<<Item::StringToItemType(presetGrowth[i])<<"\n";
			} else {
				std::cout<<"No growth defined for "<<Item::Presets[firstItemIndex+i].name<<"\n";
			}
			std::cout<<"Growth id = "<<Item::Presets[firstItemIndex+i].growth<<"\n";
#else
			if (presetGrowth[i] != "") Item::Presets[firstItemIndex+i].growth = Item::StringToItemType(presetGrowth[i]);
#endif
			for (unsigned int fruit = 0; fruit < presetFruits[i].size(); ++fruit) {
				Item::Presets[firstItemIndex+i].fruits.push_back(Item::StringToItemType(presetFruits[i][fruit]));
			}
			for (unsigned int decay = 0; decay < presetDecay[i].size(); ++decay) {
				if (boost::iequals(presetDecay[i][decay], "Filth"))
					Item::Presets[firstItemIndex+i].decayList.push_back(-1);
				else
					Item::Presets[firstItemIndex+i].decayList.push_back(Item::StringToItemType(presetDecay[i][decay]));
			}

			if (presetProjectile[i] != "") Item::Presets[firstItemIndex+i].attack.Projectile(Item::StringToItemCategory(presetProjectile[i]));
		}
	}

private:
	bool parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
#ifdef DEBUG
		std::cout<<(boost::format("new %s structure\n") % str->getName()).str();
#endif
		if (boost::iequals(str->getName(), "category_type")) {
			mode = CATEGORYMODE;
			Item::Categories.push_back(ItemCat());
			++Game::ItemCatCount;
			Item::Categories.back().name = name;
			std::string upperName = name;
			boost::to_upper(upperName);
			Item::itemCategoryNames.insert(std::pair<std::string, ItemCategory>(upperName, Game::ItemCatCount-1));
			presetCategoryParent.push_back("");
		} else if (boost::iequals(str->getName(), "item_type")) {
			mode = ITEMMODE;
			Item::Presets.push_back(ItemPreset());
			presetGrowth.push_back("");
			presetFruits.push_back(std::vector<std::string>());
			presetDecay.push_back(std::vector<std::string>());
			++Game::ItemTypeCount;
			Item::Presets.back().name = name;
			std::string upperName = name;
			boost::to_upper(upperName);
			Item::itemTypeNames.insert(std::pair<std::string, ItemType>(upperName, Game::ItemTypeCount-1));
			presetProjectile.push_back("");
		} else if (boost::iequals(str->getName(), "attack")) {
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
				ItemCategory cat = Item::StringToItemCategory((char*)TCOD_list_get(value.list,i));
				Item::Presets.back().categories.insert(cat);
				Item::Presets.back().specificCategories.insert(cat);
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
			Item::Presets.back().containInRaw = value.s;
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
			Item::Presets.back().organic = true;
		} else if (boost::iequals(name, "multiplier")) {
			Item::Presets.back().multiplier = value.i;
		} else if (boost::iequals(name, "containerSize")) {
			Item::Presets.back().container = value.i;
		} else if (boost::iequals(name, "fitsin")) {
			Item::Presets.back().fitsInRaw = value.s;
		} else if (boost::iequals(name, "constructedin")) {
			Item::Presets.back().constructedInRaw = value.s;
		} else if (boost::iequals(name, "decay")) {
			Item::Presets.back().decays = true;
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				presetDecay.back().push_back((char*)TCOD_list_get(value.list,i));
			}
		} else if (boost::iequals(name, "decaySpeed")) {
			Item::Presets.back().decaySpeed = value.i;
		} else if (boost::iequals(name,"type")) {
			Item::Presets.back().attack.Type(Attack::StringToDamageType(value.s));
		} else if (boost::iequals(name,"damage")) {
			Item::Presets.back().attack.Amount(value.dice);
		} else if (boost::iequals(name,"cooldown")) {
			Item::Presets.back().attack.CooldownMax(value.i);
		} else if (boost::iequals(name,"statusEffects")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				Item::Presets.back().attack.StatusEffects()->push_back(std::pair<StatusEffectType, int>(StatusEffect::StringToStatusEffectType((char*)TCOD_list_get(value.list,i)), 100));
			}
		} else if (boost::iequals(name,"effectChances")) {
			for (int i = 0; i < TCOD_list_size(value.list); ++i) {
				Item::Presets.back().attack.StatusEffects()->at(i).second = (intptr_t)TCOD_list_get(value.list,i);
			}
		} else if (boost::iequals(name,"ammo")) {
			presetProjectile.back() = value.s;
		} else if (boost::iequals(name,"parent")) {
			presetCategoryParent.back() = value.s;
		} else if (boost::iequals(name,"physical")) {
			Item::Presets.back().resistances[PHYSICAL_RES] = value.i;
		} else if (boost::iequals(name,"magic")) {
			Item::Presets.back().resistances[MAGIC_RES] = value.i;
		} else if (boost::iequals(name,"cold")) {
			Item::Presets.back().resistances[COLD_RES] = value.i;
		} else if (boost::iequals(name,"fire")) {
			Item::Presets.back().resistances[FIRE_RES] = value.i;
		} else if (boost::iequals(name,"poison")) {
			Item::Presets.back().resistances[POISON_RES] = value.i;
		}
		return true;
	}
	
	bool parserEndStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
#ifdef DEBUG
		std::cout<<(boost::format("end of %s structure\n") % str->getName()).str();
#endif
		return true;
	}
	void error(const char *msg) {
		LOG("ItemListener: " << msg);
		Game::Inst()->ErrorScreen();
	}
};

void Item::LoadPresets(std::string filename) {
	TCODParser parser = TCODParser();
	TCODParserStruct* categoryTypeStruct = parser.newStructure("category_type");
	categoryTypeStruct->addProperty("parent", TCOD_TYPE_STRING, false);
	
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
	itemTypeStruct->addProperty("constructedin", TCOD_TYPE_STRING, false);
	
	TCODParserStruct *attackTypeStruct = parser.newStructure("attack");
	const char* damageTypes[] = { "slashing", "piercing", "blunt", "magic", "fire", "cold", "poison", "wielded", "ranged", NULL };
	attackTypeStruct->addValueList("type", damageTypes, true);
	attackTypeStruct->addProperty("damage", TCOD_TYPE_DICE, false);
	attackTypeStruct->addProperty("cooldown", TCOD_TYPE_INT, false);
	attackTypeStruct->addListProperty("statusEffects", TCOD_TYPE_STRING, false);
	attackTypeStruct->addListProperty("effectChances", TCOD_TYPE_INT, false);
	attackTypeStruct->addProperty("ammo", TCOD_TYPE_STRING, false);
	
	itemTypeStruct->addStructure(attackTypeStruct);
	
	TCODParserStruct *resistancesStruct = parser.newStructure("resistances");
	resistancesStruct->addProperty("physical", TCOD_TYPE_INT, false);
	resistancesStruct->addProperty("magic", TCOD_TYPE_INT, false);
	resistancesStruct->addProperty("cold", TCOD_TYPE_INT, false);
	resistancesStruct->addProperty("fire", TCOD_TYPE_INT, false);
	resistancesStruct->addProperty("poison", TCOD_TYPE_INT, false);
	itemTypeStruct->addStructure(resistancesStruct);
	
	ItemListener* itemListener = new ItemListener();
	parser.run(filename.c_str(), itemListener);
	itemListener->translateNames();
}

void Item::ResolveContainers() {
	for (std::vector<ItemPreset>::iterator it = Item::Presets.begin(); it != Item::Presets.end(); ++it) {
		ItemPreset& preset = *it;
		
		if (!preset.fitsInRaw.empty()) {
			preset.fitsin = Item::StringToItemCategory(preset.fitsInRaw);
		}
		
		if (!preset.containInRaw.empty()) {
			preset.containIn = Item::StringToItemCategory(preset.containInRaw);
			preset.components.push_back(preset.containIn);
		}
		
		preset.fitsInRaw.clear();
		preset.containInRaw.clear();
	}
}
