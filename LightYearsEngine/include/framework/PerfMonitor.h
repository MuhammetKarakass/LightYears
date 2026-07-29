#pragma once
#include "framework/debug/Log.h"
#include "framework/debug/Profiler.h"

#include <atomic>

namespace ly { namespace perf {

inline std::atomic<int> g_activeActors{0};
inline std::atomic<int> g_bullets{0};
inline std::atomic<int> g_particles{0};

inline float g_reportTimer =0.0f;

// Testing flag to disable expensive light rendering
inline std::atomic<bool> g_disableLights{false};

inline void IncActiveActors() { g_activeActors.fetch_add(1, std::memory_order_relaxed); }
inline void DecActiveActors() { g_activeActors.fetch_sub(1, std::memory_order_relaxed); }
inline void IncBullets() { g_bullets.fetch_add(1, std::memory_order_relaxed); }
inline void DecBullets() { g_bullets.fetch_sub(1, std::memory_order_relaxed); }
inline void IncParticles() { g_particles.fetch_add(1, std::memory_order_relaxed); }
inline void DecParticles() { g_particles.fetch_sub(1, std::memory_order_relaxed); }

inline void TickAndReport(float deltaTime)
{
 g_reportTimer += deltaTime;
 if (g_reportTimer >=5.0f)
 {
 g_reportTimer =0.0f;
 const int activeActors = g_activeActors.load();
 const int bullets = g_bullets.load();
 const int particles = g_particles.load();
 LY_PROFILE_COUNTER("Actors", activeActors);
 LY_PROFILE_COUNTER("Bullets", bullets);
 LY_PROFILE_COUNTER("Particles", particles);
 LY_CORE_INFO(
  "PerfReport: Actors=%d Bullets=%d Particles=%d",
  activeActors,
  bullets,
  particles
 );
 }
}

}} // namespace ly::perf
