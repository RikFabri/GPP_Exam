#pragma once
#include "Exam_HelperStructs.h"
#include "..\BehaviourTree\BehaviourTree.h"

class IExamInterface;
class BaseSteeringBehaviour;

class AgentModel : 
	public AgentInfo
{
public:
	AgentModel(IExamInterface* pInterface);
	~AgentModel();

	SteeringPlugin_Output CalculateSteering(float dt);

	void SetSteeringBehaviour(BaseSteeringBehaviour* behaviour);
	const Elite::Vector2& GetTarget() const;
	const std::vector<Elite::Vector3>& GetScaredMap() const;
private:
	std::vector<Elite::Vector3> m_ScaredMap;

	BaseSteeringBehaviour* m_pCurrentSteering;
	BehaviourTree::INode* m_pBehaviourTree;
	IExamInterface* m_pInterface;

	Elite::Vector2 m_Target;
	Elite::Vector2 m_LookAt;
	bool m_Running;
	bool m_AutoOrienting;


	vector<HouseInfo> GetHousesInFOV() const;
	vector<EntityInfo> GetEntitiesInFOV() const;

	void ConstructBehaviourTree();
};

