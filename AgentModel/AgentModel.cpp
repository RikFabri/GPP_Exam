#include "stdafx.h"
#include "AgentModel.h"
#include "..\..\inc\IExamInterface.h"
#include "..\Predicates\AIPredicates.h"
#include "SteeringBehaviours\SteeringBehaviours.h"

using namespace Elite;

AgentModel::AgentModel(IExamInterface* pInterface)
	: m_Running(false)
	, m_pInterface(pInterface)
	, m_LookAt(m_Target)
	, m_AutoOrienting(true)
{
	ConstructBehaviourTree();
}

AgentModel::~AgentModel()
{
	SAFE_DELETE(m_pCurrentSteering);
	SAFE_DELETE(m_pBehaviourTree);
}

SteeringPlugin_Output AgentModel::CalculateSteering(float dt)
{
	AgentInfo::operator=(m_pInterface->Agent_GetInfo());

	int recentScares = std::count_if(m_ScaredMap.begin(), m_ScaredMap.end(), [](const Vector3& v) { return v.z >= 4; });
	m_Running = recentScares > 0;
	m_Running = m_Running || Bitten || WasBitten;

	m_ScaredMap.erase(std::remove_if(m_ScaredMap.begin(), m_ScaredMap.end(), [](const Vector3& v) { return v.z <= 0.f; }), m_ScaredMap.end());

	SteeringPlugin_Output steering{};

	m_pBehaviourTree->Run();

	steering.RunMode = m_Running;
	//Steering
	steering.LinearVelocity = m_pCurrentSteering->UpdateSteering(dt, this, m_pInterface);

	//Orienting
	steering.AutoOrient = m_AutoOrienting;
	float DesiredOrientation = Elite::GetOrientationFromVelocity(m_LookAt -Position);
	steering.AngularVelocity = DesiredOrientation - Orientation;

	for (Vector3& v : m_ScaredMap)
	{
		std::cout << "strength: " << v.z << std::endl;
		v.z -= dt;
	}
	std::cout << m_ScaredMap.size() << std::endl;

	return steering;
}

void AgentModel::SetSteeringBehaviour(BaseSteeringBehaviour* behaviour)
{
	SAFE_DELETE(m_pCurrentSteering);
	m_pCurrentSteering = behaviour;
}

const Elite::Vector2& AgentModel::GetTarget() const
{
	return m_Target;
}

const std::vector<Elite::Vector3>& AgentModel::GetScaredMap() const
{
	return m_ScaredMap;
}

vector<HouseInfo> AgentModel::GetHousesInFOV() const
{
	vector<HouseInfo> vHousesInFOV = {};

	HouseInfo hi = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetHouseByIndex(i, hi))
		{
			vHousesInFOV.push_back(hi);
			continue;
		}

		break;
	}

	return vHousesInFOV;

}

vector<EntityInfo> AgentModel::GetEntitiesInFOV() const
{
	vector<EntityInfo> vEntitiesInFOV = {};

	EntityInfo ei = {};
	for (int i = 0;; ++i)
	{
		if (m_pInterface->Fov_GetEntityByIndex(i, ei))
		{
			vEntitiesInFOV.push_back(ei);
			continue;
		}

		break;
	}

	return vEntitiesInFOV;
}

void AgentModel::ConstructBehaviourTree()
{
	//Construct behaviourTree
	using namespace BehaviourTree;
	m_pBehaviourTree = new Selector
	{ {
			new Sequence
			{ {
			new Conditional([this]()
				{
					return AIPredicates::SeesZombie(std::bind(&AgentModel::GetEntitiesInFOV, this));
				}
			),
			new Action([this]() -> ReturnState
				{
					m_AutoOrienting = true;
					auto entities = GetEntitiesInFOV();

					for (const EntityInfo& entity : entities)
					{
						if (entity.Type == eEntityType::ENEMY)
						{
							//The 6.f is how many seconds the location stays dangerous.
							m_ScaredMap.push_back(Vector3{entity.Location, 6.f});
						}
					}
					return ReturnState::Success;
				}),
		} },
		//Wander
		new PartialSequence(
			{
				new Conditional([this]() -> bool
					{
						return Elite::Distance(m_Target, Position) <= 20;
					}),
				new Action([this]() -> ReturnState
					{
						m_AutoOrienting = false;

						float distance = Elite::randomFloat(100);
						float angle = Elite::randomFloat(2.f * float(M_PI));
						m_LookAt = Vector2(cos(angle) * distance, sin(angle) * distance);

						return ReturnState::Success;
					}),
				new Action([this]() -> ReturnState
					{
						if (Elite::DistanceSquared(m_Target, Position) <= 10)
							return ReturnState::Success;

						return ReturnState::Running;
					}),
				new Action([this]() mutable -> ReturnState
				{
					m_Target = m_LookAt;
					m_AutoOrienting = true;
					m_ScaredMap.push_back(Vector3{ Position, 2.f });
					return ReturnState::Success;
				})
			})
	} };

}
