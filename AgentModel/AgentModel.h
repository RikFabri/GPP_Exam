#pragma once
#include "IExamPlugin.h"
#include "Exam_HelperStructs.h"
#include "..\BehaviourTree\BehaviourTree.h"

class SteeringBehaviour;

struct AgentModel : AgentInfo
{
	std::vector<SteeringBehaviour*> steeringBehaviour;
	SteeringBehaviour* pCurrentSteering;
	BehaviourTree::INode* pBehaviourTree;

	Elite::Vector2 Target;
};

