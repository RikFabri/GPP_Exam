#pragma once
#include <functional>

struct EntityInfo;

namespace AIPredicates
{
	bool SeesZombie(std::function<vector<EntityInfo>()> EntitiesInFOVFunction);
}