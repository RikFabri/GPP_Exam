#include "stdafx.h"
#include "Plugin.h"
#include "IExamInterface.h"
#include "Predicates\AIPredicates.h"

#include <algorithm>

using namespace Elite;

//Called only once, during initialization
void Plugin::Initialize(IBaseInterface* pInterface, PluginInfo& info)
{
	//Retrieving the interface
	//This interface gives you access to certain actions the AI_Framework can perform for you
	m_pInterface = static_cast<IExamInterface*>(pInterface);

	//Bit information about the plugin
	//Please fill this in!!
	info.BotName = "Gerry";
	info.Student_FirstName = "Rik";
	info.Student_LastName = "Fabri";
	info.Student_Class = "2DAE01";
}

//Called only once
void Plugin::DllInit()
{
	//Called when the plugin is loaded

	m_Target = Elite::Vector2(0, 0);
	m_LookAt = m_Target;
	m_AutoOrient = true;

	//Construct behaviourTree
	using namespace BehaviourTree;
	float angle = 0;
	m_pBehaviourTree = new Selector
	{ {
			new Sequence
			{ {
			new Conditional([this]()
				{
					return AIPredicates::SeesZombie(std::bind(&Plugin::GetEntitiesInFOV, this));
				}
			),
			new Action([this]() -> ReturnState 
				{
					m_AutoOrient = true;
					auto agent = m_pInterface->Agent_GetInfo();
					auto entities = GetEntitiesInFOV();

					for (const EntityInfo& entity : entities)
					{
						if (entity.Type == eEntityType::ENEMY)
						{
							//Vector2 direction{ agent.Position - entity.Location };
							//float magnitude = direction.Normalize();
							//magnitude = 1 / magnitude;
							
							m_ScaredImpulses.push_back(Vector3{entity.Location, 6.f});
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
						std::cout << "doing wander sequence" << std::endl;
						auto agent = m_pInterface->Agent_GetInfo();

						return Elite::Distance(m_Target, agent.Position) <= 20;
					}),
				new Action([this]() -> ReturnState 
					{
						m_AutoOrient = false;

						auto agent = m_pInterface->Agent_GetInfo();
						float distance = Elite::randomFloat(100);
						float angle = Elite::randomFloat(2 * M_PI);
						m_LookAt = Vector2(cos(angle) * distance, sin(angle) * distance);

						return ReturnState::Success;
					}),
				new Action([this]() -> ReturnState
					{
						auto agent = m_pInterface->Agent_GetInfo();

						if (Elite::DistanceSquared(m_Target, agent.Position) <= 10)
							return ReturnState::Success;

						return ReturnState::Running;
					}),
				new Action([this]() mutable -> ReturnState
				{
					auto agent = m_pInterface->Agent_GetInfo();

					m_Target = m_LookAt;
					m_AutoOrient = true;
					m_ScaredImpulses.push_back(Vector3{ agent.Position, 2.f });
					return ReturnState::Success;
				})
			})
	} };
}

//Called only once
void Plugin::DllShutdown()
{
	//Called when the plugin gets unloaded
	SAFE_DELETE(m_pBehaviourTree);
}

//Called only once, during initialization
void Plugin::InitGameDebugParams(GameDebugParams& params)
{
	params.AutoFollowCam = true; //Automatically follow the AI? (Default = true)
	params.RenderUI = true; //Render the IMGUI Panel? (Default = true)
	params.SpawnEnemies = true; //Do you want to spawn enemies? (Default = true)
	params.EnemyCount = 20; //How many enemies? (Default = 20)
	params.GodMode = false; //GodMode > You can't die, can be usefull to inspect certain behaviours (Default = false)
	params.AutoGrabClosestItem = true; //A call to Item_Grab(...) returns the closest item that can be grabbed. (EntityInfo argument is ignored)
}

//Only Active in DEBUG Mode
//(=Use only for Debug Purposes)
void Plugin::Update(float dt)
{
	//Demo Event Code
	//In the end your AI should be able to walk around without external input
	if (m_pInterface->Input_IsMouseButtonUp(Elite::InputMouseButton::eLeft))
	{
		//Update target based on input
		Elite::MouseData mouseData = m_pInterface->Input_GetMouseData(Elite::InputType::eMouseButton, Elite::InputMouseButton::eLeft);
		const Elite::Vector2 pos = Elite::Vector2(static_cast<float>(mouseData.X), static_cast<float>(mouseData.Y));
		m_Target = m_pInterface->Debug_ConvertScreenToWorld(pos);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Space))
	{
		m_CanRun = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Left))
	{
		m_AngSpeed -= Elite::ToRadians(10);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_Right))
	{
		m_AngSpeed += Elite::ToRadians(10);
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_G))
	{
		m_GrabItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_U))
	{
		m_UseItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyDown(Elite::eScancode_R))
	{
		m_RemoveItem = true;
	}
	else if (m_pInterface->Input_IsKeyboardKeyUp(Elite::eScancode_Space))
	{
		m_CanRun = false;
	}
}

