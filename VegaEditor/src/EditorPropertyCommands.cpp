#include "editor/EditorCommands.h"
#include "EditorCommandsInternal.h"
#include <framework/scene/SceneSettings.h>
#include <framework/World.h>
#include <script/ScriptComponent.h>
#include <cmath>
#include <unordered_set>

namespace vega
{
	using editor_commands_internal::HasSelectedAncestor;

	bool SetSceneSettingCommand::Execute(EditorContext& context)
	{
		if (!context.sceneSettings || context.playMode || context.transitioning) return false;
		context.settingErrorKey = mKey;
		auto& settings = *context.sceneSettings;
		std::optional<nlohmann::json> before;
		if (settings.contains(mKey)) before = settings.at(mKey);
		if (before == mAfter) return false;
		const auto* field = context.Engine().SceneSettings().Find(mKey);
		if (!before && mAfter && field && *mAfter == field->defaultValue) return false;
		if (!context.Engine().SceneSettings().Set(settings, mKey, mAfter, context.settingError)) return false;
		try
		{
			if (context.applySettings) context.applySettings(settings);
		}
		catch (const std::exception& error)
		{
			if (before) settings[mKey] = *before;
			else settings.erase(mKey);
			context.settingError = error.what();
			return false;
		}
		if (!mExecuted) { mBefore = std::move(before); mExecuted = true; }
		return true;
	}
	void SetSceneSettingCommand::Undo(EditorContext& context)
	{
		if (!context.sceneSettings || !mExecuted) return;
		// Restore raw authored data, including a previously invalid value.
		auto& settings = *context.sceneSettings;
		if (mBefore) settings[mKey] = *mBefore;
		else settings.erase(mKey);
		context.settingError.clear();
		if (context.applySettings) context.applySettings(settings);
	}
	bool SetSceneSettingCommand::MergeWith(const EditorCommand& command)
	{
		const auto* next = dynamic_cast<const SetSceneSettingCommand*>(&command);
		if (!next || !mGroup || next->mGroup != mGroup || next->mKey != mKey) return false;
		mAfter = next->mAfter;
		return true;
	}

	SetScriptPropertyCommand::SetScriptPropertyCommand(unsigned int id, ly::ComponentIdentity index, std::string property,
		ly::reflect::PropertyValue value, std::uint64_t group)
		: mID{ id }, mComponentIdentity{ index }, mProperty{ std::move(property) }, mAfter{ std::move(value) }, mMergeGroup{ group } {}

	bool SetScriptPropertyCommand::Execute(EditorContext& context)
	{
		const auto actor = context.Resolve(mID, mGuid);
		if (!actor) return false;
		const auto target = ResolvePropertyTarget(*actor, mComponentIdentity);
		auto* script = dynamic_cast<script::ScriptComponent*>(static_cast<ly::ActorComponent*>(target.object));
		if (!script) return false;
		std::optional<ly::reflect::PropertyValue> before;
		const auto entry = script->Overrides().find(mProperty);
		if (entry != script->Overrides().end()) before = entry->second;
		if (!script->SetProperty(mProperty, mAfter)) return false;
		if (!mExecuted) { mBeforeOverride = before; mExecuted = true; }
		return true;
	}
	void SetScriptPropertyCommand::Undo(EditorContext& context)
	{
		const auto actor = context.Resolve(mID, mGuid);
		if (!actor || !mExecuted) return;
		const auto target = ResolvePropertyTarget(*actor, mComponentIdentity);
		if (auto* script = dynamic_cast<script::ScriptComponent*>(static_cast<ly::ActorComponent*>(target.object)))
		{
			if (mBeforeOverride) script->SetProperty(mProperty, *mBeforeOverride);
			else script->ClearOverride(mProperty);
		}
	}
	bool SetScriptPropertyCommand::MergeWith(const EditorCommand& command)
	{
		const auto* next = dynamic_cast<const SetScriptPropertyCommand*>(&command);
		if (!next || !mMergeGroup || mMergeGroup != next->mMergeGroup || mID != next->mID ||
			mComponentIdentity != next->mComponentIdentity || mProperty != next->mProperty) return false;
		mAfter = next->mAfter;
		return true;
	}

	ReflectedTarget ResolvePropertyTarget(ly::Actor& actor, std::optional<ly::ComponentIdentity> componentIdentity)
	{
		auto& registry = actor.GetEngineContext().Types();
		if (!componentIdentity) return { &actor, registry.FindMostDerived(actor), {} };
		auto* component = ly::FindComponentByTypeOccurrence(actor, *componentIdentity);
		return component ? ReflectedTarget{ component, registry.FindMostDerived(*component), componentIdentity } : ReflectedTarget{};
	}

