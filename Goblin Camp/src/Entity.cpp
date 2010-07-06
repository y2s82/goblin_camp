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

#include "Entity.hpp"
#include "Logger.hpp"

Entity::Entity() : zone(0), reserved(0), name("NONAME"), faction(0) {
	uid = uids++;
}

Entity::~Entity() {
}

int Entity::uids = 0;

int Entity::x() {return _x;}
int Entity::y() {return _y;}
Coordinate Entity::Position() {return Coordinate(_x,_y);}
void Entity::Position(Coordinate pos) {_x = pos.x(); _y = pos.y();}
int Entity::Uid() {return uid;}

void Entity::Zone(int value) {zone = value;}
int Entity::Zone() {return zone;}

void Entity::Reserve(bool value) {reserved = value;}
bool Entity::Reserved() {return reserved;}

std::string Entity::Name() { return name; }
void Entity::Name(std::string newName) { name = newName; }

void Entity::CancelJob(int) {}

int Entity::Faction() const { return faction; }
void Entity::Faction(int val) { faction = val; }
