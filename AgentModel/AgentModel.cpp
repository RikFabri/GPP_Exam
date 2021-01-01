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
	, m_GunSlot(-1)
{
	SetSteeringBehaviour(new CombinedSteering(
		{
			{new ScaredSteering(), 1.f},
			{new Seek(), 1.f}
		}));
	ConstructBehaviourTree();
}

AgentModel::~AgentModel()
{
	SAFE_DELETE(m_pCurrentSteering);
	SAFE_DELETE(m_pBehaviourTree);
}

SteeringPlugin_Output AgentModel::CalculateSteering(float dt)
{
	Update(dt);

	SteeringPlugin_Output steering{};

	steering.RunMode = m_Running;

	//Steering
	steering.LinearVelocity = m_pCurrentSteering->UpdateSteering(dt, this, m_pInterface);

	//Orienting
	steering.AutoOrient = m_AutoOrienting;
	float DesiredOrientation = Elite::GetOrientationFromVelocity(m_LookAt -Position);
	steering.AngularVelocity = DesiredOrientation - Orientation;

	return steering;
}

void AgentModel::Update(float dt)
{
	AgentInfo::operator=(m_pInterface->Agent_GetInfo());

	int recentScares = std::count_if(m_ScaredMap.begin(), m_ScaredMap.end(), [](const Vector3& v) { return v.z >= 4; });
	m_Running = recentScares > 0;
	m_Running = m_Running || Bitten || WasBitten;

	m_ScaredMap.erase(std::remove_if(m_ScaredMap.begin(), m_ScaredMap.end(), [](const Vector3& v) { return v.z <= 0.f; }), m_ScaredMap.end());

	m_pBehaviourTree->Run();

	for (Vector3& v : m_ScaredMap)
	{
		std::cout << "strength: " << v.z << std::endl;
		v.z -= dt;
	}
	std::cout << m_ScaredMap.size() << std::endl;

	LookForItems();
}

void AgentModel::SetTarget(const Elite::Vector2& target)
{
	m_Target = target;
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

void AgentModel::LookForItems()
{
	auto entities = GetEntitiesInFOV();
	for (auto entity : entities)
	{
		if (entity.Type == eEntityType::ITEM)
		{
			ItemInfo itemInfo;
			m_pInterface->Item_GetInfo(entity, itemInfo);

			//Destroy item if garbage and in range, otherwise ignore it
			if (itemInfo.Type == eItemType::GARBAGE)
				if (!m_pInterface->Item_Destroy(entity))
					continue;

			//Try grabbing item, if not in range set it as target.
			if (!m_pInterface->Item_Grab(entity, itemInfo))
			{
				m_Target = entity.Location;
				break;
			}

			// Try adding item to inventory
			const UINT inventoryCapacity{ m_pInterface->Inventory_GetCapacity() };
			UINT inventorySlot{ 0 };
			bool success = false;
			do
			{
				success = m_pInterface->Inventory_AddItem(inventorySlot++, itemInfo);
			} while (!success && inventorySlot < inventoryCapacity);
		}
	}

	ManageInventory();
}

void AgentModel::ManageInventory()
{
	ItemInfo itemInfo;
	const UINT NrOfInventorySlots{ m_pInterface->Inventory_GetCapacity() };

	for (UINT slot = 0; slot < NrOfInventorySlots; ++slot)
	{
		if (!m_pInterface->Inventory_GetItem(slot, itemInfo))
			continue;

		switch (itemInfo.Type)
		{
		case eItemType::FOOD:
			if (Energy <= 10 - m_pInterface->Food_GetEnergy(itemInfo))
				if (!m_pInterface->Inventory_UseItem(slot))
					m_pInterface->Inventory_RemoveItem(slot);
			continue;
		case eItemType::GARBAGE:
			m_pInterface->Inventory_RemoveItem(slot);
			continue;
		case eItemType::MEDKIT:
			if (Health <= 10 - m_pInterface->Medkit_GetHealth(itemInfo))
				if (!m_pInterface->Inventory_UseItem(slot))
					m_pInterface->Inventory_RemoveItem(slot);
			continue;
		case eItemType::PISTOL:
			m_GunSlot = slot;
			continue;
		default:
			continue;
		}

	}
	
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
							const float lifetime{ 6.f };
							m_ScaredMap.push_back(Vector3{entity.Location, lifetime});
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
