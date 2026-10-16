#version 120

// One deep-sky object (DeepSky.h), drawn on a square quad into the stop's cache. The local frame is rotated by uAngle,
// scaled by uRadius and squashed by uAspect, so p is the object's own coordinate and |p| = 1 is its edge.
// Pass 0 returns light (added), pass 1 returns the transmission of a dark object (multiplied); other kinds return white
// in pass 1. Light is in units of uMaxBrightness, the background tier cap.
uniform vec2 uCenter;
uniform float uRadius;
uniform float uAngle;
uniform float uAspect;
uniform float uKind;
uniform float uVariant;
uniform float uSeed;
uniform vec3 uColorA;
uniform vec3 uColorB;
uniform vec3 uColorC;
uniform float uIntensity;
uniform float uPass;
uniform float uExtent;
uniform float uMaxBrightness;

const mat2 kRotate = mat2(0.8, -0.6, 0.6, 0.8);

float hash12(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * 0.1031);
	p3 += dot(p3, p3.yzx + 33.33);
	return fract((p3.x + p3.y) * p3.z);
}

vec2 hash22(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973));
	p3 += dot(p3, p3.yzx + 33.33);
	return fract((p3.xx + p3.yz) * p3.zy);
}

// Gradient noise with a quintic fade, roughly in [-0.7, 0.7].
float gnoise(vec2 p)
{
	vec2 i = floor(p);
	vec2 f = fract(p);
	vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
	float a = dot(hash22(i) * 2.0 - 1.0, f);
	float b = dot(hash22(i + vec2(1.0, 0.0)) * 2.0 - 1.0, f - vec2(1.0, 0.0));
	float c = dot(hash22(i + vec2(0.0, 1.0)) * 2.0 - 1.0, f - vec2(0.0, 1.0));
	float d = dot(hash22(i + vec2(1.0, 1.0)) * 2.0 - 1.0, f - vec2(1.0, 1.0));
	return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p)
{
	float sum = 0.0;
	float amp = 0.5;
	for (int i = 0; i < 5; i++)
	{
		sum += amp * gnoise(p);
		p = kRotate * p * 2.03 + vec2(17.1, 9.3);
		amp *= 0.5;
	}
	return sum;
}

// Puffy lobes at every scale (the cauliflower edges of gas clouds).
float billow(vec2 p)
{
	float sum = 0.0;
	float amp = 0.5;
	for (int i = 0; i < 4; i++)
	{
		sum += amp * abs(gnoise(p)) * 1.6;
		p = kRotate * p * 2.07 + vec2(3.7, 11.1);
		amp *= 0.52;
	}
	return sum;
}

// Thin bright threads where the noise crosses zero (shock filaments, dust lanes).
float threads(vec2 p, float sharpness)
{
	float n = fbm(p);
	return pow(clamp(1.0 - abs(n) * 2.2, 0.0, 1.0), sharpness);
}

// Gaussian shell of radius `at` and half-width `w`.
float shell(float r, float at, float w)
{
	float x = (r - at) / w;
	return exp(-x * x);
}

// Asteroid belt in the local frame: the stream runs along x. Three size layers of rocks on grids, each rock an
// irregular soft-edged blob shaded as a sphere lit from the frame's upper left. Returns lit colour in rgb, coverage in a.
vec4 beltRocks(vec2 p)
{
	vec3 colour = vec3(0.0);
	float cover = 0.0;
	vec3 lightDir = normalize(vec3(-0.55, -0.6, 0.6));
	for (int layer = 0; layer < 3; layer++)
	{
		float cell = layer == 0 ? 0.11 : (layer == 1 ? 0.055 : 0.028);
		float chanceScale = layer == 0 ? 0.35 : (layer == 1 ? 0.5 : 0.65);
		vec2 base = floor(p / cell);
		for (int j = -1; j <= 1; j++)
		{
			for (int i = -1; i <= 1; i++)
			{
				vec2 c = base + vec2(float(i), float(j));
				vec2 id = c + vec2(float(layer) * 37.0, uSeed);
				vec2 at = (c + 0.2 + 0.6 * hash22(id)) * cell;
				// the belt is densest along its spine, thinning out with a ragged edge
				float spine = at.y + 0.08 * gnoise(vec2(at.x * 1.5, uSeed));
				float density = exp(-spine * spine / 0.045) * chanceScale;
				if (hash12(id * 1.37) > density)
				{
					continue;
				}
				float radius = cell * (0.18 + 0.3 * hash12(id * 2.11));
				vec2 q = (p - at) / radius;
				float d = length(q) + 0.22 * gnoise(q * 1.6 + id * 3.1) + 0.12 * gnoise(q * 3.4 + id);
				float mask = 1.0 - smoothstep(0.82, 1.0, d);
				if (mask <= 0.0)
				{
					continue;
				}
				vec2 qn = q / max(d, 1e-3) * min(length(q), 1.0);
				vec3 n = vec3(qn, sqrt(max(1.0 - dot(qn, qn), 0.0)));
				float lit = clamp(dot(n, lightDir), 0.0, 1.0);
				float albedo = 0.55 + 0.45 * hash12(id * 3.7);
				// far (small) rocks are dimmer and bluer with distance
				float depth = layer == 0 ? 1.0 : (layer == 1 ? 0.75 : 0.55);
				vec3 rock = mix(uColorA * 0.12, uColorA * (0.35 + 0.9 * lit), lit) * albedo * depth + uColorB * pow(lit, 6.0) * 0.25 * depth;
				colour = mix(colour, rock, mask);
				cover = max(cover, mask * (layer == 0 ? 0.95 : (layer == 1 ? 0.85 : 0.7)));
			}
		}
	}
	return vec4(colour, cover);
}

