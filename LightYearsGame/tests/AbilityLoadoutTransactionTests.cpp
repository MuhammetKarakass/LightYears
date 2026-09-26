#include "abilities/AbilityCollection.h"
#include "abilities/AbilityDefinition.h"
#include "abilities/AbilityRuntimeSystem.h"

#include <cmath>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ly
{
	namespace
	{
		struct LoadoutTransactionAbility
		{
			sas::AbilityHandle handle;
			sas::AbilityDefinition definition;
			bool active = false;
			float cooldownRemaining = 0.f;

			LoadoutTransactionAbility(
				sas::AbilityHandle abilityHandle,
				const sas::AbilityDefinition& abilityDefinition
			)
				: handle{ abilityHandle }, definition{ abilityDefinition }
			{
			}

			const sas::AbilityDefinition& GetDefinition() const { return definition; }
			void SetRuntimeSlot(sas::AbilitySlot slot) { definition.slot = slot; }
		};

		struct AbilityStateSnapshot
		{
			sas::AbilityHandle handle;
			std::string abilityId;
			sas::AbilitySlot slot = sas::AbilitySlot::None;
			bool active = false;
			float cooldownRemaining = 0.f;
		};

		using Runtime = sas::AbilityRuntimeSystem<
			sas::AbilityDefinition,
			LoadoutTransactionAbility
		>;

		sas::AbilityDefinition MakeDefinition(
			const std::string& abilityId,
			sas::AbilitySlot slot
		)
		{
			sas::AbilityDefinition definition;
			definition.abilityId = abilityId;
			definition.slot = slot;
			definition.cooldown = 18.f;
			definition.duration = 4.f;
			return definition;
		}

		std::vector<AbilityStateSnapshot> Capture(const Runtime& runtime)
		{
			std::vector<AbilityStateSnapshot> snapshot;
			for (const sas::AbilityHandle handle : runtime.GetHandles())
			{
				const LoadoutTransactionAbility* ability = runtime.Find(handle);
				if (ability)
				{
					snapshot.push_back({
						handle,
						ability->definition.abilityId,
						ability->definition.slot,
						ability->active,
						ability->cooldownRemaining
					});
				}
			}
			return snapshot;
		}

		bool SameSnapshot(
			const std::vector<AbilityStateSnapshot>& left,
			const std::vector<AbilityStateSnapshot>& right
		)
		{
			if (left.size() != right.size()) return false;
			for (std::size_t index = 0; index < left.size(); ++index)
			{
				if (!(left[index].handle == right[index].handle) ||
					left[index].abilityId != right[index].abilityId ||
					left[index].slot != right[index].slot ||
					left[index].active != right[index].active ||
					std::fabs(left[index].cooldownRemaining - right[index].cooldownRemaining) > 0.0001f)
				{
					return false;
				}
			}
			return true;
		}

		int Fail(const char* message)
		{
			std::cerr << "[AbilityLoadoutTransactionTests] " << message << std::endl;
			return 1;
		}
	}

	int RunAbilityLoadoutTransactionTests()
	{
		{
			sas::AbilityCollection<LoadoutTransactionAbility> collection;
			const sas::AbilityHandle movingHandle{ 101 };
			const sas::AbilityHandle targetHandle{ 202 };
			const sas::AbilityHandle unexpectedHandle{ 303 };
			const sas::AbilityDefinition movingDefinition = MakeDefinition(
				"Collection.Moving",
				sas::AbilitySlot::Ability1
			);
			const sas::AbilityDefinition targetDefinition = MakeDefinition(
				"Collection.Target",
				sas::AbilitySlot::Ability2
			);
			if (!collection.Register(
				movingHandle,
				movingDefinition.abilityId,
				movingDefinition.slot,
				false,
				std::make_unique<LoadoutTransactionAbility>(movingHandle, movingDefinition)
			) ||
				!collection.Register(
					targetHandle,
					targetDefinition.abilityId,
					targetDefinition.slot,
					false,
					std::make_unique<LoadoutTransactionAbility>(targetHandle, targetDefinition)
				))
			{
				return Fail("Collection transaction fixtures could not register");
			}

			const LoadoutTransactionAbility* movingBefore = collection.Find(movingHandle);
			const LoadoutTransactionAbility* targetBefore = collection.Find(targetHandle);
			std::unique_ptr<LoadoutTransactionAbility> replaced;
			if (collection.CanRebindReplacing(
					movingHandle,
					sas::AbilitySlot::Ability2,
					unexpectedHandle
				) ||
				collection.RebindReplacing(
					movingHandle,
					sas::AbilitySlot::Ability2,
					unexpectedHandle,
					replaced
				) ||
				!(collection.FindHandle(sas::AbilitySlot::Ability1) == movingHandle) ||
				!(collection.FindHandle(sas::AbilitySlot::Ability2) == targetHandle) ||
				collection.Find(movingHandle) != movingBefore ||
				collection.Find(targetHandle) != targetBefore ||
				replaced)
			{
				return Fail("Rejected collection rebind changed the old or target binding");
			}
		}

		std::unordered_map<std::string, std::pair<bool, float>> initialStates{
			{ "Runtime.Moving", { true, 12.5f } },
			{ "Runtime.RebindTarget", { true, 7.25f } },
			{ "Runtime.GrantTarget", { true, 9.5f } }
		};
		std::map<sas::AbilityHandle, std::string> idsByHandle;
		std::vector<std::string> events;
		std::vector<std::string> cancelledIds;
		std::string rejectedValidationId;
		std::string failingCreateId;
		Runtime* runtimeForCallbacks = nullptr;
		bool cancelSawDetachedInstance = true;

		Runtime::Callbacks callbacks;
		callbacks.validate = [&](const sas::AbilityDefinition& definition, std::string* reason)
		{
			if (definition.abilityId == rejectedValidationId)
			{
				if (reason) *reason = "test validation rejection";
				return false;
			}
			return true;
		};
		callbacks.create = [&](
			sas::AbilityHandle handle,
			const sas::AbilityDefinition& definition,
			std::string* reason
		) -> std::unique_ptr<LoadoutTransactionAbility>
		{
			if (definition.abilityId == failingCreateId) return {};
			auto ability = std::make_unique<LoadoutTransactionAbility>(handle, definition);
			const auto state = initialStates.find(definition.abilityId);
			if (state != initialStates.end())
			{
				ability->active = state->second.first;
				ability->cooldownRemaining = state->second.second;
			}
			idsByHandle.emplace(handle, definition.abilityId);
			(void)reason;
			return ability;
		};
		callbacks.cancel = [&](LoadoutTransactionAbility& ability, sas::AbilityEndReason)
		{
			cancelledIds.push_back(ability.definition.abilityId);
			events.push_back("cancel:" + ability.definition.abilityId);
			if (runtimeForCallbacks && runtimeForCallbacks->Find(ability.handle) != nullptr)
			{
				cancelSawDetachedInstance = false;
			}
		};
		callbacks.removed = [&](sas::AbilityHandle handle)
		{
			events.push_back("removed:" + idsByHandle.at(handle));
		};
		callbacks.granted = [&](sas::AbilityHandle handle)
		{
			events.push_back("granted:" + idsByHandle.at(handle));
		};
		callbacks.changed = [&](sas::AbilityHandle handle)
		{
			events.push_back("changed:" + idsByHandle.at(handle));
		};

		Runtime runtime{ 0, std::move(callbacks) };
		runtimeForCallbacks = &runtime;
		const sas::AbilityHandle movingHandle = runtime.GrantAbility(
			MakeDefinition("Runtime.Moving", sas::AbilitySlot::Ability1)
		);
		const sas::AbilityHandle rebindTargetHandle = runtime.GrantAbility(
			MakeDefinition("Runtime.RebindTarget", sas::AbilitySlot::Ability2)
		);
		const sas::AbilityHandle grantTargetHandle = runtime.GrantAbility(
			MakeDefinition("Runtime.GrantTarget", sas::AbilitySlot::Ability3)
		);
		if (!movingHandle.IsValid() || !rebindTargetHandle.IsValid() || !grantTargetHandle.IsValid())
		{
			return Fail("Runtime transaction fixtures could not be granted");
		}
		events.clear();
		cancelledIds.clear();

		rejectedValidationId = "Runtime.Moving";
		const auto beforeValidationReject = Capture(runtime);
		std::string failureReason;
		if (runtime.RebindAbility(
				movingHandle,
				sas::AbilityRuntimeBinding{ sas::AbilitySlot::Ability2 },
				&failureReason
			) ||
			failureReason != "test validation rejection" ||
			!SameSnapshot(beforeValidationReject, Capture(runtime)) ||
			!events.empty() || !cancelledIds.empty())
		{
			return Fail("Validation rejection changed a target, handle, active flag, cooldown, or callback trace");
		}
		if (runtime.RebindAbility(
				movingHandle,
				sas::AbilityRuntimeBinding{ sas::AbilitySlot::Ability2 },
				nullptr
			) ||
			!SameSnapshot(beforeValidationReject, Capture(runtime)) ||
			!events.empty() || !cancelledIds.empty())
		{
			return Fail("Validation rejection with a null failure reason changed runtime state");
		}
		rejectedValidationId.clear();

		if (!runtime.RebindAbility(
			movingHandle,
			sas::AbilityRuntimeBinding{ sas::AbilitySlot::Ability2 },
			&failureReason
		))
		{
			return Fail("Valid ability rebind to an occupied slot was rejected");
		}
		const LoadoutTransactionAbility* moved = runtime.Find(movingHandle);
		if (!moved || moved->GetDefinition().slot != sas::AbilitySlot::Ability2 ||
			!moved->active || std::fabs(moved->cooldownRemaining - 12.5f) > 0.0001f ||
			runtime.Find(rebindTargetHandle) ||
			runtime.Find(sas::AbilitySlot::Ability1) ||
			runtime.Find(sas::AbilitySlot::Ability2) != moved ||
			cancelledIds != std::vector<std::string>{ "Runtime.RebindTarget" } ||
			!cancelSawDetachedInstance ||
			events != std::vector<std::string>{
				"cancel:Runtime.RebindTarget",
				"removed:Runtime.RebindTarget",
				"changed:Runtime.RebindTarget",
				"changed:Runtime.Moving"
			})
		{
			return Fail("Successful rebind lost the moving instance or notified the wrong occupant/order");
		}

		events.clear();
		cancelledIds.clear();
		const auto beforeGrantFailure = Capture(runtime);
		failingCreateId = "Runtime.GrantFailure";
		const sas::AbilityHandle failedGrant = runtime.GrantAbility(
			MakeDefinition("Runtime.GrantFailure", sas::AbilitySlot::Ability3),
			&failureReason
		);
		if (failedGrant.IsValid() || failureReason.empty() ||
			!SameSnapshot(beforeGrantFailure, Capture(runtime)) ||
			!events.empty() || !cancelledIds.empty() ||
			runtime.Find(grantTargetHandle) == nullptr)
		{
			return Fail("Failed grant removed or changed the occupied target before registration");
		}
		const sas::AbilityHandle nullReasonGrant = runtime.GrantAbility(
			MakeDefinition("Runtime.GrantFailure", sas::AbilitySlot::Ability3),
			nullptr
		);
		if (nullReasonGrant.IsValid() || !SameSnapshot(beforeGrantFailure, Capture(runtime)) ||
			!events.empty() || !cancelledIds.empty())
		{
			return Fail("Failed grant with a null failure reason changed runtime state");
		}
		failingCreateId.clear();

		const sas::AbilityHandle granted = runtime.GrantAbility(
			MakeDefinition("Runtime.GrantSuccess", sas::AbilitySlot::Ability3),
			&failureReason
		);
		const LoadoutTransactionAbility* incoming = runtime.Find(granted);
		if (!granted.IsValid() || !incoming ||
			runtime.Find(grantTargetHandle) ||
			runtime.Find(sas::AbilitySlot::Ability3) != incoming ||
			cancelledIds != std::vector<std::string>{ "Runtime.GrantTarget" } ||
			!cancelSawDetachedInstance ||
			events != std::vector<std::string>{
				"cancel:Runtime.GrantTarget",
				"removed:Runtime.GrantTarget",
				"changed:Runtime.GrantTarget",
				"granted:Runtime.GrantSuccess",
				"changed:Runtime.GrantSuccess"
			})
		{
			return Fail("Successful grant did not replace and cancel only its target occupant");
		}

		events.clear();
		cancelledIds.clear();
		const sas::AbilityHandle emptySlotGrant = runtime.GrantAbility(
			MakeDefinition("Runtime.EmptySlot", sas::AbilitySlot::Ability4)
		);
		if (!emptySlotGrant.IsValid() || !cancelledIds.empty() ||
			events != std::vector<std::string>{
				"granted:Runtime.EmptySlot",
				"changed:Runtime.EmptySlot"
			})
		{
			return Fail("Grant into an empty slot emitted replacement cancellation callbacks");
		}
		return 0;
	}
}
