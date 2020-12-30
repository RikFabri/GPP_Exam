#include "stdafx.h"
#include "AIPredicates.h"
#include "IExamInterface.h"

bool AIPredicates::SeesZombie(std::function<vector<EntityInfo>()> EntitiesInFOVFunction)
{
    auto entities = EntitiesInFOVFunction();

    for (auto entityInfo : entities)
    {
        if (entityInfo.Type == eEntityType::ENEMY)
            return true;
    }

    return false;
}
