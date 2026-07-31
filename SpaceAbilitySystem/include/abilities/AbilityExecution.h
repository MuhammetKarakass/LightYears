#pragma once

#include "abilities/AbilityPolicies.h"

#include <utility>
#include <vector>

namespace sas
{
	template <typename Spec, typename RuntimeState>
	struct ActiveAbilityAction
	{
		const Spec* spec = nullptr;
		RuntimeState runtimeState;
	};

	template <typename ActiveAction>
	struct AbilityExecution
	{
		using ActionType = ActiveAction;
		std::vector<ActiveAction> actions;
	};

	class AbilityExecutionLifecycle
	{
	public:
		template <typename Execution, typename Specs, typename Execute>
		static void Begin(
			Execution& execution,
			const Specs& specs,
			Execute&& execute
		)
		{
			execution.actions.clear();
			AddPhaseActions(execution, specs, AbilityActionPhase::OnActivate);
			AddPhaseActions(execution, specs, AbilityActionPhase::WhileActive);
			for (auto& action : execution.actions)
			{
				if (action.spec &&
					action.spec->phase == AbilityActionPhase::OnActivate)
				{
					execute(action);
				}
			}
		}

		template <typename Execution, typename Tick>
		static void Tick(
			Execution& execution,
			float deltaTime,
			Tick&& tick
		)
		{
			for (auto& action : execution.actions)
			{
				tick(action, deltaTime);
			}
		}

		template <
			typename Execution,
			typename Specs,
			typename EndActive,
			typename Execute
		>
		static void End(
			Execution& execution,
			const Specs& specs,
			EndActive&& endActive,
			Execute&& execute
		)
		{
			for (auto& action : execution.actions)
			{
				endActive(action);
			}

			Execution endExecution;
			AddPhaseActions(endExecution, specs, AbilityActionPhase::OnEnd);
			for (auto& action : endExecution.actions)
			{
				execute(action);
			}
			execution.actions.clear();
		}

	private:
		template <typename Execution, typename Specs>
		static void AddPhaseActions(
			Execution& execution,
			const Specs& specs,
			AbilityActionPhase phase
		)
		{
			for (const auto& spec : specs)
			{
				if (spec.phase == phase)
				{
					typename Execution::ActionType action;
					action.spec = &spec;
					execution.actions.push_back(std::move(action));
				}
			}
		}
	};
}
