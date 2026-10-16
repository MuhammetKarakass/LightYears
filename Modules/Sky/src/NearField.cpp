#include "sky/NearField.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include "sky/ColorRamp.h"

namespace vega::sky
{
	namespace
	{
		constexpr float kTwoPi = 6.28318530718f;
		// The outline is ramp stop 0 scaled by this: a one-art-pixel edge darker than the shadow band.
		constexpr float kOutlineScale = 0.45f;
		// Unlit debris: share of each channel pulled toward luma, and the brightness cut as a share of the cap.
		constexpr float kDesaturation = 0.35f;
		constexpr float kDebrisDarkening = 0.55f;
		// Dot of a pixel's outward direction with the light: below the shadow threshold is the dark band, above the lit
		// threshold is the lit band, and the band between is the mid tone.
		constexpr float kShadowDot = -0.2f;
		constexpr float kLitDot = 0.4f;
		constexpr float kRockDrift = 4.f;      // output pixels per second, at most
		constexpr float kMoteDepth = 1.6f;     // motes move this much faster than rocks per unit parallax
		constexpr float kMoteWiggle = 2.f;     // art pixels of sine drift
		constexpr float kRockReach = 1.3f;     // largest outline vertex, fraction of the radius
		// Rocks are at most 13 art pixels and outline vertices reach 1.3 of that, so a 36 box always holds one.
		constexpr int kMaxSpan = 36;
		// Scratch vertex capacity: the reused buffer settles here after the first frame.
		constexpr std::size_t kVertexReserve = 16384;

		// splitmix64: the same numbers on every platform and standard library.
		class Random
		{
		public:
			explicit Random(std::uint64_t seed)
				: mState(seed)
			{
			}

			// Uniform in [0, 1).
			float Unit()
			{
				mState += 0x9e3779b97f4a7c15ULL;
				std::uint64_t z = mState;
				z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
				z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
				z ^= z >> 31;
				return static_cast<float>(z >> 40) / 16777216.f;
			}

		private:
			std::uint64_t mState;
		};

		sf::Color Scale(sf::Color color, float k)
		{
			auto channel = [k](std::uint8_t c) { return static_cast<std::uint8_t>(std::clamp(static_cast<float>(c) * k, 0.f, 255.f)); };
			return sf::Color(channel(color.r), channel(color.g), channel(color.b), color.a);
		}

		// Unlit debris tone: pulled toward luma, then limited to a share of the background cap.
		sf::Color DebrisTone(sf::Color color, float cap)
		{
			const float luma = 0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b;
			auto channel = [luma](std::uint8_t c) { return static_cast<std::uint8_t>(std::clamp(c + (luma - c) * kDesaturation, 0.f, 255.f)); };
			return LimitBrightness(sf::Color(channel(color.r), channel(color.g), channel(color.b), color.a), cap * kDebrisDarkening);
		}

		// Stop `index` of a ramp with at least one stop; short ramps reuse their last stop.
		sf::Color StopAt(const ColorRamp& ramp, std::size_t index)
		{
			const std::vector<ColorStop>& stops = ramp.Stops();
			return stops[std::min(index, stops.size() - 1)].color;
		}

		float Wrap(float value, float size)
		{
			const float r = std::fmod(value, size);
			return r < 0.f ? r + size : r;
		}

		// Snaps a point to the art-pixel grid so the edges are stepped like the ships.
		sf::Vector2f Snap(sf::Vector2f v, float pixel)
		{
			return sf::Vector2f(std::round(v.x / pixel) * pixel, std::round(v.y / pixel) * pixel);
		}

		sf::Vector2f Normalized(sf::Vector2f v)
		{
			const float length = std::sqrt(v.x * v.x + v.y * v.y);
			return length > 0.f ? v / length : sf::Vector2f(0.f, -1.f);
		}

		float Dot(sf::Vector2f a, sf::Vector2f b)
		{
			return a.x * b.x + a.y * b.y;
		}

		// Even-odd test of a point against a polygon with n vertices.
		bool Inside(const std::array<sf::Vector2f, 10>& poly, int n, sf::Vector2f p)
		{
			bool inside = false;
			for (int j = 0, k = n - 1; j < n; k = j, ++j)
			{
				const sf::Vector2f a = poly[static_cast<std::size_t>(j)];
				const sf::Vector2f b = poly[static_cast<std::size_t>(k)];
				if ((a.y > p.y) != (b.y > p.y) && p.x < (b.x - a.x) * (p.y - a.y) / (b.y - a.y) + a.x)
				{
					inside = !inside;
				}
			}
			return inside;
		}

		// One art pixel square, two triangles.
		void PushSquare(std::vector<sf::Vertex>& out, sf::Vector2f origin, float size, sf::Color color)
		{
			const sf::Vector2f x(size, 0.f);
			const sf::Vector2f y(0.f, size);
			out.push_back({origin, color});
			out.push_back({origin + x, color});
			out.push_back({origin + y, color});
			out.push_back({origin + x, color});
			out.push_back({origin + x + y, color});
			out.push_back({origin + y, color});
		}

