#include <libtcod.hpp>
#include <string>
#include <vector>
#include <boost/function.hpp>


#include "UI.hpp"
#include "Announce.hpp"
#include "Game.hpp"
#include "Gcamp.hpp"
#include "Logger.hpp"
#include "Map.hpp"
#include "Job.hpp"

UI* UI::instance = 0;

UI::UI() :
	menuOpen(false),
	_state(UINORMAL),
	_blueprint(1,1),
	placeable(false),
	underCursor(std::list<boost::weak_ptr<GameEntity> >())
{
	currentMenu = Menu::MainMenu();
	menuHistory.reserve(10);
	placementCallback = boost::bind(Game::CheckPlacement, _1, _2);
	callback = boost::bind(Game::PlaceConstruction, _1, 0);
	rectCallback = boost::bind(Game::PlaceStockpile, _1, _2, 0, 0);
}

UI* UI::Inst() {
	if (!instance) instance = new UI();
	return instance;
}

void UI::Update() {
	HandleKeyboard();
	HandleMouse();
}

void UI::HandleKeyboard() {
	TCOD_key_t key = TCODConsole::checkForKeypress(TCOD_KEY_PRESSED);
	if (key.c == 'w') { if (Game::Inst()->center.y(Game::Inst()->center.y()-1) < (Game::Inst()->ScreenHeight() / 2)-1) Game::Inst()->center.y((Game::Inst()->ScreenHeight() / 2)-1); }
	if (key.c == 's') { if (Game::Inst()->center.y(Game::Inst()->center.y()+1) > 1+ GameMap::Inst()->Height() - (Game::Inst()->ScreenHeight() / 2)) Game::Inst()->center.y(1+ GameMap::Inst()->Height() - (Game::Inst()->ScreenHeight() / 2)); }
	if (key.c == 'a') { if (Game::Inst()->center.x(Game::Inst()->center.x()-1) < (Game::Inst()->ScreenWidth() / 2)-1) Game::Inst()->center.x((Game::Inst()->ScreenWidth() / 2)-1); }
	if (key.c == 'd') { if (Game::Inst()->center.x(Game::Inst()->center.x()+1) > 1+ GameMap::Inst()->Width() - (Game::Inst()->ScreenWidth() / 2)) Game::Inst()->center.x(1+ GameMap::Inst()->Width() - (Game::Inst()->ScreenWidth() / 2)); }
	if (key.c == 'q') Game::Exit();
	if (key.vk == TCODK_PRINTSCREEN) TCODSystem::saveScreenshot(0);
	if (key.c == 'v') {
		for (int i = 0; i < 3; ++i) {
			for (int e = 0; e < 3; ++e) {
				Game::Inst()->CreateWater(Coordinate(200+i,200+e), 10000);
			}
		}
	}
	if (key.c == 'g') Game::Inst()->CreateItem(Coordinate(200,200), BERRYSEED, true);
	if (key.c == 'k') Game::Inst()->CreateItem(Coordinate(200,200), 16, true);
	if (key.vk == TCODK_SPACE) Game::Inst()->Pause();
}

