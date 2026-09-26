#pragma once

#include "abilities/AbilityPolicies.h"

#include <algorithm>
#include <exception>
#include <utility>
#include <vector>

namespace sas
{
	template <typename Spec, typename RuntimeState>
	struct ActiveAbilityAction
	{
		const Spec* spec = nullptr;
		RuntimeState runtimeState;
		bool endActiveCompleted = false;
	};

	template <typename ActiveAction>
	struct AbilityExecution
	{
		using ActionType = ActiveAction;
		std::vector<ActiveAction> actions;
		bool started = false;
		bool endActionsDispatched = false;
	};

	class AbilityExecutionLifecycle
	{
	public:
		template <typename Execution, typename Specs, typename Continue, typename Execute>
		static void Begin(
			Execution& execution,
			const Specs& specs,
			Continue&& shouldContinue,
			Execute&& execute
		)
		{
			execution.actions.clear();
			execution.started = false;
			execution.endActionsDispatched = false;
			AddPhaseActions(execution, specs, AbilityActionPhase::OnActivate);
			AddPhaseActions(execution, specs, AbilityActionPhase::WhileActive);
			execution.started = true;
			for (auto& action : execution.actions)
			{
				if (!shouldContinue()) return;
				if (action.spec &&
					action.spec->phase == AbilityActionPhase::OnActivate)
				{
					execute(action);
				}
			}
		}

		template <typename Execution, typename Specs, typename Execute>
		static void Begin(
			Execution& execution,
			const Specs& specs,
			Execute&& execute
		)
		{
			Begin(execution, specs, [] { return true; }, std::forward<Execute>(execute));
		}

		template <typename Execution, typename Continue, typename Tick>
		static void Tick(
			Execution& execution,
			float deltaTime,
			Continue&& shouldContinue,
			Tick&& tick
		)
		{
			for (auto& action : execution.actions)
			{
				if (!shouldContinue()) return;
				tick(action, deltaTime);
			}
		}

		template <typename Execution, typename TickAction>
		static void Tick(
			Execution& execution,
			float deltaTime,
			TickAction&& tick
		)
		{
			Tick(execution, deltaTime, [] { return true; }, std::forward<TickAction>(tick));
		}

		template <
			typename Execution,
			typename Specs,
			typename Continue,
			typename EndActive,
			typename Execute
		>
		static void End(
			Execution& execution,
			const Specs& specs,
			Continue&& shouldContinue,
			EndActive&& endActive,
			Execute&& execute
		)
		{
			(void)shouldContinue;
			if (!execution.started)
			{
				execution.actions.clear();
				execution.endActionsDispatched = false;
				return;
			}

			std::exception_ptr error;
			for (auto& action : execution.actions)
			{
				if (action.endActiveCompleted) continue;
				try
				{
					endActive(action);
					action.endActiveCompleted = true;
				}
				catch (...)
				{
					if (!error) error = std::current_exception();
				}
			}

			if (!execution.endActionsDispatched)
			{
				// OnEnd is a one-shot lifecycle notification. Mark it before
				// callbacks so cleanup retries cannot dispatch it twice.
				execution.endActionsDispatched = true;
				for (const auto& spec : specs)
				{
					if (spec.phase != AbilityActionPhase::OnEnd) continue;
					typename Execution::ActionType action;
					action.spec = &spec;
					try { execute(action); }
					catch (...) { if (!error) error = std::current_exception(); }
				}
			}

			const bool cleanupComplete = std::all_of(
				execution.actions.begin(),
				execution.actions.end(),
				[](const auto& action) { return action.endActiveCompleted; }
			);
			if (cleanupComplete)
			{
				execution.actions.clear();
				execution.started = false;
			}
			if (error) std::rethrow_exception(error);
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
			End(execution, specs, [] { return true; }, std::forward<EndActive>(endActive), std::forward<Execute>(execute));
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
