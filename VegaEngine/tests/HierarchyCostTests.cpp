#include "framework/EngineContext.h"
#include <framework/Actor.h>
#include <framework/World.h>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <vector>

void Measure(bool hierarchy)
{
	vega::World world{ nullptr };
	auto parent = hierarchy ? world.SpawnActor<vega::Actor>().lock() : nullptr;
	std::vector<std::shared_ptr<vega::Actor>> actors;
	for (int i = 0; i < 500; ++i)
	{
		auto actor = world.SpawnActor<vega::Actor>().lock();
		actor->SetActorLocation({ static_cast<float>(i), 0.f });
		actors.push_back(actor);
		if (parent) actor->AttachTo(parent.get());
	}
	world.BeginPlayInternal();
	world.TickInternal(0.f);
	std::vector<double> samples;
	for (int batch = 0; batch < 11; ++batch)
	{
		const auto start = std::chrono::steady_clock::now();
		for (int frame = 0; frame < 2000; ++frame)
		{
			(parent ? parent : actors.front())->SetActorLocation({ static_cast<float>(frame), 0.f });
			world.TickInternal(1.f / 60.f);
		}
		const auto end = std::chrono::steady_clock::now();
		if (batch) samples.push_back(std::chrono::duration<double, std::micro>(end - start).count() / 2000.);
	}
	std::sort(samples.begin(), samples.end());
	std::cout << (hierarchy ? "500 parented actors + root" : "500 flat actors") << ", median headless frame us: " << samples[samples.size() / 2] << '\n';
}

int main()
{
	vega::EngineContext engineContext;
	vega::EngineContext::Scope contextScope{ engineContext };
	Measure(false);
	Measure(true);
}
