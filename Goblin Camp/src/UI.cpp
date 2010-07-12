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
#include "Stockpile.hpp"
#include "Farmplot.hpp"

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
	rbuttonPressed(false),
	keyHelpTextColor(0),
	draggingViewport(false),
	textMode(false),
	inputString(std::string("")),
	cursorChar('X')
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
	if (keyHelpTextColor > 0) keyHelpTextColor -= 2;
	if (keyHelpTextColor < 0) keyHelpTextColor = 0;
	HandleKeyboard();
	HandleMouse();
}

void UI::HandleKeyboard() {
	//TODO: This isn't pretty, but it works.
	key = TCODConsole::checkForKeypress(TCOD_KEY_PRESSED);
	if (!textMode) {
		if (key.c == 'q') Game::Exit();
		else if (key.c == 'b') {
			menuX = mouseInput.cx;
			menuY = mouseInput.cy;
			menuOpen = true;
			currentMenu->selected(-1);
			currentMenu = Menu::BasicsMenu();
			menuHistory.clear();
			textMode = false;
		} else if (key.c == 'w') {
			menuX = mouseInput.cx;
			menuY = mouseInput.cy;
			menuOpen = true;
			currentMenu->selected(-1);
			currentMenu = Menu::WorkshopsMenu();
			menuHistory.clear();
			textMode = false;
		} else if (key.c == 'o') {
			menuX = mouseInput.cx;
			menuY = mouseInput.cy;
			menuOpen = true;
			currentMenu->selected(-1);
			currentMenu = Menu::OrdersMenu();
			menuHistory.clear();
			textMode = false;
		} else if (key.c == 's') {
			menuX = mouseInput.cx;
			menuY = mouseInput.cy;
			menuOpen = true;
			currentMenu->selected(-1);
			currentMenu = StockManagerMenu::StocksMenu();
			menuHistory.clear();
			textMode = false;
		} else if (key.c >= '0' && key.c <= '9') {
			if (menuOpen) {
				currentMenu->selected(boost::lexical_cast<int>((char)key.c)-1);
				currentMenu->Callback(boost::lexical_cast<int>((char)key.c)-1);
			}
		} else if (key.c == 'h') {
			keyHelpTextColor = 255;
		}

		int addition = 1;
		if (ShiftPressed()) addition *= 10;
		if (key.vk == TCODK_UP) {
			if (Game::Inst()->upleft.Y(Game::Inst()->upleft.Y()-1) < -1) Game::Inst()->upleft.Y(-1);
		} else if (key.vk == TCODK_DOWN) {
			if (Game::Inst()->upleft.Y(Game::Inst()->upleft.Y()+1) > 1+ Map::Inst()->Height() - Game::Inst()->ScreenHeight()) Game::Inst()->upleft.Y(1+ Map::Inst()->Height() - Game::Inst()->ScreenHeight());
		} else if (key.vk == TCODK_LEFT) {
			if (Game::Inst()->upleft.X(Game::Inst()->upleft.X()-1) < -1) Game::Inst()->upleft.X(-1);
		} else if (key.vk == TCODK_RIGHT) {
			if (Game::Inst()->upleft.X(Game::Inst()->upleft.X()+1) > 1+ Map::Inst()->Width() - Game::Inst()->ScreenWidth()) Game::Inst()->upleft.X(1+ Map::Inst()->Width() - Game::Inst()->ScreenWidth());
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
		} else if (key.vk == TCODK_SPACE) { Game::Inst()->Pause();
		} else if (key.vk == TCODK_PRINTSCREEN) { 
			TCODSystem::saveScreenshot(0);
		}
	} else {
		if (key.c >= ' ' && key.c <= '}' && key.c != '+' && key.c != '-' && (signed int)inputString.size() < inputStringLimit) {
			inputString += key.c;
		} else if (key.vk == TCODK_BACKSPACE) {
			if (inputString.size() > 0) inputString.pop_back();
		}
	}
	if (key.vk == TCODK_ESCAPE && menuOpen) {
		rbuttonPressed = true;
	}

	if (mouseInput.x < 0) {
		Game::Inst()->upleft.X(Game::Inst()->upleft.X() + mouseInput.cx);
		mouseInput.x = 0;
		mouseInput.cx = 0;
	} else if (mouseInput.cx >= Game::Inst()->ScreenWidth()) {
		Game::Inst()->upleft.X(Game::Inst()->upleft.X() + (mouseInput.cx - (Game::Inst()->ScreenWidth()-1)));
		mouseInput.cx = Game::Inst()->ScreenWidth() - 1;
		mouseInput.x = (Game::Inst()->ScreenWidth() - 1)* Game::Inst()->CharWidth();
	}
	if (mouseInput.y < 0) {
		Game::Inst()->upleft.Y(Game::Inst()->upleft.Y() + mouseInput.cy);
		mouseInput.y = 0;
		mouseInput.cy = 0;
	} else if (mouseInput.cy >= Game::Inst()->ScreenHeight()) {
		Game::Inst()->upleft.Y(Game::Inst()->upleft.Y() + (mouseInput.cy - (Game::Inst()->ScreenHeight()-1)));
		mouseInput.cy = Game::Inst()->ScreenHeight() - 1;
		mouseInput.y = (Game::Inst()->ScreenHeight() - 1) * Game::Inst()->CharHeight();
	}
		
	if (Game::Inst()->upleft.Y(Game::Inst()->upleft.Y()-1) < -1) Game::Inst()->upleft.Y(-1);
	if (Game::Inst()->upleft.Y(Game::Inst()->upleft.Y()+1) > 1+ Map::Inst()->Height() - Game::Inst()->ScreenHeight()) Game::Inst()->upleft.Y(1+ Map::Inst()->Height() - Game::Inst()->ScreenHeight());
	if (Game::Inst()->upleft.X(Game::Inst()->upleft.X()-1) < -1) Game::Inst()->upleft.X(-1);
	if (Game::Inst()->upleft.X(Game::Inst()->upleft.X()+1) > 1+ Map::Inst()->Width() - Game::Inst()->ScreenWidth()) Game::Inst()->upleft.X(1+ Map::Inst()->Width() - Game::Inst()->ScreenWidth());
}

