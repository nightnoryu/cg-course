#pragma once

#include "../libgl/Matrix4.h"
#include <cmath>
#include <numbers>
#include <numeric>
#include <tuple>
#include <boost/math/tools/roots.hpp>

// �������� ����� ������: http://www.astronet.ru/db/msg/1190817/node21.html#ll60

// ��������� �������, �������� ����� ����� ��������������� ���������, ���������������� ������ � ������� ���������
// M = E - e*sin(E)
// M - ������� ��������
// e - �������������� ������
// E - ��������������� ��������
// ������� ���������� F(E), F'(E), F''(E)
template <class T>
struct KeplerEquationFunctor
{
	KeplerEquationFunctor(T const& meanAnomaly, T const& eccentricity)
		: m_meanAnomaly(meanAnomaly)
		, m_eccenticity(eccentricity)
	{
	}

	std::tuple<T, T, T> operator()(T const& x) const
	{
		return std::make_tuple(
			x - m_meanAnomaly - m_eccenticity * sin(x), // F(x)
			1 - m_eccenticity * cos(x), // F'(x)
			m_eccenticity * sin(x)); // F''(x)
	}

private:
	T m_meanAnomaly;
	T m_eccenticity;
};
using KeplerEquationFunctorDouble = KeplerEquationFunctor<double>;

// ������� ��������� �������� ������� ��������� �������, �������� ��������������� ��������
// ��� ��������� ������� �������� � ��������������� ������
// � �������� ������� ������������ ����� Halley (http://en.wikipedia.org/wiki/Halley's_method), ��������� �����.
// ��������� ���������� boost.
template <class T>
T SolveKeplerEquation(T const& meanAnomaly, T const& eccentricity, std::uintmax_t& maxIteractions)
{
	const int digits = std::numeric_limits<T>::digits >> 1;

	return boost::math::tools::halley_iterate(
		KeplerEquationFunctor<T>(meanAnomaly, eccentricity),
		meanAnomaly, // guessed solution
		meanAnomaly - eccentricity, // min
		meanAnomaly + eccentricity, // max
		digits, // number of digits
		maxIteractions); // maximum iteractions
}

template <class T>
T SolveKeplerEquation(T const& meanAnomaly, T const& eccentricity)
{
	std::uintmax_t iterCount = 1000;
	return SolveKeplerEquation(meanAnomaly, eccentricity, iterCount);
}

template <class T>
class CEllipticOrbit
{
public:
	CEllipticOrbit(
		T const& a = 1, // ������� ������� �������
		T const& e = 0, // �������������� ������
		T const& n = std::numbers::pi * 2, // ������� �������� (�����c�� �� ������� �������)
		T const& i = 0, // ���������� ������
		T const& w = 0, // �������� ���������
		T const& l = 0, // ������� ����������� ����
		T const& T0 = 0 // ����� ����������� ����� ���������
		)
		: _a(a)
		, _e(e)
		, _i(i)
		, _w(w)
		, _l(l)
		, _n(n)
		, _T0(T0)
	{
	}

	T const& Eccentricity() const
	{
		return _e;
	}

	T const& LargeAxis() const { return _a; }

	T const& MeanMotion() const { return _n; }

	// ������� �������� (M)
	T MeanAnomaly(T const& t, bool wrap = true) const
	{
		T M = 2 * std::numbers::pi * _n * (t - _T0);
		if (wrap)
		{
			M = fmod(M, std::numbers::pi + std::numbers::pi);
		}
		return M;
	}

	T EccentricityAnomaly(T const& t, bool wrap = true) const
	{
		return SolveKeplerEquation(MeanAnomaly(t, wrap), _e);
	}

	T TrueAnomaly(T const& t, bool wrap = true) const
	{
		// ��������������� ��������
		T E = EccentricityAnomaly(t, wrap);
		// ������� ������������� ����
		T tg_v_2 = std::sqrt((1 + _e) / (1 - _e)) * std::tan(E / 2);
		// ������������ ����
		T v_2 = std::atan(tg_v_2);
		// �������� ��������
		return v_2 + v_2;
	}

	T RadiusVectorLength(T const& t) const
	{
		T E = EccentricityAnomaly(t, true);
		return _a * (1 - _e * std::cos(E));
	}

	CMatrix4<T> OrbitRotationMatrix() const
	{
		T cl = std::cos(_l);
		T sl = std::sin(_l);

		T cw = std::cos(_w);
		T sw = std::sin(_w);

		T ci = std::cos(_i);
		T si = std::sin(_i);

		CMatrix4<T> matrix(
			// ������� 1
			cl * cw - sl * sw * ci,
			sl * cw + cl * sw * ci,
			sw * si,
			0,
			// ������� 2
			-cl * sw - sl * cw * ci,
			-sl * cw + cl * cw * ci,
			cw * si,
			0,
			// ������� 3
			sl * si,
			cl * si,
			ci,
			0,
			// ������� 4
			0, 0, 0, 1);

		return matrix;
	}

	CVector2<T> PlanetPosition2D(T const& t, bool wrap = true) const
	{
		T const r = RadiusVectorLength(t);
		T const v = TrueAnomaly(t, wrap);
		return CVector2<T>(r * std::cos(v), r * std::sin(v));
	}

	CVector3<T> PlanetPosition3D(T const& t, bool wrap = true) const
	{
		CVector2<T> pos2d = PlanetPosition2D(t, wrap);
		CVector4<T> pos3d = OrbitRotationMatrix() * CVector4<T>(pos2d.x, pos2d.y, 0, 1);
		return pos3d;
	}

private:
	T _a;
	T _e;
	T _i;
	T _w;
	T _l;
	T _n;
	T _T0;
};

/*
F(E) = E - M - e * sin(E)
F'(E) = 1 - e * cos(E)
F''(E) = e * sin(E)
*/
