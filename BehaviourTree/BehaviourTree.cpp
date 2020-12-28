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

BehaviourTree::ReturnState BehaviourTree::Conditional::Run()
{
	return m_Predicate() ? ReturnState::Success : ReturnState::Failed;
}

BehaviourTree::Action::Action(std::function<ReturnState()> action)
	: m_Action(action)
{
}

BehaviourTree::ReturnState BehaviourTree::Action::Run()
{
	return m_Action();
}

BehaviourTree::ReturnState BehaviourTree::Sequence::Run()
{
	for (auto childNodePtr : m_NodePointers)
	{
		ReturnState returnState = childNodePtr->Run();

		switch (returnState)
		{
		case BehaviourTree::ReturnState::Success:
			continue;
		default:
			return returnState;
		}
	}
}

BehaviourTree::ReturnState BehaviourTree::Selector::Run()
{
	for (auto childNodePtr : m_NodePointers)
	{
		ReturnState returnState = childNodePtr->Run();

		switch (returnState)
		{
		case BehaviourTree::ReturnState::Failed:
			continue;
		default:
			return returnState;
		}
	}
}
