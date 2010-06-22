#include <libtcod.hpp>
#include <string>
#include <vector>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>

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
	underCursor(std::list<boost::weak_ptr<Entity> >()),
	drawCursor(false),
	lbuttonPressed(false),
	mbuttonPressed(false),
	rbuttonPressed(false)
{
	currentMenu = Menu::MainMenu();
	menuHistory.reserve(10);
	placementCallback = boost::bind(Game::CheckPlacement, _1, _2);
	callback = boost::bind(Game::PlaceConstruction, _1, 0);
	rectCallback = boost::bind(Game::PlaceStockpile, _1, _2, 0, 0);
	mouseInput = TCODMouse::getStatus();
	oldMouseInput = mouseInput;
}

UI* UI::Inst() {
	if (!instance) instance = new UI();
	return instance;
}

void UI::Update() {
	if (keyHelpTextColor > 0) keyHelpTextColor -= 3;
	if (keyHelpTextColor < 0) keyHelpTextColor = 0;
	HandleKeyboard();
	HandleMouse();
}

void UI::HandleKeyboard() {
	TCOD_key_t key = TCODConsole::checkForKeypress(TCOD_KEY_PRESSED);
	if (key.c == 'q') Game::Exit();
	else if (key.c == 'b') {
		menuX = mouseInput.cx;
		menuY = mouseInput.cy;
		menuOpen = true;
		currentMenu->selected(-1);
		currentMenu = Menu::BasicsMenu();
		menuHistory.clear();
	} else if (key.c == 'w') {
		menuX = mouseInput.cx;
		menuY = mouseInput.cy;
		menuOpen = true;
		currentMenu->selected(-1);
		currentMenu = Menu::WorkshopsMenu();
		menuHistory.clear();
	} else if (key.c == 'o') {
		menuX = mouseInput.cx;
		menuY = mouseInput.cy;
		menuOpen = true;
		currentMenu->selected(-1);
		currentMenu = Menu::OrdersMenu();
		menuHistory.clear();
	} else if (key.c == 's') {
		menuX = mouseInput.cx;
		menuY = mouseInput.cy;
		menuOpen = true;
		currentMenu->selected(-1);
		currentMenu = StockManagerMenu::StocksMenu();
		menuHistory.clear();
	} else if (key.c >= '0' && key.c <= '9') {
		if (menuOpen) {
			currentMenu->selected(boost::lexical_cast<int>((char)key.c)-1);
			currentMenu->Callback(boost::lexical_cast<int>((char)key.c)-1);
		}
	} else if (key.c == 'h') {
		keyHelpTextColor = 255;
	}

	int addition = 1;
	if (key.shift) addition *= 10;
	if (key.vk == TCODK_UP) {
		if (Game::Inst()->center.y(Game::Inst()->center.y()-1) < (Game::Inst()->ScreenHeight() / 2)-1) Game::Inst()->center.y((Game::Inst()->ScreenHeight() / 2)-1);
	} else if (key.vk == TCODK_DOWN) {
		if (Game::Inst()->center.y(Game::Inst()->center.y()+1) > 1+ Map::Inst()->Height() - (Game::Inst()->ScreenHeight() / 2)) Game::Inst()->center.y(1+ Map::Inst()->Height() - (Game::Inst()->ScreenHeight() / 2));
	} else if (key.vk == TCODK_LEFT) {
		if (Game::Inst()->center.x(Game::Inst()->center.x()-1) < (Game::Inst()->ScreenWidth() / 2)-1) Game::Inst()->center.x((Game::Inst()->ScreenWidth() / 2)-1);
	} else if (key.vk == TCODK_RIGHT) {
		if (Game::Inst()->center.x(Game::Inst()->center.x()+1) > 1+ Map::Inst()->Width() - (Game::Inst()->ScreenWidth() / 2)) Game::Inst()->center.x(1+ Map::Inst()->Width() - (Game::Inst()->ScreenWidth() / 2));
	} else if (key.vk == TCODK_KP1) {
		TCODMouse::showCursor(false);
		drawCursor = true;
		mouseInput.x -= Game::Inst()->CharWidth() * addition;
		mouseInput.cx -= addition;
		mouseInput.y += Game::Inst()->CharHeight() * addition;
		mouseInput.cy += addition;
	} else if (key.vk == TCODK_KP2) {
		TCODMouse::showCursor(false);
		drawCursor = true;
		mouseInput.y += Game::Inst()->CharHeight() * addition;
		mouseInput.cy += addition;
	} else if (key.vk == TCODK_KP3) {
		TCODMouse::showCursor(false);
		drawCursor = true;
		mouseInput.x += Game::Inst()->CharWidth() * addition;
		mouseInput.cx += addition;
		mouseInput.y += Game::Inst()->CharHeight() * addition;
		mouseInput.cy += addition;
	} else if (key.vk == TCODK_KP4) {
		TCODMouse::showCursor(false);
		drawCursor = true;
		mouseInput.x -= Game::Inst()->CharWidth() * addition;
		mouseInput.cx -= addition;
	} else if (key.vk == TCODK_KP6) {
		TCODMouse::showCursor(false);
		drawCursor = true;
		mouseInput.x += Game::Inst()->CharWidth() * addition;
		mouseInput.cx += addition;
	} else if (key.vk == TCODK_KP7) {
		TCODMouse::showCursor(false);
		drawCursor = true;
		mouseInput.x -= Game::Inst()->CharWidth() * addition;
		mouseInput.cx -= addition;
		mouseInput.y -= Game::Inst()->CharHeight() * addition;
		mouseInput.cy -= addition;
	} else if (key.vk == TCODK_KP8) {
		TCODMouse::showCursor(false);
		drawCursor = true;
		mouseInput.y -= Game::Inst()->CharHeight() * addition;
		mouseInput.cy -= addition;
	} else if (key.vk == TCODK_KP9) {
		TCODMouse::showCursor(false);
		drawCursor = true;
		mouseInput.x += Game::Inst()->CharWidth() * addition;
		mouseInput.cx += addition;
		mouseInput.y -= Game::Inst()->CharHeight() * addition;
		mouseInput.cy -= addition;
	} else if (key.vk == TCODK_ENTER || key.vk == TCODK_KPENTER) {
		lbuttonPressed = true;
	} else if (key.vk == TCODK_CONTROL || key.vk == TCODK_KP0) {
		rbuttonPressed = true;
	} else if (key.vk == TCODK_SPACE) { Game::Inst()->Pause();
	} else if (key.vk == TCODK_PRINTSCREEN) { TCODSystem::saveScreenshot(0);
	}

	if (mouseInput.x < 0) {
		Game::Inst()->center.x(Game::Inst()->center.x() + mouseInput.cx);
		mouseInput.x = 0;
		mouseInput.cx = 0;
	} else if (mouseInput.cx >= Game::Inst()->ScreenWidth()) {
		Game::Inst()->center.x(Game::Inst()->center.x() + (mouseInput.cx - Game::Inst()->ScreenWidth()));
		mouseInput.cx = Game::Inst()->ScreenWidth() - 1;
		mouseInput.x = (Game::Inst()->ScreenWidth() - 1)* Game::Inst()->CharWidth();
	}
	if (mouseInput.y < 0) {
		Game::Inst()->center.y(Game::Inst()->center.y() + mouseInput.cy);
		mouseInput.y = 0;
		mouseInput.cy = 0;
	} else if (mouseInput.cy >= Game::Inst()->ScreenHeight()) {
		Game::Inst()->center.y(Game::Inst()->center.y() + (mouseInput.cy - Game::Inst()->ScreenHeight()));
		mouseInput.cy = Game::Inst()->ScreenHeight() - 1;
		mouseInput.y = (Game::Inst()->ScreenHeight() - 1) * Game::Inst()->CharHeight();
	}
		
	if (Game::Inst()->center.y(Game::Inst()->center.y()-1) < (Game::Inst()->ScreenHeight() / 2)-1) Game::Inst()->center.y((Game::Inst()->ScreenHeight() / 2)-1);
	if (Game::Inst()->center.y(Game::Inst()->center.y()+1) > 1+ Map::Inst()->Height() - (Game::Inst()->ScreenHeight() / 2)) Game::Inst()->center.y(1+ Map::Inst()->Height() - (Game::Inst()->ScreenHeight() / 2));
	if (Game::Inst()->center.x(Game::Inst()->center.x()-1) < (Game::Inst()->ScreenWidth() / 2)-1) Game::Inst()->center.x((Game::Inst()->ScreenWidth() / 2)-1);
	if (Game::Inst()->center.x(Game::Inst()->center.x()+1) > 1+ Map::Inst()->Width() - (Game::Inst()->ScreenWidth() / 2)) Game::Inst()->center.x(1+ Map::Inst()->Width() - (Game::Inst()->ScreenWidth() / 2));

}

