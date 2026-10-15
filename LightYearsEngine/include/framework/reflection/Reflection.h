#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include "framework/World.h"
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace ly { class Actor; class ActorComponent; }

namespace ly::reflect
{
	using PropertyValue = std::variant<bool, int, float, sf::Vector2f, sf::Color, std::string>;
	enum class PropertyKind { Bool, Int, Float, Vector2, Color, String, Enum };
	using EnumLabels = std::vector<std::pair<int, std::string>>;

	struct PropertyInfo
	{
		std::string name;
		PropertyKind kind = PropertyKind::Bool;
		std::function<PropertyValue(const void*)> getter;
		std::function<void(void*, const PropertyValue&)> setter;
		std::optional<float> min;
		std::optional<float> max;
		float speed = 1.f;
		std::string tooltip;
		EnumLabels enumLabels;
		std::string category;

		// All erased object pointers are pointers to the registered root, even for
		// inherited properties. This preserves pointer adjustment with multiple inheritance.
		bool SetValue(void* objectOfRoot, const PropertyValue& value, std::string* error = nullptr) const;
	};

	enum class TypeRoot { Actor, Component, Other };
	struct TypeInfo
	{
		std::string name;
		const TypeInfo* base = nullptr;
		TypeRoot root = TypeRoot::Other;
		std::function<bool(const void*)> isInstance;
		std::vector<PropertyInfo> properties;
		std::function<std::shared_ptr<Actor>(World&)> create;
		bool spawnable = false;
		std::vector<const PropertyInfo*> AllProperties() const;
		const PropertyInfo* FindProperty(const std::string& propertyName) const;
	};

	class TypeRegistry
	{
	public:
		static TypeRegistry& Get();
		TypeInfo& Register(std::type_index type, const std::string& name, const TypeInfo* base,
			TypeRoot root, std::function<bool(const void*)> isInstance);
		const TypeInfo* Find(std::type_index type) const;
		const TypeInfo* Find(const std::string& name) const;
		const TypeInfo* FindMostDerived(const Actor& actor) const;
		const TypeInfo* FindMostDerived(const ActorComponent& component) const;
		std::vector<const TypeInfo*> SpawnableActors() const
		{
			std::vector<const TypeInfo*> result;
			for (const auto& entry : mTypes)
				if (entry.second->root == TypeRoot::Actor && entry.second->spawnable && entry.second->create)
					result.push_back(entry.second.get());
			return result;
		}

	private:
		TypeRegistry() = default;
		TypeRegistry(const TypeRegistry&) = delete;
		TypeRegistry& operator=(const TypeRegistry&) = delete;
		const TypeInfo* FindMostDerived(const void* object, TypeRoot root) const;
		std::vector<std::pair<std::type_index, std::unique_ptr<TypeInfo>>> mTypes;
	};

	template<typename T, typename Base = void>
	class Class
	{
		using Root = std::conditional_t<std::is_base_of_v<Actor, T>, Actor,
			std::conditional_t<std::is_base_of_v<ActorComponent, T>, ActorComponent, T>>;
	public:
		explicit Class(const std::string& name)
			: mType{ TypeRegistry::Get().Register(typeid(T), name, BaseType(), RootKind(),
				[](const void* object) { return object && dynamic_cast<const T*>(static_cast<const Root*>(object)); }) }
		{
		}

		template<typename Getter>
		Class& Property(const std::string& name, Getter getter)
		{
			using Value = std::decay_t<std::invoke_result_t<Getter, const T&>>;
			// Repeated registration does not invalidate existing descriptors.
			for (std::size_t index = 0; index < mType.properties.size(); ++index)
			{
				if (mType.properties[index].name == name)
				{
					mProperty = index;
					return *this;
				}
			}
			PropertyInfo property;
			property.name = name;
			property.kind = Kind<Value>();
			property.getter = [getter](const void* object) -> PropertyValue
			{
				const T& instance = *dynamic_cast<const T*>(static_cast<const Root*>(object));
				if constexpr (std::is_enum_v<Value>) return static_cast<int>(std::invoke(getter, instance));
				else return std::invoke(getter, instance);
			};
			mType.properties.push_back(std::move(property));
			mProperty = mType.properties.size() - 1;
			return *this;
		}

		template<typename Getter, typename Setter>
		Class& Property(const std::string& name, Getter getter, Setter setter)
		{
			using Value = std::decay_t<std::invoke_result_t<Getter, const T&>>;
			Property(name, getter);
			Current().setter = [setter](void* object, const PropertyValue& value)
			{
				T& instance = *dynamic_cast<T*>(static_cast<Root*>(object));
				if constexpr (std::is_enum_v<Value>) std::invoke(setter, instance, static_cast<Value>(std::get<int>(value)));
				else std::invoke(setter, instance, std::get<Value>(value));
			};
			return *this;
		}

		Class& Range(float min, float max) { Current().min = min; Current().max = max; return *this; }
		Class& Spawnable(std::function<std::shared_ptr<Actor>(World&)> factory)
		{
			static_assert(std::is_base_of_v<Actor, T>, "Only actors can be spawnable");
			mType.create = std::move(factory);
			mType.spawnable = static_cast<bool>(mType.create);
			return *this;
		}
		Class& Spawnable()
		{
			static_assert(std::is_constructible_v<T, World*>, "Provide a factory for actors with constructor arguments");
			return Spawnable([](World& world) { return world.SpawnActor<T>().lock(); });
		}
		Class& Speed(float speed) { Current().speed = speed; return *this; }
		Class& Step(float step) { return Speed(step); }
		Class& Tooltip(const std::string& tooltip) { Current().tooltip = tooltip; return *this; }
		Class& Category(const std::string& category) { Current().category = category; return *this; }
		Class& Labels(EnumLabels labels) { Current().enumLabels = std::move(labels); return *this; }

	private:
		static const TypeInfo* BaseType()
		{
			if constexpr (std::is_void_v<Base>) return nullptr;
			else
			{
				static_assert(std::is_base_of_v<Base, T>, "Reflection base must be a C++ base");
				const auto* base = TypeRegistry::Get().Find(typeid(Base));
				if (!base) throw std::logic_error("Register reflection bases before derived types");
				return base;
			}
		}
		static constexpr TypeRoot RootKind()
		{
			if constexpr (std::is_base_of_v<Actor, T>) return TypeRoot::Actor;
			else if constexpr (std::is_base_of_v<ActorComponent, T>) return TypeRoot::Component;
			else return TypeRoot::Other;
		}
		template<typename V> static constexpr PropertyKind Kind()
		{
			if constexpr (std::is_enum_v<V>) return PropertyKind::Enum;
			else if constexpr (std::is_same_v<V, bool>) return PropertyKind::Bool;
			else if constexpr (std::is_same_v<V, int>) return PropertyKind::Int;
			else if constexpr (std::is_same_v<V, float>) return PropertyKind::Float;
			else if constexpr (std::is_same_v<V, sf::Vector2f>) return PropertyKind::Vector2;
			else if constexpr (std::is_same_v<V, sf::Color>) return PropertyKind::Color;
			else { static_assert(std::is_same_v<V, std::string>, "Unsupported reflected value"); return PropertyKind::String; }
		}
		PropertyInfo& Current() { return mType.properties.at(mProperty); }
		TypeInfo& mType;
		std::size_t mProperty = 0;
	};

	void RegisterEngineTypes();
}
