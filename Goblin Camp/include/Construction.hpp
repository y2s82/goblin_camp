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
#include <boost/serialization/split_member.hpp>

#include "Entity.hpp"
#include "Item.hpp"
#include "Container.hpp"
#include "UI/UIComponents.hpp"
#include "Tile.hpp"

#include "ConstructionVisitor.hpp"

class Job;

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
};

class Construction : public Entity {
	friend class boost::serialization::access;
	friend class Game;
	friend class ConstructionListener;
private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

protected:
	Construction(ConstructionType = 0, Coordinate = Coordinate(0,0));

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
public:
	virtual ~Construction();

	static Coordinate Blueprint(ConstructionType);
	static Coordinate ProductionSpot(ConstructionType);
	static std::vector<int> AllowedAmount;
	void Condition(int);
	int Condition();
	int GetMaxCondition() const;
	virtual void Draw(Coordinate, TCODConsole*);
	int GetGraphicsHint() const;
	int Build();
	ConstructionType Type();
	std::list<ItemCategory>* MaterialList();
	bool Producer();
	std::vector<ItemType>* Products();
	ItemType Products(int);
	void AddJob(ItemType);
	virtual void CancelJob(int=0);
	std::deque<ItemType>* JobList();
	ItemType JobList(int);
	virtual int Use();
	static std::vector<ConstructionPreset> Presets;
	static std::set<std::string> Categories;
	static void LoadPresets(std::string);
	static void ResolveProducts();
	virtual boost::weak_ptr<Container> Storage();
	bool HasTag(ConstructionTag);
	virtual void Update();
	virtual void Dismantle(Coordinate location=Coordinate(-1,-1));
	bool DismantlingOrdered();
	bool CheckMaterialsPresent();
	void ReserveComponents(bool);
	virtual Panel *GetContextMenu();
	Coordinate Center();
	void Damage(Attack*);
	void Explode();
	void BurnToTheGround();
	bool Built();
	bool IsFlammable();
	int Repair();
	void SpawnRepairJob();

	virtual void AcceptVisitor(ConstructionVisitor& visitor);
	
	static boost::unordered_map<std::string, ConstructionType> constructionNames;
	static ConstructionType StringToConstructionType(std::string);
	static std::string ConstructionTypeToString(ConstructionType);
};
