#include "framework/reflection/Reflection.h"
#include "framework/Actor.h"
#include "framework/ActorComponent.h"
#include <algorithm>
#include <cmath>

namespace ly::reflect
{
	bool PropertyInfo::SetValue(void* object, const PropertyValue& value) const
	{
		const std::size_t index = kind == PropertyKind::Enum ? 1u : static_cast<std::size_t>(kind);
		if (!object || !setter || value.index() != index) return false;
		if (const auto* number = std::get_if<float>(&value); number && !std::isfinite(*number)) return false;
		if (const auto* vector = std::get_if<sf::Vector2f>(&value);
			vector && (!std::isfinite(vector->x) || !std::isfinite(vector->y))) return false;
		if (kind == PropertyKind::Enum && !enumLabels.empty() &&
			std::none_of(enumLabels.begin(), enumLabels.end(), [&](const auto& label) { return label.first == std::get<int>(value); })) return false;
		setter(object, value);
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
