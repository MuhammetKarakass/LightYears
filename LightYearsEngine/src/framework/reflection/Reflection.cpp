#include "framework/reflection/Reflection.h"
#include "framework/Actor.h"
#include "framework/ActorComponent.h"
#include <algorithm>
#include <cmath>

namespace ly::reflect
{
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
			if (std::get<PropertyList>(value).size() > 4096) return fail("List exceeds 4096 elements");
			const auto element = Element();
			for (const auto& item : std::get<PropertyList>(value))
				if (!element.ValidateValue(ToValue(item), error)) return false;
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
		const auto* ownerWorld = world ? world(object) : nullptr;
		if (ownerWorld && !requiredType.empty())
		{
			const auto accepts = [&](const ly::ActorRef& ref)
			{
				const auto actor = ref.Resolve(*const_cast<World*>(ownerWorld));
				const auto* filter = TypeRegistry::Get().Find(requiredType);
				return !actor || (filter && filter->root == TypeRoot::Actor && filter->isInstance(actor.get()));
			};
			bool accepted = true;
			if (kind == PropertyKind::ActorRef) accepted = accepts(std::get<ly::ActorRef>(value));
			if (kind == PropertyKind::List && elementKind == PropertyKind::ActorRef)
				for (const auto& item : std::get<PropertyList>(value)) accepted = accepts(std::get<ly::ActorRef>(item)) && accepted;
			if (!accepted) { if (error) *error = "Actor reference requires " + requiredType; return false; }
		}
		try
		{
			setter(object, value);
		}
		catch (const std::exception& failure) { if (error) *error = failure.what(); return false; }
		catch (...) { if (error) *error = "Unknown property setter error"; return false; }
		return true;
	}

	bool PropertyInfo::ResolveReferences(void* object, World& world, std::string* error) const
	{
		if (!setter || (kind != PropertyKind::ActorRef && !(kind == PropertyKind::List && elementKind == PropertyKind::ActorRef))) return true;
		auto value = getter(object);
		bool valid = true;
		const auto bind = [&](ly::ActorRef& ref)
		{
			if (ref.Guid().empty()) return;
			const auto actor = ref.Resolve(world);
			if (!actor) { valid = false; if (error) *error = "Dangling actor reference: " + ref.Guid(); return; }
			const auto* filter = requiredType.empty() ? nullptr : TypeRegistry::Get().Find(requiredType);
			if (!requiredType.empty() && (!filter || filter->root != TypeRoot::Actor || !filter->isInstance(actor.get())))
			{
				valid = false;
				if (error) *error = "Actor reference requires " + requiredType;
				ref = ly::ActorRef{};
			}
		};
		if (kind == PropertyKind::ActorRef) bind(std::get<ly::ActorRef>(value));
		else for (auto& item : std::get<PropertyList>(value)) bind(std::get<ly::ActorRef>(item));
		setter(object, value);
		return valid;
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
