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
	void Update(float dt);

	void SetTarget(const Elite::Vector2& target);
	void SetSteeringBehaviour(BaseSteeringBehaviour* behaviour);
	const Elite::Vector2& GetTarget() const;
	const std::vector<Elite::Vector3>& GetScaredMap() const;
private:
	std::vector<HouseInfo> m_RememberedHouses;
	std::vector<Elite::Vector3> m_ScaredMap;

	BaseSteeringBehaviour* m_pCurrentSteering;
	BehaviourTree::INode* m_pBehaviourTree;
	IExamInterface* m_pInterface;

	EnemyInfo m_TargetZombie;
	Elite::Vector2 m_Target;
	Elite::Vector2 m_LookAt;
	bool m_AutoOrienting;
	bool m_Shooting;
	bool m_Running;

	float m_DefenseTimer;
	int m_NrOfGuns;
	bool m_Looting;

	// Exploration path
	const float m_MaxExploreDistance;
	const float m_ExplorePathAngles;
	const float m_ExploreDistanceChanges;
	float m_CurrentExploreDistance;
	float m_CurrentExploreAngle;
	bool m_ExplorePathGrowing;

	void LookForItems();
	void ManageInventory();

	vector<HouseInfo> GetHousesInFOV() const;
	vector<EntityInfo> GetEntitiesInFOV() const;

	void ConstructBehaviourTree();
};

