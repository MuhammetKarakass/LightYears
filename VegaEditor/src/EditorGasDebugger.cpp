#include "editor/EditorGasDebugger.h"
#include "editor/EditorCommands.h"
#include "editor/EditorScript.h"
#include <framework/World.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cstring>
#include <limits>

namespace vega
{
	void EditorGasDebugger::InstallSettings()
	{
		ImGuiSettingsHandler handler;
		handler.TypeName = "VegaGasDebugger";
		handler.TypeHash = ImHashStr(handler.TypeName);
		handler.UserData = this;
		handler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler* h, const char*) -> void* { return h->UserData; };
		handler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler*, void* entry, const char* line)
		{
			auto& self = *static_cast<EditorGasDebugger*>(entry);
			int visible = 0, flags = 31, refresh = 100;
			if (std::sscanf(line, "State=%d,%d,%d", &visible, &flags, &refresh) == 3)
			{
				self.visible = visible != 0;
				for (int i = 0; i < 5; ++i) self.mOpen[i] = (flags & (1 << i)) != 0;
				self.mRefreshMs = std::clamp(refresh, 20, 2000);
			}
		};
		handler.WriteAllFn = [](ImGuiContext*, ImGuiSettingsHandler* h, ImGuiTextBuffer* out)
		{
			const auto& self = *static_cast<EditorGasDebugger*>(h->UserData);
			int flags = 0;
			for (int i = 0; i < 5; ++i) if (self.mOpen[i]) flags |= 1 << i;
			out->appendf("[VegaGasDebugger][Panel]\nState=%d,%d,%d\n\n", self.visible, flags, self.mRefreshMs);
		};
		ImGui::AddSettingsHandler(&handler);
	}

	void EditorGasDebugger::Clear()
	{
		mSnapshot = {};
		mTarget.reset();
		mHistory.clear();
		mTags = {};
		mPeriods.clear();
		mDefinitions.clear();
		mStatus.clear();
		mNextRefresh = 0;
	}

	std::shared_ptr<ly::Actor> EditorGasDebugger::Target(EditorContext& context) const
	{
		if (!context.world || !context.playMode || context.transitioning) return {};
		auto actor = context.selection.Get();
		if (actor && actor->GetWorld() != context.world) actor.reset();
		if (!actor && sas::AbilitySystemDebugView::player) actor = sas::AbilitySystemDebugView::player(*context.world);
		return actor;
	}

	bool EditorGasDebugger::Section(int index, const char* name)
	{
		ImGui::SetNextItemOpen(mOpen[index]);
		const bool open = ImGui::CollapsingHeader(name);
		if (open != mOpen[index]) { mOpen[index] = open; ImGui::MarkIniSettingsDirty(); }
		if (open) { ImGui::PushID(index); mFilters[index].Draw("Search"); ImGui::PopID(); }
		return open;
	}

	void EditorGasDebugger::Refresh(sas::AbilitySystemComponent& asc)
	{
		LY_PROFILE_SCOPE("GAS Snapshot");
		mSnapshot = asc.BuildDebugSnapshot();
		mPeriods.clear();
		if (sas::AbilitySystemDebugView::effectPeriod)
			for (const auto& effect : mSnapshot.effects) mPeriods[effect.handle.id] = sas::AbilitySystemDebugView::effectPeriod(effect);
		const double now = ImGui::GetTime();
		mNextRefresh = now + mRefreshMs / 1000.0;
		for (const auto& attribute : mSnapshot.attributes)
		{
			auto& history = mHistory[std::string{ attribute.id.GetName() }];
			history.erase(history.begin(), std::find_if(history.begin(), history.end(), [now](const auto& sample) { return sample.first >= now - 10.0; }));
			history.emplace_back(now, attribute.currentValue);
			if (history.size() > 500) history.erase(history.begin());
		}
		mTags = {};
		for (const auto& tag : mSnapshot.tags)
		{
			auto* node = &mTags;
			std::size_t start = 0;
			do
			{
				const auto end = tag.first.name.find('.', start);
				node = &node->children[tag.first.name.substr(start, end - start)];
				node->count += tag.second;
				if (end == std::string::npos) break;
				start = end + 1;
			} while (start < tag.first.name.size());
		}
	}

	void EditorGasDebugger::Build(EditorContext& context)
	{
		LY_PROFILE_SCOPE("Editor Ability System");
		ImGui::SetNextWindowSize({ 720.f, 760.f }, ImGuiCond_FirstUseEver);
		if (mFocus) { ImGui::SetNextWindowFocus(); mFocus = false; }
		const bool wasVisible = visible;
		if (ImGui::Begin("Ability System", &visible))
		{
			auto actor = Target(context);
			auto* asc = actor && sas::AbilitySystemDebugView::resolve ? sas::AbilitySystemDebugView::resolve(*actor) : nullptr;
			const bool paused = context.world && context.world->IsPaused();
			if (mTarget.lock() != actor)
			{
				Clear();
				mTarget = actor;
				// A newly selected actor gets one initial sample even when already paused.
				if (asc) Refresh(*asc);
			}
			else if (asc && !paused && ImGui::GetTime() >= mNextRefresh) Refresh(*asc);
			if (!asc) ImGui::TextWrapped(context.playMode ? "Selected actor has no ability system." : "Enter Play to inspect the selected actor or player.");
			else
			{
				ImGui::Text("%s #%u%s", ActorTypeName(*actor).c_str(), actor->GetUniqueID(), paused ? " (PAUSED / frozen)" : "");
				ImGui::SetNextItemWidth(130.f);
				if (ImGui::DragInt("Refresh ms", &mRefreshMs, 10.f, 20, 2000, "%d", ImGuiSliderFlags_AlwaysClamp)) ImGui::MarkIniSettingsDirty();
				ImGui::BeginDisabled(paused);
				if (ImGui::Button("Debug: reset cooldowns")) asc->ReduceAbilityCooldowns(std::numeric_limits<float>::max());
				ImGui::EndDisabled();
				if (Section(0, "Abilities"))
				{
					for (const auto& entry : mSnapshot.abilities)
					{
						const auto& a = entry.runtime;
						if (!mFilters[0].PassFilter(a.abilityId.c_str()) && !mFilters[0].PassFilter(entry.displayName.c_str())) continue;
						ImGui::PushID(static_cast<int>(a.handle.id));
						ImGui::Text("Slot %d | %s | L%d | %s%s", static_cast<int>(a.slot), entry.displayName.c_str(), a.level, entry.State(), entry.passive ? " | passive" : "");
						ImGui::TextDisabled("%s", a.abilityId.c_str());
						if (!entry.blockedReason.empty()) ImGui::TextWrapped("%s", entry.blockedReason.c_str());
						const auto label = std::to_string(a.cooldownRemaining) + " s / charges " + std::to_string(a.charges) + "/" + (a.chargeCapacity <= 0 ? "unlimited" : std::to_string(a.chargeCapacity));
						ImGui::ProgressBar(a.cooldownDuration > 0 ? std::clamp(a.cooldownRemaining / a.cooldownDuration, 0.f, 1.f) : 0.f, { -1.f, 14.f }, label.c_str());
						ImGui::BeginDisabled(paused);
						int level = a.level;
						ImGui::SetNextItemWidth(90.f);
						if (ImGui::InputInt("Debug level", &level)) asc->SetAbilityLevel(a.handle, level);
						ImGui::SameLine();
						if (ImGui::SmallButton("Debug +charge")) asc->GrantAbilityCharge(a.abilityId, 1);
						ImGui::EndDisabled();
						ImGui::PopID();
					}
				}
				if (Section(1, "Active effects"))
				{
					ImGui::BeginDisabled(paused);
					if (ImGui::Button("Debug: apply effect"))
					{
						mDefinitions = sas::AbilitySystemDebugView::effectDefinitions ? sas::AbilitySystemDebugView::effectDefinitions() : ly::List<sas::GameplayEffectSpec>{};
						ImGui::OpenPopup("Apply effect");
					}
					if (ImGui::BeginPopup("Apply effect"))
					{
						mEffectFilter.Draw("Find definition");
						for (const auto& spec : mDefinitions)
							if (mEffectFilter.PassFilter(spec.definition.effectId.c_str()) && ImGui::Selectable(spec.definition.effectId.c_str()))
								mStatus = asc->ApplyGameplayEffect(spec).IsValid() ? "Debug effect applied" : "Effect refused or instant (no active handle)";
						ImGui::EndPopup();
					}
					ImGui::EndDisabled();
					for (const auto& effect : mSnapshot.effects)
					{
						if (!mFilters[1].PassFilter(effect.effectId.c_str())) continue;
						ImGui::PushID(static_cast<int>(effect.handle.id));
						ImGui::TextWrapped("%s | stacks %d | source %s", effect.effectId.c_str(), effect.stackCount, effect.source.c_str());
						const auto duration = effect.infinite ? std::string{ "infinite" } : std::to_string(effect.remainingDuration) + " s";
						ImGui::ProgressBar(effect.infinite ? 1.f : effect.totalDuration > 0.f ? std::clamp(effect.remainingDuration / effect.totalDuration, 0.f, 1.f) : 0.f, { -1.f, 14.f }, duration.c_str());
						const float period = mPeriods[effect.handle.id];
						if (period > 0.f) ImGui::Text("Period: %.3f s", period);
						else ImGui::TextDisabled("Period: none");
						for (const auto& modifier : effect.modifiers)
						{
							const char* operation = modifier.operation == sas::AttributeModifierOperation::Add ? "add" : modifier.operation == sas::AttributeModifierOperation::Multiply ? "multiply" : "override";
							ImGui::Text("%s %s %+.3f", modifier.attributeId.GetName().data(), operation, modifier.magnitude);
						}
						for (const auto& value : effect.runtimeAttributes) ImGui::Text("%s = %.3f", value.id.GetName().data(), value.currentValue);
						for (const auto& tag : effect.grantedTags) ImGui::TextDisabled("+ %s", tag.name.c_str());
						ImGui::BeginDisabled(paused);
						if (ImGui::SmallButton("Debug remove")) asc->RemoveGameplayEffect(effect.handle);
						ImGui::EndDisabled();
						ImGui::PopID();
					}
				}
				if (Section(2, "Attributes"))
				{
					if (ImGui::BeginTable("Attributes", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable))
					{
						ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthStretch);
						for (const auto* column : { "Base", "Current", "Delta" }) ImGui::TableSetupColumn(column, ImGuiTableColumnFlags_WidthFixed, 65.f);
						ImGui::TableHeadersRow();
						for (const auto& value : mSnapshot.attributes)
						{
							if (!mFilters[2].PassFilter(value.id.GetName().data())) continue;
							ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::TextUnformatted(value.id.GetName().data());
							if (ImGui::IsItemHovered())
							{
								std::vector<float> samples;
								for (const auto& sample : mHistory[std::string{ value.id.GetName() }]) samples.push_back(sample.second);
								ImGui::BeginTooltip();
								ImGui::TextUnformatted("Last 10 seconds (snapshot samples)");
								ImGui::PlotLines("##history", samples.data(), static_cast<int>(samples.size()), 0, nullptr, FLT_MAX, FLT_MAX, { 260.f, 70.f });
								ImGui::EndTooltip();
							}
							ImGui::TableNextColumn(); ImGui::Text("%.3f", value.baseValue);
							ImGui::TableNextColumn(); ImGui::TextColored(value.currentValue != value.baseValue ? ImVec4{ 1.f, .75f, .3f, 1.f } : ImGui::GetStyleColorVec4(ImGuiCol_Text), "%.3f", value.currentValue);
							ImGui::TableNextColumn(); ImGui::Text("%+.3f", value.currentValue - value.baseValue);
						}
						ImGui::EndTable();
					}
				}
				if (Section(3, "Owned tags"))
				{
					ImGui::BeginDisabled(paused);
					ImGui::InputText("Debug tag", mTag, sizeof(mTag));
					if (ImGui::SmallButton("Debug add tag") && mTag[0]) asc->AddOwnedTag(ly::GameplayTag{ mTag });
					ImGui::SameLine();
					if (ImGui::SmallButton("Debug remove tag") && mTag[0]) asc->RemoveOwnedTag(ly::GameplayTag{ mTag });
					ImGui::EndDisabled();
					std::function<void(const TagNode&, const std::string&)> draw = [&](const TagNode& node, const std::string& prefix)
					{
						for (const auto& child : node.children)
						{
							const auto path = prefix + child.first;
							if (mFilters[3].IsActive())
							{
								if (mFilters[3].PassFilter(path.c_str())) ImGui::Text("%s (%d)", path.c_str(), child.second.count);
								draw(child.second, path + ".");
							}
							else if (ImGui::TreeNodeEx(path.c_str(), child.second.children.empty() ? ImGuiTreeNodeFlags_Leaf : ImGuiTreeNodeFlags_None, "%s (%d)", child.first.c_str(), child.second.count))
							{
								draw(child.second, path + "."); ImGui::TreePop();
							}
						}
					};
					draw(mTags, "");
				}
				if (Section(4, "Event log (last 100)"))
					for (const auto& event : mSnapshot.events)
						if (mFilters[4].PassFilter(event.message.c_str())) ImGui::TextWrapped("%.3f  %s", event.seconds, event.message.c_str());
				if (!mStatus.empty()) ImGui::TextWrapped("%s", mStatus.c_str());
			}
		}
		ImGui::End();
		if (visible != wasVisible) ImGui::MarkIniSettingsDirty();
	}
}