void main()
{
	vec2 px = gl_TexCoord[0].xy;
	vec2 d = px - uCenter;
	float ca = cos(uAngle);
	float sa = sin(uAngle);
	vec2 local = vec2(ca * d.x + sa * d.y, -sa * d.x + ca * d.y) / uRadius;
	vec2 p = vec2(local.x, local.y / uAspect);
	float r = length(p);
	vec2 s = vec2(uSeed * 1.37, uSeed * 0.71);
	int kind = int(uKind + 0.5);
	// every object fades out before the quad edge, so the quad never shows
	float quadFade = 1.0 - smoothstep(0.9, 1.0, length(local) / uExtent);

	if (uPass > 0.5)
	{
		vec3 trans = vec3(1.0);
		if (kind == 13)
		{
			trans = vec3(1.0 - beltRocks(p).a * quadFade);
		}
		if (kind == 5)
		{
			vec2 warp = vec2(fbm(p * 1.2 + s), fbm(p * 1.2 + s + 5.2));
			vec2 q = p + warp * 0.5;
			float mass = fbm(q * 1.7 + s) * 1.0 + (1.0 - length(q)) * 1.0 - 0.55;
			float dens = smoothstep(0.0, 0.6, mass) * (1.0 - smoothstep(0.65, 1.0, r));
			dens *= 0.55 + 0.45 * smoothstep(-0.3, 0.3, fbm(q * 4.0 + s + 70.0));
			// extinction is stronger for blue light: thin edges redden, the core turns black
			trans = exp(-dens * 2.3 * uIntensity * vec3(0.8, 1.0, 1.25));
			trans = mix(vec3(1.0), trans, quadFade);
		}
		gl_FragColor = vec4(trans, 1.0);
		return;
	}

	vec3 light = vec3(0.0);

	if (kind == 0)
	{
		// Emission (HII region): billowing gas, rose H-alpha body, [O III] teal near the hot core, dark lanes.
		// The outline comes from a strongly warped frame, so the cloud is ragged and lopsided, never a ball.
		vec2 warp = vec2(fbm(p * 1.1 + s), fbm(p * 1.1 + s + 5.2));
		vec2 q = p + warp * 0.6;
		float rq = length(q);
		float body = fbm(q * 1.8 + s) * 0.9 + billow(q * 2.6 + s) * 0.3;
		float env = 1.0 - smoothstep(0.1, 0.95, rq);
		float hollow = uVariant > 0.5 ? smoothstep(0.1, 0.4, rq + 0.12 * fbm(p * 3.0 + s)) : 1.0;
		float dens = smoothstep(0.05, 0.95, body + env * 0.95 - 0.5) * env * hollow;
		// faint wisps reach beyond the bright body
		dens += 0.3 * env * threads(q * 2.2 + s + 40.0, 3.0) * (1.0 - smoothstep(0.0, 0.4, dens));
		float lanes = threads(q * 1.5 + s + 20.0, 4.0);
		dens *= 1.0 - 0.55 * lanes * smoothstep(0.1, 0.5, dens);
		float hot = uVariant > 0.5 ? shell(rq, 0.32, 0.18) : exp(-rq * rq * 4.5);
		float oxygen = hot * smoothstep(0.15, 0.7, dens);
		vec3 colour = mix(uColorA, uColorB, clamp(oxygen * 1.1, 0.0, 0.85));
		// bright ionization fronts where the gas thins
		float front = smoothstep(0.05, 0.3, dens) * (1.0 - smoothstep(0.3, 0.65, dens));
		// A light ramp, not flat paint: thin gas is deep and saturated, dense gas runs toward the bright highlight colour.
		vec3 ramp = mix(colour * 0.75, mix(colour, uColorC, 0.55), smoothstep(0.35, 1.0, dens));
		// Volume, not a flat stain: thin gas turns see-through (steep curve), the side facing the ionizing stars is
		// brighter, and bright ionized filaments run through the body.
		float lightSide = 0.55 + 0.7 * smoothstep(-0.7, 0.7, dot(q, vec2(cos(uSeed), sin(uSeed))));
		float filaments = threads(q * 3.2 + s + 60.0, 7.0) * env * smoothstep(0.05, 0.45, dens);
		float volume = pow(dens, 1.5) * lightSide;
		light = ramp * volume + mix(colour, uColorC, 0.6) * filaments * 0.35 + uColorC * front * 0.12 * env;
	}
	else if (kind == 1)
	{
		// Reflection nebula: dust lit by a hot star, blue, striated along the frame's x axis.
		float streak = fbm(vec2(p.x * 0.9, p.y * 3.8) + s);
		float glow = exp(-r * r * 2.6);
		float dens = glow * (0.35 + 0.85 * smoothstep(-0.35, 0.45, streak)) * (0.75 + 0.25 * billow(p * 2.0 + s));
		vec3 colour = mix(uColorA, uColorB, exp(-r * r * 10.0));
		light = colour * dens + uColorC * exp(-r * r * 500.0) * 1.4;
	}
	else if (kind == 2)
	{
		// Planetary nebula: [O III] fill, H-alpha rim with cometary knots, faint outer halo, white-dwarf core.
		float rr;
		if (uVariant > 0.5)
		{
			// bipolar: two lobes along x with a pinched waist
			vec2 q = vec2(abs(p.x) - 0.42, p.y * (1.25 + 0.6 * smoothstep(0.4, 0.0, abs(p.x))));
			rr = length(q) / 0.52 * 0.58 + 0.05 * fbm(p * 4.0 + s);
		}
		else
		{
			rr = r + 0.07 * fbm(p * 4.0 + s);
		}
		float texture = 0.55 + 0.6 * smoothstep(-0.4, 0.4, fbm(p * 3.5 + s));
		float rim = shell(rr, 0.6, 0.2) * texture;
		// the [O III] zone glows brightest just inside the rim, fading toward the centre
		float inner = (1.0 - smoothstep(0.35, 0.62, rr)) * (0.45 + 0.55 * smoothstep(0.0, 0.5, rr));
		float knots = smoothstep(0.25, 0.5, fbm(p * 8.0 + s + 3.0)) * shell(rr, 0.64, 0.1);
		float halo = shell(rr, 0.88, 0.14) * 0.15 * (0.6 + 0.6 * fbm(p * 3.0 + s));
		vec3 fill = uColorB * inner * texture * 0.6;
		light = fill + mix(uColorA, uColorB, 0.15) * (rim * 0.55 + halo) + uColorC * knots * 0.35;
		light += vec3(0.8, 0.9, 1.0) * exp(-r * r * 1500.0) * 1.3;
	}
	else if (kind == 3)
	{
		if (uVariant > 0.5)
		{
			// Pulsar-wind nebula (Crab-like): blue synchrotron body caged by red filaments.
			float rr = r + 0.15 * fbm(p * 2.0 + s);
			float body = (1.0 - smoothstep(0.2, 0.8, rr)) * (0.6 + 0.5 * fbm(p * 3.0 + s));
			float cage = threads(p * 3.2 + s + 4.0, 7.0) * (1.0 - smoothstep(0.55, 0.9, rr));
			light = uColorB * body * 0.9 + uColorA * cage * 0.55 + uColorC * exp(-r * r * 300.0);
		}
		else
		{
			// Shell remnant (Veil-like): thin shock filaments, [O III] just outside the H-alpha, broken coverage.
			// Shock fronts are long sheets seen edge-on: wavy threads running along the shell, brightening and fading.
			vec2 dir = normalize(p + 1e-4);
			float rr = r + 0.08 * fbm(dir * 2.0 + s) + 0.03 * fbm(p * 5.0 + s);
			float wave = fbm(dir * 3.0 + s + 11.0) * 3.0 + fbm(p * 2.5 + s) * 1.5;
			float filH = smoothstep(0.7, 1.0, cos(rr * 28.0 + wave)) * (0.25 + 0.9 * smoothstep(-0.2, 0.4, fbm(dir * 4.0 + s + 3.0)));
			float filO = smoothstep(0.75, 1.0, cos(rr * 26.0 + wave * 1.3 + 1.7)) * (0.25 + 0.9 * smoothstep(-0.2, 0.4, fbm(dir * 4.0 + s + 8.0)));
			// only arcs of the shell are bright, the rest is a faint veil
			float coverage = 0.15 + 0.85 * smoothstep(0.0, 0.35, fbm(dir * 1.4 + s + 7.0));
			float hydrogen = shell(rr, 0.76, 0.09) * (0.25 + 1.3 * filH);
			float oxygen = shell(rr, 0.84, 0.07) * (0.2 + 1.3 * filO);
			float interior = (1.0 - smoothstep(0.4, 0.85, rr)) * 0.06;
			light = (uColorA * hydrogen + uColorB * oxygen) * coverage + uColorB * interior;
		}
	}
	else if (kind == 4)
	{
		// Wind bubble (Wolf-Rayet / LBV): thin bright shell, limb-brightened, around an off-centre hot star.
		float rr = r + 0.05 * fbm(p * 3.0 + s);
		float rough = r + 0.1 * fbm(p * 2.5 + s + 4.0);
		float edge = shell(rough, 0.68, 0.16) * (0.45 + 0.8 * smoothstep(-0.3, 0.45, fbm(p * 2.5 + s)))
			* (0.5 + 0.7 * smoothstep(-0.3, 0.3, fbm(normalize(p + 1e-4) * 2.0 + s + 9.0)));
		edge += shell(rough, 0.82, 0.26) * 0.25;
		float inner = (1.0 - smoothstep(0.3, 0.72, rough)) * 0.14 * (0.6 + 0.8 * fbm(p * 3.0 + s));
		vec3 colour = mix(uColorA, uColorB, smoothstep(0.62, 0.78, rr));
		vec2 star = p - vec2(0.18, -0.12);
		light = colour * (edge + inner) + uColorC * exp(-dot(star, star) * 600.0) * 1.3;
		if (uVariant > 0.5)
		{
			// Thor's-helmet wings: outflow lobes along x
			float wing = shell(length(vec2(abs(p.x) - 0.55, p.y * 2.2)) + 0.08 * fbm(p * 3.0 + s), 0.25, 0.14);
			light += uColorB * wing * 0.2 * (0.5 + fbm(p * 3.0 + s));
		}
	}
	else if (kind == 5)
	{
		// Dark cloud: the transmission pass darkens; here only the rim lit by nearby stars.
		vec2 warp = vec2(fbm(p * 1.2 + s), fbm(p * 1.2 + s + 5.2));
		vec2 q = p + warp * 0.5;
		float mass = fbm(q * 1.7 + s) * 1.0 + (1.0 - length(q)) * 1.0 - 0.55;
		float dens = smoothstep(0.0, 0.6, mass) * (1.0 - smoothstep(0.65, 1.0, r));
		dens *= 0.55 + 0.45 * smoothstep(-0.3, 0.3, fbm(q * 4.0 + s + 70.0));
		float rim = smoothstep(0.0, 0.25, dens) * (1.0 - smoothstep(0.2, 0.65, dens));
		// lit from the side the rim colour faces (frame -y), dimmer on the far side
		float facing = 0.25 + 0.75 * smoothstep(-0.6, 0.6, -p.y);
		light = uColorA * rim * facing * 0.35;
	}
	else if (kind == 6)
	{
		// Fog: broad, low-frequency coloured haze with dark rifts.
		vec2 warp = vec2(fbm(p * 0.7 + s), fbm(p * 0.7 + s + 4.1));
		float body = fbm(p * 0.9 + warp * 0.7 + s);
		float rift = smoothstep(-0.2, 0.35, fbm(p * 1.6 + warp + s + 31.0));
		float env = 1.0 - smoothstep(0.5, 1.0, r);
		float dens = smoothstep(-0.35, 0.45, body) * (0.2 + 0.8 * rift) * env;
		vec3 colour = mix(uColorA, uColorB, smoothstep(-0.3, 0.3, fbm(p * 0.55 + s + 50.0)));
		colour = mix(colour, uColorC, smoothstep(0.55, 0.95, dens) * 0.5);
		light = colour * dens * 0.6;
	}
	else if (kind == 7)
	{
		// Star cluster: only the unresolved light of the stars too faint or too crowded to resolve. The resolved stars
		// are added to the star field (StarField::AddCluster), so they look exactly like the rest of the sky.
		float open = uVariant;
		// A globular reads as a bright, smoothly concentrated core with a grainy halo of resolved stars around it.
		// An open cluster sits in a faint blue reflection haze lit by its own hot stars.
		float glow = open > 0.5 ? exp(-r * r * 4.0) * 0.16 * (0.6 + 0.8 * smoothstep(-0.3, 0.4, fbm(vec2(p.x, p.y * 2.5) * 1.8 + s)))
			: (exp(-r * r * 40.0) * 0.9 + exp(-r * 6.0) * 0.25);
		light = uColorA * glow * (0.9 + 0.2 * fbm(p * 3.0 + s));
	}
	else if (kind == 8)
	{
		// A distant galaxy as a telescope sees it: smooth exponential disc and bulge, low-contrast spiral structure,
		// a dust lane along the major axis when tilted, faint pink knots; muted colours, soft edge.
		float rr = max(r, 1e-3);
		float tilt = 1.0 - uAspect;
		if (uVariant > 0.5)
		{
			float profile = exp(-pow(rr / 0.06, 0.5) * 2.2);
			light = mix(uColorC, uColorA, smoothstep(0.0, 0.5, rr)) * profile * 2.6;
		}
		else
		{
			float a = atan(p.y, p.x);
			float phase = 2.0 * (a - log(rr) * 2.2) + uSeed;
			float arm = 0.5 + 0.5 * cos(phase + 0.6 * fbm(p * 3.0 + s));
			arm = smoothstep(0.2, 1.0, arm);
			float disk = exp(-rr / 0.22);
			float bulge = exp(-pow(rr / 0.05, 0.5) * 2.0) * 1.6;
			float knots = arm * smoothstep(0.35, 0.6, fbm(p * 16.0 + s)) * smoothstep(0.1, 0.35, rr) * disk;
			vec3 diskColour = mix(mix(uColorC, uColorA, 0.55), vec3(0.85, 0.88, 0.95), 0.3);
			light = (uColorC * bulge + diskColour * disk * (0.55 + 0.5 * arm) + uColorB * knots * 0.35) * 2.2;
			// dust lane along the major axis, offset to the near side, stronger the more edge-on the disc is
			float laneY = local.y + 0.025 * uAspect;
			float lane = exp(-laneY * laneY / (0.0009 + 0.004 * uAspect)) * (1.0 - smoothstep(0.15, 0.85, abs(local.x)));
			light *= 1.0 - 0.75 * lane * tilt;
		}
		light *= 1.0 - smoothstep(0.6, 1.0, rr);
	}
	else if (kind == 9)
	{
		// Radio filaments: many parallel threads along y, gently curved, brightening and fading along their length.
		float x = p.x + p.y * p.y * 0.18 + 0.05 * fbm(vec2(p.x * 1.5, p.y * 0.7) + s);
		float k = 13.0;
		float lane = floor(x * k);
		float f = fract(x * k) - 0.5;
		float width = 0.05 + 0.08 * hash12(vec2(lane, uSeed));
		float line = exp(-(f * f) / (width * width));
		float strength = step(0.35, hash12(vec2(lane * 1.7, uSeed + 3.0))) * (0.3 + 0.7 * hash12(vec2(lane, uSeed + 9.0)));
		float along = smoothstep(-0.2, 0.4, fbm(vec2(lane * 0.37, p.y * 1.4) + s));
		float env = (1.0 - smoothstep(0.55, 1.0, abs(p.y))) * (1.0 - smoothstep(0.6, 0.95, abs(p.x)));
		light = mix(uColorA, uColorB, hash12(vec2(lane, uSeed + 5.0))) * line * strength * along * env;
		light += uColorA * env * 0.04;
	}
	else if (kind == 10)
	{
		// Outflow lobes (Fermi / eROSITA bubbles): two huge bubbles along y, edge-brightened shells, faint fill.
		vec2 q = vec2(p.x, abs(p.y) - 0.5);
		float rl = length(q * vec2(1.0, 1.08)) + 0.05 * fbm(p * 3.0 + s);
		float edge = shell(rl, 0.42, 0.11) * (0.3 + 0.8 * smoothstep(-0.3, 0.4, fbm(p * 2.5 + s)));
		float fill = (1.0 - smoothstep(0.1, 0.46, rl)) * 0.22 * (0.5 + 0.7 * fbm(p * 1.8 + s));
		light = uColorA * edge * 0.6 + uColorB * fill;
	}
	else if (kind == 11)
	{
		// Bipolar jet with knots and bow shocks (Herbig-Haro); variant = precession (a corkscrew, like SS 433).
		float ax = abs(p.x);
		float y = p.y - (uVariant > 0.5 ? sin(p.x * 9.0 + uSeed) * 0.07 * ax : 0.0);
		float w = 0.015 + 0.05 * ax;
		float beam = exp(-(y * y) / (w * w)) * (1.0 - smoothstep(0.65, 0.85, ax));
		float knots = pow(0.5 + 0.5 * sin(ax * 26.0 + uSeed), 6.0);
		float bow = shell(length(vec2(ax - 0.72, y * 1.6)) + 0.05 * fbm(p * 6.0 + s), 0.14, 0.07) * smoothstep(0.6, 0.8, ax);
		light = uColorA * beam * (0.25 + 1.0 * knots) + uColorB * bow * 0.4;
		light += uColorC * exp(-r * r * 900.0) * 1.2;
	}
	else if (kind == 13)
	{
		// Asteroid belt: the rocks' lit faces (their shadow is the transmission pass) and the belt's faint dust glow.
		vec4 rocks = beltRocks(p);
		float spine = p.y + 0.08 * gnoise(vec2(p.x * 1.5, uSeed));
		float dust = exp(-spine * spine / 0.06) * (0.5 + 0.5 * fbm(p * 3.0 + s)) * 0.18;
		light = rocks.rgb + uColorC * dust * (1.0 - rocks.a);
		light *= 1.0 - smoothstep(0.85, 1.0, abs(p.x));
	}
	else if (kind == 14)
	{
		// Giant star: a white-hot core, a halo in the star's colour, and four soft diffraction spikes along the frame axes.
		float distPx = length(local) * uRadius;
		float core = exp(-(distPx * distPx) / (uRadius * uRadius * 0.0012 + 2.0));
		float halo = exp(-distPx / (uRadius * 0.08 + 1.0)) * 0.55 + exp(-distPx / (uRadius * 0.3)) * 0.12;
		vec2 a = abs(local) * uRadius;
		float spikes = exp(-a.y / 1.1) * exp(-a.x / (uRadius * 0.35)) + exp(-a.x / 1.1) * exp(-a.y / (uRadius * 0.35));
		light = mix(uColorA, vec3(1.0), 0.4) * core * 2.2 + uColorA * (halo + spikes * 0.55);
	}
	else
	{
		// Sgr A West mini-spiral: ionized streamers falling inward, inside the circumnuclear ring of molecular gas.
		float rr = max(r, 1e-3);
		float a = atan(p.y, p.x);
		float arms = pow(max(cos(1.5 * (a - log(rr) * 1.8) + uSeed + 0.5 * fbm(p * 2.0 + s)), 0.0), 2.5);
		float streamers = arms * exp(-rr * 2.0) * (1.0 - smoothstep(0.55, 0.75, rr)) * (0.6 + 0.6 * fbm(p * 6.0 + s));
		float ring = shell(rr + 0.06 * fbm(p * 3.0 + s), 0.8, 0.12) * (0.4 + 0.8 * smoothstep(-0.3, 0.4, fbm(p * 4.0 + s)));
		light = uColorA * streamers * 1.2 + uColorB * ring * 0.6 + uColorC * exp(-rr * rr * 200.0) * 0.6;
	}

	vec3 c = light * uIntensity * quadFade * uMaxBrightness;
	// Soft shoulder into the cap so bright cores keep their hue instead of clipping.
	float peak = max(c.r, max(c.g, c.b));
	float knee = 0.6 * uMaxBrightness;
	if (peak > knee)
	{
		float room = uMaxBrightness - knee;
		c *= (knee + room * (1.0 - exp(-(peak - knee) / room))) / peak;
	}
	gl_FragColor = vec4(c, 1.0);
}
