// MIT License

// Copyright (c) 2019 Erin Catto

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "box2d/b2_draw.h"
#include "box2d/b2_rope.h"

struct b2RopeStretch
{
	int32 i1, i2;
	float invMass1, invMass2;
	float L;
};

struct b2RopeBend
{
	int32 i1, i2, i3;
	float invMass1, invMass2, invMass3;
	float invEffectiveMass;
	float lambda;
	float L1, L2;
};

b2Rope::b2Rope()
{
	m_position.SetZero();
	m_count = 0;
	m_stretchCount = 0;
	m_bendCount = 0;
	m_stretchConstraints = nullptr;
	m_bendConstraints = nullptr;
	m_bindPositions = nullptr;
	m_ps = nullptr;
	m_p0s = nullptr;
	m_vs = nullptr;
	m_invMasses = nullptr;
	m_gravity.SetZero();
}

b2Rope::~b2Rope()
{
	b2Free(m_stretchConstraints);
	b2Free(m_bendConstraints);
	b2Free(m_bindPositions);
	b2Free(m_ps);
	b2Free(m_p0s);
	b2Free(m_vs);
	b2Free(m_invMasses);
}

void b2Rope::Create(const b2RopeDef& def)
{
	b2Assert(def.count >= 3);
	m_position = def.position;
	m_count = def.count;
	m_bindPositions = (b2Vec2*)b2Alloc(m_count * sizeof(b2Vec2));
	m_ps = (b2Vec2*)b2Alloc(m_count * sizeof(b2Vec2));
	m_p0s = (b2Vec2*)b2Alloc(m_count * sizeof(b2Vec2));
	m_vs = (b2Vec2*)b2Alloc(m_count * sizeof(b2Vec2));
	m_invMasses = (float*)b2Alloc(m_count * sizeof(float));

	for (int32 i = 0; i < m_count; ++i)
	{
		m_bindPositions[i] = def.vertices[i];
		m_ps[i] = def.vertices[i] + m_position;
		m_p0s[i] = def.vertices[i] + m_position;
		m_vs[i].SetZero();

		float m = def.masses[i];
		if (m > 0.0f)
		{
			m_invMasses[i] = 1.0f / m;
		}
		else
		{
			m_invMasses[i] = 0.0f;
		}
	}

	m_stretchCount = m_count - 1;
	m_bendCount = m_count - 2;

	m_stretchConstraints = (b2RopeStretch*)b2Alloc(m_stretchCount * sizeof(b2RopeStretch));
	m_bendConstraints = (b2RopeBend*)b2Alloc(m_bendCount * sizeof(b2RopeBend));


	for (int32 i = 0; i < m_stretchCount; ++i)
	{
		b2Vec2 p1 = m_ps[i];
		b2Vec2 p2 = m_ps[i+1];

		m_stretchConstraints[i].i1 = i;
		m_stretchConstraints[i].i2 = i + 1;
		m_stretchConstraints[i].L = b2Distance(p1, p2);
		m_stretchConstraints[i].invMass1 = m_invMasses[i];
		m_stretchConstraints[i].invMass2 = m_invMasses[i + 1];
	}

	for (int32 i = 0; i < m_bendCount; ++i)
	{
		b2RopeBend& c = m_bendConstraints[i];

		b2Vec2 p1 = m_ps[i];
		b2Vec2 p2 = m_ps[i + 1];
		b2Vec2 p3 = m_ps[i + 2];

		c.i1 = i;
		c.i2 = i + 1;
		c.i3 = i + 2;
		c.invMass1 = m_invMasses[i];
		c.invMass2 = m_invMasses[i + 1];
		c.invMass3 = m_invMasses[i + 2];
		c.invEffectiveMass = 0.0f;
		c.L1 = b2Distance(p1, p2);
		c.L2 = b2Distance(p2, p3);
		c.lambda = 0.0f;

		// Pre-compute effective mass
		b2Vec2 d1 = p2 - p1;
		b2Vec2 d2 = p3 - p2;
		float L1sqr = d1.LengthSquared();
		float L2sqr = d2.LengthSquared();

		if (L1sqr * L2sqr == 0.0f)
		{
			continue;
		}

		b2Vec2 Jd1 = (-1.0f / L1sqr) * d1.Skew();
		b2Vec2 Jd2 = (1.0f / L2sqr) * d2.Skew();

		b2Vec2 J1 = -Jd1;
		b2Vec2 J2 = Jd1 - Jd2;
		b2Vec2 J3 = Jd2;

		c.invEffectiveMass = c.invMass1 * b2Dot(J1, J1) + c.invMass2 * b2Dot(J2, J2) + c.invMass3 * b2Dot(J3, J3);
	}

	m_gravity = def.gravity;
	m_tuning = def.tuning;
}

