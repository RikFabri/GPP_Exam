#pragma once
#include "Exam_HelperStructs.h"

struct AgentModel;
class IExamInterface;

class SteeringBehaviour
{
public:
	SteeringBehaviour() = default;
	virtual ~SteeringBehaviour() = default;

	virtual Elite::Vector2 UpdateSteering(float dt, AgentModel* pAgent, const IExamInterface* pInterface) = 0;
private:

};

