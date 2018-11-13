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
#include "stdafx.hpp"

#include "Announce.hpp"
#include "GCamp.hpp"
#include "Coordinate.hpp"
#include "Game.hpp"

AnnounceMessage::AnnounceMessage(std::string nmsg, TCODColor col, Coordinate pos) :
msg(nmsg),
	counter(1),
	color(col),
	target(pos) {}

std::string AnnounceMessage::ToString() {
	result.str("");
	result<<msg;
	if (counter > 1) {
		result<<" x"<<counter;
	}
	if (target.X() > -1 && target.Y() > -1) {
		result<<" "<<(char)TCOD_CHAR_ARROW_E;
	}
	return result.str();
}

Announce* Announce::instance = 0;

Announce* Announce::Inst() {
	if (!instance) instance = new Announce();
	return instance;
}

Announce::Announce() :
timer(0),
	length(0),
	height(0)
{}

void Announce::AddMsg(std::string msg, TCODColor color, const Coordinate& coordinate) {
	msg = msg.substr(0,ANNOUNCE_MAX_LENGTH);
	if (!messageQueue.empty() && messageQueue.back()->msg == msg && messageQueue.back()->target == coordinate) {
		messageQueue.back()->counter++;
		if (messageQueue.size() == 1) timer = 0;
	} else {
		messageQueue.push_back(new AnnounceMessage(msg, color, coordinate));
		if (messageQueue.size() <= ANNOUNCE_HEIGHT) {
			if (msg.length() > length) length = msg.length();
			height = messageQueue.size();
		}
	}
}

void Announce::Update() {
	if (!messageQueue.empty()) {
		++timer;
		if (timer > 0.5*UPDATES_PER_SECOND) {
			if (messageQueue.size() > (unsigned int)ANNOUNCE_HEIGHT) {
				history.push_back(messageQueue.front());
				if (history.size() > 1000) {
					delete(history.front());
					history.pop_front();
				}
				messageQueue.pop_front();
				length = 0;
				for (std::deque<AnnounceMessage*>::iterator msgi = messageQueue.begin(); msgi != messageQueue.end(); ++msgi) {
					if ((*msgi)->msg.length() > length) length = (*msgi)->msg.length();
				}
			}
			timer = 0;
		}
	} else timer = 0;
}

void Announce::AnnouncementClicked(AnnounceMessage *msg) {
	if(msg->target.X() > -1 && msg->target.Y() > -1) {
		Game::Inst()->CenterOn(msg->target);
	}
}

void Announce::AnnouncementClicked(int i) {
	if(i >= 0 && i < (signed int)history.size()) {
		AnnouncementClicked(history[i]);
	}
}

MenuResult Announce::Update(int x, int y, bool clicked) {
	if(x < (signed int)length + 6 && y >= (signed int)top) {
		if(clicked && y > (signed int)top && (y - top - 1) >= 0 && (y - top - 1) < messageQueue.size()) {
			AnnouncementClicked(messageQueue[y - top - 1]);
		}
		return MENUHIT;
	}
	return NOMENUHIT;
}

void Announce::Draw(TCODConsole* console) {
	console->setAlignment(TCOD_LEFT);

	top = console->getHeight() - 1 - height;
	if (height > 0 && (signed int)height < console->getHeight() - 1) {
		console->hline(0, top, length+6);
		console->putChar(length+5, top, TCOD_CHAR_NE, TCOD_BKGND_SET);
		console->vline(length+5, top + 1, height);
		console->rect(0, top + 1, length+5, height, true);

		for (int i = std::min((int)messageQueue.size() - 1, (int)height-1); i >= 0; --i) {
			AnnounceMessage* msg = messageQueue[i];
			console->setDefaultForeground(msg->color);
			console->print(0, console->getHeight()-(height-i), msg->ToString().c_str());
		}
	}
}

void Announce::Draw(Coordinate pos, int from, int amount, TCODConsole* console) {
	int count = 0;
	for (std::deque<AnnounceMessage*>::reverse_iterator ani = messageQueue.rbegin(); ani != messageQueue.rend(); ++ani) {
		if (count++ >= from) {
			console->print(pos.X(), pos.Y()+count-from, (*ani)->ToString().c_str());
			if (count-from+1 == amount) return;
		}
	}
	for (std::deque<AnnounceMessage*>::reverse_iterator ani = history.rbegin(); ani != history.rend(); ++ani) {
		if (count++ >= from) {
			console->print(pos.X(), pos.Y()+count-from, (*ani)->ToString().c_str());
			if (count-from+1 == amount) return;
		}
	}
}

void Announce::EmptyMessageQueue() {
	while (!messageQueue.empty()) {
		history.push_front(messageQueue.front());
		if (history.size() > 1000) {
			delete(history.back());
			history.pop_back();
		}
		messageQueue.pop_front();
	}
	length = 0;
}

int Announce::AnnounceAmount() {
	return history.size()+messageQueue.size();
}

Coordinate Announce::CurrentCoordinate() {
	return messageQueue.front()->target;
}

void Announce::Reset() {
	delete instance;
	instance = 0;
}
