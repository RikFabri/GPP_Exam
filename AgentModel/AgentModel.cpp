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
	, m_NrOfGuns(0)
	, m_TargetZombie()
	, m_ExplorePathAngles(float(M_PI) / 2.f)
	, m_MaxExploreDistance(200)
	, m_CurrentExploreAngle(float(M_PI) / 4.f)
	, m_CurrentExploreDistance(50)
	, m_ExploreDistanceChanges(10)
	, m_ExplorePathGrowing(true)
	, m_Looting(false)
{
	m_TargetZombie.EnemyHash = -1;

	SetSteeringBehaviour(new CombinedSteering(
		{
			{new ScaredSteering(), 0.5f},
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
	float DesiredOrientation = Elite::GetOrientationFromVelocity(m_LookAt - Position);
	steering.AngularVelocity = DesiredOrientation - Orientation;

	return steering;
}

void AgentModel::Update(float dt)
{
	// Sync model to actual agent
	AgentInfo::operator=(m_pInterface->Agent_GetInfo());

	// Decide on whether to run or not
	int recentScares = std::count_if(m_ScaredMap.begin(), m_ScaredMap.end(), [](const Vector3& v) { return v.z >= 4; });
	m_Running = recentScares > 0;
	m_Running = m_Running || Bitten || WasBitten;

	// Clean up scared map
	m_ScaredMap.erase(std::remove_if(m_ScaredMap.begin(), m_ScaredMap.end(), [](const Vector3& v) { return v.z <= 0.f; }), m_ScaredMap.end());

	// Run behaviourtree
	m_pBehaviourTree->Run(dt);

	// Update scared map
	for (Vector3& v : m_ScaredMap)
		v.z -= dt;

	// Remember zombies
	m_TargetZombie.EnemyHash = -1;
	const auto entities = GetEntitiesInFOV();

	for (const EntityInfo& entity : entities)
	{
		if (entity.Type == eEntityType::ENEMY)
		{
			EnemyInfo enemyInfo;
			m_pInterface->Enemy_GetInfo(entity, enemyInfo);

			if (enemyInfo.Type != eEnemyType::ZOMBIE_NORMAL || m_TargetZombie.EnemyHash == -1)
				m_TargetZombie = enemyInfo;

			//if (Distance(Position, enemyInfo.Location) < 10)
			//	m_Shooting = true;

			const float lifetime{ 6.f };
			m_ScaredMap.push_back(Vector3{ entity.Location, lifetime });
		}
	}

	// Remember houses
	const auto houses = GetHousesInFOV();

	for (const HouseInfo& houseInfo : houses)
	{
		const auto houseIterator = std::find_if(m_RememberedHouses.begin(), m_RememberedHouses.end(), [houseInfo](auto& h) {return h.Center == houseInfo.Center; });
		if (houseIterator == m_RememberedHouses.end())
			m_RememberedHouses.push_back(houseInfo);
	}

	if (m_NrOfGuns <= 0 && m_RememberedHouses.size() > 0 && !m_Looting)
	{
		m_Looting = true;
		m_Target = m_RememberedHouses[std::rand() % m_RememberedHouses.size()].Center;
	}

	if (m_NrOfGuns > 0)
		m_Looting = false;

	LookForItems();

	// Reset flags
	m_Shooting = false;
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
			{
				m_pInterface->Item_Destroy(entity);
				continue;
			}

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

			if (success)
				if (itemInfo.Type == eItemType::PISTOL)
					++m_NrOfGuns;
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
			if (m_Shooting)
			{
				m_Shooting = false;
				if (!m_pInterface->Inventory_UseItem(slot))
				{
					m_pInterface->Inventory_RemoveItem(slot);
					--m_NrOfGuns;
				}
			}
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
	// Construct behaviourTree
	using namespace BehaviourTree;
	m_pBehaviourTree = new Selector
	{ {
			// Defending upon damage
			new PartialSequence
			{{
					new Conditional([this]() -> bool { return (Bitten || WasBitten) && m_NrOfGuns > 0; }),
					// Rotate towards target
					new Action([this](float dt) -> ReturnState
						{
							m_DefenseTimer = 0;
							m_AutoOrienting = false;

							if (m_TargetZombie.EnemyHash != -1)
							{
								m_LookAt = m_TargetZombie.Location;
							}
							else
							{
								m_LookAt = Position - LinearVelocity;
								return ReturnState::Running;
							}

							const float aimOffset{ float(M_PI) / 70.f };
							if (std::abs(GetOrientationFromVelocity(m_LookAt - Position) - Orientation) > aimOffset)
								return ReturnState::Running;

							return ReturnState::Success;
						}),
					new Action([this](float dt)
						{
							m_DefenseTimer += dt;
							if (m_DefenseTimer >= 4)
								return ReturnState::Failed;

							// Keep aim
							m_LookAt = m_TargetZombie.Location;

							if (m_TargetZombie.EnemyHash != -1)
							{
								// Keep aim
								const float aimOffset{ float(M_PI) / 70.f };
								if (std::abs(GetOrientationFromVelocity(m_LookAt - Position) - Orientation) <= aimOffset)
								{
									m_Shooting = true;
									std::cout << "shoot" << std::endl;
									ReturnState::Success;
								}
								return ReturnState::Running;
							}
							else
							{
								m_AutoOrienting = true;
								return ReturnState::Success;
							}

						})
			}},
			new Sequence
			{ {
			new Conditional([this]()
				{
					return AIPredicates::SeesZombie(std::bind(&AgentModel::GetEntitiesInFOV, this));
				}
			),
			new Action([this](float dt) -> ReturnState
				{
					m_AutoOrienting = true;
					return ReturnState::Success;
				})
		} },
		// Go to position, look around when turning
		new PartialSequence(
			{
				new Conditional([this]() -> bool
					{
						return Elite::Distance(m_Target, Position) <= 20;
					}),
				new Action([this](float dt) -> ReturnState
					{
						m_AutoOrienting = false;

						float sign = m_ExplorePathGrowing ? 1 : -1;
						m_CurrentExploreDistance += m_ExploreDistanceChanges * sign;
						m_CurrentExploreAngle += m_ExplorePathAngles;

						if (m_CurrentExploreAngle >= 2.f * float(M_PI))
							m_CurrentExploreAngle -= 2.f * float(M_PI);

						if (m_CurrentExploreDistance > m_MaxExploreDistance || m_CurrentExploreDistance < m_ExploreDistanceChanges)
							m_ExplorePathGrowing = !m_ExplorePathGrowing;
						
						m_LookAt = Vector2(
							cos(m_CurrentExploreAngle) * m_CurrentExploreDistance,
							sin(m_CurrentExploreAngle) * m_CurrentExploreDistance);

						return ReturnState::Success;
					}),
				new Action([this](float dt) -> ReturnState
					{
						if (Elite::DistanceSquared(m_Target, Position) <= 10)
							return ReturnState::Success;

						return ReturnState::Running;
					}),
				new Action([this](float dt) mutable -> ReturnState
				{
					m_Target = m_LookAt;
					m_AutoOrienting = true;
					m_ScaredMap.push_back(Vector3{ Position, 2.f });
					return ReturnState::Success;
				})
			})
	} };
}
