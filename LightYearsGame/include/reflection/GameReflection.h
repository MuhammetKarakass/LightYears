#pragma once

namespace vega
{
	class EngineContext;
	namespace reflect { class TypeRegistry; }

	// Reflected game types only; repeated calls into the same registry are no-ops.
	void RegisterGameTypes(reflect::TypeRegistry& registry);
	// Scene spawn factories for catalog-driven actors; they live on the context, not the registry.
	void RegisterGameSpawnFactories(EngineContext& context);
	// Both of the above for one context, as scene and prefab loading need them together.
	void RegisterGameSceneTypes(EngineContext& context);
}