void b2Rope::SetTuning(const b2RopeTuning& tuning)
{
	m_tuning = tuning;
}

void b2Rope::Step(float dt, int32 iterations, const b2Vec2& position)
{
	if (dt == 0.0)
	{
		return;
	}

	const float inv_dt = 1.0f / dt;
	float d = expf(- dt * m_tuning.damping);

	// Apply gravity and damping
	for (int32 i = 0; i < m_count; ++i)
	{
		if (m_invMasses[i] > 0.0f)
		{
			m_vs[i] += dt * m_gravity;
			m_vs[i] *= d;
		}
		else
		{
			m_vs[i] = inv_dt * (m_bindPositions[i] + position - m_p0s[i]);
		}
	}

	// Apply bending spring
	if (m_tuning.bendingModel == b2_springAngleBendingModel)
	{
		ApplyBendForces(dt);
	}

	if (m_tuning.bendingModel == b2_softAngleBendingModel && m_tuning.warmStart)
	{
		for (int32 i = 0; i < m_bendCount; ++i)
		{
			const b2RopeBend& c = m_bendConstraints[i];

			b2Vec2 p1 = m_ps[c.i1];
			b2Vec2 p2 = m_ps[c.i2];
			b2Vec2 p3 = m_ps[c.i3];

			b2Vec2 d1 = p2 - p1;
			b2Vec2 d2 = p3 - p2;

			float L1sqr, L2sqr;

			if (m_tuning.isometric)
			{
				L1sqr = c.L1 * c.L1;
				L2sqr = c.L2 * c.L2;
			}
			else
			{
				L1sqr = d1.LengthSquared();
				L2sqr = d2.LengthSquared();
			}

			if (L1sqr * L2sqr == 0.0f)
			{
				continue;
			}

			b2Vec2 Jd1 = (-1.0f / L1sqr) * d1.Skew();
			b2Vec2 Jd2 = (1.0f / L2sqr) * d2.Skew();

			b2Vec2 J1 = -Jd1;
			b2Vec2 J2 = Jd1 - Jd2;
			b2Vec2 J3 = Jd2;

			m_vs[c.i1] += (c.invMass1 * c.lambda) * J1;
			m_vs[c.i2] += (c.invMass2 * c.lambda) * J2;
			m_vs[c.i3] += (c.invMass3 * c.lambda) * J3;
		}
	}
	else
	{
		for (int32 i = 0; i < m_bendCount; ++i)
		{
			m_bendConstraints[i].lambda = 0.0f;
		}
	}

	// Update position
	for (int32 i = 0; i < m_count; ++i)
	{
		m_ps[i] += dt * m_vs[i];
	}

	// Solve constraints
	for (int32 i = 0; i < iterations; ++i)
	{
		if (m_tuning.bendingModel == b2_pbdAngleBendingModel)
		{
			SolveBend_PBD_Angle();
		}
		else if (m_tuning.bendingModel == b2_xpbdAngleBendingModel)
		{
			SolveBend_XPBD_Angle(dt);
		}
		else if (m_tuning.bendingModel == b2_softAngleBendingModel)
		{
			SolveBend_Soft_Angle(dt);
		}
		else if (m_tuning.bendingModel == b2_pbdDistanceBendingModel)
		{
			SolveBend_PBD_Distance();
		}
		else if (m_tuning.bendingModel == b2_pbdHeightBendingModel)
		{
			SolveBend_PBD_Height();
		}

		SolveStretch();
	}

	// Constrain velocity
	for (int32 i = 0; i < m_count; ++i)
	{
		m_vs[i] = inv_dt * (m_ps[i] - m_p0s[i]);
		m_p0s[i] = m_ps[i];
	}
}

