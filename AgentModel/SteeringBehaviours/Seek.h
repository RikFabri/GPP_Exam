#pragma once
#include "SteeringBehaviour.h"

class Seek :
    public SteeringBehaviour
{
public:

    virtual Elite::Vector2 UpdateSteering(float dt, AgentModel* pAgent, const IExamInterface* pInterface) override;

private:

};