void UI::HandleMouse() {
	int tmp;
	MenuResult menuResult;
	bool xswap = false, yswap = false;
	TCOD_mouse_t tempStatus = TCODMouse::getStatus();
	if (tempStatus.x != oldMouseInput.x || tempStatus.y != oldMouseInput.y) {
		mouseInput = tempStatus;
		oldMouseInput = mouseInput;
		drawCursor = false;
		TCODMouse::showCursor(true);
	}

	if (tempStatus.lbutton_pressed) lbuttonPressed = true;
	if (tempStatus.mbutton_pressed) mbuttonPressed = true;
	if (tempStatus.rbutton_pressed) rbuttonPressed = true;

	Coordinate center = Game::Inst()->center;
	
    if (_state == UINORMAL) {
        HandleUnderCursor(Coordinate(mouseInput.cx + center.x() - Game::Inst()->ScreenWidth() / 2, mouseInput.cy + center.y() - Game::Inst()->ScreenHeight() / 2));
    }

	if (_state == UIPLACEMENT || _state == UIABPLACEMENT || _state == UIRECTPLACEMENT) {
		placeable = placementCallback(Coordinate(mouseInput.cx + center.x() - Game::Inst()->ScreenWidth() / 2, mouseInput.cy + center.y() - Game::Inst()->ScreenHeight() / 2), _blueprint);
	}

	if (_state == UIABPLACEMENT || _state == UIRECTPLACEMENT) {
		b.x(mouseInput.cx + center.x() - Game::Inst()->ScreenWidth() / 2);
		b.y(mouseInput.cy + center.y() - Game::Inst()->ScreenHeight() / 2);
	}

	if (lbuttonPressed) {
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

	if (rbuttonPressed) {
		menuX = mouseInput.cx;
		menuY = mouseInput.cy;
		menuOpen = !menuOpen;
		currentMenu->selected(-1);
		currentMenu = Menu::MainMenu();
		menuHistory.clear();
		if (!menuOpen) { _state = UINORMAL; a.x(0); a.y(0); }
	}

	if (mbuttonPressed && menuOpen && !menuHistory.empty()) {
		currentMenu->selected(-1);
		_state = UINORMAL; a.x(0); a.y(0);
		currentMenu = menuHistory.back();
		menuHistory.pop_back();
	}

	if (tempStatus.lbutton && _state == UINORMAL) {
	    Game::Inst()->center.x(Game::Inst()->center.x() - (tempStatus.dx / 3));
	    Game::Inst()->center.y(Game::Inst()->center.y() - (tempStatus.dy / 3));
	    if (Game::Inst()->center.y(Game::Inst()->center.y()-1) < (Game::Inst()->ScreenHeight() / 2)-1) Game::Inst()->center.y((Game::Inst()->ScreenHeight() / 2)-1);
        if (Game::Inst()->center.y(Game::Inst()->center.y()+1) > 1+ Map::Inst()->Height() - (Game::Inst()->ScreenHeight() / 2)) Game::Inst()->center.y(1+ Map::Inst()->Height() - (Game::Inst()->ScreenHeight() / 2));
		if (Game::Inst()->center.x(Game::Inst()->center.x()-1) < (Game::Inst()->ScreenWidth() / 2)-1) Game::Inst()->center.x((Game::Inst()->ScreenWidth() / 2)-1);
		if (Game::Inst()->center.x(Game::Inst()->center.x()+1) > 1+ Map::Inst()->Width() - (Game::Inst()->ScreenWidth() / 2)) Game::Inst()->center.x(1+ Map::Inst()->Width() - (Game::Inst()->ScreenWidth() / 2));
    }

	lbuttonPressed = false;
	mbuttonPressed = false;
	rbuttonPressed = false;
}
void UI::Draw(Coordinate center) {
	int tmp;
	bool xswap = false, yswap = false;

    DrawTopBar();

	if (menuOpen) {
		currentMenu->Draw(menuX, menuY);
	} else if (_state == UINORMAL && !underCursor.empty() && underCursor.begin()->lock()) {
	    int y = 0;
	    for (std::list<boost::weak_ptr<Entity> >::iterator ucit = underCursor.begin(); ucit != underCursor.end(); ++ucit) {
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

	if (drawCursor) TCODConsole::root->putCharEx(mouseInput.cx, mouseInput.cy, 'X', TCODColor::azure, TCODColor::black);
}

void UI::DrawTopBar() {
    TCODConsole::root->setAlignment(TCOD_CENTER);
    TCODConsole::root->print(Game::Inst()->ScreenWidth() / 2, 0, "Orcs: %d   Goblins: %d  -  %s", Game::Inst()->OrcCount(),
                             Game::Inst()->GoblinCount(), Game::Inst()->SeasonToString(Game::Inst()->Season()).c_str());

	if (Game::Inst()->Paused()) {
		TCODConsole::root->setForegroundColor(TCODColor::red);
		TCODConsole::root->print(Game::Inst()->ScreenWidth() / 2, 1, "- - - - PAUSED - - - -");
	}
    TCODConsole::root->setAlignment(TCOD_LEFT);

	if (keyHelpTextColor > 0) {
		int x = Game::Inst()->ScreenWidth() / 2 - 15;
		TCODConsole::root->setForegroundColor(TCODColor(0,keyHelpTextColor,0));
		TCODConsole::root->print(x++, 3, "Q");
		TCODConsole::root->setForegroundColor(TCODColor(keyHelpTextColor,keyHelpTextColor,keyHelpTextColor));
		TCODConsole::root->print(x, 3, "uit");
		x += 3 + 2;
		TCODConsole::root->setForegroundColor(TCODColor(0,keyHelpTextColor,0));
		TCODConsole::root->print(x++, 3, "B");
		TCODConsole::root->setForegroundColor(TCODColor(keyHelpTextColor,keyHelpTextColor,keyHelpTextColor));
		TCODConsole::root->print(x, 3, "asics");
		x += 5 + 2;
		TCODConsole::root->setForegroundColor(TCODColor(0,keyHelpTextColor,0));
		TCODConsole::root->print(x++, 3, "W");
		TCODConsole::root->setForegroundColor(TCODColor(keyHelpTextColor,keyHelpTextColor,keyHelpTextColor));
		TCODConsole::root->print(x, 3, "orkshops");
		x += 8 + 2;
		TCODConsole::root->setForegroundColor(TCODColor(0,keyHelpTextColor,0));
		TCODConsole::root->print(x++, 3, "O");
		TCODConsole::root->setForegroundColor(TCODColor(keyHelpTextColor,keyHelpTextColor,keyHelpTextColor));
		TCODConsole::root->print(x, 3, "rders");
	}
	TCODConsole::root->setForegroundColor(TCODColor::white);
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

boost::weak_ptr<Entity> UI::GetEntity(Coordinate pos) {
    if (pos.x() >= 0 && pos.x() < Map::Inst()->Width() && pos.y() >= 0 && pos.y() < Map::Inst()->Height()) {
        std::set<int> *npcList = Map::Inst()->NPCList(pos.x(), pos.y());
        if (!npcList->empty()) return Game::Inst()->npcList[(*npcList->begin())];

        std::set<int> *itemList = Map::Inst()->ItemList(pos.x(), pos.y());
        if (!itemList->empty()) {
            std::set<boost::weak_ptr<Item> >::iterator itemi = Game::Inst()->freeItems.find(Game::Inst()->itemList[*itemList->begin()]);
            if (itemi != Game::Inst()->freeItems.end()) {
                return *itemi;
            }
        }

        int entity = Map::Inst()->NatureObject(pos.x(), pos.y());
        if (entity > -1) return (Game::Inst()->natureList[entity]);

        entity = Map::Inst()->Construction(pos.x(), pos.y());
        if (entity > -1) return (Game::Inst()->constructionList[entity]);
    }
    return boost::weak_ptr<Entity>();
}

void UI::HandleUnderCursor(Coordinate pos) {
    underCursor.clear();

   if (pos.x() >= 0 && pos.x() < Map::Inst()->Width() && pos.y() >= 0 && pos.y() < Map::Inst()->Height()) {
        std::set<int> *npcList = Map::Inst()->NPCList(pos.x(), pos.y());
        if (!npcList->empty()) {
            for (std::set<int>::iterator npci = npcList->begin(); npci != npcList->end(); ++npci) {
                underCursor.push_back(Game::Inst()->npcList[*npci]);
            }
            return;
        }

        std::set<int> *itemList = Map::Inst()->ItemList(pos.x(), pos.y());
        if (!itemList->empty()) {
            std::set<boost::weak_ptr<Item> >::iterator itemi = Game::Inst()->freeItems.find(Game::Inst()->itemList[*itemList->begin()]);
            if (itemi != Game::Inst()->freeItems.end()) {
                underCursor.push_back(*itemi);
                return;
            }
        }

        int entity = Map::Inst()->NatureObject(pos.x(), pos.y());
        if (entity > -1) {
            underCursor.push_back(Game::Inst()->natureList[entity]);
            return;
        }

        entity = Map::Inst()->Construction(pos.x(), pos.y());
        if (entity > -1) {
            underCursor.push_back(Game::Inst()->constructionList[entity]);
            return;
        }
    }
 }

int UI::KeyHelpTextColor() const { return keyHelpTextColor; }