void UI::HandleMouse() {
	int tmp;
	MenuResult menuResult = NOMENUHIT;
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

    if (_state == UINORMAL) {
        HandleUnderCursor(Coordinate(mouseInput.cx + Game::Inst()->upleft.X(), mouseInput.cy + Game::Inst()->upleft.Y()));
    }

	if (_state == UIPLACEMENT || _state == UIABPLACEMENT || _state == UIRECTPLACEMENT) {
		placeable = placementCallback(Coordinate(mouseInput.cx + Game::Inst()->upleft.X(), mouseInput.cy + Game::Inst()->upleft.Y()), _blueprint);
	}

	if (_state == UIABPLACEMENT || _state == UIRECTPLACEMENT) {
		b.X(mouseInput.cx + Game::Inst()->upleft.X());
		b.Y(mouseInput.cy + Game::Inst()->upleft.Y());
	}

	currentMenu->Update(mouseInput.cx, mouseInput.cy);
	if (lbuttonPressed && draggingViewport) draggingViewport = false;
	else if (lbuttonPressed) {
		menuResult = sideBar.Update(mouseInput.cx, mouseInput.cy);
	    if (menuResult == NOMENUHIT) {
			if (menuOpen) {menuResult = currentMenu->Update(mouseInput.cx, mouseInput.cy, true); lbuttonPressed = false; }
			if (menuResult == NOMENUHIT) {
				if (_state == UIPLACEMENT && placeable) {
					callback(Coordinate(mouseInput.cx + Game::Inst()->upleft.X(), mouseInput.cy + Game::Inst()->upleft.Y()));
				} else if (_state == UIABPLACEMENT && placeable) {
					if (a.X() == 0) {
						a.X(mouseInput.cx + Game::Inst()->upleft.X());
						a.Y(mouseInput.cy + Game::Inst()->upleft.Y());
					}
					else {
						//Place construction from a->b
						if (a.X() > b.X()) {
							tmp = a.X();
							a.X(b.X());
							b.X(tmp);
							xswap = true;
						}
						if (a.Y() > b.Y()) {
							tmp = a.Y();
							a.Y(b.Y());
							b.Y(tmp);
							yswap = true;
						}
						for (int ix = a.X(); ix <= b.X(); ++ix) {
							if (!yswap) {
								if (placementCallback(Coordinate(ix, a.Y()), _blueprint))
									callback(Coordinate(ix, a.Y()));
							} else {
								if (placementCallback(Coordinate(ix, b.Y()), _blueprint))
									callback(Coordinate(ix, b.Y()));
							}
						}
						for (int iy = a.Y(); iy <= b.Y(); ++iy) {
							if (!xswap) {
								if (placementCallback(Coordinate(b.X(), iy), _blueprint))
									callback(Coordinate(b.X(), iy));
							} else {
								if (placementCallback(Coordinate(a.X(), iy), _blueprint))
									callback(Coordinate(a.X(), iy));
							}
						}
						a.X(0); a.Y(0); b.X(0); b.Y(0);
					}
				} else if (_state == UIRECTPLACEMENT && placeable) {
					if (a.X() == 0) {
						a.X(mouseInput.cx + Game::Inst()->upleft.X());
						a.Y(mouseInput.cy + Game::Inst()->upleft.Y());
					}
					else {
						//Place construction from a->b
						if (a.X() > b.X()) {
							tmp = a.X();
							a.X(b.X());
							b.X(tmp);
							xswap = true;
						}
						if (a.Y() > b.Y()) {
							tmp = a.Y();
							a.Y(b.Y());
							b.Y(tmp);
							yswap = true;
						}

						if (placementCallback(Coordinate(a.X(), a.Y()), _blueprint))
							rectCallback(a,b);

						a.X(0); a.Y(0); b.X(0); b.Y(0);
					}
				} else { //Current state is not any kind of placement, so open construction/npc context menu if over one
					if (!underCursor.empty()) sideBar.SetEntity(*underCursor.begin());
					else sideBar.SetEntity(boost::weak_ptr<Entity>());
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
		if (!menuOpen) { _state = UINORMAL; a.X(0); a.Y(0); }
		SetTextMode(false);
	}

	if (mbuttonPressed && menuOpen && !menuHistory.empty()) {
		currentMenu->selected(-1);
		_state = UINORMAL; a.X(0); a.Y(0);
		currentMenu = menuHistory.back();
		menuHistory.pop_back();
		SetTextMode(false);
	}

	if (tempStatus.lbutton && _state == UINORMAL) {
	    Game::Inst()->upleft.X(Game::Inst()->upleft.X() - (tempStatus.dx / 3));
	    Game::Inst()->upleft.Y(Game::Inst()->upleft.Y() - (tempStatus.dy / 3));
	    if (Game::Inst()->upleft.Y(Game::Inst()->upleft.Y()-1) < -1) Game::Inst()->upleft.Y(-1);
        if (Game::Inst()->upleft.Y(Game::Inst()->upleft.Y()+1) > 1+ Map::Inst()->Height() - Game::Inst()->ScreenHeight()) Game::Inst()->upleft.Y(1+ Map::Inst()->Height() - Game::Inst()->ScreenHeight());
		if (Game::Inst()->upleft.X(Game::Inst()->upleft.X()-1) < -1) Game::Inst()->upleft.X(-1);
		if (Game::Inst()->upleft.X(Game::Inst()->upleft.X()+1) > 1+ Map::Inst()->Width() - Game::Inst()->ScreenWidth()) Game::Inst()->upleft.X(1+ Map::Inst()->Width() - Game::Inst()->ScreenWidth());
		if (tempStatus.dx > 0 || tempStatus.dy > 0) draggingViewport = true;
    }

	lbuttonPressed = false;
	mbuttonPressed = false;
	rbuttonPressed = false;
}
void UI::Draw(Coordinate upleft, TCODConsole* console) {
	int tmp;
	bool xswap = false, yswap = false;

    DrawTopBar(console);
	sideBar.Draw(console);

	if (menuOpen) {
		currentMenu->Draw(menuX, menuY, console);
	} else if (_state == UINORMAL && !underCursor.empty() && underCursor.begin()->lock()) {
	    int y = 0;
	    for (std::list<boost::weak_ptr<Entity> >::iterator ucit = underCursor.begin(); ucit != underCursor.end(); ++ucit) {
            console->print(std::min(console->getWidth()-1, mouseInput.cx+1), std::max(0, mouseInput.cy-1-y), ucit->lock()->Name().c_str());
            ++y;
	    }
	}

	if (_state == UIPLACEMENT || ((_state == UIABPLACEMENT || _state == UIRECTPLACEMENT) && a.X() == 0)) {
		for (int x = mouseInput.cx; x < mouseInput.cx + _blueprint.X() && x < console->getWidth(); ++x) {
			for (int y = mouseInput.cy; y < mouseInput.cy + _blueprint.Y() && y < console->getHeight(); ++y) {
				if (!placeable) console->putCharEx(x,y, cursorChar, TCODColor::red, TCODColor::black);
				else console->putCharEx(x,y, cursorChar, TCODColor::green, TCODColor::black);
			}
		}
	} else if (_state == UIABPLACEMENT && a.X() > 0) {
		if (a.X() > b.X()) {
			tmp = a.X();
			a.X(b.X());
			b.X(tmp);
			xswap = true;
		}
		if (a.Y() > b.Y()) {
			tmp = a.Y();
			a.Y(b.Y());
			b.Y(tmp);
			yswap = true;
		}
		for (int ix = a.X()-upleft.X(); ix <= b.X()-upleft.X() && ix < console->getWidth(); ++ix) {
			if (!yswap) {
				if (!placeable) console->putCharEx(ix,a.Y()-upleft.Y(), cursorChar, TCODColor::red, TCODColor::black);
				else console->putCharEx(ix,a.Y()-upleft.Y(), cursorChar, TCODColor::green, TCODColor::black);
			}
			else {
				if (!placeable) console->putCharEx(ix,b.Y()-upleft.Y(), cursorChar, TCODColor::red, TCODColor::black);
				else console->putCharEx(ix,b.Y()-upleft.Y(), cursorChar, TCODColor::green, TCODColor::black);
			}
		}
		for (int iy = a.Y()-upleft.Y(); iy <= b.Y()-upleft.Y() && iy < console->getHeight(); ++iy) {
			if (!xswap) {
				if (!placeable) console->putCharEx(b.X()-upleft.X(),iy, cursorChar, TCODColor::red, TCODColor::black);
				else console->putCharEx(b.X()-upleft.X(),iy, cursorChar, TCODColor::green, TCODColor::black);
			}
			else {
				if (!placeable) console->putCharEx(a.X()-upleft.X(),iy, cursorChar, TCODColor::red, TCODColor::black);
				else console->putCharEx(a.X()-upleft.X(),iy, cursorChar, TCODColor::green, TCODColor::black);
			}
		}
		if (xswap) {
			tmp = a.X();
			a.X(b.X());
			b.X(tmp);
		}
		if (yswap) {
			tmp = a.Y();
			a.Y(b.Y());
			b.Y(tmp);
		}
	} else if (_state == UIRECTPLACEMENT && a.X() > 0) {
		if (a.X() > b.X()) {
			tmp = a.X();
			a.X(b.X());
			b.X(tmp);
			xswap = true;
		}
		if (a.Y() > b.Y()) {
			tmp = a.Y();
			a.Y(b.Y());
			b.Y(tmp);
			yswap = true;
		}
		for (int ix = a.X()-upleft.X(); ix <= b.X()-upleft.X() && ix < console->getWidth(); ++ix) {
			for (int iy = a.Y()-upleft.Y(); iy <= b.Y()-upleft.Y() && iy < console->getHeight(); ++iy) {
				if (!placeable) console->putCharEx(ix,iy,cursorChar, TCODColor::red, TCODColor::black);
				else console->putCharEx(ix,iy,cursorChar, TCODColor::green, TCODColor::black);
			}
		}
		if (xswap) {
			tmp = a.X();
			a.X(b.X());
			b.X(tmp);
		}
		if (yswap) {
			tmp = a.Y();
			a.Y(b.Y());
			b.Y(tmp);
		}
	}

	if (drawCursor) console->putCharEx(mouseInput.cx, mouseInput.cy, 'X', TCODColor::azure, TCODColor::black);
}

void UI::DrawTopBar(TCODConsole* console) {
    console->setAlignment(TCOD_CENTER);
	console->setForegroundColor(TCODColor::white);
    console->print(console->getWidth() / 2, 0, "Orcs: %d   Goblins: %d  -  %s", Game::Inst()->OrcCount(),
                             Game::Inst()->GoblinCount(), Game::Inst()->SeasonToString(Game::Inst()->CurrentSeason()).c_str());

	if (Game::Inst()->Paused()) {
		console->setForegroundColor(TCODColor::red);
		console->print(Game::Inst()->ScreenWidth() / 2, 1, "- - - - PAUSED - - - -");
	}
    console->setAlignment(TCOD_LEFT);

	if (keyHelpTextColor > 0) {
		int x = console->getWidth() / 2 - 15;
		console->setForegroundColor(TCODColor(0,keyHelpTextColor,0));
		console->print(x++, 3, "Q");
		console->setForegroundColor(TCODColor(keyHelpTextColor,keyHelpTextColor,keyHelpTextColor));
		console->print(x, 3, "uit");
		x += 3 + 2;
		console->setForegroundColor(TCODColor(0,keyHelpTextColor,0));
		console->print(x++, 3, "B");
		console->setForegroundColor(TCODColor(keyHelpTextColor,keyHelpTextColor,keyHelpTextColor));
		console->print(x, 3, "asics");
		x += 5 + 2;
		console->setForegroundColor(TCODColor(0,keyHelpTextColor,0));
		console->print(x++, 3, "W");
		console->setForegroundColor(TCODColor(keyHelpTextColor,keyHelpTextColor,keyHelpTextColor));
		console->print(x, 3, "orkshops");
		x += 8 + 2;
		console->setForegroundColor(TCODColor(0,keyHelpTextColor,0));
		console->print(x++, 3, "O");
		console->setForegroundColor(TCODColor(keyHelpTextColor,keyHelpTextColor,keyHelpTextColor));
		console->print(x, 3, "rders");
	}

	console->setForegroundColor(TCODColor::white);
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
	UI::Inst()->SetCursor('C');
}

void UI::ChooseStockpile(ConstructionType stockpile) {
	int stockpileSymbol = '%';
	UI::Inst()->SetRectCallback(boost::bind(Game::PlaceStockpile, _1, _2, stockpile, stockpileSymbol));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckPlacement, _1, _2));
	UI::Inst()->blueprint(Construction::Blueprint(stockpile));
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetCursor('=');
}

void UI::ChooseTreeFelling() {
    UI::Inst()->state(UIRECTPLACEMENT);
    UI::Inst()->SetRectCallback(boost::bind(Game::FellTree, _1, _2));
    UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, _2));
    UI::Inst()->blueprint(Coordinate(1,1));
	UI::Inst()->SetCursor('X');
}

