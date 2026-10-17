#include "framework/reflection/Reflection.h"
#include "framework/Actor.h"
#include "framework/World.h"
#include "framework/EngineContext.h"
#include "framework/ActorComponent.h"
#include <algorithm>
#include <cmath>
#include <charconv>
#include <limits>

namespace vega::reflect
{
	void TypeRegistry::SetCollisionLabels(const EnumLabels& labels, const EnumLabels& aliases)
	{
		for (auto& entry : mTypes)
			if (entry.second->name == "CollisionComponent")
				for (auto& property : entry.second->properties)
					if (property.name == "Collision Layer" || property.name == "Collision Mask")
					{
						property.enumLabels = labels;
						property.flagAliases = aliases;
					}
	}

	bool PropertyInfo::ParseFlags(const std::string& text, int& bits, bool* legacy) const
	{
		if (legacy) *legacy = false;
		if (kind != PropertyKind::Flags) return false;
		int result = 0;
		std::size_t begin = 0;
		do
		{
			const auto end = text.find('|', begin);
			const auto part = text.substr(begin, end == std::string::npos ? end : end - begin);
			const auto first = part.find_first_not_of(" \t");
			if (first == std::string::npos) return false;
			const auto labelName = part.substr(first, part.find_last_not_of(" \t") - first + 1);
			bool found = labelName == "None";
			for (const auto& label : enumLabels)
				if (label.second == labelName) { result |= label.first; found = true; break; }
			if (!found)
			{
				for (const auto& alias : flagAliases)
					if (alias.second == labelName)
					{
						result |= alias.first;
						found = true;
						if (legacy) *legacy = true;
						break;
					}
			}
			if (!found)
			{
				int numeric = 0;
				const auto parsed = std::from_chars(labelName.data(), labelName.data() + labelName.size(), numeric);
				if (parsed.ec != std::errc{} || parsed.ptr != labelName.data() + labelName.size() || !ValidateValue(numeric)) return false;
				result |= numeric;
				if (legacy) *legacy = true;
			}
			if (end == std::string::npos) break;
			begin = end + 1;
		} while (true);
		if (!ValidateValue(result)) return false;
		bits = result;
		return true;
	}

	bool PropertyInfo::ValidateValue(const PropertyValue& value, std::string* error) const
	{
		if (error) error->clear();
		const auto fail = [error](const std::string& reason)
		{
			if (error) *error = reason;
			return false;
		};
		const auto valueKind = (kind == PropertyKind::Enum || kind == PropertyKind::Flags) ? PropertyKind::Int : kind;
		if (KindOf(value) != valueKind) return fail("Incompatible property value");
		if (kind == PropertyKind::List)
		{
			const auto& items = std::get<PropertyList>(value);
			if (items.size() > 4096) return fail("List exceeds 4096 elements");
			auto element = Element();
			for (std::size_t index = 0; index < items.size(); ++index)
			{
				// Struct errors name the element, e.g. Waves[2].Count.
				if (elementKind == PropertyKind::Struct) element.name = name + "[" + std::to_string(index) + "]";
				if (!element.ValidateValue(ToValue(items[index]), error)) return false;
			}
		}
		if (kind == PropertyKind::Struct)
		{
			if (!structInfo) return fail("Struct type is not registered");
			for (const auto& entry : std::get<StructValue>(value))
			{
				const auto* field = structInfo->FindProperty(entry.first);
				// Unknown and read-only names are skipped, as WriteStruct skips them.
				if (!field || !field->setter) continue;
				std::string reason;
				if (!field->ValidateValue(ToPropertyValue(entry.second), &reason)) return fail(name + "." + entry.first + ": " + reason);
			}
		}
		if (const auto* number = std::get_if<float>(&value); number && !std::isfinite(*number)) return fail("Expected finite number");
		if (const auto* vector = std::get_if<sf::Vector2f>(&value);
			vector && (!std::isfinite(vector->x) || !std::isfinite(vector->y))) return fail("Expected finite vector");
		// A labelled enum is a closed set: an unlisted integer (from a hand-edited scene, a script or the
		// Inspector) could be out of range for the C++ enum and reach code that indexes or switches on it.
		// Saving still tolerates such a value (written as an integer) so the editor never crashes on save.
		if (kind == PropertyKind::Enum && !enumLabels.empty() &&
			std::none_of(enumLabels.begin(), enumLabels.end(), [&](const auto& label) { return label.first == std::get<int>(value); }))
			return fail("Value " + std::to_string(std::get<int>(value)) + " is not one of the enum's labels");
		if (kind == PropertyKind::Flags)
		{
			int allowed = 0;
			for (const auto& label : enumLabels) allowed |= label.first;
			if ((std::get<int>(value) & ~allowed) != 0) return fail("Flags contain undeclared bits");
		}
		return true;
	}

