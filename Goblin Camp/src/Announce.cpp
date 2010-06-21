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
	if (counter == 1) return msg;
	result.str("");
	result<<msg<<" x"<<counter;
	return result.str();
}

Announce* Announce::instance = 0;

Announce* Announce::Inst() {
	if (!instance) instance = new Announce();
	return instance;
}

Announce::Announce() : timer(0) {}

void Announce::AddMsg(std::string msg, TCODColor color) {
	if (!messageQueue.empty() && messageQueue.back()->msg == msg) {
		messageQueue.back()->counter++;
		if (messageQueue.size() == 1) timer = 0;
	} else {
		messageQueue.push_back(new AnnounceMessage(msg.substr(0,ANNOUNCE_MAX_LENGTH-4), color));
	}
}

void Announce::Update() {
	if (!messageQueue.empty()) {
		++timer;
		if (timer > 1.5*UPDATES_PER_SECOND) {
			history.push_back(messageQueue.front());
			if (history.size() > 1000) {
			    delete(history.front());
			    history.pop_front();
			}
			messageQueue.pop_front();
			timer = 0;
		}
	} else timer = 0;
}

void Announce::Draw(Coordinate pos, unsigned int height) {
    if (height > history.size()+1) height = history.size()+1;

    if (height > 0 && (signed int)height < Game::Inst()->ScreenHeight() - 1) {
        TCODConsole::root->hline(0, Game::Inst()->ScreenHeight()-1-height, ANNOUNCE_MAX_LENGTH);
        TCODConsole::root->putChar(ANNOUNCE_MAX_LENGTH, Game::Inst()->ScreenHeight()-1-height, TCOD_CHAR_NE, TCOD_BKGND_SET);
        TCODConsole::root->vline(ANNOUNCE_MAX_LENGTH, Game::Inst()->ScreenHeight()-height, height);
        TCODConsole::root->rect(0, Game::Inst()->ScreenHeight()-height, ANNOUNCE_MAX_LENGTH, height, true);


        while (height > 1) {
            AnnounceMessage* msg = history[history.size()-(height-1)];
            TCODConsole::root->setForegroundColor(msg->color);
			TCODConsole::root->print(0, Game::Inst()->ScreenHeight()-height, msg->ToString().c_str());
			--height;
        }

        if (!messageQueue.empty()) {
            TCODConsole::root->setForegroundColor(messageQueue.front()->color);
			TCODConsole::root->print(0, Game::Inst()->ScreenHeight()-height, messageQueue.front()->ToString().c_str());
        }

		TCODConsole::root->setForegroundColor(TCODColor::white);
	}
}

void Announce::Draw(Coordinate pos, int from, int amount) {
    EmptyMessageQueue();
    int count = 0;
    for (std::deque<AnnounceMessage*>::iterator ani = history.begin(); ani != history.end(); ++ani) {
        if (count++ >= from) {
            TCODConsole::root->print(pos.x(), pos.y()+count-from, (*ani)->ToString().c_str());
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
}

int Announce::AnnounceAmount() {
    return history.size();
}

Coordinate Announce::CurrentCoordinate() {
    return messageQueue.front()->target;
}