void UI::HandleMouse() {
	int tmp;
	MenuResult menuResult;
	bool xswap = false, yswap = false;
	mouseInput = TCODMouse::getStatus();
	Coordinate center = Game::Inst()->center;
	
    if (_state == UINORMAL) {
        //underCursor = GetEntity(Coordinate(mouseInput.cx + center.x() - Game::Inst()->ScreenWidth() / 2, mouseInput.cy + center.y() - Game::Inst()->ScreenHeight() / 2));
        HandleUnderCursor(Coordinate(mouseInput.cx + center.x() - Game::Inst()->ScreenWidth() / 2, mouseInput.cy + center.y() - Game::Inst()->ScreenHeight() / 2));
    }

	if (_state == UIPLACEMENT || _state == UIABPLACEMENT || _state == UIRECTPLACEMENT) {
		placeable = placementCallback(Coordinate(mouseInput.cx + center.x() - Game::Inst()->ScreenWidth() / 2, mouseInput.cy + center.y() - Game::Inst()->ScreenHeight() / 2), _blueprint);
	}

	if (_state == UIABPLACEMENT || _state == UIRECTPLACEMENT) {
		b.x(mouseInput.cx + center.x() - Game::Inst()->ScreenWidth() / 2);
		b.y(mouseInput.cy + center.y() - Game::Inst()->ScreenHeight() / 2);
	}

	if (mouseInput.lbutton_pressed) {
	    if (menuOpen) menuResult = currentMenu->Update(mouseInput.cx, mouseInput.cy);
		if (!menuOpen || menuResult == NOMENUHIT) {
			if (_state == UIPLACEMENT && placeable) {
				callback(Coordinate(mouseInput.cx + center.x() - Game::Inst()->ScreenWidth() / 2, mouseInput.cy + center.y() - Game::Inst()->ScreenHeight() / 2));
			} else if (_state == UIABPLACEMENT && placeable) {
				if (a.x() == 0) {
					a.x(mouseInput.cx + center.x() - Game::Inst()->ScreenWidth() / 2);
					a.y(mouseInput.cy + center.y() - Game::Inst()->ScreenHeight() / 2);
				}
				else {
					//Place construction from a->b
					if (a.x() > b.x()) {
						tmp = a.x();
						a.x(b.x());
						b.x(tmp);
						xswap = true;
					}
					if (a.y() > b.y()) {
						tmp = a.y();
						a.y(b.y());
						b.y(tmp);
						yswap = true;
					}
					for (int ix = a.x(); ix <= b.x(); ++ix) {
						if (!yswap) {
							if (placementCallback(Coordinate(ix, a.y()), _blueprint))
								callback(Coordinate(ix, a.y()));
						} else {
							if (placementCallback(Coordinate(ix, b.y()), _blueprint))
								callback(Coordinate(ix, b.y()));
						}
					}
					for (int iy = a.y(); iy <= b.y(); ++iy) {
						if (!xswap) {
							if (placementCallback(Coordinate(b.x(), iy), _blueprint))
								callback(Coordinate(b.x(), iy));
						} else {
							if (placementCallback(Coordinate(a.x(), iy), _blueprint))
								callback(Coordinate(a.x(), iy));
						}
					}
					a.x(0); a.y(0); b.x(0); b.y(0);
				}
			} else if (_state == UIRECTPLACEMENT && placeable) {
				if (a.x() == 0) {
					a.x(mouseInput.cx + center.x() - Game::Inst()->ScreenWidth() / 2);
					a.y(mouseInput.cy + center.y() - Game::Inst()->ScreenHeight() / 2);
				}
				else {
					//Place construction from a->b
					if (a.x() > b.x()) {
						tmp = a.x();
						a.x(b.x());
						b.x(tmp);
						xswap = true;
					}
					if (a.y() > b.y()) {
						tmp = a.y();
						a.y(b.y());
						b.y(tmp);
						yswap = true;
					}

                    if (placementCallback(Coordinate(a.x(), a.y()), _blueprint))
                        rectCallback(a,b);

					a.x(0); a.y(0); b.x(0); b.y(0);
				}
			} else { //Current state is not any kind of placement, so open construction/npc context menu if over one
			    if (!underCursor.empty() && underCursor.begin()->lock()) {
			        if (dynamic_cast<Construction*>(underCursor.begin()->lock().get())) {
                        UI::ChangeMenu(ConstructionMenu::ConstructionInfoMenu(static_cast<Construction*>(underCursor.begin()->lock().get())));
                        menuOpen = true;
			        }
			    }
			}
		}
	} else { currentMenu->Update(); }

	if (mouseInput.rbutton_pressed) {
		menuX = mouseInput.cx;
		menuY = mouseInput.cy;
		menuOpen = !menuOpen;
		currentMenu->selected(-1);
		currentMenu = Menu::MainMenu();
		menuHistory.clear();
		if (!menuOpen) { _state = UINORMAL; a.x(0); a.y(0); }
	}

	if (mouseInput.mbutton_pressed && menuOpen && !menuHistory.empty()) {
		currentMenu->selected(-1);
		_state = UINORMAL; a.x(0); a.y(0);
		currentMenu = menuHistory.back();
		menuHistory.pop_back();
	}

	if (mouseInput.lbutton && _state == UINORMAL) {
	    Game::Inst()->center.x(Game::Inst()->center.x() - (mouseInput.dx / 3));
	    Game::Inst()->center.y(Game::Inst()->center.y() - (mouseInput.dy / 3));
	    if (Game::Inst()->center.y(Game::Inst()->center.y()-1) < (Game::Inst()->ScreenHeight() / 2)-1) Game::Inst()->center.y((Game::Inst()->ScreenHeight() / 2)-1);
        if (Game::Inst()->center.y(Game::Inst()->center.y()+1) > 1+ GameMap::Inst()->Height() - (Game::Inst()->ScreenHeight() / 2)) Game::Inst()->center.y(1+ GameMap::Inst()->Height() - (Game::Inst()->ScreenHeight() / 2));
		if (Game::Inst()->center.x(Game::Inst()->center.x()-1) < (Game::Inst()->ScreenWidth() / 2)-1) Game::Inst()->center.x((Game::Inst()->ScreenWidth() / 2)-1);
		if (Game::Inst()->center.x(Game::Inst()->center.x()+1) > 1+ GameMap::Inst()->Width() - (Game::Inst()->ScreenWidth() / 2)) Game::Inst()->center.x(1+ GameMap::Inst()->Width() - (Game::Inst()->ScreenWidth() / 2));
    }
}
void UI::Draw(Coordinate center) {
	int tmp;
	bool xswap = false, yswap = false;

    DrawTopBar();

	if (menuOpen) {
		currentMenu->Draw(menuX, menuY);
	} else if (_state == UINORMAL && !underCursor.empty() && underCursor.begin()->lock()) {
	    int y = 0;
	    for (std::list<boost::weak_ptr<GameEntity> >::iterator ucit = underCursor.begin(); ucit != underCursor.end(); ++ucit) {
            TCODConsole::root->print(std::min(Game::Inst()->ScreenWidth()-1, mouseInput.cx+1), std::max(0, mouseInput.cy-1-y), ucit->lock()->Name().c_str());
            ++y;
	    }
	}

	if (_state == UIPLACEMENT || ((_state == UIABPLACEMENT || _state == UIRECTPLACEMENT) && a.x() == 0)) {
		for (int x = mouseInput.cx; x < mouseInput.cx + _blueprint.x() && x < Game::Inst()->ScreenWidth(); ++x) {
			for (int y = mouseInput.cy; y < mouseInput.cy + _blueprint.y() && y < Game::Inst()->ScreenHeight(); ++y) {
				if (!placeable) TCODConsole::root->putCharEx(x,y, 'C', TCODColor::red, TCODColor::black);
				else TCODConsole::root->putCharEx(x,y, 'C', TCODColor::green, TCODColor::black);
			}
		}
	} else if (_state == UIABPLACEMENT && a.x() > 0) {
		if (a.x() > b.x()) {
			tmp = a.x();
			a.x(b.x());
			b.x(tmp);
			xswap = true;
		}
		if (a.y() > b.y()) {
			tmp = a.y();
			a.y(b.y());
			b.y(tmp);
			yswap = true;
		}
		for (int ix = a.x()-center.x()+Game::Inst()->ScreenWidth()/2; ix <= b.x()-center.x()+Game::Inst()->ScreenWidth()/2 && ix < Game::Inst()->ScreenWidth(); ++ix) {
			if (!yswap) {
				if (!placeable) TCODConsole::root->putCharEx(ix,a.y()-center.y()+Game::Inst()->ScreenHeight()/2, 'C', TCODColor::red, TCODColor::black);
				else TCODConsole::root->putCharEx(ix,a.y()-center.y()+Game::Inst()->ScreenHeight()/2, 'C', TCODColor::green, TCODColor::black);
			}
			else {
				if (!placeable) TCODConsole::root->putCharEx(ix,b.y()-center.y()+Game::Inst()->ScreenHeight()/2, 'C', TCODColor::red, TCODColor::black);
				else TCODConsole::root->putCharEx(ix,b.y()-center.y()+Game::Inst()->ScreenHeight()/2, 'C', TCODColor::green, TCODColor::black);
			}
		}
		for (int iy = a.y()-center.y()+Game::Inst()->ScreenHeight()/2; iy <= b.y()-center.y()+Game::Inst()->ScreenHeight()/2 && iy < Game::Inst()->ScreenHeight(); ++iy) {
			if (!xswap) {
				if (!placeable) TCODConsole::root->putCharEx(b.x()-center.x()+Game::Inst()->ScreenWidth()/2,iy, 'C', TCODColor::red, TCODColor::black);
				else TCODConsole::root->putCharEx(b.x()-center.x()+Game::Inst()->ScreenWidth()/2,iy, 'C', TCODColor::green, TCODColor::black);
			}
			else {
				if (!placeable) TCODConsole::root->putCharEx(a.x()-center.x()+Game::Inst()->ScreenWidth()/2,iy, 'C', TCODColor::red, TCODColor::black);
				else TCODConsole::root->putCharEx(a.x()-center.x()+Game::Inst()->ScreenWidth()/2,iy, 'C', TCODColor::green, TCODColor::black);
			}
		}
		if (xswap) {
			tmp = a.x();
			a.x(b.x());
			b.x(tmp);
		}
		if (yswap) {
			tmp = a.y();
			a.y(b.y());
			b.y(tmp);
		}
	} else if (_state == UIRECTPLACEMENT && a.x() > 0) {
		if (a.x() > b.x()) {
			tmp = a.x();
			a.x(b.x());
			b.x(tmp);
			xswap = true;
		}
		if (a.y() > b.y()) {
			tmp = a.y();
			a.y(b.y());
			b.y(tmp);
			yswap = true;
		}
		for (int ix = a.x()-center.x()+Game::Inst()->ScreenWidth()/2; ix <= b.x()-center.x()+Game::Inst()->ScreenWidth()/2 && ix < Game::Inst()->ScreenWidth(); ++ix) {
			for (int iy = a.y()-center.y()+Game::Inst()->ScreenHeight()/2; iy <= b.y()-center.y()+Game::Inst()->ScreenHeight()/2 && iy < Game::Inst()->ScreenHeight(); ++iy) {
				if (!placeable) TCODConsole::root->putCharEx(ix,iy,'C', TCODColor::red, TCODColor::black);
				else TCODConsole::root->putCharEx(ix,iy,'C', TCODColor::green, TCODColor::black);
			}
		}
		if (xswap) {
			tmp = a.x();
			a.x(b.x());
			b.x(tmp);
		}
		if (yswap) {
			tmp = a.y();
			a.y(b.y());
			b.y(tmp);
		}
	}
}