	bool PropertyInfo::SetValue(void* object, const PropertyValue& value, std::string* error) const
	{
		if (!object || !setter)
		{
			if (error) *error = object ? "Read-only property" : "Property target is unavailable";
			return false;
		}
		if (!ValidateValue(value, error)) return false;
		// Fields have no setter that enforces Range, so the bounds apply to the stored copy.
		PropertyValue stored = value;
		if (clampToRange && min && max && kind == PropertyKind::Float)
			std::get<float>(stored) = std::min(std::max(std::get<float>(stored), *min), *max);
		else if (clampToRange && min && max && kind == PropertyKind::Int)
		{
			// Integer bounds round inward so a clamped value always lies inside the declared range.
			const double low = std::max<double>(std::ceil(*min), std::numeric_limits<int>::min());
			const double high = std::min<double>(std::floor(*max), std::numeric_limits<int>::max());
			std::get<int>(stored) = static_cast<int>(std::min(std::max<double>(std::get<int>(stored), low), high));
		}
		const auto* ownerWorld = world ? world(object) : nullptr;
		if (ownerWorld)
		{
			const auto accepts = [&](const vega::ActorRef& ref, const std::string& type)
			{
				const auto actor = ref.Resolve(*const_cast<World*>(ownerWorld));
				const auto* filter = ownerWorld->GetEngineContext().Types().Find(type);
				return !actor || (filter && filter->root == TypeRoot::Actor && filter->isInstance(actor.get()));
			};
			// Each reference is checked against its own requiredType, which may be a struct field's.
			std::string required;
			ForEachActorRef(*this, stored, [&](vega::ActorRef& ref, const std::string& type)
			{
				if (required.empty() && !type.empty() && !accepts(ref, type)) required = type;
			});
			if (!required.empty()) { if (error) *error = "Actor reference requires " + required; return false; }
		}
		try
		{
			setter(object, stored);
		}
		catch (const std::exception& failure) { if (error) *error = failure.what(); return false; }
		catch (...) { if (error) *error = "Unknown property setter error"; return false; }
		return true;
	}

	bool PropertyInfo::ResolveReferences(void* object, World& world, std::string* error) const
	{
		const bool holdsRefs = kind == PropertyKind::ActorRef || kind == PropertyKind::Struct ||
			(kind == PropertyKind::List && (elementKind == PropertyKind::ActorRef || elementKind == PropertyKind::Struct));
		if (!setter || !holdsRefs) return true;
		auto value = getter(object);
		bool valid = true;
		ForEachActorRef(*this, value, [&](vega::ActorRef& ref, const std::string& type)
		{
			if (ref.Guid().empty()) return;
			const auto actor = ref.Resolve(world);
			if (!actor) { valid = false; if (error) *error = "Dangling actor reference: " + ref.Guid(); return; }
			const auto* filter = type.empty() ? nullptr : world.GetEngineContext().Types().Find(type);
			if (!type.empty() && (!filter || filter->root != TypeRoot::Actor || !filter->isInstance(actor.get())))
			{
				valid = false;
				if (error) *error = "Actor reference requires " + type;
				ref = vega::ActorRef{};
			}
		});
		setter(object, value);
		return valid;
	}

	namespace
	{
		void VisitStructRefs(const TypeInfo* info, StructValue& fields, const std::function<void(vega::ActorRef&, const std::string&)>& visit)
		{
			for (auto& entry : fields)
				if (const auto* field = info ? info->FindProperty(entry.first) : nullptr; field && field->kind == PropertyKind::ActorRef)
					visit(std::get<vega::ActorRef>(entry.second), field->requiredType);
		}
	}

