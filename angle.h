#pragma once

constexpr auto RadPi = 3.14159265358979323846;
constexpr auto DegPi = 180.0;

const float XM_PI = 3.141592654f;
const float XM_2PI = 6.283185307f;
const float XM_1DIVPI = 0.318309886f;
const float XM_1DIV2PI = 0.159154943f;
const float XM_PIDIV2 = 1.570796327f;
const float XM_PIDIV4 = 0.785398163f;

class ang_t {
public:
	// data member variables.
	float x, y, z;

public:
	// constructors.
	__forceinline ang_t() : x{}, y{}, z{} {}
	__forceinline ang_t(float x, float y, float z) : x{ x }, y{ y }, z{ z } {}

	// at-accesors.
	__forceinline float& at(const size_t index) {
		return ((float*)this)[index];
	}

	__forceinline float& at(const size_t index) const {
		return ((float*)this)[index];
	}

	// index operators.
	__forceinline float& operator( )(const size_t index) {
		return at(index);
	}

	__forceinline const float& operator( )(const size_t index) const {
		return at(index);
	}

	__forceinline float& operator[ ](const size_t index) {
		return at(index);
	}

	__forceinline const float& operator[ ](const size_t index) const {
		return at(index);
	}

	// equality operators.
	__forceinline bool operator==(const ang_t& v) const {
		return v.x == x && v.y == y && v.z == z;
	}

	__forceinline bool operator!=(const ang_t& v) const {
		return v.x != x || v.y != y || v.z != z;
	}

	__forceinline bool operator !() const {
		return !x && !y && !z;
	}

	// copy assignment.
	__forceinline ang_t& operator=(const ang_t& v) {
		x = v.x;
		y = v.y;
		z = v.z;

		return *this;
	}

	// negation-operator.
	__forceinline ang_t operator-() const {
		return ang_t(-x, -y, -z);
	}

	// arithmetic operators.
	__forceinline ang_t operator+(const ang_t& v) const {
		return {
			x + v.x,
			y + v.y,
			z + v.z
		};
	}

	__forceinline ang_t operator-(const ang_t& v) const {
		return {
			x - v.x,
			y - v.y,
			z - v.z
		};
	}

	__forceinline ang_t operator*(const ang_t& v) const {
		return {
			x * v.x,
			y * v.y,
			z * v.z
		};
	}

	__forceinline ang_t operator/(const ang_t& v) const {
		return {
			x / v.x,
			y / v.y,
			z / v.z
		};
	}

	// compound assignment operators.
	__forceinline ang_t& operator+=(const ang_t& v) {
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	__forceinline ang_t& operator-=(const ang_t& v) {
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	__forceinline ang_t& operator*=(const ang_t& v) {
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}

	__forceinline ang_t& operator/=(const ang_t& v) {
		x /= v.x;
		y /= v.y;
		z /= v.z;
		return *this;
	}

	// arithmetic operators w/ float.
	__forceinline ang_t operator+(float f) const {
		return {
			x + f,
			y + f,
			z + f
		};
	}

	__forceinline ang_t operator-(float f) const {
		return {
			x - f,
			y - f,
			z - f
		};
	}

	__forceinline ang_t operator*(float f) const {
		return {
			x * f,
			y * f,
			z * f
		};
	}

	__forceinline ang_t operator/(float f) const {
		return {
			x / f,
			y / f,
			z / f
		};
	}

	// compound assignment operators w/ float.
	__forceinline ang_t& operator+=(float f) {
		x += f;
		y += f;
		z += f;
		return *this;
	}

	__forceinline ang_t& operator-=(float f) {
		x -= f;
		y -= f;
		z -= f;
		return *this;
	}

	__forceinline ang_t& operator*=(float f) {
		x *= f;
		y *= f;
		z *= f;
		return *this;
	}

	__forceinline ang_t& operator/=(float f) {
		x /= f;
		y /= f;
		z /= f;
		return *this;
	}

	template<typename T>
	T ToRadians(T degrees) {
		return (degrees * (static_cast<T>(RadPi) / static_cast<T>(DegPi)));
	}

	inline void XMScalarSinCos
	(
		float* pSin,
		float* pCos,
		float  Value
	)
	{



		// Map Value to y in [-pi,pi], x = 2*pi*quotient + remainder.
		float quotient = XM_1DIV2PI * Value;
		if (Value >= 0.0f)
		{
			quotient = static_cast<float>(static_cast<int>(quotient + 0.5f));
		}
		else
		{
			quotient = static_cast<float>(static_cast<int>(quotient - 0.5f));
		}
		float y = Value - XM_2PI * quotient;

		// Map y to [-pi/2,pi/2] with sin(y) = sin(Value).
		float sign;
		if (y > XM_PIDIV2)
		{
			y = XM_PI - y;
			sign = -1.0f;
		}
		else if (y < -XM_PIDIV2)
		{
			y = -XM_PI - y;
			sign = -1.0f;
		}
		else
		{
			sign = +1.0f;
		}

		float y2 = y * y;

		// 11-degree mcfgmax approximation
		*pSin = (((((-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f) * y2 + 0.0083333310f) * y2 - 0.16666667f) * y2 + 1.0f) * y;

		// 10-degree mcfgmax approximation
		float p = ((((-2.6051615e-07f * y2 + 2.4760495e-05f) * y2 - 0.0013888378f) * y2 + 0.041666638f) * y2 - 0.5f) * y2 + 1.0f;
		*pCos = sign * p;
	}

	vec3_t ToVectors(vec3_t* side = nullptr, vec3_t* up = nullptr) {
		float  rad_pitch = ToRadians(this->x);
		float  rad_yaw = ToRadians(this->y);
		float sin_pitch;
		float sin_yaw;
		float sin_roll;
		float cos_pitch;
		float cos_yaw;
		float cos_roll;


		XMScalarSinCos(&sin_pitch, &cos_pitch, rad_pitch);
		XMScalarSinCos(&sin_yaw, &cos_yaw, rad_yaw);

		if (side || up)
			XMScalarSinCos(&sin_roll, &cos_roll, ToRadians(this->z));

		if (side) {
			side->x = -1.0f * sin_roll * sin_pitch * cos_yaw + -1.0f * cos_roll * -sin_yaw;
			side->y = -1.0f * sin_roll * sin_pitch * sin_yaw + -1.0f * cos_roll * cos_yaw;
			side->z = -1.0f * sin_roll * cos_pitch;
		}

		if (up) {
			up->x = cos_roll * sin_pitch * cos_yaw + -sin_roll * -sin_yaw;
			up->y = cos_roll * sin_pitch * sin_yaw + -sin_roll * cos_yaw;
			up->z = cos_roll * cos_pitch;
		}

		return { cos_pitch * cos_yaw, cos_pitch * sin_yaw, -sin_pitch };
	}

	// methods.
	__forceinline void clear() {
		x = y = z = 0.f;
	}

	__forceinline void normalize() {
		math::NormalizeAngle(x);
		math::NormalizeAngle(y);
		math::NormalizeAngle(z);
	}

	__forceinline ang_t normalized() const {
		auto vec = *this;
		vec.normalize();
		return vec;
	}

	__forceinline void clamp() {
		math::clamp(x, -89.f, 89.f);
		math::clamp(y, -180.f, 180.f);
		math::clamp(z, -90.f, 90.f);
	}

	__forceinline void SanitizeAngle() {
		math::NormalizeAngle(y);
		clamp();
	}
};