void b2Rope::Reset(const b2Vec2& position)
{
	m_position = position;

	for (int32 i = 0; i < m_count; ++i)
	{
		m_ps[i] = m_bindPositions[i] + m_position;
		m_p0s[i] = m_bindPositions[i] + m_position;
		m_vs[i].SetZero();
	}

	for (int32 i = 0; i < m_bendCount; ++i)
	{
		m_bendConstraints[i].lambda = 0.0f;
	}
}

void b2Rope::SolveStretch()
{
	const float stiffness = m_tuning.stretchStiffness;

	for (int32 i = 0; i < m_stretchCount; ++i)
	{
		const b2RopeStretch& c = m_stretchConstraints[i];

		b2Vec2 p1 = m_ps[c.i1];
		b2Vec2 p2 = m_ps[c.i2];

		b2Vec2 d = p2 - p1;
		float L = d.Normalize();

		float sum = c.invMass1 + c.invMass2;
		if (sum == 0.0f)
		{
			continue;
		}

		float s1 = c.invMass1 / sum;
		float s2 = c.invMass2 / sum;

		p1 -= stiffness * s1 * (c.L - L) * d;
		p2 += stiffness * s2 * (c.L - L) * d;

		m_ps[c.i1] = p1;
		m_ps[c.i2] = p2;
	}
}

void b2Rope::SolveBend_PBD_Angle()
{
	const float stiffness = m_tuning.bendStiffness;

	for (int32 i = 0; i < m_bendCount; ++i)
	{
		const b2RopeBend& c = m_bendConstraints[i];

		b2Vec2 p1 = m_ps[c.i1];
		b2Vec2 p2 = m_ps[c.i2];
		b2Vec2 p3 = m_ps[c.i3];

		b2Vec2 d1 = p2 - p1;
		b2Vec2 d2 = p3 - p2;
		float a = b2Cross(d1, d2);
		float b = b2Dot(d1, d2);

		float angle = b2Atan2(a, b);

		float L1sqr, L2sqr;
		
		if (m_tuning.isometric)
		{
			L1sqr = c.L1 * c.L1;
			L2sqr = c.L2 * c.L2;
		}
		else
		{
			L1sqr = d1.LengthSquared();
			L2sqr = d2.LengthSquared();
		}

		if (L1sqr * L2sqr == 0.0f)
		{
			continue;
		}

		b2Vec2 Jd1 = (-1.0f / L1sqr) * d1.Skew();
		b2Vec2 Jd2 = (1.0f / L2sqr) * d2.Skew();

		b2Vec2 J1 = -Jd1;
		b2Vec2 J2 = Jd1 - Jd2;
		b2Vec2 J3 = Jd2;

		float sum;
		if (m_tuning.fixedEffectiveMass)
		{
			sum = c.invEffectiveMass;
		}
		else
		{
			sum = c.invMass1 * b2Dot(J1, J1) + c.invMass2 * b2Dot(J2, J2) + c.invMass3 * b2Dot(J3, J3);
		}

		if (sum == 0.0f)
		{
			sum = c.invEffectiveMass;
		}

		float mass = 1.0f / sum;
		float C = angle;
		float impulse = -stiffness * mass * C;

		p1 += (c.invMass1 * impulse) * J1;
		p2 += (c.invMass2 * impulse) * J2;
		p3 += (c.invMass3 * impulse) * J3;

		m_ps[c.i1] = p1;
		m_ps[c.i2] = p2;
		m_ps[c.i3] = p3;
	}
}