		// Built count to show for a density in [0, 2]: density 1 shows the nominal count (half the built ones).
		std::size_t Shown(std::size_t built, float density)
		{
			return std::min(built, static_cast<std::size_t>(std::lround(static_cast<float>(built) * density * 0.5f)));
		}

		// Calls emit with every copy of an element at pos (wrapped over the tile) that touches the target.
		template <typename Emit>
		void ForEachTiled(sf::Vector2f pos, float extent, sf::Vector2f tile, Emit&& emit)
		{
			const sf::Vector2f wrapped(Wrap(pos.x, tile.x), Wrap(pos.y, tile.y));
			for (int iy = -1; iy <= 1; ++iy)
			{
				for (int ix = -1; ix <= 1; ++ix)
				{
					const sf::Vector2f c = wrapped + sf::Vector2f(static_cast<float>(ix) * tile.x, static_cast<float>(iy) * tile.y);
					if (c.x + extent < 0.f || c.x - extent > tile.x || c.y + extent < 0.f || c.y - extent > tile.y)
					{
						continue;
					}
					emit(c);
				}
			}
		}
	}

	NearField::NearField(unsigned seed, int rockCount, int moteCount)
	{
		Random random(seed);

		const int rocks = std::max(rockCount, 0) * 2;
		mRocks.reserve(static_cast<std::size_t>(rocks));
		for (int i = 0; i < rocks; ++i)
		{
			Rock rock;
			rock.home.x = random.Unit();
			rock.home.y = random.Unit();
			rock.drift.x = (random.Unit() - 0.5f) * 2.f * kRockDrift;
			rock.drift.y = (random.Unit() - 0.5f) * 2.f * kRockDrift;
			rock.depth = 0.8f + 0.5f * random.Unit();
			// Small rocks are more common than large ones: 5 to 13 art pixels.
			const float u = random.Unit();
			rock.radius = 5.f + 8.f * u * u * u;
			rock.phase = random.Unit() * kTwoPi;
			rock.spin = (random.Unit() - 0.5f) * 0.25f;
			rock.shade = 0.8f + 0.2f * random.Unit();
			rock.vertexCount = 6 + static_cast<int>(random.Unit() * 5.f);
			const float step = kTwoPi / static_cast<float>(rock.vertexCount);
			for (int j = 0; j < rock.vertexCount; ++j)
			{
				const float angle = (static_cast<float>(j) + (random.Unit() - 0.5f) * 0.5f) * step;
				const float radius = 0.7f + 0.6f * random.Unit();
				rock.outline[static_cast<std::size_t>(j)] = {std::cos(angle) * radius, std::sin(angle) * radius};
			}
			mRocks.push_back(rock);
		}

		const int motes = std::max(moteCount, 0) * 2;
		mMotes.reserve(static_cast<std::size_t>(motes));
		for (int i = 0; i < motes; ++i)
		{
			Mote mote;
			mote.home.x = random.Unit();
			mote.home.y = random.Unit();
			mote.depth = 0.8f + 0.6f * random.Unit();
			mote.phase = random.Unit() * kTwoPi;
			mote.speed = 0.2f + 0.4f * random.Unit();
			mote.bright = random.Unit() < 0.3f;
			mote.alpha = 0.35f + 0.35f * random.Unit();
			mMotes.push_back(mote);
		}
	}

	void NearField::Draw(sf::RenderTarget& target, const Params& params) const
	{
		const sf::Vector2f tile(target.getSize());
		if (tile.x <= 0.f || tile.y <= 0.f)
		{
			return;
		}
		const float density = std::clamp(params.density, 0.f, 2.f);

		Frame frame;
		frame.pixel = std::max(params.pixelSize, 1.f);
		frame.time = params.timeSeconds;
		frame.cap = params.maxBrightness;
		frame.light = Normalized(params.lightDirection);
		frame.rockRamp = params.rockRamp;
		frame.moteRamp = params.moteRamp;

		mVertices.clear();
		if (mVertices.capacity() < kVertexReserve)
		{
			mVertices.reserve(kVertexReserve);
		}

		if (frame.rockRamp != nullptr && !frame.rockRamp->Empty())
		{
			const std::size_t shown = Shown(mRocks.size(), density);
			for (std::size_t i = 0; i < shown; ++i)
			{
				const Rock& rock = mRocks[i];
				const float shift = params.parallax * rock.depth;
				const sf::Vector2f pos = sf::Vector2f(rock.home.x * tile.x, rock.home.y * tile.y) - params.scroll * shift + rock.drift * params.timeSeconds;
				const float extent = rock.radius * frame.pixel * kRockReach + frame.pixel;
				ForEachTiled(pos, extent, tile, [&](sf::Vector2f c) { AddRock(rock, Snap(c, frame.pixel), frame); });
			}
		}

		if (frame.moteRamp != nullptr && !frame.moteRamp->Empty())
		{
			const std::size_t shown = Shown(mMotes.size(), density);
			for (std::size_t i = 0; i < shown; ++i)
			{
				const Mote& mote = mMotes[i];
				const float t = params.timeSeconds * mote.speed + mote.phase;
				const sf::Vector2f wiggle(std::sin(t), std::cos(t * 0.7f));
				const float shift = params.parallax * kMoteDepth * mote.depth;
				const sf::Vector2f pos = sf::Vector2f(mote.home.x * tile.x, mote.home.y * tile.y)
					- params.scroll * shift + wiggle * (kMoteWiggle * frame.pixel);
				ForEachTiled(pos, frame.pixel, tile, [&](sf::Vector2f c) { AddMote(mote, Snap(c, frame.pixel), frame); });
			}
		}

		if (!mVertices.empty())
		{
			target.draw(mVertices.data(), mVertices.size(), sf::PrimitiveType::Triangles, sf::RenderStates(sf::BlendAlpha));
		}
	}

