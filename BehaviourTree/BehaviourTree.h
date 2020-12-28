#pragma once

namespace BehaviourTree
{
	// ------------------------ ReturnStates --------------------
	enum class ReturnState
	{
		Success,
		Running,
		Failed
	};

	// -------------------- BaseNode interface ---------------------
	struct INode
	{
		INode() = default;
		virtual ~INode() = default;

		virtual ReturnState Run() = 0;
	};
	// --------------------------------------------------------

	// -------------------- Composite nodes --------------------
	// Base composite node
	class Composite : public INode
	{
	public:
		Composite(std::vector<INode*> NodePtrs);
		virtual ~Composite();

		virtual ReturnState Run() override = 0;
	protected:
		std::vector<INode*> m_NodePointers;
	};

	// Sequence node
	class Sequence final : public Composite
	{
	public:
		ReturnState Run() override;
	private:
	};

	// Selector node
	class Selector final : public Composite
	{
	public:
		ReturnState Run() override;
	};
	// --------------------------------------------------------


	// ---------------------- Leave nodes ----------------------
	// Conditional
	class Conditional final : public INode
	{
	public:
		Conditional(std::function<bool()> predicate);

		ReturnState Run() override;
	private:
		std::function<bool()> m_Predicate;
	};

	// Action
	class Action final : public INode
	{
	public:
		Action(std::function<ReturnState()> action);

		ReturnState Run() override;
	private:
		std::function<ReturnState()> m_Action;
	};
	// --------------------------------------------------------
};