void b2Rope::SolveBend_XPBD_Angle(float dt)
{
	b2Assert(dt > 0.0f);

	// omega = 2 * pi * hz
	const float omega = 2.0f * b2_pi * m_tuning.bendHertz;

	for (int32 i = 0; i < m_bendCount; ++i)
	{
		b2RopeBend& c = m_bendConstraints[i];

		b2Vec2 p1 = m_ps[c.i1];
		b2Vec2 p2 = m_ps[c.i2];
		b2Vec2 p3 = m_ps[c.i3];

		b2Vec2 dp1 = p1 - m_p0s[c.i1];
		b2Vec2 dp2 = p2 - m_p0s[c.i2];
		b2Vec2 dp3 = p3 - m_p0s[c.i3];

		b2Vec2 d1 = p2 - p1;
		b2Vec2 d2 = p3 - p2;

		float L1sqr, L2sqr;

		if (m_tuning.isometric)
		{
			L1sqr = c.L1 * c.L1;
			L2sqr = c.L2 * c.L2;
		}
		else
		{
			L1sqr = d1.LengthSquared();
			L2sqr = d2.LengthSquared();
		}

		if (L1sqr * L2sqr == 0.0f)
		{
			continue;
		}

		float a = b2Cross(d1, d2);
		float b = b2Dot(d1, d2);

		float angle = b2Atan2(a, b);

		b2Vec2 Jd1 = (-1.0f / L1sqr) * d1.Skew();
		b2Vec2 Jd2 = (1.0f / L2sqr) * d2.Skew();

		b2Vec2 J1 = -Jd1;
		b2Vec2 J2 = Jd1 - Jd2;
		b2Vec2 J3 = Jd2;

		float sum;
		if (m_tuning.fixedEffectiveMass)
		{
			sum = c.invEffectiveMass;
		}
		else
		{
			sum = c.invMass1 * b2Dot(J1, J1) + c.invMass2 * b2Dot(J2, J2) + c.invMass3 * b2Dot(J3, J3);
		}

		if (sum == 0.0f)
		{
			continue;
		}

		float mass = 1.0f / sum;

		const float spring = mass * omega * omega;
		const float damper = 2.0f * mass * m_tuning.bendDamping * omega;

		const float alpha = 1.0f / (spring * dt * dt);
		const float beta = dt * dt * damper;
		const float gamma = alpha * beta / dt;
		float C = angle;

		// This is using the initial velocities
		float Cdot = b2Dot(J1, dp1) + b2Dot(J2, dp2) + b2Dot(J3, dp3);

		float B = C + alpha * c.lambda + gamma * Cdot;
		float sum2 = (1.0f + alpha * beta / dt) * sum + alpha;

		float impulse = -B / sum2;

		p1 += (c.invMass1 * impulse) * J1;
		p2 += (c.invMass2 * impulse) * J2;
		p3 += (c.invMass3 * impulse) * J3;

		m_ps[c.i1] = p1;
		m_ps[c.i2] = p2;
		m_ps[c.i3] = p3;
		c.lambda += impulse;
	}
}

