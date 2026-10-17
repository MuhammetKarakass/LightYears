#pragma once

#include "framework/reflection/ReflectionDeclare.h"

// Declare reflected classes with VEGA_CLASS in their headers, describe them with
// VEGA_REFLECT in their .cpp files, and call VEGA_REGISTER from the module's
// registration list. ADL finds the hidden friend through TypeTag<Type>; each
// registration file must include the registered type's header. Keep bases
// before derived types because Class resolves its base during construction.
// Explicit calls retain registration code from static libraries that the linker
// could otherwise discard. Property names are persistent scene/prefab keys and
// must not be renamed.
#define VEGA_REFLECT(Type, Base) \
	struct VegaReflection_##Type \
	{ \
		using Self = Type; \
		using Builder = ::ly::reflect::Class<Type, Base>; \
		static void Describe(Builder& type); \
	}; \
	void VegaReflect(::ly::reflect::TypeTag<Type>) \
	{ \
		VegaReflect(::ly::reflect::TypeTag<Type>{}, ::ly::reflect::CurrentTypeRegistry()); \
	} \
	void VegaReflect(::ly::reflect::TypeTag<Type>, ::ly::reflect::TypeRegistry& registry) \
	{ \
		VegaReflection_##Type::Builder type{ #Type, registry }; \
		VegaReflection_##Type::Describe(type); \
	} \
	void VegaReflection_##Type::Describe(Builder& type)

#define VEGA_REFLECT_ROOT(Type) VEGA_REFLECT(Type, void)
#define VEGA_REGISTER(Type) VegaReflect(::ly::reflect::TypeTag<Type>{})
#define VEGA_REGISTER_IN(Type, Registry) VegaReflect(::ly::reflect::TypeTag<Type>{}, Registry)

// Calls through lambdas preserve overloaded/default-argument setter semantics.
#define VEGA_PROPERTY(Name, Getter, Setter) \
	type.Property(Name, [](const Self& o) { return o.Getter(); }, \
		[](Self& o, const auto& v) { o.Setter(v); })
#define VEGA_READONLY(Name, Getter) \
	type.Property(Name, [](const Self& o) { return o.Getter(); })
#define VEGA_ACTOR_REF(Name, Getter, Setter) VEGA_PROPERTY(Name, Getter, Setter)
#define VEGA_LIST(Name, Getter, Setter) VEGA_PROPERTY(Name, Getter, Setter)
// Fields bind a private or public data member directly, so no accessor pair is written.
// Chain .Range() to clamp numeric values on write; NOTIFY runs its hook after each stored write.
#define VEGA_FIELD(Name, Member) type.Field(Name, &Self::Member)
#define VEGA_FIELD_NOTIFY(Name, Member, OnChanged) type.Field(Name, &Self::Member, &Self::OnChanged)
#define VEGA_FIELD_READONLY(Name, Member) type.FieldReadOnly(Name, &Self::Member)
#define VEGA_CATEGORY(Name) type.DefaultCategory(Name)
#define VEGA_SPAWNABLE(...) type.Spawnable(__VA_ARGS__)
#define VEGA_ADDABLE(...) \
	type.Addable([](::ly::Actor& a) -> ::ly::ActorComponent& { return a.AddComponent<Self>(__VA_ARGS__); })
#define VEGA_REMOVABLE() type.Removable()
#define VEGA_REQUIRES(ComponentName) type.RequiresComponent(#ComponentName)