//Update
//This function calculates the new SteeringOutput, called once per frame
SteeringPlugin_Output Plugin::UpdateSteering(float dt)
{
	m_pBehaviourTree->Run();

	m_ScaredImpulses.erase(std::remove_if(m_ScaredImpulses.begin(), m_ScaredImpulses.end(), [](const Vector3& v) { return v.z <= 0.f; }), m_ScaredImpulses.end());

	auto steering = SteeringPlugin_Output();

	//Use the Interface (IAssignmentInterface) to 'interface' with the AI_Framework
	auto agentInfo = m_pInterface->Agent_GetInfo();

	auto nextTargetPos = m_Target; //To start you can use the mouse position as guidance
	nextTargetPos = m_pInterface->NavMesh_GetClosestPathPoint(m_Target); 

	auto vHousesInFOV = GetHousesInFOV();//uses m_pInterface->Fov_GetHouseByIndex(...)
	auto vEntitiesInFOV = GetEntitiesInFOV(); //uses m_pInterface->Fov_GetEntityByIndex(...)

	for (auto& e : vEntitiesInFOV)
	{
		if (e.Type == eEntityType::PURGEZONE)
		{
			PurgeZoneInfo zoneInfo;
			m_pInterface->PurgeZone_GetInfo(e, zoneInfo);
			std::cout << "Purge Zone in FOV:" << e.Location.x << ", "<< e.Location.y <<  " ---EntityHash: " << e.EntityHash << "---Radius: "<< zoneInfo.Radius << std::endl;
		}
	}
	

	//INVENTORY USAGE DEMO
	//********************

	if (m_GrabItem)
	{
		ItemInfo item;
		//Item_Grab > When DebugParams.AutoGrabClosestItem is TRUE, the Item_Grab function returns the closest item in range
		//Keep in mind that DebugParams are only used for debugging purposes, by default this flag is FALSE
		//Otherwise, use GetEntitiesInFOV() to retrieve a vector of all entities in the FOV (EntityInfo)
		//Item_Grab gives you the ItemInfo back, based on the passed EntityHash (retrieved by GetEntitiesInFOV)
		if (m_pInterface->Item_Grab({}, item))
		{
			//Once grabbed, you can add it to a specific inventory slot
			//Slot must be empty
			m_pInterface->Inventory_AddItem(0, item);
		}
	}

	if (m_UseItem)
	{
		//Use an item (make sure there is an item at the given inventory slot)
		m_pInterface->Inventory_UseItem(0);
	}

	if (m_RemoveItem)
	{
		//Remove an item from a inventory slot
		m_pInterface->Inventory_RemoveItem(0);
	}

	//Simple Seek Behaviour (towards Target)
	steering.LinearVelocity = nextTargetPos - agentInfo.Position; //Desired Velocity
	steering.LinearVelocity.Normalize(); //Normalize Desired Velocity
	steering.LinearVelocity *= agentInfo.MaxLinearSpeed; //Rescale to Max Speed

	steering.LinearVelocity = { 0, 0 };
	Vector2 scaredVector{ 0, 0 };
	for (Vector3& vector : m_ScaredImpulses)
	{
		Vector2 direction{ agentInfo.Position - Vector2{vector.x, vector.y} };
		float magnitude = direction.Normalize();
		magnitude = 1 / (magnitude * 0.7);
		float strength = vector.z;

		Vector2 addedImpulse = direction * magnitude * strength;
		//auto sumVector = scaredVector + addedImpulse;
		//if (sumVector.SqrtMagnitude() <= 1.2f)
		//	addedImpulse += Vector2{ -addedImpulse.y, addedImpulse.x };

		scaredVector += addedImpulse;
		vector.z -= dt;
		//std::cout << vector.z << std::endl;
	}
	//std::cout << m_ScaredImpulses.size() << std::endl;

	steering.LinearVelocity += Elite::GetNormalized(nextTargetPos - agentInfo.Position);
	steering.LinearVelocity += scaredVector;
	steering.LinearVelocity = Elite::GetNormalized(steering.LinearVelocity) * agentInfo.MaxLinearSpeed;

	//if (Distance(nextTargetPos, agentInfo.Position) < 2.f)
	//{
	//	steering.LinearVelocity = Elite::ZeroVector2;
	//}

	//steering.AngularVelocity = m_AngSpeed; //Rotate your character to inspect the world while walking
	steering.AutoOrient = m_AutoOrient; //Setting AutoOrientate to True overrides the AngularVelocity
	//steering.AngularVelocity = 10;
	float DesiredOrientation = Elite::GetOrientationFromVelocity(m_LookAt - agentInfo.Position);
	float velocity = DesiredOrientation - agentInfo.Orientation;
	steering.AngularVelocity = velocity;

	

	steering.RunMode = m_CanRun; //If RunMode is True > MaxLinSpd is increased for a limited time (till your stamina runs out)

								 //SteeringPlugin_Output is works the exact same way a SteeringBehaviour output

								 //@End (Demo Purposes)
	m_GrabItem = false; //Reset State
	m_UseItem = false;
	m_RemoveItem = false;

	return steering;
}

//This function should only be used for rendering debug elements
void Plugin::Render(float dt) const
{
	//This Render function should only contain calls to Interface->Draw_... functions
	m_pInterface->Draw_SolidCircle(m_Target, .7f, { 0,0 }, { 1, 0, 0 });
}

vector<HouseInfo> Plugin::GetHousesInFOV() const
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

vector<EntityInfo> Plugin::GetEntitiesInFOV() const
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
