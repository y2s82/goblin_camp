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
	msg = msg.substr(0,ANNOUNCE_MAX_LENGTH-4);
	if (!messageQueue.empty() && messageQueue.back()->msg == msg) {
		messageQueue.back()->counter++;
		if (messageQueue.size() == 1) timer = 0;
	} else {
		messageQueue.push_back(new AnnounceMessage(msg, color));
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

void Announce::Draw(unsigned int height, TCODConsole* console) {
    if (height > history.size()+1) height = history.size()+1;

	if (height > 0 && (signed int)height < console->getHeight() - 1) {
        console->hline(0, console->getHeight()-1-height, ANNOUNCE_MAX_LENGTH);
        console->putChar(ANNOUNCE_MAX_LENGTH, console->getHeight()-1-height, TCOD_CHAR_NE, TCOD_BKGND_SET);
        console->vline(ANNOUNCE_MAX_LENGTH, console->getHeight()-height, height);
        console->rect(0, console->getHeight()-height, ANNOUNCE_MAX_LENGTH, height, true);


        while (height > 1) {
            AnnounceMessage* msg = history[history.size()-(height-1)];
            console->setForegroundColor(msg->color);
			console->print(0, console->getHeight()-height, msg->ToString().c_str());
			--height;
        }

        if (!messageQueue.empty()) {
            console->setForegroundColor(messageQueue.front()->color);
			console->print(0, console->getHeight()-height, messageQueue.front()->ToString().c_str());
        }

		console->setForegroundColor(TCODColor::white);
	}
}

void Announce::Draw(Coordinate pos, int from, int amount, TCODConsole* console) {
    EmptyMessageQueue();
    int count = 0;
    for (std::deque<AnnounceMessage*>::iterator ani = history.begin(); ani != history.end(); ++ani) {
        if (count++ >= from) {
            console->print(pos.x(), pos.y()+count-from, (*ani)->ToString().c_str());
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
