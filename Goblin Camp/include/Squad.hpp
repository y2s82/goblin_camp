#pragma once

#include <boost/weak_ptr.hpp>
#include <string>
#include <list>

#include "Coordinate.hpp"
#include "Entity.hpp"

enum Orders {
	GUARD,
	PATROL,
	ESCORT
};

class Squad : public boost::enable_shared_from_this<Squad> {
private:
	std::string name;
	int memberReq;
	//List of NPC uid's
	std::list<int> members;
	Orders order;
	Coordinate targetCoordinate;
	boost::weak_ptr<Entity> targetEntity;
	int priority;
public:
	Squad(std::string name="Noname nancyboys", int members=0, int priority=0);
	//Recruits one member if needed and returns true if the squad still requires more members
	bool UpdateMembers();
	Orders Order();
	void Order(Orders);
	Coordinate TargetCoordinate();
	void TargetCoordinate(Coordinate);
	boost::weak_ptr<Entity> TargetEntity();
	void TargetEntity(boost::weak_ptr<Entity>);
	int MemberCount();
	int MemberLimit();
	void MemberLimit(int);
};