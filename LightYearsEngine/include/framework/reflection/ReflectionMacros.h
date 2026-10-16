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
		VegaReflection_##Type::Builder type{ #Type }; \
		VegaReflection_##Type::Describe(type); \
	} \
	void VegaReflection_##Type::Describe(Builder& type)

#define VEGA_REFLECT_ROOT(Type) VEGA_REFLECT(Type, void)
#define VEGA_REGISTER(Type) VegaReflect(::ly::reflect::TypeTag<Type>{})

// Calls through lambdas preserve overloaded/default-argument setter semantics.
#define VEGA_PROPERTY(Name, Getter, Setter) \
	type.Property(Name, [](const Self& o) { return o.Getter(); }, \
		[](Self& o, const auto& v) { o.Setter(v); })
#define VEGA_READONLY(Name, Getter) \
	type.Property(Name, [](const Self& o) { return o.Getter(); })
#define VEGA_CATEGORY(Name) type.DefaultCategory(Name)
#define VEGA_SPAWNABLE(...) type.Spawnable(__VA_ARGS__)
#define VEGA_ADDABLE(...) \
	type.Addable([](::ly::Actor& a) -> ::ly::ActorComponent& { return a.AddComponent<Self>(__VA_ARGS__); })
#define VEGA_REMOVABLE() type.Removable()
#define VEGA_REQUIRES(ComponentName) type.RequiresComponent(#ComponentName)
