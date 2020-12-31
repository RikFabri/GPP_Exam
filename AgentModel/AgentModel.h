#pragma once
#include "IExamPlugin.h"
#include "Exam_HelperStructs.h"
#include "..\BehaviourTree\BehaviourTree.h"
#include <vector>

class BaseSteeringBehaviour;

struct AgentModel : AgentInfo
{
	std::vector<Elite::Vector3> ScaredMap;

	BaseSteeringBehaviour* pCurrentSteering;

	BehaviourTree::INode* pBehaviourTree;

	Elite::Vector2 Target;

	void SetAgentInfo(const AgentInfo& agentInfo);
	
	~AgentModel();
};