void UI::ChoosePlantHarvest() {
    UI::Inst()->state(UIRECTPLACEMENT);
    UI::Inst()->SetRectCallback(boost::bind(Game::HarvestWildPlant, _1, _2));
    UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, _2));
    UI::Inst()->blueprint(Coordinate(1,1));
	UI::Inst()->SetCursor('X');
}

void UI::ChooseOrderTargetCoordinate(boost::shared_ptr<Squad> squad) {
	UI::Inst()->state(UIPLACEMENT);
	UI::Inst()->SetCallback(boost::bind(Game::SetSquadTargetCoordinate, _1, squad));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckPlacement, _1, Coordinate(1,1)));
	UI::Inst()->blueprint(Coordinate(1,1));
	UI::Inst()->SetCursor('X');
}

void UI::ChooseOrderTargetEntity(boost::shared_ptr<Squad> squad) {
	UI::Inst()->state(UIPLACEMENT);
	UI::Inst()->SetCallback(boost::bind(Game::SetSquadTargetEntity, _1, squad));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckPlacement, _1, Coordinate(1,1)));
	UI::Inst()->blueprint(Coordinate(1,1));
	UI::Inst()->SetCursor('X');
}

void UI::ChooseDesignateTree() {
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(boost::bind(Game::DesignateTree, _1, _2));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, _2));
	UI::Inst()->blueprint(Coordinate(1,1));
	UI::Inst()->SetCursor('T');
}

