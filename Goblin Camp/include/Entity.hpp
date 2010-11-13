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

#include <set>
#include <string>
#include <list>
#include <boost/enable_shared_from_this.hpp>
#include <boost/serialization/set.hpp>

#include "Coordinate.hpp"
#include "UI/UIComponents.hpp"
#include "UI/Tooltip.hpp"

#define ENTITYHEIGHT 5

struct FlightPath {
	FlightPath(Coordinate);
	Coordinate coord;
	int height;
};

class Entity: public boost::enable_shared_from_this<Entity> {
	friend class boost::serialization::access;
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()
protected:
	unsigned int x, y;
	int uid;
	int zone;
	bool reserved;
	std::string name;
	int faction;
	int velocity, nextVelocityMove;
	Coordinate velocityTarget;
	std::list<FlightPath> flightPath;
	int bulk;
public:
	Entity();
	virtual ~Entity();
	int X();
	int Y();
	int Uid();
	static int uids;
	virtual Coordinate Position();
	virtual Coordinate Center();
	virtual void Position(Coordinate);
	void Zone(int);
	int Zone();
	virtual void Reserve(bool);
	bool Reserved();
	std::string Name();
	void Name(std::string);
	virtual void CancelJob(int=0);
	virtual void SetFaction(int);
	virtual int GetFaction() const;
    
    virtual Panel* GetContextMenu() {return 0;}
	virtual void GetTooltip(int x, int y, Tooltip *tooltip);

	int GetVelocity();
	virtual void SetVelocity(int);
	Coordinate GetVelocityTarget();
	int GetHeight();
	void SetVelocityTarget(Coordinate);
	void CalculateFlightPath(Coordinate, int speed, int initialHeight=0);

	void SetBulk(int);
	int GetBulk();
	void AddBulk(int);
	void RemoveBulk(int);
};