	ReflectedTarget FindPropertyTarget(ly::Actor& actor, const std::string& targetName)
	{
		auto result = ResolvePropertyTarget(actor, {});
		if (targetName == "Actor") return result;
		result = {};
		
		actor.ForEachComponent([&](ly::ActorComponent& component)
		{
			const auto* type = component.GetOwner().GetEngineContext().Types().FindMostDerived(component);
			for (const auto* base = type; base; base = base->base)
			{
				if (!result.object && base->name == targetName) result = { &component, type, ly::IdentifyComponent(actor, component) };
			}

		});
		return result;
	}

	SetPropertyCommand::SetPropertyCommand(unsigned int id, std::optional<ly::ComponentIdentity> componentIdentity,
		std::string property, ly::reflect::PropertyValue after, std::uint64_t group, bool relative)
		: mID{ id }, mComponentIdentity{ componentIdentity }, mProperty{ std::move(property) },
		mAfter{ std::move(after) }, mMergeGroup{ group }, mRelative{ relative }
	{
	}

	bool SetPropertyCommand::Execute(EditorContext& context)
	{
		context.propertyError.clear();
		context.propertyErrorActor = mID;
		context.propertyErrorComponent = mComponentIdentity;
		context.propertyErrorName = mProperty;
		const auto actor = context.Resolve(mID, mGuid);
		if (!actor) { context.propertyError = "Property target is unavailable"; return false; }
		const auto target = ResolvePropertyTarget(*actor, mComponentIdentity);
		const auto* property = target.type ? target.type->FindProperty(mProperty) : nullptr;
		if (!property) { context.propertyError = "Unknown property"; return false; }
		const auto before = mRelative && !mComponentIdentity && mProperty == "Location" ? ly::reflect::PropertyValue{ actor->GetRelativeLocation() } :
			mRelative && !mComponentIdentity && mProperty == "Rotation" ? ly::reflect::PropertyValue{ actor->GetRelativeRotation() } : property->getter(target.object);
		if (mRelative && !mComponentIdentity && mProperty == "Location")
		{
			const auto* value = std::get_if<sf::Vector2f>(&mAfter);
			if (!value || !std::isfinite(value->x) || !std::isfinite(value->y)) return false;
			actor->SetRelativeLocation(*value);
		}
		else if (mRelative && !mComponentIdentity && mProperty == "Rotation")
		{
			const auto* value = std::get_if<float>(&mAfter);
			if (!value || !std::isfinite(*value)) return false;
			actor->SetRelativeRotation(*value);
		}
		else if (!property->SetValue(target.object, mAfter, &context.propertyError)) return false;
		if (!mExecuted) { mBefore = before; mActorName = ActorTypeName(*actor); mExecuted = true; }
		return true;
	}

	void SetPropertyCommand::Undo(EditorContext& context)
	{
		const auto actor = context.Resolve(mID, mGuid);
		if (!actor || !mExecuted) return;
		const auto target = ResolvePropertyTarget(*actor, mComponentIdentity);
		if (const auto* property = target.type ? target.type->FindProperty(mProperty) : nullptr)
		{
			context.propertyErrorActor = mID;
			context.propertyErrorComponent = mComponentIdentity;
			context.propertyErrorName = mProperty;
			if (mRelative && !mComponentIdentity && mProperty == "Location") actor->SetRelativeLocation(std::get<sf::Vector2f>(mBefore));
			else if (mRelative && !mComponentIdentity && mProperty == "Rotation") actor->SetRelativeRotation(std::get<float>(mBefore));
			else property->SetValue(target.object, mBefore, &context.propertyError);
		}
	}

	std::string SetPropertyCommand::Name() const
	{
		if (!mComponentIdentity && (mProperty == "Location" || mProperty == "Rotation"))
			return std::string{ mProperty == "Location" ? "Move " : "Rotate " } + mActorName + " #" + std::to_string(mID);
		return "Set " + mProperty + " #" + std::to_string(mID) +
			(mComponentIdentity ? " component " + mComponentIdentity->typeName + "[" + std::to_string(mComponentIdentity->occurrence) + "]" : "");
	}

	bool SetPropertyCommand::MergeWith(const EditorCommand& command)
	{
		const auto* next = dynamic_cast<const SetPropertyCommand*>(&command);
		if (!next || !mMergeGroup || mMergeGroup != next->mMergeGroup || mID != next->mID || mRelative != next->mRelative ||
			mComponentIdentity != next->mComponentIdentity || mProperty != next->mProperty) return false;
		mAfter = next->mAfter;
		return true;
	}

	std::vector<SelectionTransform> CaptureSelectionTransform(const EditorSelection& selection)
	{
		std::vector<SelectionTransform> result;
		for (const auto& actor : selection.GetAll())
			if (!HasSelectedAncestor(selection, *actor)) result.push_back({ actor->GetUniqueID(), actor->GetActorLocation(), actor->GetActorRotation() });
		return result;
	}

