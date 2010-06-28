#include "Squad.hpp"
#include "Game.hpp"

Squad::Squad(std::string nameValue, int memberValue, int pri) :
	name(nameValue),
	memberReq(memberValue),
	order(NOORDER),
	targetCoordinate(Coordinate(0,0)),
	priority(pri)
	{}

bool Squad::UpdateMembers() {
#ifdef DEBUG
	std::cout<<"members.size(): "<<members.size()<<" memberReq: "<<memberReq<<"\n";
#endif
	if ((signed int)members.size() < memberReq) {
#ifdef DEBUG
		std::cout<<"Searching for member\n";
#endif
		int newMember = Game::Inst()->FindMilitaryRecruit();
		if (newMember >= 0) { 
			members.push_back(newMember);
			Game::Inst()->npcList[newMember]->MemberOf(shared_from_this());
		}
	}
	if ((signed int)members.size() < memberReq) return true;
	return false;
}

Orders Squad::Order() {return order;}

void Squad::Order(Orders newOrder) {order = newOrder;}

Coordinate Squad::TargetCoordinate() {return targetCoordinate;}
void Squad::TargetCoordinate(Coordinate newTarget) {targetCoordinate = newTarget;}

boost::weak_ptr<Entity> Squad::TargetEntity() {return targetEntity;}
void Squad::TargetEntity(boost::weak_ptr<Entity> newEntity) {targetEntity = newEntity;}

int Squad::MemberCount() { return members.size(); }
int Squad::MemberLimit() { return memberReq; }
void Squad::MemberLimit(int val) { memberReq = val; }
std::string Squad::Name() { return name; }

void Squad::Leave(int member) {
	for (std::list<int>::iterator membi = members.begin(); membi != members.end(); ++membi) {
		if (*membi == member) {
			members.erase(membi);
			break;
		}
	}
}