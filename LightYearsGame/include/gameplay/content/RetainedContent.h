#pragma once
#include <vector>
#include <utility>

namespace vega::content
{
	// Runtime objects can keep raw definition pointers. Retain replaced generations
	// for the process lifetime so reloading only changes future catalog lookups.
	template <typename T>
	void RetainContent(std::vector<T>& current)
	{
		static std::vector<std::vector<T>> generations;
		if (!current.empty()) generations.push_back(std::move(current));
		current = {};
	}
}