void UI::ChooseDismantle() {
	UI::Inst()->state(UIRECTPLACEMENT);
	UI::Inst()->SetRectCallback(boost::bind(Game::DismantleConstruction, _1, _2));
	UI::Inst()->SetPlacementCallback(boost::bind(Game::CheckTree, _1, _2));
	UI::Inst()->blueprint(Coordinate(1,1));
	UI::Inst()->SetCursor('D');
}

boost::weak_ptr<Entity> UI::GetEntity(Coordinate pos) {
    if (pos.X() >= 0 && pos.X() < Map::Inst()->Width() && pos.Y() >= 0 && pos.Y() < Map::Inst()->Height()) {
        std::set<int> *npcList = Map::Inst()->NPCList(pos.X(), pos.Y());
        if (!npcList->empty()) return Game::Inst()->npcList[(*npcList->begin())];

        std::set<int> *itemList = Map::Inst()->ItemList(pos.X(), pos.Y());
        if (!itemList->empty()) {
            std::set<boost::weak_ptr<Item> >::iterator itemi = Game::Inst()->freeItems.find(Game::Inst()->itemList[*itemList->begin()]);
            if (itemi != Game::Inst()->freeItems.end()) {
                return *itemi;
            }
        }

        int entity = Map::Inst()->NatureObject(pos.X(), pos.Y());
        if (entity > -1) return (Game::Inst()->natureList[entity]);

        entity = Map::Inst()->Construction(pos.X(), pos.Y());
        if (entity > -1) return Game::Inst()->GetConstruction(entity);
    }
    return boost::weak_ptr<Entity>();
}