namespace vega
{
	bool EditorGasDebugger::Execute(EditorContext& context, const EditorScriptCommand& command, std::string& detail)
	{
		using Op = ScriptOperation;
		if (command.operation == Op::GasFilter)
		{
			const char* sections[]{ "abilities", "effects", "attributes", "tags", "events" };
			for (int i = 0; i < 5; ++i)
				if (command.text == sections[i])
				{
					std::snprintf(mFilters[i].InputBuf, sizeof(mFilters[i].InputBuf), "%s", command.property.c_str());
					mFilters[i].Build();
					return true;
				}
			return false;
		}
		if (command.operation == Op::GasPanel)
		{
			visible = command.text == "on";
			mFocus = visible;
			ImGui::MarkIniSettingsDirty();
			return true;
		}
		if (!context.playMode || context.transitioning || !context.world)
		{
			detail = "GAS commands require a live Play world";
			return false;
		}
		if (command.operation == Op::GasSelectPlayer)
		{
			auto player = sas::AbilitySystemDebugView::player ? sas::AbilitySystemDebugView::player(*context.world) : nullptr;
			if (!player) { detail = "No player"; return false; }
			context.selection.Select(player);
			visible = mFocus = true;
			return true;
		}
		auto actor = Target(context);
		auto* asc = actor && sas::AbilitySystemDebugView::resolve ? sas::AbilitySystemDebugView::resolve(*actor) : nullptr;
		if (!asc) { detail = "Actor has no ASC"; return false; }
		const auto snapshot = asc->BuildDebugSnapshot();
		const bool expected = command.property == "true";
		switch (command.operation)
		{
		case Op::ExpectAbility:
			for (const auto& entry : snapshot.abilities)
				if (entry.runtime.abilityId == command.text) { detail = entry.State(); return command.property == entry.State(); }
			detail = "Ability not found";
			return false;
		case Op::ExpectEffect:
			return std::any_of(snapshot.effects.begin(), snapshot.effects.end(), [&](const auto& effect) { return effect.effectId == command.text; }) == expected;
		case Op::ExpectTag:
			return asc->HasOwnedTag(ly::GameplayTag{ command.text }) == expected;
		case Op::ExpectAttribute:
			for (const auto& attribute : snapshot.attributes)
			{
				if (attribute.id.GetName() != command.text) continue;
				const float actual = attribute.currentValue, value = command.factor;
				detail = "actual=" + std::to_string(actual);
				if (command.property == "==") return actual == value;
				if (command.property == "<") return actual < value;
				if (command.property == ">") return actual > value;
				if (command.property == "<=") return actual <= value;
				if (command.property == ">=") return actual >= value;
			}
			detail = "Attribute not found";
			return false;
		default: break;
		}
		if (context.world->IsPaused()) { detail = "Debug mutations require unpaused Play"; return false; }
		if (command.operation == Op::GasResetCooldowns)
		{
			asc->ReduceAbilityCooldowns(std::numeric_limits<float>::max());
			return true;
		}
		if (command.operation == Op::GasActivate)
		{
			for (const auto& entry : snapshot.abilities)
				if (entry.runtime.abilityId == command.text && !entry.passive)
				{
					return asc->DebugActivateAbility(entry.runtime.handle);
				}
			return false;
		}
		if (command.operation == Op::GasRemoveEffect)
		{
			bool removed = false;
			for (const auto& effect : snapshot.effects)
				if (effect.effectId == command.text) { asc->RemoveGameplayEffect(effect.handle); removed = true; }
			return removed;
		}
		if (command.operation == Op::GasApplyEffect && sas::AbilitySystemDebugView::effectDefinitions)
		{
			for (const auto& spec : sas::AbilitySystemDebugView::effectDefinitions())
				if (spec.definition.effectId == command.text)
				{
					detail = "Debug apply from content defaults / first authored ability spec";
					return asc->ApplyGameplayEffect(spec).IsValid();
				}
			detail = "Unknown effect definition";
		}
		return false;
	}
}
