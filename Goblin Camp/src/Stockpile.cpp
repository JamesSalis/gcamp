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

#include "Stockpile.hpp"
#include "Game.hpp"
#include "Map.hpp"

Stockpile::Stockpile(ConstructionType type, int newSymbol, Coordinate target) :
Construction(type, target),
	symbol(newSymbol),
	a(target),
	b(target)
{
	condition = maxCondition;
	reserved.insert(std::pair<Coordinate,bool>(target,false));
	containers.insert(std::pair<Coordinate,boost::shared_ptr<Container> >(target, boost::shared_ptr<Container>(new Container(target, 0, 1, -1))));
	for (int i = 0; i < Game::ItemCatCount; ++i) {
		amount.insert(std::pair<ItemCategory, int>(i,0));
		allowed.insert(std::pair<ItemCategory, bool>(i,true));
	}
}

Stockpile::~Stockpile() {
	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			if (Map::Inst()->Construction(x,y) == uid) {
				Map::Inst()->Buildable(x,y,true);
				Map::Inst()->Walkable(x,y,true);
				Map::Inst()->Construction(x,y,-1);
			}
		}
	}
}

int Stockpile::Build() {return 1;}

boost::weak_ptr<Item> Stockpile::FindItemByCategory(ItemCategory cat, int flags) {
	for (std::map<Coordinate, boost::shared_ptr<Container> >::iterator conti = containers.begin(); conti != containers.end(); ++conti) {
		if (!conti->second->empty()) {
			boost::weak_ptr<Item> item = *conti->second->begin();
			if (item.lock()) {
				if (item.lock()->IsCategory(cat) && !item.lock()->Reserved()) {
					if (flags & NOTFULL && boost::dynamic_pointer_cast<Container>(item.lock())) {
						if (!boost::static_pointer_cast<Container>(item.lock())->Full()) return item;
					} else return item;
				}
				if (boost::dynamic_pointer_cast<Container>(item.lock())) {
					boost::weak_ptr<Container> cont = boost::static_pointer_cast<Container>(item.lock());
					for (std::set<boost::weak_ptr<Item> >::iterator itemi = cont.lock()->begin(); itemi != cont.lock()->end(); ++itemi) {
						if (itemi->lock() && itemi->lock()->IsCategory(cat) && !itemi->lock()->Reserved())
							return *itemi;
					}
				}
			}
		}
	}
	return boost::weak_ptr<Item>();
}

boost::weak_ptr<Item> Stockpile::FindItemByType(ItemType typeValue, int flags) {
	for (std::map<Coordinate, boost::shared_ptr<Container> >::iterator conti = containers.begin(); conti != containers.end(); ++conti) {
		if (!conti->second->empty()) {
			boost::weak_ptr<Item> item = *conti->second->begin();
			if (item.lock()->Type() == typeValue && !item.lock()->Reserved()) {
				if (flags & NOTFULL && boost::dynamic_pointer_cast<Container>(item.lock())) {
					if (!boost::static_pointer_cast<Container>(item.lock())->Full()) return item;
				} else return item;
			}
			if (boost::dynamic_pointer_cast<Container>(item.lock())) {
				boost::weak_ptr<Container> cont = boost::static_pointer_cast<Container>(item.lock());
				for (std::set<boost::weak_ptr<Item> >::iterator itemi = cont.lock()->begin(); itemi != cont.lock()->end(); ++itemi) {
					if (itemi->lock() && itemi->lock()->Type() == typeValue && !itemi->lock()->Reserved())
						return *itemi;
				}
			}
		}
	}
	return boost::weak_ptr<Item>();
}

