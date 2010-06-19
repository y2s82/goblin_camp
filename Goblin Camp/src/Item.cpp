#include <libtcod.hpp>
#include <ticpp.h>
#include <boost/shared_ptr.hpp>

#include "Item.hpp"
#include "Game.hpp"
#include "Map.hpp"
#include "Logger.hpp"

std::vector<ItemPreset> Item::Presets = std::vector<ItemPreset>();
std::vector<ItemCat> Item::Categories = std::vector<ItemCat>();
std::map<std::string, ItemType> Item::itemTypeNames = std::map<std::string, ItemType>();
std::map<std::string, ItemType> Item::itemCategoryNames = std::map<std::string, ItemType>();

Item::Item(Coordinate pos, ItemType typeval, std::vector<boost::weak_ptr<Item> > components) : GameEntity(),
	type(typeval),
	attemptedStore(false),
	decayCounter(-1),
	container(boost::weak_ptr<Item>())
{
	_x = pos.x();
	_y = pos.y();

    name = Item::Presets[type].name;
	categories = Item::Presets[type].categories;
	graphic = Item::Presets[type].graphic;
	color = Item::Presets[type].color;
	if (Item::Presets[type].decays) decayCounter = Item::Presets[type].decaySpeed;

    for (int i = 0; i < (signed int)components.size(); ++i) {
        color = TCODColor::lerp(color, components[i].lock()->Color(), 0.5f);
    }
}

Item::~Item() {
    Logger::Inst()->output<<"Item destroyed\n";
}

void Item::Draw(Coordinate center) {
    int screenx = _x - center.x() + Game::Inst()->ScreenWidth() / 2;
    int screeny = _y - center.y() + Game::Inst()->ScreenHeight() / 2;
	if (!container.lock() && screenx >= 0 && screenx < Game::Inst()->ScreenWidth() && screeny >= 0 && screeny < Game::Inst()->ScreenHeight()) {
		TCODConsole::root->putCharEx(screenx, screeny, graphic, color, TCODColor::black);
	}
}

ItemType Item::Type() {return type;}
bool Item::IsCategory(ItemCategory category) { return (categories.find(category) != categories.end());}
TCODColor Item::Color() {return color;}

void Item::Position(Coordinate pos) {
    _x = pos.x(); _y = pos.y();
}
Coordinate Item::Position() {return this->GameEntity::Position();}

void Item::Reserve(bool value) {
    reserved = value;
    if (!reserved && !container.lock() && !attemptedStore) {
        Game::Inst()->StockpileItem(boost::static_pointer_cast<Item>(shared_from_this()));
        attemptedStore = true;
    }
}

void Item::PutInContainer(boost::weak_ptr<Item> con) {
    container = con;
    attemptedStore = false;

    if (container.lock()) Game::Inst()->ItemContained(boost::static_pointer_cast<Item>(shared_from_this()), true);
    else Game::Inst()->ItemContained(boost::static_pointer_cast<Item>(shared_from_this()), false);

    if (!container.lock() && !reserved) {
        Game::Inst()->StockpileItem(boost::static_pointer_cast<Item>(shared_from_this()));
        attemptedStore = true;
    }
}
boost::weak_ptr<Item> Item::ContainedIn() {return container;}

int Item::Graphic() {return graphic;}

std::string Item::ItemTypeToString(ItemType type) {
    return Item::Presets[type].name;
}

ItemType Item::StringToItemType(std::string str) {
    return itemTypeNames[str];
}

std::string Item::ItemCategoryToString(ItemCategory category) {
    return Item::Categories[category].name;
}

ItemCategory Item::StringToItemCategory(std::string str) {
    return Item::itemCategoryNames[str];
}

std::vector<ItemCategory> Item::Components(ItemType type) {
    return Item::Presets[type].components;
}

ItemCategory Item::Components(ItemType type, int index) {
    return Item::Presets[type].components[index];
}

