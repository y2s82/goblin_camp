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

#include <boost/serialization/split_member.hpp>

#include "Stockpile.hpp"

class FarmPlot : public Stockpile {
	friend class boost::serialization::access;
	friend class Game;

private:
	FarmPlot(ConstructionType=0, int symbol='?', Coordinate=Coordinate(0,0));
	bool tilled;
	std::map<ItemType, bool> allowedSeeds;
	std::map<Coordinate, int> growth;

public:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

	void Update();
	virtual void Draw(Coordinate, TCODConsole*);
	virtual int Use();
	void AllowSeed(ItemType, bool);
	void SwitchAllowed(int);
	bool SeedAllowed(ItemType);
	std::map<ItemType, bool>* AllowedSeeds();
	virtual void AcceptVisitor(ConstructionVisitor& visitor);
};

BOOST_CLASS_VERSION(FarmPlot, 0)

template<class Archive>
void FarmPlot::save(Archive & ar, const unsigned int version) const {
	ar & boost::serialization::base_object<Stockpile>(*this);
	ar & tilled;
	ar & allowedSeeds;
	ar & growth;
}

template<class Archive>
void FarmPlot::load(Archive & ar, const unsigned int version) {
	ar & boost::serialization::base_object<Stockpile>(*this);
	ar & tilled;
	ar & allowedSeeds;
	ar & growth;
}
