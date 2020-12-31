#pragma once
#include "Exam_HelperStructs.h"

struct AgentModel;
class IExamInterface;

class BaseSteeringBehaviour
{
public:
	BaseSteeringBehaviour() = default;
	virtual ~BaseSteeringBehaviour() = default;

	virtual Elite::Vector2 UpdateSteering(float dt, AgentModel* pAgent, const IExamInterface* pInterface) = 0;
private:

};