void Item::LoadPresets(ticpp::Document doc) {
    std::string strVal;
    int intVal = 0;
    ticpp::Element* parent = doc.FirstChildElement();

    Logger::Inst()->output<<"Reading items.xml\n";
    Logger::Inst()->output.flush();
    try {
        ticpp::Iterator<ticpp::Node> node;
        for (node = node.begin(parent); node != node.end(); ++node) {
            if (node->Value() == "category") {
                Logger::Inst()->output<<"Category\n";
                Categories.push_back(ItemCat());
                ++Game::ItemCatCount;
                ticpp::Iterator<ticpp::Node> child;
                for (child = child.begin(node->ToElement()); child != child.end(); ++child) {
                    Logger::Inst()->output<<"Children\n";
                    Logger::Inst()->output.flush();
                    if (child->Value() == "name") {
                        Categories.back().name = child->ToElement()->GetText();
                        itemCategoryNames.insert(std::pair<std::string, ItemCategory>(Categories.back().name, Game::ItemCatCount-1));
                    } else if (child->Value() == "flammable") {
                        Categories.back().flammable = (child->ToElement()->GetText() == "true") ? true : false;
                    }
                }
            } else if (node->Value() == "item") {
                Logger::Inst()->output<<"Item\n";
                Presets.push_back(ItemPreset());
                ++Game::ItemTypeCount;
                ticpp::Iterator<ticpp::Node> child;
                for (child = child.begin(node->ToElement()); child != child.end(); ++child) {
                    if (child->Value() == "name") {
                        Presets.back().name = child->ToElement()->GetText();
                        Logger::Inst()->output<<"Name: "<<Presets.back().name<<"\n";
                        itemTypeNames.insert(std::pair<std::string, ItemType>(Presets.back().name, Game::ItemTypeCount-1));
                    } else if (child->Value() == "category") {
                        Presets.back().categories.insert(Item::StringToItemCategory(child->ToElement()->GetText()));
                    } else if (child->Value() == "graphic") {
                        child->ToElement()->GetText(&intVal);
                        Presets.back().graphic = intVal;
                    } else if (child->Value() == "color") {
                        int r = -1,g = -1,b = -1;
                        ticpp::Iterator<ticpp::Node> c;
                        for (c = c.begin(child->ToElement()); c != c.end(); ++c) {
                            c->ToElement()->GetTextOrDefault(&intVal, 0);
                            if (r == -1) r = intVal;
                            else if (g == -1) g = intVal;
                            else b = intVal;
                        }
                        Presets.back().color = TCODColor(r,g,b);
                    } else if (child->Value() == "components") {
                        ticpp::Iterator<ticpp::Node> c;
                        for (c = c.begin(child->ToElement()); c != c.end(); ++c) {
                            Presets.back().components.push_back(Item::StringToItemCategory(c->ToElement()->GetText()));
                        }
                    } else if (child->Value() == "seasons") {
                        Logger::Inst()->output<<"Seasons\n";
                        ticpp::Iterator<ticpp::Node> seasons;
                        for (seasons = seasons.begin(child->ToElement()); seasons != seasons.end(); ++seasons) {
                            strVal = seasons->ToElement()->GetText();
                            if (strVal == "spring") { Presets.back().season |= SPRING; Logger::Inst()->output<<"Found seed for spring"; }
                            else if (strVal == "summer") { Presets.back().season |= SUMMER; Logger::Inst()->output<<"Found seed for summer"; }
                            else if (strVal == "fall") { Presets.back().season |= FALL; Logger::Inst()->output<<"Found seed for fall"; }
                        }
                    } else if (child->Value() == "nutrition") {
                        child->ToElement()->GetText(&intVal);
                        Presets.back().nutrition = intVal;
                        Presets.back().organic = true;
                    } else if (child->Value() == "growth") {
                        Presets.back().growth = Item::StringToItemType(child->ToElement()->GetText());
                        Presets.back().organic = true;
                    } else if (child->Value() == "fruits") {
                        Logger::Inst()->output<<"Fruits\n";
                        ticpp::Iterator<ticpp::Node> fruits;
                        for (fruits = fruits.begin(child->ToElement()); fruits != fruits.end(); ++fruits) {
                            fruits->ToElement()->GetText(&intVal);
                            Presets.back().fruits.push_back(intVal);
                        }
                    } else if (child->Value() == "organic") {
                        Presets.back().organic = true;
                    } else if (child->Value() == "multiplier") {
                        child->ToElement()->GetText(&intVal);
                        Presets.back().multiplier = intVal;
                    } else if (child->Value() == "container") {
                        child->ToElement()->GetText(&intVal);
                        Presets.back().container = intVal;
                    } else if (child->Value() == "fitsin") {
                        Presets.back().fitsin = Item::StringToItemCategory(child->ToElement()->GetText());
                    } else if (child->Value() == "decay") {
                        Presets.back().decays = true;
                        ticpp::Iterator<ticpp::Node> decit;
                        for (decit = decit.begin(child->ToElement()); decit != decit.end(); ++decit) {
                            if (decit->Value() == "speed") {
                                decit->ToElement()->GetText(&intVal);
                                Presets.back().decaySpeed = intVal;
                            } else if (decit->Value() == "item") {
                                strVal = decit->ToElement()->GetText();
                                if (strVal != "Filth") {
                                    Presets.back().decayList.push_back(Item::StringToItemType(strVal));
                                } else {
                                    Presets.back().decayList.push_back(-1);
                                }
                            }
                        }
                    }
                }
            }
        }
    } catch (ticpp::Exception& ex) {
        Logger::Inst()->output<<"Failed reading items.xml!\n";
        Logger::Inst()->output<<ex.what()<<'\n';
        Game::Inst()->Exit();
    }

    Logger::Inst()->output<<"Finished reading items.xml\nItems: "<<Presets.size()<<'\n';
}

ItemCat::ItemCat() {}

ItemPreset::ItemPreset() :
        graphic('?'),
        color(TCODColor::pink),
        name("Preset default"),
        categories(std::set<ItemCategory>()),
        season(-1),
        nutrition(-1),
        growth(-1),
        fruits(std::list<ItemType>()),
        organic(false),
        container(0),
        multiplier(1),
        fitsin(-1),
        decays(false),
        decaySpeed(0),
        decayList(std::vector<ItemType>())
{}

OrganicItem::OrganicItem(Coordinate pos, ItemType typeVal) : Item(pos, typeVal),
    season(-1),
    nutrition(-1),
    growth(-1)
{}

SeasonType OrganicItem::Season() { return season; }
void OrganicItem::Season(SeasonType val) { season = val; }

int OrganicItem::Nutrition() { return nutrition; }
void OrganicItem::Nutrition(int val) { nutrition = val; }

ItemType OrganicItem::Growth() { return growth; }
void OrganicItem::Growth(ItemType val) { growth = val; }