void UI::HandleUnderCursor(Coordinate pos) {
    underCursor.clear();

   if (pos.X() >= 0 && pos.X() < Map::Inst()->Width() && pos.Y() >= 0 && pos.Y() < Map::Inst()->Height()) {
        std::set<int> *npcList = Map::Inst()->NPCList(pos.X(), pos.Y());
        if (!npcList->empty()) {
            for (std::set<int>::iterator npci = npcList->begin(); npci != npcList->end(); ++npci) {
                underCursor.push_back(Game::Inst()->npcList[*npci]);
            }
            return;
        }

        std::set<int> *itemList = Map::Inst()->ItemList(pos.X(), pos.Y());
        if (!itemList->empty()) {
            std::set<boost::weak_ptr<Item> >::iterator itemi = Game::Inst()->freeItems.find(Game::Inst()->itemList[*itemList->begin()]);
            if (itemi != Game::Inst()->freeItems.end()) {
                underCursor.push_back(*itemi);
                return;
            }
        }

        int entity = Map::Inst()->NatureObject(pos.X(), pos.Y());
        if (entity > -1) {
            underCursor.push_back(Game::Inst()->natureList[entity]);
            return;
        }

        entity = Map::Inst()->Construction(pos.X(), pos.Y());
        if (entity > -1) {
            underCursor.push_back(Game::Inst()->GetConstruction(entity));
            return;
        }
    }
 }

