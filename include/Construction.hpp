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

#include <vector>
#include <deque>
#include <map>
#include <list>
#include <set>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/unordered_map.hpp>

#include "Entity.hpp"
#include "UI/UIComponents.hpp"
#include "Tile.hpp"
#include "StatusEffect.hpp"
#include "Attack.hpp"

#include "ConstructionVisitor.hpp"

#include "data/Serialization.hpp"

class Job;
typedef int ItemType;
typedef int ItemCategory;
class Container;

enum BuildResult {
	BUILD_NOMATERIAL = -99999
};

typedef int ConstructionType;

enum ConstructionTag {
	STOCKPILE,
	FARMPLOT,
	DOOR,
	WALL,
	BED,
	WORKSHOP,
	FURNITURE,
	CENTERSCAMP,
	SPAWNINGPOOL,
	BRIDGE,
	TRAP,
	RANGEDADVANTAGE,
	PERMANENT,
	TAGCOUNT
};

struct ConstructionPreset {
	ConstructionPreset();
	int maxCondition;
	std::vector<int> graphic;
	bool walkable;
	std::list<ItemCategory> materials;
	bool producer;
	std::vector<ItemType> products;
	std::string name;
	Coordinate blueprint;
	bool tags[TAGCOUNT];
	Coordinate productionSpot;
	bool dynamic;
	std::string spawnCreaturesTag;
	int spawnFrequency;
	std::string category;
	int placementType;
	bool blocksLight;
	bool permanent;
	TCODColor color;
	std::set<TileType> tileReqs;
	int tier;
	std::string description;
	std::string fallbackGraphicsSet;
	int graphicsHint;
	Coordinate chimney;
	Attack trapAttack;
	ItemCategory trapReloadItem;
	int moveSpeedModifier;
	std::vector<StatusEffectType> passiveStatusEffects;
};

class Construction : public Entity {
	GC_SERIALIZABLE_CLASS
	
	friend class Game;
	friend class ConstructionListener;
protected:
	//note: optional constructors do not make sense, but they are
	//required by the Boost::serialization library. More precisely, it
	//would be possible to define {save,load}_construct_data method,
	//but I'm unsure how to do it without incurring a version change.
	//TODO investigate
	Construction(ConstructionType = 0, const Coordinate& = zero);

	int condition, maxCondition;
	std::vector<int> graphic;
	TCODColor color;
	ConstructionType type;
	bool walkable;
	std::list<ItemCategory> materials;
	bool producer;
	std::vector<ItemType> products;
	std::deque<ItemType> jobList;
	int progress;
	bool SpawnProductionJob();
	boost::shared_ptr<Container> container;
	boost::shared_ptr<Container> materialsUsed;
	bool stockpile, farmplot;
	bool dismantle;
	int time;
	bool built;
	void UpdateWallGraphic(bool recurse = true, bool self = true);
	bool flammable;
	int smoke;
	boost::weak_ptr<Job> repairJob;
	Map* map;
public:
	virtual ~Construction();

	static Coordinate Blueprint(ConstructionType);
	static Coordinate ProductionSpot(ConstructionType);
	static std::vector<int> AllowedAmount;
	void Condition(int);
	int Condition() const;
	int GetMaxCondition() const;
	virtual void Draw(Coordinate, TCODConsole*);
	int GetGraphicsHint() const;
	virtual int Build();
	ConstructionType Type() const;
	std::list<ItemCategory>* MaterialList();
	bool Producer() const;
	std::vector<ItemType>* Products();
	ItemType Products(int) const;
	void AddJob(ItemType);
	virtual void CancelJob(int=0);
	std::deque<ItemType>* JobList();
	ItemType JobList(int) const;
	virtual int Use();
	static std::vector<ConstructionPreset> Presets;
	static std::set<std::string> Categories;
	static void LoadPresets(std::string);
	static void ResolveProducts();
	virtual boost::weak_ptr<Container> Storage() const;
	bool HasTag(ConstructionTag) const;
	virtual void Update();
	virtual void Dismantle(const Coordinate& p);
	bool DismantlingOrdered();
	bool CheckMaterialsPresent();
	void ReserveComponents(bool);
	virtual Panel *GetContextMenu();
	Coordinate Center() const;
	void Damage(Attack*);
	void Explode();
	void BurnToTheGround();
	bool Built();
	bool IsFlammable() const;
	int Repair();
	virtual void SpawnRepairJob();
	int GetMoveSpeedModifier();
	virtual bool CanStrobe();

	virtual void AcceptVisitor(ConstructionVisitor& visitor);

	virtual void SetMap(Map* map);
	
	static boost::unordered_map<std::string, ConstructionType> constructionNames;
	static ConstructionType StringToConstructionType(std::string);
	static std::string ConstructionTypeToString(ConstructionType);
};

BOOST_CLASS_VERSION(Construction, 0)
