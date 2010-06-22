#include "Map.hpp"
#include "Game.hpp"

Map::Map() {
	tileMap.resize(boost::extents[500][500]);
	for (int i = 0; i < (signed int)tileMap.size(); ++i) {
	    for (int e = 0; e < (signed int)tileMap[0].size(); ++e) {
	        tileMap[i][e].type(tileMap[i][e].type());
	    }
	}
	width = tileMap.size();
	height = tileMap[0].size();
};

Map* Map::instance = 0;

Map* Map::Inst() {
	if (!instance) instance = new Map();
	return instance;
}

float Map::getWalkCost(int x0, int y0, int x1, int y1, void *data) const {
	if (Walkable(x1,y1)) return (float)tileMap[x0][y0].moveCost();
	return 0.0f;
}

bool Map::Walkable(int x, int y) const {
	return tileMap[x][y].Walkable();
}
void Map::Walkable(int x,int y, bool value) { tileMap[x][y].Walkable(value); }

int Map::Width() { return width; }
int Map::Height() { return height; }
bool Map::Buildable(int x, int y) const { return tileMap[x][y].Buildable(); }
void Map::Buildable(int x, int y, bool value) { tileMap[x][y].Buildable(value); }
TileType Map::Type(int x, int y) { return tileMap[x][y].type(); }
void Map::Type(int x, int y, TileType ntype) { tileMap[x][y].type(ntype); }
bool Map::MoveTo(int x, int y, int uid) {
    if (x >= 0 && x < Width() && y >= 0 && y < Height()) {
        return tileMap[x][y].MoveTo(uid);
    } else return false;
}
void Map::MoveFrom(int x, int y, int uid) { tileMap[x][y].MoveFrom(uid); }

void Map::Construction(int x, int y, int uid) { tileMap[x][y].Construction(uid); }
int Map::Construction(int x, int y) { return tileMap[x][y].Construction(); }

void Map::Draw(Coordinate center) {
	int screenDeltaX = center.x() - Game::Inst()->ScreenWidth() / 2;
	int screenDeltaY = center.y() - Game::Inst()->ScreenHeight() / 2;
	for (int y = center.y() - Game::Inst()->ScreenHeight() / 2; y < Game::Inst()->ScreenHeight() / 2 + center.y(); ++y) {
		for (int x = center.x() - Game::Inst()->ScreenWidth() / 2; x < Game::Inst()->ScreenWidth() / 2 + center.x(); ++x) {
			if (x >= 0 && x < width && y >= 0 && y < height) {

			    TCODConsole::root->putCharEx(x-screenDeltaX,y-(screenDeltaY), Graphic(x,y), Color(x,y), TCODColor::black);

				boost::weak_ptr<WaterNode> water = GetWater(x,y);
				if (water.lock()) {
					water.lock()->Draw(center);
				}
				boost::weak_ptr<FilthNode> filth = GetFilth(x,y);
				if (filth.lock()) {
					filth.lock()->Draw(center);
				}

			}
			else {
				TCODConsole::root->putCharEx(x-screenDeltaX,y-screenDeltaY, TCOD_CHAR_BLOCK3, TCODColor::black, TCODColor::white);
			}
		}
	}
}

boost::weak_ptr<WaterNode> Map::GetWater(int x, int y) { return tileMap[x][y].GetWater(); }
void Map::SetWater(int x, int y, boost::shared_ptr<WaterNode> value) { tileMap[x][y].SetWater(value); }

bool Map::Low(int x, int y) const { return tileMap[x][y].Low(); }
void Map::Low(int x, int y, bool value) { tileMap[x][y].Low(value); }

bool Map::BlocksWater(int x, int y) const { return tileMap[x][y].BlocksWater(); }
void Map::BlocksWater(int x, int y, bool value) { tileMap[x][y].BlocksWater(value); }

std::set<int>* Map::NPCList(int x, int y) { return &tileMap[x][y].npcList; }
std::set<int>* Map::ItemList(int x, int y) { return &tileMap[x][y].itemList; }

int Map::Graphic(int x, int y) const { return tileMap[x][y].Graphic(); }
TCODColor Map::Color(int x, int y) const { return tileMap[x][y].Color(); }

void Map::NatureObject(int x, int y, int val) { tileMap[x][y].NatureObject(val); }
int Map::NatureObject(int x, int y) const { return tileMap[x][y].NatureObject(); }

boost::weak_ptr<FilthNode> Map::GetFilth(int x, int y) { return tileMap[x][y].GetFilth(); }
void Map::SetFilth(int x, int y, boost::shared_ptr<FilthNode> value) { tileMap[x][y].SetFilth(value); }

bool Map::BlocksLight(int x, int y) const { return tileMap[x][y].BlocksLight(); }
void Map::BlocksLight(int x, int y, bool val) { tileMap[x][y].BlocksLight(val); }