void Stockpile::Expand(Coordinate from, Coordinate to) {
	//We can assume that from < to
	if (from.X() > b.X() || to.X() < a.X()) return;
	if (from.Y() > b.Y() || to.Y() < a.Y()) return;

	//The algorithm: Check each tile inbetween from and to, and if a tile is adjacent to this
	//stockpile, add it. Do this max(width,height) times.
	int repeats = std::max(to.X() - from.X(), to.Y() - from.Y());
	for (int repeatCount = 0; repeatCount <= repeats; ++repeatCount) {
		for (int ix = from.X(); ix <= to.X(); ++ix) {
			for (int iy = from.Y(); iy <= to.Y(); ++iy) {
				if (Map::Inst()->Construction(ix,iy) == -1 && Map::Inst()->Walkable(ix,iy)) {
					if (Map::Inst()->Construction(ix-1,iy) == uid ||
						Map::Inst()->Construction(ix+1,iy) == uid ||
						Map::Inst()->Construction(ix,iy-1) == uid ||
						Map::Inst()->Construction(ix,iy+1) == uid) {
							//Current tile is walkable, buildable, and adjacent to the current stockpile
							Map::Inst()->Construction(ix,iy,uid);
							Map::Inst()->Buildable(ix,iy,false);
							//Update corner values
							if (ix < a.X()) a.X(ix);
							if (ix > b.X()) b.X(ix);
							if (iy < a.Y()) a.Y(iy);
							if (iy > b.Y()) b.Y(iy);
							reserved.insert(std::pair<Coordinate,bool>(Coordinate(ix,iy),false));
							containers.insert(std::pair<Coordinate,boost::shared_ptr<Container> >(Coordinate(ix,iy), boost::shared_ptr<Container>(new Container(Coordinate(ix,iy), 0, 1, -1))));
					}
				}
			}
		}
	}
}

void Stockpile::Draw(Coordinate upleft, TCODConsole* console) {
	int screenx, screeny;

	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			if (Map::Inst()->Construction(x,y) == uid) {
				screenx = x  - upleft.X();
				screeny = y - upleft.Y();
				if (screenx >= 0 && screenx < console->getWidth() && screeny >= 0 &&
					screeny < console->getHeight()) {
						console->setFore(screenx, screeny, TCODColor::white);
						console->setChar(screenx,	screeny, (graphic[1]));

						if (!containers[Coordinate(x,y)]->empty()) {
							boost::weak_ptr<Item> item = *containers[Coordinate(x,y)]->begin();
							if (item.lock()) {
								console->putCharEx(screenx, screeny, item.lock()->Graphic(), item.lock()->Color(), TCODColor::black);
							}
						}
				}
			}
		}
	}
}

bool Stockpile::Allowed(ItemCategory cat) {
	return allowed[cat];
}

bool Stockpile::Allowed(std::set<ItemCategory> cats) {
	for (std::set<ItemCategory>::iterator cati = cats.begin(); cati != cats.end(); ++cati) {
		if (Allowed(*cati)) return true;
	}
	return false;
}

bool Stockpile::Full()
{
	for (int ix = a.X(); ix <= b.X(); ++ix) {
		for (int iy = a.Y(); iy <= b.Y(); ++iy) {
			if (Map::Inst()->Construction(ix,iy) == uid) {
				if (containers[Coordinate(ix,iy)]->empty()) return false;
			}
		}
	}
	return true;
}

Coordinate Stockpile::FreePosition() {
	for (int ix = a.X(); ix <= b.X(); ++ix) {
		for (int iy = a.Y(); iy <= b.Y(); ++iy) {
			if (Map::Inst()->Construction(ix,iy) == uid) {
				if (containers[Coordinate(ix,iy)]->empty() && !reserved[Coordinate(ix,iy)]) return Coordinate(ix,iy);
			}
		}
	}
	return Coordinate(-1,-1);
}

void Stockpile::ReserveSpot(Coordinate pos, bool val) { reserved[pos] = val; }

boost::weak_ptr<Container> Stockpile::Storage(Coordinate pos) {
	return containers[pos];
}

void Stockpile::SwitchAllowed(ItemCategory cat) {
	allowed[cat] = !allowed[cat];
}
