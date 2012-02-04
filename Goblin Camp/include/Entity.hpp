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

#include <set>
#include <string>
#include <list>
#include <boost/enable_shared_from_this.hpp>

#include "UI/UIComponents.hpp"
#include "UI/Tooltip.hpp"

#include "Coordinate.hpp"
#include "data/Serialization.hpp"

#define ENTITYHEIGHT 5

class Map;

struct FlightPath {
	FlightPath(Coordinate);
	Coordinate coord;
	int height;
};

class Entity: public boost::enable_shared_from_this<Entity> {
	GC_SERIALIZABLE_CLASS
	
protected:
	Coordinate pos;
	int uid;
	int zone;
	bool reserved;
	std::string name;
	int faction;
	int velocity, nextVelocityMove;
	Coordinate velocityTarget;
	std::list<FlightPath> flightPath;
	int bulk;
	float strobe;
	Map* map;
public:
	Entity();
	virtual ~Entity();
	virtual int X();
	virtual int Y();
	virtual Coordinate Position() const;
	virtual Coordinate Center() const;
	virtual void Position(const Coordinate& );
	int Uid();
	static int uids;
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
	virtual int GetHeight() const;
	void SetVelocityTarget(Coordinate);
	void CalculateFlightPath(Coordinate, int speed, int initialHeight=0);

	void SetBulk(int);
	int GetBulk();
	void AddBulk(int);
	void RemoveBulk(int);

	void Strobe();
	void ResetStrobe();
	virtual bool CanStrobe();

	virtual void SetMap(Map* map);
};

BOOST_CLASS_VERSION(Entity, 0)
