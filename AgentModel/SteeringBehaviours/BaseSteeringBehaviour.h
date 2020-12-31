#pragma once
#include "Exam_HelperStructs.h"

class AgentModel;
class IExamInterface;

class BaseSteeringBehaviour
{
public:
	BaseSteeringBehaviour() = default;
	virtual ~BaseSteeringBehaviour() = default;

	virtual Elite::Vector2 UpdateSteering(float dt, const AgentModel* pAgent, const IExamInterface* pInterface) = 0;
private:

};

