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

#include <boost/unordered_map.hpp>

#include "Entity.hpp"
#include "Attack.hpp"

typedef int SpellType;

class SpellListener;

class SpellPreset {
public:
	SpellPreset(std::string);
	std::string name;
	std::list<Attack> attacks;
	bool immaterial;
	int graphic;
	int speed;
	TCODColor color;
};

class Spell : public Entity {
	friend class boost::serialization::access;
	friend class SpellListener;

private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

	TCODColor color;
	int graphic;
	int type;
	bool dead;
	std::list<Attack> attacks;
	bool immaterial;
	
	static boost::unordered_map<std::string, SpellType> spellTypeNames;
public:
	Spell(Coordinate=Coordinate(0,0),int=0);
	~Spell();

	static std::vector<SpellPreset> Presets;

	void Draw(Coordinate, TCODConsole*);
	void UpdateVelocity();
	void Impact(int speedChange);
	bool IsDead();

	static int StringToSpellType(std::string);

	static void LoadPresets(std::string);
};