void UI::DrawTopBar() {
    TCODConsole::root->setAlignment(TCOD_CENTER);
    TCODConsole::root->print(Game::Inst()->ScreenWidth() / 2, 0, "Orcs: %d   Goblins: %d  -  %s", Game::Inst()->OrcCount(),
                             Game::Inst()->GoblinCount(), Game::Inst()->SeasonToString(Game::Inst()->Season()).c_str());

	if (Game::Inst()->Paused()) {
		TCODConsole::root->setForegroundColor(TCODColor::red);
		TCODConsole::root->print(Game::Inst()->ScreenWidth() / 2, 1, "- - - - PAUSED - - - -");
		TCODConsole::root->setForegroundColor(TCODColor::white);
	}
    TCODConsole::root->setAlignment(TCOD_LEFT);
}

void UI::blueprint(Coordinate newBlue) { _blueprint = newBlue; }
void UI::state(UIState newState) { _state = newState; }

void UI::ChangeMenu(Menu* menu) {
	UI::Inst()->CurrentMenu()->selected(-1);
	UI::Inst()->AddToHistory(UI::Inst()->CurrentMenu());
	UI::Inst()->CurrentMenu(menu);
}

void UI::AddToHistory(Menu* menu) {menuHistory.push_back(menu);}
Menu* UI::CurrentMenu() {return currentMenu;}
void UI::CurrentMenu(Menu* menu) {currentMenu = menu;}

