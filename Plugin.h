#pragma once
#include "IExamPlugin.h"
#include "Exam_HelperStructs.h"
#include "BehaviourTree\BehaviourTree.h"
#include "AgentModel\AgentModel.h"

class IBaseInterface;
class IExamInterface;

class Plugin :public IExamPlugin
{
public:
	Plugin() {};
	virtual ~Plugin() {};

	void Initialize(IBaseInterface* pInterface, PluginInfo& info) override;
	void DllInit() override;
	void DllShutdown() override;

	void InitGameDebugParams(GameDebugParams& params) override;
	void Update(float dt) override;

	SteeringPlugin_Output UpdateSteering(float dt) override;
	void Render(float dt) const override;

private:
	AgentModel* m_pAgentModel;

	////Tests
	//Elite::Vector2 m_ArbitraryGoal;
	//bool m_IsInHouse;
	//HouseInfo* m_pLastHouse;
	//std::vector<Elite::Vector3> m_ScaredImpulses;
	//BehaviourTree::INode* m_pBehaviourTree;
	//Elite::Vector2 m_Forward;
	//Elite::Vector2 m_LookAt;
	//std::vector<HouseInfo> m_SpottedHouses;
	//bool m_AutoOrient;

	//Interface, used to request data from/perform actions with the AI Framework
	IExamInterface* m_pInterface = nullptr;
	vector<HouseInfo> GetHousesInFOV() const;
	vector<EntityInfo> GetEntitiesInFOV() const;

	Elite::Vector2 m_Target = {};
	bool m_CanRun = false; //Demo purpose
	bool m_GrabItem = false; //Demo purpose
	bool m_UseItem = false; //Demo purpose
	bool m_RemoveItem = false; //Demo purpose
	float m_AngSpeed = 0.f; //Demo purpose
};

//ENTRY
//This is the first function that is called by the host program
//The plugin returned by this function is also the plugin used by the host program
extern "C"
{
	__declspec (dllexport) IPluginBase* Register()
	{
		return new Plugin();
	}
}