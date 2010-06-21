#include "GameEntity.hpp"
#include "Logger.hpp"

GameEntity::GameEntity() : zone(0), reserved(0), name("NONAME"), faction(0) {
	uid = uids++;
}

GameEntity::~GameEntity() {
}

int GameEntity::uids = 0;

int GameEntity::x() {return _x;}
int GameEntity::y() {return _y;}
Coordinate GameEntity::Position() {return Coordinate(_x,_y);}
void GameEntity::Position(Coordinate pos) {_x = pos.x(); _y = pos.y();}
int GameEntity::Uid() {return uid;}

void GameEntity::Zone(int value) {zone = value;}
int GameEntity::Zone() {return zone;}

void GameEntity::Reserve(bool value) {reserved = value;}
bool GameEntity::Reserved() {return reserved;}

std::string GameEntity::Name() { return name; }

void GameEntity::CancelJob(int) {}

int GameEntity::Faction() const { return faction; }
void GameEntity::Faction(int val) { faction = val; }
