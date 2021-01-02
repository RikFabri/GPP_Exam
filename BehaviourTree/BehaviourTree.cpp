#include "stdafx.h"
#include "BehaviourTree.h"

BehaviourTree::Composite::Composite(std::vector<INode*> NodePtrs)
	: m_NodePointers(NodePtrs)
{
}

BehaviourTree::Composite::~Composite()
{
	for (auto nodePtr : m_NodePointers)
	{
		delete nodePtr;
	}
}

BehaviourTree::Conditional::Conditional(std::function<bool()> predicate)
	: m_Predicate(predicate)
{
}

BehaviourTree::ReturnState BehaviourTree::Conditional::Run(float dt)
{
	return m_Predicate() ? ReturnState::Success : ReturnState::Failed;
}

BehaviourTree::Action::Action(std::function<ReturnState(float)> action)
	: m_Action(action)
{
}

BehaviourTree::ReturnState BehaviourTree::Action::Run(float dt)
{
	return m_Action(dt);
}

BehaviourTree::Sequence::Sequence(std::vector<INode*> NodePtrs)
	: Composite(NodePtrs)
{
}

BehaviourTree::ReturnState BehaviourTree::Sequence::Run(float dt)
{
	for (auto childNodePtr : m_NodePointers)
	{
		ReturnState returnState = childNodePtr->Run(dt);

		switch (returnState)
		{
		case BehaviourTree::ReturnState::Success:
			continue;
		default:
			return returnState;
		}
	}

	return ReturnState::Failed;
}

BehaviourTree::Selector::Selector(std::vector<INode*> NodePtrs)
	: Composite(NodePtrs)
{
}

BehaviourTree::ReturnState BehaviourTree::Selector::Run(float dt)
{
	for (auto childNodePtr : m_NodePointers)
	{
		ReturnState returnState = childNodePtr->Run(dt);

		switch (returnState)
		{
		case BehaviourTree::ReturnState::Failed:
			continue;
		default:
			return returnState;
		}
	}

	return ReturnState::Success;
}

BehaviourTree::PartialSequence::PartialSequence(std::vector<INode*> NodePtrs)
	: Composite(NodePtrs)
	, m_RunningIndex(0)
{
}

BehaviourTree::ReturnState BehaviourTree::PartialSequence::Run(float dt)
{
	for (size_t index = m_RunningIndex; index < m_NodePointers.size(); ++index)
	{
		ReturnState returnState = m_NodePointers[index]->Run(dt);

		switch (returnState)
		{
		case BehaviourTree::ReturnState::Success:
			m_RunningIndex = 0;
			continue;
		case BehaviourTree::ReturnState::Running:
			m_RunningIndex = index;
		default:
			return returnState;
		}
	}

	return ReturnState::Success;
}