	void NearField::AddRock(const Rock& rock, sf::Vector2f center, const Frame& frame) const
	{
		const ColorRamp& ramp = *frame.rockRamp;
		// Bands come from ramp stops 0 to 2 only (shadow, mid, lit); the outline is stop 0 darkened further.
		const sf::Color outline = DebrisTone(Scale(StopAt(ramp, 0), kOutlineScale), frame.cap);
		const sf::Color shadow = DebrisTone(Scale(StopAt(ramp, 0), rock.shade), frame.cap);
		const sf::Color mid = DebrisTone(Scale(StopAt(ramp, 1), rock.shade), frame.cap);
		const sf::Color lit = DebrisTone(Scale(StopAt(ramp, 2), rock.shade), frame.cap);

		const float rot = rock.phase + rock.spin * frame.time;
		const float cs = std::cos(rot);
		const float sn = std::sin(rot);
		const int n = rock.vertexCount;

		// Outline polygon in art-pixel units around the centre; the centre sits on the art-pixel grid.
		std::array<sf::Vector2f, 10> poly{};
		for (int j = 0; j < n; ++j)
		{
			const sf::Vector2f o = rock.outline[static_cast<std::size_t>(j)];
			const sf::Vector2f turned(o.x * cs - o.y * sn, o.x * sn + o.y * cs);
			poly[static_cast<std::size_t>(j)] = turned * rock.radius;
		}
		sf::Vector2f lo = poly[0];
		sf::Vector2f hi = poly[0];
		for (int j = 1; j < n; ++j)
		{
			const sf::Vector2f v = poly[static_cast<std::size_t>(j)];
			lo = sf::Vector2f(std::min(lo.x, v.x), std::min(lo.y, v.y));
			hi = sf::Vector2f(std::max(hi.x, v.x), std::max(hi.y, v.y));
		}

		const int x0 = static_cast<int>(std::floor(lo.x));
		const int y0 = static_cast<int>(std::floor(lo.y));
		const int w = static_cast<int>(std::floor(hi.x)) - x0 + 1;
		const int h = static_cast<int>(std::floor(hi.y)) - y0 + 1;
		if (w > kMaxSpan || h > kMaxSpan)
		{
			return;
		}

		// Inside flags over the bounding box, one per art pixel; anything outside the box counts as outside.
		std::array<bool, kMaxSpan * kMaxSpan> mask{};
		auto at = [&](int x, int y) { return x >= 0 && y >= 0 && x < w && y < h && mask[static_cast<std::size_t>(y * kMaxSpan + x)]; };
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				const sf::Vector2f cell(static_cast<float>(x0 + x), static_cast<float>(y0 + y));
				mask[static_cast<std::size_t>(y * kMaxSpan + x)] = Inside(poly, n, cell + sf::Vector2f(0.5f, 0.5f));
			}
		}

		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				if (!at(x, y))
				{
					continue;
				}
				// A pixel with an outside 4-neighbour is the silhouette edge and takes the outline colour.
				const bool edge = !at(x - 1, y) || !at(x + 1, y) || !at(x, y - 1) || !at(x, y + 1);
				sf::Color tone = outline;
				if (!edge)
				{
					const sf::Vector2f cell(static_cast<float>(x0 + x), static_cast<float>(y0 + y));
					const float d = Dot(Normalized(cell + sf::Vector2f(0.5f, 0.5f)), frame.light);
					tone = d < kShadowDot ? shadow : (d > kLitDot ? lit : mid);
				}
				const sf::Vector2f origin = center + sf::Vector2f(static_cast<float>(x0 + x), static_cast<float>(y0 + y)) * frame.pixel;
				PushSquare(mVertices, origin, frame.pixel, tone);
			}
		}
	}

	void NearField::AddMote(const Mote& mote, sf::Vector2f origin, const Frame& frame) const
	{
		const ColorRamp& ramp = *frame.moteRamp;
		const sf::Color base = LimitBrightness(StopAt(ramp, mote.bright ? 3 : 2), frame.cap);
		const auto alpha = static_cast<std::uint8_t>(std::clamp(mote.alpha, 0.f, 1.f) * 255.f);
		const sf::Color color(base.r, base.g, base.b, alpha);
		PushSquare(mVertices, origin, frame.pixel, color);
	}
}