void UI::SetCallback(boost::function<void(Coordinate)> newCallback) {callback = newCallback;}
void UI::SetRectCallback(boost::function<void(Coordinate,Coordinate)> newCallback) {rectCallback = newCallback;}
void UI::SetPlacementCallback(boost::function<bool(Coordinate,Coordinate)> newCallback) {placementCallback = newCallback;}

void UI::ChooseConstruct(ConstructionType construct, UIState state) {
	UI::Inst()->SetCallback(boost::bind(Game::PlaceConstruction, _1, construct));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckPlacement, _1, _2));
	UI::Inst()->blueprint(Construction::Blueprint(construct));
	UI::Inst()->state(state);
}

void UI::ChooseStockpile(ConstructionType stockpile) {
	int stockpileSymbol = '%';
	UI::Inst()->SetRectCallback(boost::bind(Game::PlaceStockpile, _1, _2, stockpile, stockpileSymbol));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckPlacement, _1, _2));
	UI::Inst()->blueprint(Construction::Blueprint(stockpile));
	UI::Inst()->state(UIRECTPLACEMENT);
}

void UI::ChooseTreeFelling() {
    UI::Inst()->state(UIRECTPLACEMENT);
    UI::Inst()->SetRectCallback(boost::bind(Game::FellTree, _1, _2));
    UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, _2));
    UI::Inst()->blueprint(Coordinate(1,1));
}

