#include "stdafx.h"
#include "AgentModel.h"
#include "SteeringBehaviours\SteeringBehaviours.h"

AgentModel::~AgentModel()
{
	SAFE_DELETE(pCurrentSteering);
}

void AgentModel::SetAgentInfo(const AgentInfo& agentInfo)
{
	AgentInfo::operator=(agentInfo);
}
