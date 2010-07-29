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

#include <string>

#include <libtcod.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "UIComponents.hpp"
#include "UI.hpp"

void Label::Draw(int x, int y, TCODConsole *console) {
    console->setAlignment(TCOD_CENTER);
    console->setForegroundColor(TCODColor::white);
    console->print(x + _x, y + _y, text.c_str());
}

void Button::Draw(int x, int y, TCODConsole *console) {
    if(selected) {
        console->setForegroundColor(TCODColor::black);
        console->setBackgroundColor(TCODColor::white);
    } else {
        console->setForegroundColor(TCODColor::white);
        console->setBackgroundColor(TCODColor::black);
    }
    console->setAlignment(TCOD_CENTER);
    console->printFrame(x + _x, y + _y, width, 3);
    console->print(x + _x + width/2, y + _y + 1, text.c_str());
}

MenuResult Button::Update(int x, int y, bool clicked, TCOD_key_t key) {
    if(key.c == shortcut) {
        callback();
        return MENUHIT;
    }
    if(x >= _x && x < _x + width && y >= _y && y < _y + 3) {
        selected = true;
        if(clicked && callback) {
            callback();
        }
        return MENUHIT;
    } else {
        selected = false;
        return NOMENUHIT;
    }
    
}

void ScrollPanel::Draw(int x, int y, TCODConsole *console) {
    if (scroll < 0) {
        scroll = 0;
    }
	if (scroll + height - 2 > contents->TotalHeight()) {
        scroll = std::max(0, contents->TotalHeight() - (height - 2));
    }
	scrollBar = (int)((height-3) * ((double)scroll / (double)std::max(1, contents->TotalHeight() - (height-2)))) + 2;
	scrollBar = std::min(scrollBar, height - 4);
    
    if(drawFrame) {
        console->printFrame(x + _x, y + _y, width, height);
    }
    
	contents->Draw(x + _x + 1, y + _y + 1, scroll, width - 2, height - 2, console);
	console->putChar(x + _x + width - 2, y + _y + 1, TCOD_CHAR_ARROW_N, TCOD_BKGND_SET);
	console->putChar(x + _x + width - 2, y + _y + height - 2, TCOD_CHAR_ARROW_S, TCOD_BKGND_SET);
	console->putChar(x + _x + width - 2, y + _y + scrollBar, 219, TCOD_BKGND_SET);
}

MenuResult ScrollPanel::Update(int x, int y, bool clicked, TCOD_key_t key) {
    if (x >= _x && x < _x + width && y >= _y && y < _y + height) {
        if (clicked) {
            if (x == _x + width - 2) {
                if (y == _y + 1) {
                    scroll--;
                } else if (y == _y + height - 2) {
                    scroll++;
                } else if (y < scrollBar - _y) {
                    scroll -= height;
                } else if (y > scrollBar - _y) {
                    scroll += height; 
                }
            } else {
                contents->Update(x, y, clicked, key);
            }
        } else {
            contents->Update(x, y, clicked, key);
        }
        return MENUHIT;
    }
    return NOMENUHIT;
}

void Panel::selected(int newSel) {}
void Panel::Open() {}
void Panel::Close() {
	UI::Inst()->SetTextMode(false);
}

void Panel::ShowModal() {
	TCODConsole *background = new TCODConsole(Game::Inst()->ScreenWidth(), Game::Inst()->ScreenHeight());
	TCODConsole::blit (TCODConsole::root, 0, 0, Game::Inst()->ScreenWidth(), Game::Inst()->ScreenHeight(),
                       background, 0, 0);
    
	int _x = (Game::Inst()->ScreenWidth() - width) / 2;
	int _y = (Game::Inst()->ScreenHeight() - height) / 2;
	TCOD_key_t key;
	TCOD_mouse_t mouseStatus;
    
	while (true) {
		TCODConsole::root->clear();
		TCODConsole::root->setForegroundColor(TCODColor::white);
		TCODConsole::root->setBackgroundColor(TCODColor::black);
		TCODConsole::blit(background, 0, 0, Game::Inst()->ScreenWidth(), Game::Inst()->ScreenHeight(),
                          TCODConsole::root, 0, 0, 0.7, 1.0);
        
		Draw(_x, _y, TCODConsole::root);
		TCODConsole::root->flush();
        
		key = TCODConsole::checkForKeypress();
		mouseStatus = TCODMouse::getStatus();
        
		if((Update(mouseStatus.cx, mouseStatus.cy, mouseStatus.lbutton_pressed, key) == MENUHIT && mouseStatus.lbutton_pressed) ||
           mouseStatus.rbutton_pressed || key.vk == TCODK_ESCAPE) {
            delete this;
            return;
        }
	}    
}

Dialog::Dialog(std::vector<Drawable *> ncomponents, std::string ntitle, int nwidth, int nheight):
components(ncomponents), title(ntitle), Panel(nwidth, nheight) {
    _x = (Game::Inst()->ScreenWidth() - width) / 2;
    _y = (Game::Inst()->ScreenHeight() - height) / 2;
}

void Dialog::AddComponent(Drawable *component) {
    components.push_back(component);
}

void Dialog::Draw(int x, int y, TCODConsole *console) {
    console->printFrame(_x, _y, width, height, true, TCOD_BKGND_SET, title.empty() ? 0 : title.c_str());
    for(std::vector<Drawable *>::iterator it = components.begin(); it != components.end(); it++) {
        Drawable *component = *it;
        component->Draw(_x, _y, console);
    }
}

Dialog::~Dialog() {
    for(std::vector<Drawable *>::iterator it = components.begin(); it != components.end(); it++) {
        delete *it;
    }
}

MenuResult Dialog::Update(int x, int y, bool clicked, TCOD_key_t key) {
    MenuResult rtn = NOMENUHIT;
    for(std::vector<Drawable *>::iterator it = components.begin(); it != components.end(); it++) {
        Drawable *component = *it;
        if(component->Update(x - _x, y - _y, clicked, key) == MENUHIT) {
            rtn = MENUHIT;
        }
    }
    return rtn;
}