void b2Rope::SolveBend_Soft_Angle(float dt)
{
	b2Assert(dt > 0.0f);

	const float inv_dt = 1.0f / dt;

	// omega = 2 * pi * hz
	const float omega = 2.0f * b2_pi * m_tuning.bendHertz;

	for (int32 i = 0; i < m_bendCount; ++i)
	{
		b2RopeBend& c = m_bendConstraints[i];

		b2Vec2 p1 = m_ps[c.i1];
		b2Vec2 p2 = m_ps[c.i2];
		b2Vec2 p3 = m_ps[c.i3];

		b2Vec2 v1 = inv_dt * (p1 - m_p0s[c.i1]);
		b2Vec2 v2 = inv_dt * (p2 - m_p0s[c.i2]);
		b2Vec2 v3 = inv_dt * (p3 - m_p0s[c.i3]);

		b2Vec2 d1 = p2 - p1;
		b2Vec2 d2 = p3 - p2;

		float L1sqr, L2sqr;

		if (m_tuning.isometric)
		{
			L1sqr = c.L1 * c.L1;
			L2sqr = c.L2 * c.L2;
		}
		else
		{
			L1sqr = d1.LengthSquared();
			L2sqr = d2.LengthSquared();
		}

		if (L1sqr * L2sqr == 0.0f)
		{
			continue;
		}

		float a = b2Cross(d1, d2);
		float b = b2Dot(d1, d2);

		float C = b2Atan2(a, b);

		b2Vec2 Jd1 = (-1.0f / L1sqr) * d1.Skew();
		b2Vec2 Jd2 = (1.0f / L2sqr) * d2.Skew();

		b2Vec2 J1 = -Jd1;
		b2Vec2 J2 = Jd1 - Jd2;
		b2Vec2 J3 = Jd2;

		float sum = c.invMass1 * b2Dot(J1, J1) + c.invMass2 * b2Dot(J2, J2) + c.invMass3 * b2Dot(J3, J3);
		if (sum == 0.0f)
		{
			continue;
		}

		float mass = 1.0f / sum;

		const float spring = mass * omega * omega;
		const float damper = 2.0f * mass * m_tuning.bendDamping * omega;

		float gamma = dt * (damper + dt * spring);
		if (gamma != 0.0f)
		{
			gamma = 1.0f / gamma;
		}
		mass = 1.0f / (sum + gamma);

		float bias = C * dt * spring * gamma;

		// This is using the initial velocities
		float Cdot = b2Dot(J1, v1) + b2Dot(J2, v2) + b2Dot(J3, v3);

		float impulse = -mass * (Cdot + bias + gamma * c.lambda);
		v1 += (c.invMass1 * impulse) * J1;
		v2 += (c.invMass2 * impulse) * J2;
		v3 += (c.invMass3 * impulse) * J3;

		m_ps[c.i1] = m_p0s[c.i1] + dt * v1;
		m_ps[c.i2] = m_p0s[c.i2] + dt * v2;
		m_ps[c.i3] = m_p0s[c.i3] + dt * v3;
		c.lambda += impulse;
	}
}

void b2Rope::ApplyBendForces(float dt)
{
	// omega = 2 * pi * hz
	const float omega = 2.0f * b2_pi * m_tuning.bendHertz;

	for (int32 i = 0; i < m_bendCount; ++i)
	{
		const b2RopeBend& c = m_bendConstraints[i];

		b2Vec2 p1 = m_ps[c.i1];
		b2Vec2 p2 = m_ps[c.i2];
		b2Vec2 p3 = m_ps[c.i3];

		b2Vec2 v1 = m_vs[c.i1];
		b2Vec2 v2 = m_vs[c.i2];
		b2Vec2 v3 = m_vs[c.i3];

		b2Vec2 d1 = p2 - p1;
		b2Vec2 d2 = p3 - p2;

		float L1sqr, L2sqr;

		if (m_tuning.isometric)
		{
			L1sqr = c.L1 * c.L1;
			L2sqr = c.L2 * c.L2;
		}
		else
		{
			L1sqr = d1.LengthSquared();
			L2sqr = d2.LengthSquared();
		}

		if (L1sqr * L2sqr == 0.0f)
		{
			continue;
		}

		float a = b2Cross(d1, d2);
		float b = b2Dot(d1, d2);

		float angle = b2Atan2(a, b);

		b2Vec2 Jd1 = (-1.0f / L1sqr) * d1.Skew();
		b2Vec2 Jd2 = (1.0f / L2sqr) * d2.Skew();

		b2Vec2 J1 = -Jd1;
		b2Vec2 J2 = Jd1 - Jd2;
		b2Vec2 J3 = Jd2;

		float sum;
		if (m_tuning.fixedEffectiveMass)
		{
			sum = c.invEffectiveMass;
		}
		else
		{
			sum = c.invMass1 * b2Dot(J1, J1) + c.invMass2 * b2Dot(J2, J2) + c.invMass3 * b2Dot(J3, J3);
		}

		if (sum == 0.0f)
		{
			continue;
		}

		float mass = 1.0f / sum;

		const float spring = mass * omega * omega;
		const float damper = 2.0f * mass * m_tuning.bendDamping * omega;

		float C = angle;
		float Cdot = b2Dot(J1, v1) + b2Dot(J2, v2) + b2Dot(J3, v3);

		float impulse = -dt * (spring * C + damper * Cdot);

		m_vs[c.i1] += (c.invMass1 * impulse) * J1;
		m_vs[c.i2] += (c.invMass2 * impulse) * J2;
		m_vs[c.i3] += (c.invMass3 * impulse) * J3;
	}
}

