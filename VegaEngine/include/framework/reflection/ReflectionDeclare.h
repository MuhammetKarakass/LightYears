#pragma once

namespace vega::reflect
{
	class TypeRegistry;
	TypeRegistry& CurrentTypeRegistry(); // Legacy registration boundary.
	// Carries a type through argument-dependent lookup; it is never instantiated with data.
	template<typename T>
	struct TypeTag
	{
	};
	// Return type of the hidden friend VEGA_STRUCT declares. The friend is never called; its presence marks a struct.
	struct StructTag
	{
	};
}

// Marks a class as reflected, like Unreal's GENERATED_BODY(). Put it first in the class body
// and describe the class with VEGA_REFLECT in its .cpp. It declares a hidden friend that
// VEGA_REGISTER finds through argument-dependent lookup, so the registration list needs no
// forward declarations and works from any namespace. TypeTag<T> does not convert to a base
// tag, so forgetting VEGA_CLASS is a compile error instead of silently registering the base.
// The describing struct is a friend so VEGA_FIELD can bind private data members.
#define VEGA_CLASS(Type) \
	friend void VegaReflect(::vega::reflect::TypeTag<Type>); \
	friend void VegaReflect(::vega::reflect::TypeTag<Type>, ::vega::reflect::TypeRegistry&); \
	friend struct VegaReflection_##Type;

// Marks a plain struct as reflected, the struct counterpart of VEGA_CLASS. Put it first in the struct body.
// It declares the hidden friend that Class<T> detects by ADL, so an undeclared struct is never treated as one.
#define VEGA_STRUCT(Type) \
	VEGA_CLASS(Type) \
	friend ::vega::reflect::StructTag VegaStructTag(::vega::reflect::TypeTag<Type>);