	void ForEachActorRef(const PropertyInfo& property, PropertyValue& value,
		const std::function<void(vega::ActorRef&, const std::string&)>& visit)
	{
		if (property.kind == PropertyKind::ActorRef) visit(std::get<vega::ActorRef>(value), property.requiredType);
		else if (property.kind == PropertyKind::Struct) VisitStructRefs(property.structInfo, std::get<StructValue>(value), visit);
		else if (property.kind == PropertyKind::List)
			for (auto& item : std::get<PropertyList>(value))
			{
				if (property.elementKind == PropertyKind::ActorRef) visit(std::get<vega::ActorRef>(item), property.requiredType);
				else if (property.elementKind == PropertyKind::Struct) VisitStructRefs(property.structInfo, std::get<StructValue>(item), visit);
			}
	}

	StructValue ReadStruct(const TypeInfo& type, const void* object)
	{
		StructValue result;
		for (const auto& field : type.properties)
			if (field.getter && field.setter) result.emplace_back(field.name, ToLeaf(field.getter(object)));
		return result;
	}

	void WriteStruct(const TypeInfo& type, void* object, const StructValue& value)
	{
		for (const auto& entry : value)
		{
			const auto* field = type.FindProperty(entry.first);
			// Unknown and read-only names are ignored so that files written by other versions still load.
			if (!field || !field->setter) continue;
			std::string error;
			if (!field->SetValue(object, ToPropertyValue(entry.second), &error)) throw std::runtime_error(entry.first + ": " + error);
		}
	}

	std::vector<const PropertyInfo*> TypeInfo::AllProperties() const
	{
		std::vector<const PropertyInfo*> result = base ? base->AllProperties() : std::vector<const PropertyInfo*>{};
		for (const auto& property : properties) result.push_back(&property);
		return result;
	}

	const PropertyInfo* TypeInfo::FindProperty(const std::string& propertyName) const
	{
		for (const auto& property : properties) if (property.name == propertyName) return &property;
		return base ? base->FindProperty(propertyName) : nullptr;
	}

	std::vector<const FunctionInfo*> TypeInfo::AllFunctions() const
	{
		std::vector<const FunctionInfo*> result = base ? base->AllFunctions() : std::vector<const FunctionInfo*>{};
		for (const auto& function : functions) result.push_back(&function);
		return result;
	}

	const FunctionInfo* TypeInfo::FindFunction(const std::string& functionName) const
	{
		for (const auto& function : functions) if (function.name == functionName) return &function;
		return base ? base->FindFunction(functionName) : nullptr;
	}

	TypeRegistry& CurrentTypeRegistry() { return EngineContext::Current().Types(); }

	TypeRegistry& TypeRegistry::Get()
	{
		return CurrentTypeRegistry();
	}

	TypeInfo& TypeRegistry::Register(std::type_index type, const std::string& name, const TypeInfo* base,
		TypeRoot root, std::function<bool(const void*)> isInstance)
	{
		for (auto& entry : mTypes) if (entry.first == type) return *entry.second;
		auto info = std::make_unique<TypeInfo>();
		info->name = name;
		info->base = base;
		info->root = root;
		info->isInstance = std::move(isInstance);
		mTypes.emplace_back(type, std::move(info));
		return *mTypes.back().second;
	}

	const TypeInfo* TypeRegistry::Find(std::type_index type) const
	{
		for (const auto& entry : mTypes) if (entry.first == type) return entry.second.get();
		return nullptr;
	}

	const TypeInfo* TypeRegistry::Find(const std::string& name) const
	{
		for (const auto& entry : mTypes) if (entry.second->name == name) return entry.second.get();
		return nullptr;
	}

	const TypeInfo* TypeRegistry::FindMostDerived(const void* object, TypeRoot root) const
	{
		const TypeInfo* best = nullptr;
		std::size_t bestDepth = 0;
		for (const auto& entry : mTypes)
		{
			const TypeInfo& type = *entry.second;
			if (type.root != root || !type.isInstance(object)) continue;
			std::size_t depth = 1;
			for (const TypeInfo* base = type.base; base; base = base->base) ++depth;
			if (depth > bestDepth) { best = &type; bestDepth = depth; }
		}
		return best;
	}

	const TypeInfo* TypeRegistry::FindMostDerived(const Actor& actor) const
	{
		return FindMostDerived(&actor, TypeRoot::Actor);
	}
	const TypeInfo* TypeRegistry::FindMostDerived(const ActorComponent& component) const
	{
		return FindMostDerived(&component, TypeRoot::Component);
	}
}
