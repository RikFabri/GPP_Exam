#pragma once
#include "BaseSteeringBehaviour.h"
// ----------------------------- Seek -----------------------------
class Seek :
    public BaseSteeringBehaviour
{
public:
    virtual Elite::Vector2 UpdateSteering(float dt, const AgentModel* pAgent, const IExamInterface* pInterface) override;
};


// ----------------------------- CombinedSteering -----------------------------
class CombinedSteering final
    : public BaseSteeringBehaviour
{
    struct WeightedSteering
    {
        BaseSteeringBehaviour* pSteering;
        float weight;
    };

public:
    CombinedSteering(std::vector<WeightedSteering> steeringBehaviours);

    virtual Elite::Vector2 UpdateSteering(float dt, const AgentModel* pAgent, const IExamInterface* pInterface) override;
private:
    std::vector<WeightedSteering> m_WeightedSteeringBehaviours;
};

// ----------------------------- Scared-avoidance -----------------------------
class ScaredSteering final
    : public BaseSteeringBehaviour
{
public:
    Elite::Vector2 UpdateSteering(float dt, const AgentModel* pAgent, const IExamInterface* pInterface) override;
};