void b2Rope::SolveBend_PBD_Distance()
{
	const float stiffness = m_tuning.bendStiffness;

	for (int32 i = 0; i < m_bendCount; ++i)
	{
		const b2RopeBend& c = m_bendConstraints[i];

		int32 i1 = c.i1;
		int32 i2 = c.i3;

		b2Vec2 p1 = m_ps[i1];
		b2Vec2 p2 = m_ps[i2];

		b2Vec2 d = p2 - p1;
		float L = d.Normalize();

		float sum = c.invMass1 + c.invMass3;
		if (sum == 0.0f)
		{
			continue;
		}

		float s1 = c.invMass1 / sum;
		float s2 = c.invMass3 / sum;

		p1 -= stiffness * s1 * (c.L1 + c.L2 - L) * d;
		p2 += stiffness * s2 * (c.L1 + c.L2 - L) * d;

		m_ps[i1] = p1;
		m_ps[i2] = p2;
	}
}

void b2Rope::SolveBend_PBD_Height()
{
	const float stiffness = m_tuning.bendStiffness;

	for (int32 i = 0; i < m_bendCount; ++i)
	{
		const b2RopeBend& c = m_bendConstraints[i];

		b2Vec2 p1 = m_ps[c.i1];
		b2Vec2 p2 = m_ps[c.i2];
		b2Vec2 p3 = m_ps[c.i3];

		b2Vec2 e1 = p2 - p1;
		b2Vec2 e2 = p3 - p2;
		b2Vec2 r = p3 - p1;

		float rr = r.LengthSquared();
		if (rr == 0.0f)
		{
			continue;
		}

		float alpha = b2Dot(e2, r) / rr;
		float beta = b2Dot(e1, r) / rr;
		b2Vec2 d = alpha * p1 + beta * p3 - p2;

		float dLen = d.Length();

		if (dLen == 0.0f)
		{
			continue;
		}

		b2Vec2 dHat = (1.0f / dLen) * d;

		b2Vec2 J1 = alpha * dHat;
		b2Vec2 J2 = -dHat;
		b2Vec2 J3 = beta * dHat;

		float sum = c.invMass1 * b2Dot(J1, J1) + c.invMass2 * b2Dot(J2, J2) + c.invMass3 * b2Dot(J3, J3);

		if (sum == 0.0f)
		{
			continue;
		}

		float C = dLen;
		float mass = 1.0f / sum;
		float impulse = -stiffness * mass * C;

		p1 += (c.invMass1 * impulse) * J1;
		p2 += (c.invMass2 * impulse) * J2;
		p3 += (c.invMass3 * impulse) * J3;

		m_ps[c.i1] = p1;
		m_ps[c.i2] = p2;
		m_ps[c.i3] = p3;
	}
}
void b2Rope::Draw(b2Draw* draw) const
{
	b2Color c(0.4f, 0.5f, 0.7f);
	b2Color pg(0.1f, 0.8f, 0.1f);
	b2Color pd(0.7f, 0.2f, 0.4f);

	for (int32 i = 0; i < m_count - 1; ++i)
	{
		draw->DrawSegment(m_ps[i], m_ps[i+1], c);

		const b2Color& pc = m_invMasses[i] > 0.0f ? pd : pg;
		draw->DrawPoint(m_ps[i], 5.0f, pc);
	}

	const b2Color& pc = m_invMasses[m_count - 1] > 0.0f ? pd : pg;
	draw->DrawPoint(m_ps[m_count - 1], 5.0f, pc);
}
