#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include "framework/World.h"
#include "framework/ActorRef.h"
#include "framework/reflection/AssetKind.h"
#include "framework/reflection/ReflectionDeclare.h"
#include <functional>
#include <memory>
#include <map>
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
	// Struct fields are leaves in v1: a struct cannot hold another struct or a list.
	using LeafValue = std::variant<bool, int, float, sf::Vector2f, sf::Color, std::string, ly::ActorRef>;
	// Ordered field name -> value. Missing fields keep the struct's default; unknown names are ignored.
	using StructValue = std::vector<std::pair<std::string, LeafValue>>;
	using ScalarValue = std::variant<bool, int, float, sf::Vector2f, sf::Color, std::string, ly::ActorRef, StructValue>;
	using PropertyList = std::vector<ScalarValue>;
	using PropertyValue = std::variant<bool, int, float, sf::Vector2f, sf::Color, std::string, ly::ActorRef, StructValue, PropertyList>;
	enum class PropertyKind { Bool, Int, Float, Vector2, Color, String, Enum, Flags, ActorRef, List, Struct };
	template<typename T> struct ListTraits { static constexpr bool value = false; };
	template<typename T> struct ListTraits<std::vector<T>> { static constexpr bool value = true; using Element = T; };
	inline PropertyValue ToValue(const ScalarValue& value)
	{
		return std::visit([](const auto& item) -> PropertyValue { return item; }, value);
	}
	inline ScalarValue ToScalar(const PropertyValue& value)
	{
		return std::visit([](const auto& item) -> ScalarValue
		{
			if constexpr (std::is_same_v<std::decay_t<decltype(item)>, PropertyList>) throw std::logic_error("Nested lists are unsupported");
			else return item;
		}, value);
	}
	// A struct field is a leaf, so its conversion can never see a struct or a list.
	inline PropertyValue ToPropertyValue(const LeafValue& value)
	{
		return std::visit([](const auto& item) -> PropertyValue { return item; }, value);
	}
	inline LeafValue ToLeaf(const PropertyValue& value)
	{
		return std::visit([](const auto& item) -> LeafValue
		{
			using T = std::decay_t<decltype(item)>;
			if constexpr (std::is_same_v<T, StructValue> || std::is_same_v<T, PropertyList>) throw std::logic_error("Struct fields must be leaf values");
			else return item;
		}, value);
	}
	// The first six kinds describe the variant alternatives; enums and flags use int.
	static_assert(static_cast<std::size_t>(PropertyKind::Bool) == 0 && std::is_same_v<std::variant_alternative_t<0, PropertyValue>, bool>);
	static_assert(static_cast<std::size_t>(PropertyKind::Int) == 1 && std::is_same_v<std::variant_alternative_t<1, PropertyValue>, int>);
	static_assert(static_cast<std::size_t>(PropertyKind::Float) == 2 && std::is_same_v<std::variant_alternative_t<2, PropertyValue>, float>);
	static_assert(static_cast<std::size_t>(PropertyKind::Vector2) == 3 && std::is_same_v<std::variant_alternative_t<3, PropertyValue>, sf::Vector2f>);
	static_assert(static_cast<std::size_t>(PropertyKind::Color) == 4 && std::is_same_v<std::variant_alternative_t<4, PropertyValue>, sf::Color>);
	static_assert(static_cast<std::size_t>(PropertyKind::String) == 5 && std::is_same_v<std::variant_alternative_t<5, PropertyValue>, std::string>);
	static_assert(std::is_same_v<std::variant_alternative_t<7, PropertyValue>, StructValue> && std::is_same_v<std::variant_alternative_t<8, PropertyValue>, PropertyList>);
	static_assert(static_cast<std::size_t>(PropertyKind::Struct) == 10);
	inline PropertyKind KindOf(const PropertyValue& value)
	{
		return value.index() == 6 ? PropertyKind::ActorRef : value.index() == 7 ? PropertyKind::Struct :
			value.index() == 8 ? PropertyKind::List : static_cast<PropertyKind>(value.index());
	}
	using EnumLabels = std::vector<std::pair<int, std::string>>;
	struct TypeInfo;

	struct PropertyInfo
	{
		std::string name;
		PropertyKind kind = PropertyKind::Bool;
		std::function<PropertyValue(const void*)> getter;
		std::function<void(void*, const PropertyValue&)> setter;
		std::function<World*(const void*)> world;
		std::optional<float> min;
		std::optional<float> max;
		// Set by fields, which have no setter to enforce Range; read at write time so Range may follow Field.
		bool clampToRange = false;
		float speed = 1.f;
		std::string tooltip;
		EnumLabels enumLabels;
		// Read-only compatibility spellings never appear in Inspector choices or saves.
		EnumLabels flagAliases;
		bool ParseFlags(const std::string& text, int& bits, bool* legacy = nullptr) const;
		std::string category;
		PropertyKind elementKind = PropertyKind::Bool;
		std::string requiredType;
		// Registered struct of a Struct property or of list elements. The name is for tools and the
		// golden; structInfo supplies the field rules that validation applies to each struct value.
		std::string structType;
		const TypeInfo* structInfo = nullptr;
		// Editor metadata only: a String property whose value is an asset path of this kind.
		AssetKind assetKind = AssetKind::None;
		PropertyInfo Element() const
		{
			auto element = *this;
			element.kind = elementKind;
			return element;
		}
		bool ValidateValue(const PropertyValue& value, std::string* error = nullptr) const;
		// Bind weak caches after the entire scene exists, including forward references.
		bool ResolveReferences(void* object, World& world, std::string* error = nullptr) const;

		// All erased object pointers are pointers to the registered root, even for
		// inherited properties. This preserves pointer adjustment with multiple inheritance.
		bool SetValue(void* objectOfRoot, const PropertyValue& value, std::string* error = nullptr) const;
	};

	// A zero-argument member function an editor may call, the analogue of an Unreal
	// UFUNCTION(CallInEditor). invoke receives a pointer to the registered root so the
	// same pointer adjustment as property access applies; it throws whatever the function throws.
	struct FunctionInfo
	{
		// Identifier used by editor scripts and lookups; label is the Inspector button text.
		std::string name;
		std::string label;
		std::string tooltip;
		// False keeps the function edit-mode only; true lets the Inspector call it while playing.
		bool callableInPlay = false;
		std::function<void(void*)> invoke;
	};
	// Visits every actor reference in value, including references inside structs and lists, with the
	// requiredType that applies to it. The visitor may rewrite the reference it receives.
	void ForEachActorRef(const PropertyInfo& property, PropertyValue& value,
		const std::function<void(ly::ActorRef&, const std::string&)>& visit);
	// Reads each writable field of a registered struct in declaration order.
	StructValue ReadStruct(const TypeInfo& type, const void* object);
	// Applies the known writable fields to object, which must be a default-constructed struct, so fields
	// absent from value keep their default. Field errors are thrown as "Field: reason".
	void WriteStruct(const TypeInfo& type, void* object, const StructValue& value);

	enum class TypeRoot { Actor, Component, Other };
	struct ConstructorComponentInventory
	{
		std::map<std::string, std::size_t> counts;
		std::vector<std::string> order;
	};
	struct TypeInfo
	{
		// Cached constructor ownership belongs to this registry generation.
		mutable std::optional<ConstructorComponentInventory> constructorComponents;
		std::string name;
		const TypeInfo* base = nullptr;
		TypeRoot root = TypeRoot::Other;
		std::function<bool(const void*)> isInstance;
		std::vector<PropertyInfo> properties;
		// Editor-callable actions declared on this type; inherited like properties.
		std::vector<FunctionInfo> functions;
		std::function<std::shared_ptr<Actor>(World&)> create;
		bool spawnable = false;
		std::function<ActorComponent&(Actor&)> addComponent;
		bool removable = false;
		std::vector<std::string> requiredComponents;
		// Reflected structs only: the field values of a default-constructed struct. Editors use it for a new list element.
		std::function<StructValue()> makeDefault;
		bool RequiresComponent(const std::string& component) const
		{
			for (const auto& required : requiredComponents) if (required == component) return true;
			return base && base->RequiresComponent(component);
		}
		std::vector<const PropertyInfo*> AllProperties() const;
		const PropertyInfo* FindProperty(const std::string& propertyName) const;
		// Functions follow the same inheritance order as properties: base first, local last.
		std::vector<const FunctionInfo*> AllFunctions() const;
		const FunctionInfo* FindFunction(const std::string& functionName) const;
	};

	class TypeRegistry
	{
	public:
		TypeRegistry() = default;
		void SetCollisionLabels(const EnumLabels& labels, const EnumLabels& aliases);
		// Deprecated: use the owning EngineContext::Types().
		static TypeRegistry& Get();
		TypeInfo& Register(std::type_index type, const std::string& name, const TypeInfo* base,
			TypeRoot root, std::function<bool(const void*)> isInstance);
		const TypeInfo* Find(std::type_index type) const;
		const TypeInfo* Find(const std::string& name) const;
		const TypeInfo* FindMostDerived(const Actor& actor) const;
		const TypeInfo* FindMostDerived(const ActorComponent& component) const;
		std::vector<const TypeInfo*> RegisteredTypes() const
		{
			std::vector<const TypeInfo*> result;
			for (const auto& entry : mTypes) result.push_back(entry.second.get());
			return result;
		}
		std::vector<const TypeInfo*> SpawnableActors() const
		{
			std::vector<const TypeInfo*> result;
			for (const auto& entry : mTypes)
				if (entry.second->root == TypeRoot::Actor && entry.second->spawnable && entry.second->create)
					result.push_back(entry.second.get());
			return result;
		}

		std::vector<const TypeInfo*> AddableComponents() const
		{
			std::vector<const TypeInfo*> result;
			for (const auto& entry : mTypes)
				if (entry.second->root == TypeRoot::Component && entry.second->addComponent)
					result.push_back(entry.second.get());
			return result;
		}

	private:
		friend void RegisterEngineTypes(TypeRegistry& registry);
		bool mEngineTypesRegistered = false;
		TypeRegistry(const TypeRegistry&) = delete;
		TypeRegistry& operator=(const TypeRegistry&) = delete;
		const TypeInfo* FindMostDerived(const void* object, TypeRoot root) const;
		std::vector<std::pair<std::type_index, std::unique_ptr<TypeInfo>>> mTypes;
	};

	// VEGA_STRUCT declares the hidden friend VegaStructTag; ADL on TypeTag<T> finds it only for reflected structs.
	template<typename T, typename = void> struct IsReflectedStruct : std::false_type {};
	template<typename T> struct IsReflectedStruct<T, std::void_t<decltype(VegaStructTag(TypeTag<T>{}))>> : std::true_type {};
	// Struct conversions look up the struct's TypeInfo at call time through the owner's registry.
	template<typename V> const TypeInfo& RegisteredStruct(const TypeRegistry& registry)
	{
		const auto* info = registry.Find(typeid(V));
		if (!info) throw std::logic_error(std::string{ "Register struct types before their users: " } + typeid(V).name());
		return *info;
	}
	template<typename V> StructValue StructToValue(const TypeRegistry& registry, const V& item)
	{
		return ReadStruct(RegisteredStruct<V>(registry), &item);
	}
	template<typename V> V StructFromValue(const TypeRegistry& registry, const StructValue& value)
	{
		V item{};
		WriteStruct(RegisteredStruct<V>(registry), &item, value);
		return item;
	}
	// The element type of a list, or the type itself for a scalar.
	template<typename V, bool = ListTraits<V>::value> struct ItemOf { using type = V; };
	template<typename V> struct ItemOf<V, true> { using type = typename ListTraits<V>::Element; };

	template<typename T, typename Base = void>
	class Class
	{
		using Root = std::conditional_t<std::is_base_of_v<Actor, T>, Actor,
			std::conditional_t<std::is_base_of_v<ActorComponent, T>, ActorComponent, T>>;
	public:
		explicit Class(const std::string& name, TypeRegistry& registry = CurrentTypeRegistry())
			: mType{ registry.Register(typeid(T), name, BaseType(registry), RootKind(),
				[](const void* object) { return object && dynamic_cast<const T*>(static_cast<const Root*>(object)); }) }, mRegistry{ &registry }
		{
			// A new list element starts from the same defaults as a freshly constructed struct.
			if constexpr (IsReflectedStruct<T>::value && std::is_default_constructible_v<T>)
				mType.makeDefault = [owner = &registry] { return StructToValue<T>(*owner, T{}); };
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
			property.category = mDefaultCategory;
			property.kind = Kind<Value>();
			if constexpr (std::is_base_of_v<Actor, T> || std::is_base_of_v<ActorComponent, T>)
				property.world = [](const void* object) { return dynamic_cast<const T*>(static_cast<const Root*>(object))->GetWorld(); };
			if constexpr (ListTraits<Value>::value) property.elementKind = Kind<typename ListTraits<Value>::Element>();
			// A registered struct must be registered before any property that uses it, as with bases.
			if constexpr (IsReflectedStruct<typename ItemOf<Value>::type>::value)
			{
				property.structInfo = &RegisteredStruct<typename ItemOf<Value>::type>(*mRegistry);
				property.structType = property.structInfo->name;
			}
			// v1 keeps struct fields to leaves; the error names the property so the broken field is easy to find.
			if constexpr (IsReflectedStruct<T>::value)
				if (property.kind == PropertyKind::Struct || property.kind == PropertyKind::List)
					throw std::logic_error("Struct field must be a leaf value: " + name);
			property.getter = [getter, registry = mRegistry](const void* object) -> PropertyValue
			{
				const T& instance = *dynamic_cast<const T*>(static_cast<const Root*>(object));
				if constexpr (std::is_enum_v<Value>) return static_cast<int>(std::invoke(getter, instance));
				else if constexpr (ListTraits<Value>::value)
				{
					PropertyList list;
					for (const auto& item : std::invoke(getter, instance))
					{
						using E = typename ListTraits<Value>::Element;
						if constexpr (std::is_enum_v<E>) list.emplace_back(static_cast<int>(item));
						else if constexpr (IsReflectedStruct<E>::value) list.emplace_back(StructToValue<E>(*registry, item));
						else list.emplace_back(static_cast<E>(item));
					}
					return list;
				}
				else if constexpr (IsReflectedStruct<Value>::value) return StructToValue<Value>(*registry, std::invoke(getter, instance));
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
			Current().setter = [setter, registry = mRegistry](void* object, const PropertyValue& value)
			{
				T& instance = *dynamic_cast<T*>(static_cast<Root*>(object));
				if constexpr (std::is_enum_v<Value>) std::invoke(setter, instance, static_cast<Value>(std::get<int>(value)));
				else if constexpr (ListTraits<Value>::value)
				{
					Value list;
					using E = typename ListTraits<Value>::Element;
					for (const auto& item : std::get<PropertyList>(value))
					{
						if constexpr (std::is_enum_v<E>) list.push_back(static_cast<E>(std::get<int>(item)));
						else if constexpr (IsReflectedStruct<E>::value) list.push_back(StructFromValue<E>(*registry, std::get<StructValue>(item)));
						else list.push_back(std::get<E>(item));
					}
					std::invoke(setter, instance, list);
				}
				else if constexpr (IsReflectedStruct<Value>::value) std::invoke(setter, instance, StructFromValue<Value>(*registry, std::get<StructValue>(value)));
				else std::invoke(setter, instance, std::get<Value>(value));
			};
			return *this;
		}

		// A field binds a data member directly; its generated setter has no domain rules, so SetValue
		// clamps numeric values to Range when the flag is set.
		template<typename V>
		Class& Field(const std::string& name, V T::*member)
		{
			Property(name, [member](const T& instance) -> V { return instance.*member; },
				[member](T& instance, const V& value) { instance.*member = value; });
			Current().clampToRange = true;
			return *this;
		}
		// The hook runs after the value is stored, once per successful write.
		template<typename V, typename Hook>
		Class& Field(const std::string& name, V T::*member, Hook onChanged)
		{
			// Wrapped in std::function because the setter is invoked through a const reference.
			std::function<void(T&)> notify = [onChanged](T& instance) mutable { std::invoke(onChanged, instance); };
			Property(name, [member](const T& instance) -> V { return instance.*member; },
				[member, notify](T& instance, const V& value) { instance.*member = value; notify(instance); });
			Current().clampToRange = true;
			return *this;
		}
		template<typename V>
		Class& FieldReadOnly(const std::string& name, V T::*member)
		{
			Property(name, [member](const T& instance) -> V { return instance.*member; });
			return *this;
		}
		// Configures the function most recently registered by Function. A nested class so it can
		// reach the builder's private state without exposing it more broadly.
		class FunctionBuilder
		{
		public:
			explicit FunctionBuilder(Class& owner) : mOwner{ &owner } {}
			FunctionBuilder& Tooltip(const std::string& text) { mOwner->LastFunction().tooltip = text; return *this; }
			FunctionBuilder& CallableInPlay(bool callable = true) { mOwner->LastFunction().callableInPlay = callable; return *this; }
		private:
			Class* mOwner = nullptr;
		};
		// Registers a zero-argument member function an editor can call. name identifies it for scripts;
		// label is shown on the Inspector button. Chain Tooltip and CallableInPlay on the result.
		FunctionBuilder Function(const std::string& name, const std::string& label, void (T::*method)())
		{
			FunctionInfo function;
			function.name = name;
			function.label = label;
			function.invoke = [method](void* object) { ((dynamic_cast<T*>(static_cast<Root*>(object)))->*method)(); };
			mType.functions.push_back(std::move(function));
			return FunctionBuilder{ *this };
		}
		// Convenience overload where the identifier doubles as the button label.
		FunctionBuilder Function(const std::string& label, void (T::*method)())
		{
			return Function(label, label, method);
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
		Class& Addable(std::function<ActorComponent&(Actor&)> factory)
		{
			static_assert(std::is_base_of_v<ActorComponent, T>, "Only components can be addable");
			mType.addComponent = std::move(factory);
			mType.removable = true;
			return *this;
		}
		Class& Addable()
		{
			static_assert(std::is_constructible_v<T, Actor&>, "Provide a factory for component constructor arguments");
			return Addable([](Actor& actor) -> ActorComponent& { return actor.AddComponent<T>(); });
		}
		Class& Removable(bool removable = true) { mType.removable = removable; return *this; }
		Class& RequiresComponent(const std::string& component)
		{
			static_assert(std::is_base_of_v<Actor, T>, "Only actors can require components");
			if (!mType.RequiresComponent(component)) mType.requiredComponents.push_back(component);
			return *this;
		}
		Class& Speed(float speed) { Current().speed = speed; return *this; }
		Class& Step(float step) { return Speed(step); }
		Class& Tooltip(const std::string& tooltip) { Current().tooltip = tooltip; return *this; }
		Class& Category(const std::string& category) { Current().category = category; return *this; }
		Class& DefaultCategory(std::string category) { mDefaultCategory = std::move(category); return *this; }
		Class& Labels(EnumLabels labels) { Current().enumLabels = std::move(labels); return *this; }
		Class& RequiredType(const std::string& name) { Current().requiredType = name; return *this; }
		Class& Flags(EnumLabels labels) { Current().kind = PropertyKind::Flags; Current().enumLabels = std::move(labels); return *this; }
		// Marks a String property as an asset path. Throws std::logic_error at registration for any
		// other kind, so an asset picker can never be attached to a value that is not a path.
		Class& Asset(AssetKind kind)
		{
			if (kind == AssetKind::None) throw std::logic_error("Asset kind must not be None: " + Current().name);
			if (Current().kind != PropertyKind::String) throw std::logic_error("Asset kind requires a string property: " + Current().name);
			Current().assetKind = kind;
			return *this;
		}

	private:
		static const TypeInfo* BaseType(TypeRegistry& registry)
		{
			if constexpr (std::is_void_v<Base>) return nullptr;
			else
			{
				static_assert(std::is_base_of_v<Base, T>, "Reflection base must be a C++ base");
				const auto* base = registry.Find(typeid(Base));
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
			else if constexpr (ListTraits<V>::value) return PropertyKind::List;
			else if constexpr (IsReflectedStruct<V>::value) return PropertyKind::Struct;
			else if constexpr (std::is_same_v<V, ly::ActorRef>) return PropertyKind::ActorRef;
			else if constexpr (std::is_same_v<V, bool>) return PropertyKind::Bool;
			else if constexpr (std::is_same_v<V, int>) return PropertyKind::Int;
			else if constexpr (std::is_same_v<V, float>) return PropertyKind::Float;
			else if constexpr (std::is_same_v<V, sf::Vector2f>) return PropertyKind::Vector2;
			else if constexpr (std::is_same_v<V, sf::Color>) return PropertyKind::Color;
			else { static_assert(std::is_same_v<V, std::string>, "Unsupported reflected value"); return PropertyKind::String; }
		}
		PropertyInfo& Current() { return mType.properties.at(mProperty); }
		FunctionInfo& LastFunction() { return mType.functions.back(); }
		TypeInfo& mType;
		// Owner of the types this builder describes; struct lookups use it when values are converted.
		TypeRegistry* mRegistry = nullptr;
		std::string mDefaultCategory;
		std::size_t mProperty = 0;
	};

	void RegisterEngineTypes();
	void RegisterEngineTypes(TypeRegistry& registry);
}

#include "framework/reflection/ReflectionMacros.h"