	bool TransformSelection(EditorContext& context, CommandHistory& history, const std::vector<SelectionTransform>& starts,
		sf::Vector2f pivot, sf::Vector2f delta, std::optional<float> angle, std::uint64_t group)
	{
		auto command = std::make_unique<CompoundCommand>(angle ? "Rotate selection" : "Move selection", group);
		const float radians = angle.value_or(0.f) * 3.141592654f / 180.f;
		bool changed = false;
		for (const auto& start : starts)
		{
			const auto actor = context.Resolve(start.id);
			if (!actor || !context.selection.Contains(actor)) return false;
			const auto offset = start.location - pivot;
			const sf::Vector2f location = pivot + delta + sf::Vector2f{
				offset.x * std::cos(radians) - offset.y * std::sin(radians),
				offset.x * std::sin(radians) + offset.y * std::cos(radians) };
			changed = changed || location != actor->GetActorLocation() || (angle && start.rotation + *angle != actor->GetActorRotation());
			command->Add(std::make_unique<SetPropertyCommand>(start.id, std::nullopt, "Location", location, group));
			if (angle) command->Add(std::make_unique<SetPropertyCommand>(start.id, std::nullopt, "Rotation", start.rotation + *angle, group));
		}
		return changed && history.Execute(std::move(command));
	}

	const std::vector<CommonProperty>& MultiInspectorCache::Get(EditorContext& context)
	{
		const auto world = context.world ? std::optional<unsigned int>{ context.world->GetUniqueID() } : std::nullopt;
		if (mWorld == world && mSceneRevision == context.sceneRevision && mSelectionRevision == context.selection.Revision()) return mProperties;
		mWorld = world;
		mSceneRevision = context.sceneRevision;
		mSelectionRevision = context.selection.Revision();
		mProperties.clear();
		const auto actors = context.selection.GetAll();
		if (actors.empty()) return mProperties;
		const auto collect = [&](std::vector<ReflectedTarget> targets)
		{
			const auto* common = targets.front().type;
			for (const auto& target : targets)
				while (common && (!target.object || !common->isInstance(target.object))) common = common->base;
			if (!common) return;
			for (const auto* property : common->AllProperties())
			{
				// A derived descriptor shadows a property of the same name on its base.
				if (common->FindProperty(property->name) != property) continue;
				CommonProperty row{ common->name, property, {}, property->getter(targets.front().object), false };
				for (const auto& target : targets)
				{
					row.targets.push_back(target.componentIdentity);
					row.mixed = row.mixed || property->getter(target.object) != row.value;
				}
				mProperties.push_back(std::move(row));
			}
		};
		std::vector<ReflectedTarget> roots;
		for (const auto& actor : actors) roots.push_back(ResolvePropertyTarget(*actor, {}));
		collect(std::move(roots));
		std::unordered_set<std::string> componentSets;
		actors.front()->ForEachComponent([&](ly::ActorComponent& component)
		{
			const auto* type = component.GetOwner().GetEngineContext().Types().FindMostDerived(component);
			// Match occurrences within a reflected component chain. Unrelated sibling
			// components do not become common merely by inheriting ActorComponent.
			for (const auto* base = type; base && (base == type || base->name != "ActorComponent"); base = base->base)
			{
				std::size_t occurrence = 0;
				bool reached = false;
				actors.front()->ForEachComponent([&](ly::ActorComponent& entry)
				{
					if (&entry == &component) reached = true;
					if (!reached && base->isInstance(&entry)) ++occurrence;
				});
				std::vector<ReflectedTarget> targets;
				std::string key;
				for (const auto& actor : actors)
				{
					std::size_t index = 0;
					ReflectedTarget target;
					actor->ForEachComponent([&](ly::ActorComponent& entry)
					{
						if (base->isInstance(&entry) && index++ == occurrence)
							target = { &entry, base, ly::IdentifyComponent(*actor, entry) };
					});
					if (!target.object || !target.componentIdentity) break;
					key += target.componentIdentity->typeName + ":" + std::to_string(target.componentIdentity->occurrence) + ";";
					targets.push_back(std::move(target));
				}
				if (targets.size() == actors.size())
				{
					if (componentSets.insert(key).second) collect(std::move(targets));
					break;
				}
			}
		});
		return mProperties;
	}

	bool SetCommonProperty(EditorContext& context, CommandHistory& history, const CommonProperty& row,
		const ly::reflect::PropertyValue& value, std::uint64_t group)
	{
		const auto actors = context.selection.GetAll();
		if (!row.property || !row.property->setter || actors.size() != row.targets.size()) return false;
		auto command = std::make_unique<CompoundCommand>("Set selection " + row.section + "/" + row.property->name, group);
		for (std::size_t i = 0; i < actors.size(); ++i)
			command->Add(std::make_unique<SetPropertyCommand>(actors[i]->GetUniqueID(), row.targets[i], row.property->name, value, group));
		return history.Execute(std::move(command));
	}
}