int UI::KeyHelpTextColor() const { return keyHelpTextColor; }

void UI::SetTextMode(bool val, int limit) { 
	textMode = val; 
	if (!textMode) inputString = "";
	inputStringLimit = limit; 
}

std::string UI::InputString() { return inputString;}
void UI::InputString(std::string value) { inputString = value; }

void UI::HideMenu() {
	menuOpen = false;
	textMode = false;
}

void UI::CloseMenu() {
	menuOpen = false;
	_state = UINORMAL;
	a.X(0); a.Y(0);
	textMode = false;
	currentMenu = Menu::MainMenu();
}

void UI::SetCursor(int value) { cursorChar = value; }

SideBar::SideBar() :
    width(19),
	height(30),
	topY(0),
	npc(false),
	construction(false),
	stockpile(false),
	farmplot(false)
{}

MenuResult SideBar::Update(int x, int y) {
	if (entity.lock() && x > Game::Inst()->ScreenWidth() - width) {
		if (stockpile) {
			int i = y - (topY + 15);			
			if (i >= 0 && i < (signed int)Item::Categories.size()) {
				boost::static_pointer_cast<Stockpile>(entity.lock())->SwitchAllowed(i);
				return MENUHIT;
			}
		}
	}
	return NOMENUHIT;
}