void UI::ChoosePlantHarvest() {
    UI::Inst()->state(UIRECTPLACEMENT);
    UI::Inst()->SetRectCallback(boost::bind(Game::HarvestWildPlant, _1, _2));
    UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, _2));
    UI::Inst()->blueprint(Coordinate(1,1));
}

boost::weak_ptr<GameEntity> UI::GetEntity(Coordinate pos) {
    if (pos.x() >= 0 && pos.x() < GameMap::Inst()->Width() && pos.y() >= 0 && pos.y() < GameMap::Inst()->Height()) {
        std::set<int> *npcList = GameMap::Inst()->NPCList(pos.x(), pos.y());
        if (!npcList->empty()) return Game::Inst()->npcList[(*npcList->begin())];

        std::set<int> *itemList = GameMap::Inst()->ItemList(pos.x(), pos.y());
        if (!itemList->empty()) {
            std::set<boost::weak_ptr<Item> >::iterator itemi = Game::Inst()->freeItems.find(Game::Inst()->itemList[*itemList->begin()]);
            if (itemi != Game::Inst()->freeItems.end()) {
                return *itemi;
            }
        }

        int entity = GameMap::Inst()->NatureObject(pos.x(), pos.y());
        if (entity > -1) return (Game::Inst()->natureList[entity]);

        entity = GameMap::Inst()->Construction(pos.x(), pos.y());
        if (entity > -1) return (Game::Inst()->constructionList[entity]);
    }
    return boost::weak_ptr<GameEntity>();
}

void UI::HandleUnderCursor(Coordinate pos) {
    underCursor.clear();

   if (pos.x() >= 0 && pos.x() < GameMap::Inst()->Width() && pos.y() >= 0 && pos.y() < GameMap::Inst()->Height()) {
        std::set<int> *npcList = GameMap::Inst()->NPCList(pos.x(), pos.y());
        if (!npcList->empty()) {
            for (std::set<int>::iterator npci = npcList->begin(); npci != npcList->end(); ++npci) {
                underCursor.push_back(Game::Inst()->npcList[*npci]);
            }
            return;
        }

        std::set<int> *itemList = GameMap::Inst()->ItemList(pos.x(), pos.y());
        if (!itemList->empty()) {
            std::set<boost::weak_ptr<Item> >::iterator itemi = Game::Inst()->freeItems.find(Game::Inst()->itemList[*itemList->begin()]);
            if (itemi != Game::Inst()->freeItems.end()) {
                underCursor.push_back(*itemi);
                return;
            }
        }

        int entity = GameMap::Inst()->NatureObject(pos.x(), pos.y());
        if (entity > -1) {
            underCursor.push_back(Game::Inst()->natureList[entity]);
            return;
        }

        entity = GameMap::Inst()->Construction(pos.x(), pos.y());
        if (entity > -1) {
            underCursor.push_back(Game::Inst()->constructionList[entity]);
            return;
        }
    }
 }

