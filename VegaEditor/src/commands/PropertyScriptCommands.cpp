#include "editor/ScriptExecution.h"
#include "editor/EditorAssetField.h"
#include "editor/JsonDocument.h"
#include <framework/World.h>
#include <framework/scene/SceneEntityComponent.h>
#include <cmath>
#include "ScriptPropertyValues.h"
#include <algorithm>

namespace vega
{
	namespace
	{
		EditorScriptParseResult DecodeSetRef(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& verb = arguments.tokens[0];
			const auto integer = [&arguments](std::size_t i, unsigned int& v) { return arguments.Integer(i, v); };
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() < (verb == "list-add-default" ? 3u : 4u) || tokens[1].empty() || tokens[2].empty()) return bad();
			command.text = tokens[1];
			command.property = tokens[2];
			if (verb == "set-ref" || verb == "expect-ref")
			{
				if (tokens.size() != 4 || tokens[3].empty()) return bad();
				command.values = {tokens[3]};
			}
			else if (verb == "list-remove" || verb == "expect-list-size")
			{
				if (tokens.size() != 4 || !integer(3, command.number)) return bad();
			}
			else if (verb == "list-add-default")
			{
				if (tokens.size() != 3) return bad();
			}
			else if (verb == "list-move")
			{
				if (tokens.size() != 5 || !integer(3, command.number) || !integer(4, command.destination)) return bad();
			}
			else
			{
				const auto begin = verb == "list-set" ? 4 : 3;
				if (verb == "list-set" && (tokens.size() < 5 || !integer(3, command.number))) return bad();
				command.values.assign(tokens.begin() + begin, tokens.end());
			}
			return {command, {}};
		}
		EditorScriptParseResult DecodeSetPropAll(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() < 3 || tokens[1].empty()) return bad();
			command.property = tokens[1];
			command.values.assign(tokens.begin() + 2, tokens.end());
			return {command, {}};
		}
		EditorScriptParseResult DecodeAddComponent(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& verb = arguments.tokens[0];
			const auto integer = [&arguments](std::size_t i, unsigned int& v) { return arguments.Integer(i, v); };
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() < 2 || tokens[1].empty()) return bad();
			command.text = tokens[1];
			if (verb == "add-component")
			{
				if (tokens.size() != 2) return bad();
			}
			else if (verb == "remove-component")
			{
				if (tokens.size() > 3 || (tokens.size() == 3 && !integer(2, command.number))) return bad();
			}
			else
			{
				if (tokens.size() != 3 || (tokens[2] != "present" && tokens[2] != "absent")) return bad();
				command.property = tokens[2];
			}
			return {command, {}};
		}
		EditorScriptParseResult DecodeExpectPropError(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 2 || tokens[1].empty()) return bad();
			command.text = tokens[1];
			return {command, {}};
		}
		EditorScriptParseResult DecodeSetProp(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() < 4 || tokens[1].empty() || tokens[2].empty()) return bad();
			command.text = tokens[1];
			command.property = tokens[2];
			command.values.assign(tokens.begin() + 3, tokens.end());
			// Arity and scalar types depend on the selected object's descriptor.
			return {command, {}};
		}
		EditorScriptParseResult DecodeMove(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto& verb = arguments.tokens[0];
			const auto real = [&arguments](std::size_t i, float& v) { return arguments.Real(i, v); };
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };

			const bool expect = verb == "expect-location";
			if (tokens.size() < 3 || tokens.size() > (expect ? 4u : 3u) || !real(1, command.location.x) || !real(2, command.location.y) ||
			    (tokens.size() == 4 && (!real(3, command.tolerance) || command.tolerance < 0.f)))
				return bad();
			return {command, {}};
		}
		EditorScriptParseResult DecodeRotate(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			const auto real = [&arguments](std::size_t i, float& v) { return arguments.Real(i, v); };
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 2 || !real(1, command.degrees)) return bad();
			return {command, {}};
		}
		void ExecuteSetRef(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& history = execution.history;
			auto& success = execution.success;
			auto& detail = execution.detail;
			auto& mPendingPropertyError = execution.state.mPendingPropertyError;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			const auto find = [&](const std::string& name) -> std::shared_ptr<vega::Actor>
			{
				if (const auto found = context.ResolveGuid(name)) return found;
				for (const auto& weak : context.world->GetActorsByTypeIncludingPending<vega::Actor>())
					if (const auto candidate = weak.lock(); candidate && !candidate->GetIsPendingDestroy())
						if (const auto* identity = candidate->FindComponent<vega::SceneEntityComponent>(); identity && identity->name == name) return candidate;
				return {};
			};
			auto owner = actor;
			auto target = actor ? FindPropertyTarget(*actor, command.text) : ReflectedTarget{};
			if (!target.object)
			{
				owner = find(command.text);
				if (owner) target = ResolvePropertyTarget(*owner, {});
			}
			const auto* property = target.type ? target.type->FindProperty(command.property) : nullptr;
			if (!property)
			{
				detail = "Unknown target or property";
				return;
			}
			auto value = property->getter(target.object);
			const auto parse = [&](const vega::reflect::PropertyInfo& descriptor, vega::reflect::PropertyValue& parsed)
			{
				if (descriptor.kind == vega::reflect::PropertyKind::ActorRef)
				{
					if (command.values.size() != 1) return false;
					const auto other = find(command.values[0]);
					if (!other && command.values[0] != "none") return false;
					parsed = other ? vega::ActorRef{*other} : vega::ActorRef{};
					return true;
				}
				float tolerance = 0.f;
				return ParsePropertyValue(descriptor, command.values, false, parsed, tolerance);
			};
			if (command.operation == "set-ref" || command.operation == "expect-ref")
			{
				if (property->kind != vega::reflect::PropertyKind::ActorRef || !parse(*property, value))
				{
					detail = "Expected actor reference and live target or none";
					return;
				}
				if (command.operation == "expect-ref")
				{
					success = property->getter(target.object) == value;
					return;
				}
			}
			else
			{
				if (property->kind != vega::reflect::PropertyKind::List)
				{
					detail = "Expected list property";
					return;
				}
				auto& list = std::get<vega::reflect::PropertyList>(value);
				if (command.operation == "expect-list-size")
				{
					success = list.size() == command.number;
					detail = "size " + std::to_string(list.size());
					return;
				}
				if (command.operation != "list-add" && command.operation != "list-add-default" && command.number >= list.size())
				{
					detail = "List index out of range (zero-based)";
					return;
				}
				if (command.operation == "list-remove") list.erase(list.begin() + command.number);
				else if (command.operation == "list-move")
				{
					if (command.destination >= list.size()) { detail = "List index out of range (zero-based)"; return; }
					const auto from = list.begin() + command.number;
					const auto to = list.begin() + command.destination;
					// The rotation pivot depends on direction so that only the moved element changes position.
					if (from < to) std::rotate(from, from + 1, to + 1);
					else std::rotate(to, from, from + 1);
				}
				else
				{
					vega::reflect::PropertyValue item;
					const bool structElement = property->elementKind == vega::reflect::PropertyKind::Struct;
					if (command.operation == "list-add-default")
					{
						// A new struct element starts from the field defaults, so no value is parsed.
						if (!structElement || !property->structInfo || !property->structInfo->makeDefault) { detail = "Expected a list of structs"; return; }
						item = property->structInfo->makeDefault();
					}
					else if (structElement)
					{
						detail = "Struct elements are added with list-add-default and edited with Name[i].Field";
						return;
					}
					else if (!parse(property->Element(), item))
					{
						detail = "Invalid list element";
						return;
					}
					if (command.operation == "list-add" || command.operation == "list-add-default") list.push_back(vega::reflect::ToScalar(item));
					else list[command.number] = vega::reflect::ToScalar(item);
				}
			}
			success = history.Execute(std::make_unique<SetPropertyCommand>(owner->GetUniqueID(), target.componentIdentity, property->name, value));
			detail = success ? "Property set" : context.propertyError;
			if (!success) mPendingPropertyError = detail;
		}
		void ExecuteSetPropAll(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& history = execution.history;
			auto& success = execution.success;
			auto& detail = execution.detail;
			MultiInspectorCache cache;
			const auto& rows = cache.Get(context);
			const CommonProperty* match = nullptr;
			bool ambiguous = false;
			for (const auto& row : rows)
				if (row.property->name == command.property)
				{
					if (match) ambiguous = true;
					match = &row;
				}
			vega::reflect::PropertyValue value;
			float tolerance = command.tolerance;
			if (!match || ambiguous) detail = "Unknown or ambiguous common property";
			else if (!ParsePropertyValue(*match->property, command.values, false, value, tolerance)) detail = "Invalid common property value";
			else success = SetCommonProperty(context, history, *match, value);
		}
		void ExecuteAddComponent(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& history = execution.history;
			auto& success = execution.success;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			if (actor) success = history.Execute(std::make_unique<AddComponentCommand>(actor->GetUniqueID(), command.text));
		}
		void ExecuteRemoveComponent(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& history = execution.history;
			auto& success = execution.success;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			if (actor)
				success = history.Execute(std::make_unique<RemoveComponentCommand>(actor->GetUniqueID(), vega::ComponentIdentity{command.text, command.number}));
		}
		void ExecuteExpectComponent(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& success = execution.success;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			if (actor) success = (vega::FindComponentByTypeOccurrence(*actor, {command.text, 0}) != nullptr) == (command.property == "present");
		}
		void ExecuteExpectPropError(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& success = execution.success;
			auto& detail = execution.detail;
			auto& mPendingPropertyError = execution.state.mPendingPropertyError;
			success = mPendingPropertyError && mPendingPropertyError->find(command.text) != std::string::npos;
			detail = mPendingPropertyError.value_or("No failed property write to handle");
			mPendingPropertyError.reset();
		}
		void ExecuteSetProp(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& history = execution.history;
			auto& success = execution.success;
			auto& detail = execution.detail;
			auto& mPendingPropertyError = execution.state.mPendingPropertyError;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			const auto target = actor ? FindPropertyTarget(*actor, command.text) : ReflectedTarget{};
			const auto* property = target.type ? target.type->FindProperty(command.property) : nullptr;
			vega::reflect::PropertyValue value;
			float tolerance = command.tolerance;
			const bool expect = command.operation == "expect-prop";
			if (!property && actor && target.object && command.property.find_first_of(".[") != std::string::npos)
			{
				// A struct field is written as its whole top-level value, so validation and undo match a whole-property edit.
				StructLeafPath path;
				if (!ResolveStructLeafPath(target, command.property, path, detail)) return;
				auto whole = path.property->getter(target.object);
				vega::reflect::PropertyValue parsed;
				if (!ParsePropertyValue(*path.leaf, command.values, expect, parsed, tolerance)) { detail = "Invalid property value or tolerance"; return; }
				auto* stored = FindStructLeaf(path, whole, detail);
				if (!stored) return;
				if (expect)
				{
					success = PropertyEquals(vega::reflect::ToPropertyValue(*stored), parsed, tolerance);
					detail = success ? "Property matched" : "Property value differs";
					return;
				}
				*stored = vega::reflect::ToLeaf(parsed);
				success = history.Execute(std::make_unique<SetPropertyCommand>(actor->GetUniqueID(), target.componentIdentity, path.property->name, whole));
				detail = success ? "Property set" : context.propertyError;
				if (!success) mPendingPropertyError = detail;
				return;
			}
			if (!property) detail = "Unknown target or property: " + command.text + " / " + command.property;
			else if (!ParsePropertyValue(*property, command.values, expect, value, tolerance)) detail = "Invalid property value or tolerance";
			else if (expect)
			{
				success = PropertyEquals(property->getter(target.object), value, tolerance);
				detail = success ? "Property matched" : "Property value differs";
			}
			else
			{
				success = history.Execute(std::make_unique<SetPropertyCommand>(actor->GetUniqueID(), target.componentIdentity, property->name, value));
				detail = success ? "Property set" : context.propertyError;
				if (!success) mPendingPropertyError = detail;
			}
		}
		void ExecuteMove(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& history = execution.history;
			auto& success = execution.success;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			if (actor) success = history.Execute(std::make_unique<SetPropertyCommand>(actor->GetUniqueID(), std::nullopt, "Location", command.location));
		}
		void ExecuteExpectLocation(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& detail = execution.detail;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			auto& success = execution.success;
			const auto expectLocation = [&](sf::Vector2f expected)
			{
				if (!actor) return false;
				const auto actual = actor->GetActorLocation();
				detail = "actual " + std::to_string(actual.x) + " " + std::to_string(actual.y) + " expected " + std::to_string(expected.x) + " " +
				    std::to_string(expected.y);
				return std::abs(actual.x - expected.x) <= command.tolerance && std::abs(actual.y - expected.y) <= command.tolerance;
			};
			success = expectLocation(command.location);
		}
		EditorScriptParseResult DecodeDropAsset(const ScriptArguments& arguments)
		{
			const auto& tokens = arguments.tokens;
			EditorScriptCommand command;
			command.operation = arguments.tokens[0];
			const auto bad = []() { return EditorScriptParseResult{{}, "Invalid arguments"}; };
			if (tokens.size() != 4 || tokens[1].empty() || tokens[2].empty() || tokens[3].empty()) return bad();
			command.text = tokens[1];
			command.property = tokens[2];
			command.values = { tokens[3] };
			return {command, {}};
		}
		// Same validation and undo path as an Inspector drop; a refusal is readable by expect-prop-error.
		void ExecuteDropAsset(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& context = execution.context;
			auto& success = execution.success;
			auto& detail = execution.detail;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			const auto target = actor ? FindPropertyTarget(*actor, command.text) : ReflectedTarget{};
			const auto* property = target.type ? target.type->FindProperty(command.property) : nullptr;
			if (!property) { detail = "Unknown target or property"; return; }
			const auto root = context.document ? context.document->Root() : std::filesystem::path{};
			std::string error;
			success = AssignAssetProperty(context, execution.history, *actor, target, *property, root, std::filesystem::u8path(command.values[0]), error);
			detail = success ? "Asset assigned" : error;
			if (!success) execution.state.mPendingPropertyError = error;
		}
		void ExecuteRotate(ScriptExecution& execution, const EditorScriptCommand& command)
		{
			auto& history = execution.history;
			auto& success = execution.success;
			auto actor = execution.context.selection.Get();
			if (actor && actor->GetWorld() != execution.context.world) actor.reset();
			if (actor) success = history.Execute(std::make_unique<SetPropertyCommand>(actor->GetUniqueID(), std::nullopt, "Rotation", command.degrees));
		}
	} // namespace
	void RegisterPropertyScriptCommands(ScriptCommandRegistry& registry)
	{
		registry.Add({"set-ref", Schema("target:text property:text actor:text"), "Set an actor reference by name or GUID; use none to clear it.",
		    ScriptMode::Any, DecodeSetRef, ExecuteSetRef});
		registry.Add({"expect-ref", Schema("target:text property:text actor:text"), "Assert an actor reference by name or GUID; none means no actor.",
		    ScriptMode::Any, DecodeSetRef, ExecuteSetRef});
		registry.Add({"list-add", Schema("target:text property:text value:value..."), "Append a reflected list element using its descriptor type.",
		    ScriptMode::Any, DecodeSetRef, ExecuteSetRef});
		registry.Add({"list-add-default", Schema("target:text property:text"),
		    "Append a struct list element that starts from its field defaults; edit its fields with Name[i].Field.", ScriptMode::Any, DecodeSetRef,
		    ExecuteSetRef});
		registry.Add({"list-move", Schema("target:text property:text from:uint to:uint"), "Move a reflected list element to another zero-based index.",
		    ScriptMode::Any, DecodeSetRef, ExecuteSetRef});
		registry.Add({"list-set", Schema("target:text property:text index:uint value:value..."), "Set a reflected list element at a zero-based index.",
		    ScriptMode::Any, DecodeSetRef, ExecuteSetRef});
		registry.Add({"list-remove", Schema("target:text property:text index:uint"), "Remove a reflected list element at a zero-based index.", ScriptMode::Any,
		    DecodeSetRef, ExecuteSetRef});
		registry.Add({"expect-list-size", Schema("target:text property:text size:uint"), "Assert the number of elements in a reflected list.", ScriptMode::Any,
		    DecodeSetRef, ExecuteSetRef});
		registry.Add({"set-prop-all", Schema("property:text value:value..."), "Set a common reflected property on every selected actor.", ScriptMode::Any,
		    DecodeSetPropAll, ExecuteSetPropAll});
		registry.Add({"add-component", Schema("type:text"), "Add a reflected component to the selected actor.", ScriptMode::Any, DecodeAddComponent,
		    ExecuteAddComponent});
		registry.Add({"remove-component", Schema("type:text [occurrence:uint]"), "Remove a component by type and zero-based occurrence (defaults to zero).",
		    ScriptMode::Any, DecodeAddComponent, ExecuteRemoveComponent});
		registry.Add({"expect-component", Schema("type:text presence:present|absent"), "Assert whether the selected actor has a component of a type.",
		    ScriptMode::Any, DecodeAddComponent, ExecuteExpectComponent});
		registry.Add({"expect-prop-error", Schema("message:text"), "Consume an immediately preceding failed property write containing the supplied message.",
		    ScriptMode::Any, DecodeExpectPropError, ExecuteExpectPropError});
		registry.Add({"set-prop", Schema("target:text property:text value:value..."),
		    "Set a reflected property using its descriptor type; Name.Field and Name[i].Field address struct fields.", ScriptMode::Any,
		    DecodeSetProp, ExecuteSetProp});
		registry.Add({"expect-prop", Schema("target:text property:text value:value..."),
		    "Assert a reflected property, optionally followed by nonnegative tolerance; struct fields use Name.Field or Name[i].Field.",
		    ScriptMode::Any, DecodeSetProp, ExecuteSetProp});
		registry.Add({"move", Schema("x:real y:real"), "Set the selected actor location.", ScriptMode::Any, DecodeMove, ExecuteMove});
		registry.Add({"expect-location", Schema("x:real y:real [tolerance:real]"), "Assert the selected actor location with optional nonnegative tolerance.",
		    ScriptMode::Any, DecodeMove, ExecuteExpectLocation});
		registry.Add({"rotate", Schema("degrees:real"), "Set the selected actor rotation in degrees.", ScriptMode::Any, DecodeRotate, ExecuteRotate});
		registry.Add({"drop-asset", Schema("target:text property:text path:text"), "Assign an asset path to an asset property, as an Inspector drop does.",
		    ScriptMode::Edit, DecodeDropAsset, ExecuteDropAsset});
	}
} // namespace vega
