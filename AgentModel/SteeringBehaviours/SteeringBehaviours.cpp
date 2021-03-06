#include "stdafx.h"
#include "SteeringBehaviours.h"
#include "IExamInterface.h"
#include "..\AgentModel.h"
#include <numeric>
#include "..\inc\EliteMath\EMath.h"

using namespace Elite;

#pragma region Constructors
CombinedSteering::CombinedSteering(std::vector<WeightedSteering> steeringBehaviours)
    : m_WeightedSteeringBehaviours(steeringBehaviours)
{
}
#pragma endregion

#pragma region Steering Behaviours
Vector2 Seek::UpdateSteering(float dt, const AgentModel* pAgent, const IExamInterface* pInterface)
{
    auto dest = pInterface->NavMesh_GetClosestPathPoint(pAgent->GetTarget());

    return GetNormalized(dest - pAgent->Position) *pAgent->MaxLinearSpeed;
}

Vector2 CombinedSteering::UpdateSteering(float dt, const AgentModel* pAgent, const IExamInterface* pInterface)
{
    Vector2 steering{ 0, 0 };

    steering = std::accumulate(m_WeightedSteeringBehaviours.begin(), m_WeightedSteeringBehaviours.end(), steering,
        [&](Vector2& v, const WeightedSteering& ws) { return v + ws.pSteering->UpdateSteering(dt, pAgent, pInterface) * ws.weight; });

    return GetNormalized(steering) * pAgent->MaxLinearSpeed;
}

Vector2 ScaredSteering::UpdateSteering(float dt, const AgentModel* pAgent, const IExamInterface* pInterface)
{
	Vector2 scaredVector{ 0, 0 };

	for (const Vector3& vector : pAgent->GetScaredMap())
	{
		Vector2 direction{ pAgent->Position - Vector2{vector.x, vector.y} };
		float magnitude = direction.Normalize();
		magnitude = 1.f / float(magnitude * 0.7f);
		float strength = vector.z;

		Vector2 addedImpulse = direction * magnitude * strength;

		scaredVector += addedImpulse;
	}
	return scaredVector * pAgent->MaxLinearSpeed;
}
#pragma endregion

