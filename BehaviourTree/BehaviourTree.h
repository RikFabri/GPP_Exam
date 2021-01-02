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

		virtual ReturnState Run(float dt) = 0;
	};
	// --------------------------------------------------------

	// -------------------- Composite nodes --------------------
	// Base composite node
	class Composite : public INode
	{
	public:
		Composite(std::vector<INode*> NodePtrs);
		virtual ~Composite();

		virtual ReturnState Run(float dt) override = 0;
	protected:
		std::vector<INode*> m_NodePointers;
	};

	// Sequence node
	class Sequence final : public Composite
	{
	public:
		Sequence(std::vector<INode*> NodePtrs);

		ReturnState Run(float dt) override;
	private:
	};

	// Partial sequence node
	class PartialSequence final : public Composite
	{
	public:
		PartialSequence(std::vector<INode*> NodePtrs);

		ReturnState Run(float dt) override;
	private:
		size_t m_RunningIndex;
	};

	// Selector node
	class Selector final : public Composite
	{
	public:
		Selector(std::vector<INode*> NodePtrs);

		ReturnState Run(float dt) override;
	};
	// --------------------------------------------------------


	// ---------------------- Leave nodes ----------------------
	// Conditional
	class Conditional final : public INode
	{
	public:
		Conditional(std::function<bool()> predicate);

		ReturnState Run(float dt) override;
	private:
		std::function<bool()> m_Predicate;
	};

	// Action
	class Action final : public INode
	{
	public:
		Action(std::function<ReturnState(float)> action);

		ReturnState Run(float dt) override;
	private:
		std::function<ReturnState(float)> m_Action;
	};
	// --------------------------------------------------------
};