#include "framework/reflection/Reflection.h"
#include "framework/Actor.h"
#include "framework/ActorComponent.h"
#include <algorithm>
#include <cmath>

namespace ly::reflect
{
	bool PropertyInfo::SetValue(void* object, const PropertyValue& value, std::string* error) const
	{
		if (error) error->clear();
		const auto fail = [error](const std::string& reason)
		{
			if (error) *error = reason;
			return false;
		};
		const std::size_t index = kind == PropertyKind::Enum ? 1u : static_cast<std::size_t>(kind);
		if (!object) return fail("Property target is unavailable");
		if (!setter) return fail("Read-only property");
		if (value.index() != index) return fail("Incompatible property value");
		if (const auto* number = std::get_if<float>(&value); number && !std::isfinite(*number)) return fail("Expected finite number");
		if (const auto* vector = std::get_if<sf::Vector2f>(&value);
			vector && (!std::isfinite(vector->x) || !std::isfinite(vector->y))) return fail("Expected finite vector");
		// A labelled enum is a closed set: an unlisted integer (from a hand-edited scene, a script or the
		// Inspector) could be out of range for the C++ enum and reach code that indexes or switches on it.
		// Saving still tolerates such a value (written as an integer) so the editor never crashes on save.
		if (kind == PropertyKind::Enum && !enumLabels.empty() &&
			std::none_of(enumLabels.begin(), enumLabels.end(), [&](const auto& label) { return label.first == std::get<int>(value); }))
			return fail("Value " + std::to_string(std::get<int>(value)) + " is not one of the enum's labels");
		try
		{
			setter(object, value);
		}
		catch (const std::exception& failure) { return fail(failure.what()); }
		catch (...) { return fail("Unknown property setter error"); }
		return true;
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

	TypeRegistry& TypeRegistry::Get()
	{
		static TypeRegistry registry;
		return registry;
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
