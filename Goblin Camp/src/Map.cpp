#include "Map.hpp"
#include "Game.hpp"

GameMap::GameMap() {
	tileMap.resize(boost::extents[500][500]);
	for (int i = 0; i < (signed int)tileMap.size(); ++i) {
	    for (int e = 0; e < (signed int)tileMap[0].size(); ++e) {
	        tileMap[i][e].type(tileMap[i][e].type());
	    }
	}
	width = tileMap.size();
	height = tileMap[0].size();
};

GameMap* GameMap::instance = 0;

GameMap* GameMap::Inst() {
	if (!instance) instance = new GameMap();
	return instance;
}

float GameMap::getWalkCost(int x0, int y0, int x1, int y1, void *data) const {
	if (Walkable(x1,y1)) return tileMap[x0][y0].moveCost();
	return 0.0f;
}

bool GameMap::Walkable(int x, int y) const {
	return tileMap[x][y].Walkable();
}
void GameMap::Walkable(int x,int y, bool value) { tileMap[x][y].Walkable(value); }

int GameMap::Width() { return width; }
int GameMap::Height() { return height; }
bool GameMap::Buildable(int x, int y) const { return tileMap[x][y].Buildable(); }
void GameMap::Buildable(int x, int y, bool value) { tileMap[x][y].Buildable(value); }
TileType GameMap::Type(int x, int y) { return tileMap[x][y].type(); }
void GameMap::Type(int x, int y, TileType ntype) { tileMap[x][y].type(ntype); }
bool GameMap::MoveTo(int x, int y, int uid) {
    if (x >= 0 && x < Width() && y >= 0 && y < Height()) {
        return tileMap[x][y].MoveTo(uid);
    } else return false;
}
void GameMap::MoveFrom(int x, int y, int uid) { tileMap[x][y].MoveFrom(uid); }

void GameMap::Construction(int x, int y, int uid) { tileMap[x][y].Construction(uid); }
int GameMap::Construction(int x, int y) { return tileMap[x][y].Construction(); }

void GameMap::Draw(Coordinate center) {
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

boost::weak_ptr<WaterNode> GameMap::GetWater(int x, int y) { return tileMap[x][y].GetWater(); }
void GameMap::SetWater(int x, int y, boost::shared_ptr<WaterNode> value) { tileMap[x][y].SetWater(value); }

bool GameMap::Low(int x, int y) const { return tileMap[x][y].Low(); }
void GameMap::Low(int x, int y, bool value) { tileMap[x][y].Low(value); }

bool GameMap::BlocksWater(int x, int y) const { return tileMap[x][y].BlocksWater(); }
void GameMap::BlocksWater(int x, int y, bool value) { tileMap[x][y].BlocksWater(value); }

std::set<int>* GameMap::NPCList(int x, int y) { return &tileMap[x][y].npcList; }
std::set<int>* GameMap::ItemList(int x, int y) { return &tileMap[x][y].itemList; }

int GameMap::Graphic(int x, int y) const { return tileMap[x][y].Graphic(); }
TCODColor GameMap::Color(int x, int y) const { return tileMap[x][y].Color(); }

void GameMap::NatureObject(int x, int y, int val) { tileMap[x][y].NatureObject(val); }
int GameMap::NatureObject(int x, int y) const { return tileMap[x][y].NatureObject(); }

boost::weak_ptr<FilthNode> GameMap::GetFilth(int x, int y) { return tileMap[x][y].GetFilth(); }
void GameMap::SetFilth(int x, int y, boost::shared_ptr<FilthNode> value) { tileMap[x][y].SetFilth(value); }

bool GameMap::BlocksLight(int x, int y) const { return tileMap[x][y].BlocksLight(); }
void GameMap::BlocksLight(int x, int y, bool val) { tileMap[x][y].BlocksLight(val); }
