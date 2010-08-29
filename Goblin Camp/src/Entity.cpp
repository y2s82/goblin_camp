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
#include <iostream>
#endif
#include <boost/utility.hpp>

#include "Entity.hpp"
#include "Logger.hpp"

FlightPath::FlightPath(Coordinate c) : coord(c), height(-1) {}

Entity::Entity() :
	zone(0), reserved(0), name("NONAME"), faction(-1),
	velocity(0), nextVelocityMove(0), velocityTarget(Coordinate(0,0))
{
	uid = uids++; //FIXME: Entity should keep track of freed uids
}

Entity::~Entity() {
}

int Entity::uids = 0;

int Entity::X() {return x;}
int Entity::Y() {return y;}
Coordinate Entity::Position() {return Coordinate(x,y);}
void Entity::Position(Coordinate pos) {x = pos.X(); y = pos.Y();}
int Entity::Uid() {return uid;}

void Entity::Zone(int value) {zone = value;}
int Entity::Zone() {return zone;}

void Entity::Reserve(bool value) {reserved = value;}
bool Entity::Reserved() {return reserved;}

std::string Entity::Name() { return name; }
void Entity::Name(std::string newName) { name = newName; }

void Entity::CancelJob(int) {}

int Entity::GetFaction() const { return faction; }
void Entity::SetFaction(int val) { faction = val; }

void Entity::GetTooltip(int x, int y, Tooltip *tooltip) {
	tooltip->AddEntry(TooltipEntry(name, TCODColor::white));
}

int Entity::GetVelocity() { return velocity; }
void Entity::SetVelocity(int value) { velocity = value; }

Coordinate Entity::GetVelocityTarget() { return velocityTarget; }
void Entity::SetVelocityTarget(Coordinate value) { velocityTarget = value; }

int Entity::GetHeight() { return flightPath.size() ? flightPath.back().height : 0; }

void Entity::CalculateFlightPath(Coordinate target, int speed, int initialHeight) {
#ifdef DEBUG
	std::cout<<"Calculating flightpath for "<<name<<" from "<<x<<","<<y<<" to "<<target.X()<<","<<target.Y()<<" at v:"<<speed<<"\n";
#endif
	velocityTarget = target;
	flightPath.clear();
	TCODLine::init(target.X(), target.Y(), x, y);
	int px = target.X();
	int py = target.Y();
	do {
		flightPath.push_back(FlightPath(Coordinate(px,py)));
	} while (!TCODLine::step(&px, &py));

	if (flightPath.size() > 0) {
		int h = 0;
		int hAdd = std::max(1, 50 / speed); /* The lower the speed, the higher the entity has to arch in order
							   for it to fly the distance */
		
		std::list<FlightPath>::iterator begIt = flightPath.begin();
		std::list<FlightPath>::iterator endIt = flightPath.end();
		--endIt;

		while (begIt->height == -1 && endIt->height == -1) {
			begIt->height = h;
			endIt->height = std::max(initialHeight, h);
			h += hAdd;
			++begIt;
			--endIt;
			if (begIt == flightPath.end() || endIt == flightPath.end()) break;
		}
		flightPath.pop_back(); //Last coordinate is the entity's coordinate
	}
	SetVelocity(speed);
}