void SideBar::Draw(TCODConsole* console) {
	if (entity.lock()) {
		int edgeX = console->getWidth();
		topY = std::max(0,(console->getHeight() - height) / 2);
		TCODConsole minimap(11,11);

		if (npc) {
			console->rect(edgeX - (width-1), topY+1, width-2, height-2, true);
			boost::shared_ptr<NPC> npc(boost::static_pointer_cast<NPC>(entity.lock()));
			console->printFrame(edgeX-(width-1), topY+14, width-2, 12, false, TCOD_BKGND_DEFAULT, "Effects");
			int y = 0;
			for (std::list<StatusEffect>::iterator effectI = npc->StatusEffects()->begin(); effectI != npc->StatusEffects()->end(); ++effectI) {
				console->setForegroundColor(effectI->color);
				console->print(edgeX - width + 2, topY+15+y, "%c%s", effectI->graphic, effectI->name.c_str());
				if (++y > 10) break;
			}
			console->setForegroundColor(TCODColor::white);
			if (npc->MemberOf().lock()) { //Member of a squad
				console->print(edgeX - width + 1, topY+28, npc->MemberOf().lock()->Name().c_str());
			}
		} else if (construction) {
			console->rect(edgeX - (width-1), topY+1, width-2, height-2, true);

			console->printFrame(edgeX-(width-1), topY+14, width-2, 12, false, TCOD_BKGND_DEFAULT, "Production");
			for (int jobi = 0; jobi < std::min(10, (signed int)boost::static_pointer_cast<Construction>(entity.lock())->JobList()->size()); ++jobi) {
				console->setForegroundColor(jobi == 0 ? TCODColor::white : TCODColor::grey);
				console->print(edgeX - width + 2, topY+15+jobi, Item::ItemTypeToString(boost::static_pointer_cast<Construction>(entity.lock())->JobList(jobi)).c_str());
			}		
		} else if (stockpile) {
			console->rect(edgeX - (width-1), topY+1, width-2, height-2, true);

			console->printFrame(edgeX-(width-1), topY+14, width-2, 30, false, TCOD_BKGND_DEFAULT, "Categories");
			boost::shared_ptr<Stockpile> sp(boost::static_pointer_cast<Stockpile>(entity.lock()));
			for (unsigned int i = 0; i < Item::Categories.size(); ++i) {
				console->setForegroundColor(sp->Allowed(i) ? TCODColor::green : TCODColor::red);
				console->print(edgeX-(width-2),topY+15+i, sp->Allowed(i) ? "+ %s" : "- %s", Item::Categories[i].name.c_str());
			}
			console->setForegroundColor(TCODColor::white);
		} else if (farmplot) {
			console->rect(edgeX - (width-1), topY+1, width-2, height-2, true);
		}
		
		Game::Inst()->Draw(entity.lock()->Position()-5, &minimap, false);
		console->setForegroundColor(TCODColor::white);
		console->printFrame(edgeX - width, topY, width, height, false, TCOD_BKGND_DEFAULT, entity.lock()->Name().c_str());
		minimap.flush();
		TCODConsole::blit(&minimap, 0, 0, 11, 11, console, edgeX - (width-4), topY + 2);
	}
	console->setForegroundColor(TCODColor::white);
}

void SideBar::SetEntity(boost::weak_ptr<Entity> ent) {
	entity = ent;
	npc = construction = stockpile = farmplot = false;
	if (boost::dynamic_pointer_cast<NPC>(entity.lock())) {
			height = 30;
			npc = true;
		} else if (boost::dynamic_pointer_cast<FarmPlot>(entity.lock())) {
			height = 50;
			farmplot = true;
		} else if (boost::dynamic_pointer_cast<Stockpile>(entity.lock())) {
			height = 50;
			stockpile = true;
		} else if (boost::dynamic_pointer_cast<Construction>(entity.lock())) {
			height = 30;
			construction = true;
		}
}

bool UI::ShiftPressed() { return TCODConsole::isKeyPressed(TCODK_SHIFT); }
TCOD_key_t UI::getKey() { return key; }