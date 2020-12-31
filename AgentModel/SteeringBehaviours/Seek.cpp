#include "stdafx.h"
#include "Seek.h"
#include "IExamInterface.h"
#include "..\AgentModel.h"

Elite::Vector2 Seek::UpdateSteering(float dt, AgentModel* pAgent, const IExamInterface* pInterface)
{
    auto dest = pInterface->NavMesh_GetClosestPathPoint(pAgent->Target);

    return Elite::GetNormalized(dest - pAgent->Position) * pAgent->MaxLinearSpeed;
}
