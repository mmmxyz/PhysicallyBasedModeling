#pragma once

#include <iostream>
#include <cstdint>
#include <cmath>

template <class T>
class vec2;
template <class T>
class vec3;
template <class T>
class vec4;
template <class T>
class quaternion;

template <class T>
class mat2;
template <class T>
class mat3;
template <class T>
class mat32;
template <class T>
class mat4;

using ivec2 = vec2<int32_t>;
using fvec2	  = vec2<float>;
using dvec2	  = vec2<double>;
using fvec3	  = vec3<float>;
using dvec3	  = vec3<double>;
using fvec4	  = vec4<float>;
using dvec4	  = vec4<double>;
using fquaternion = quaternion<float>;
using dquaternion = quaternion<double>;

using fmat2  = mat2<float>;
using dmat2  = mat2<double>;
using fmat3  = mat3<float>;
using dmat3  = mat3<double>;
using fmat32 = mat32<float>;
using dmat32 = mat32<double>;
using fmat4  = mat4<float>;
using dmat4  = mat4<double>;

//template class fvec2;
//template class fvec3;
//template class fvec4;
//template class fmat2;
//template class fmat3;
//template class fmat32;
//template class fmat4;

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
class vec2 {
    public:
	union {
		T cmp[2];
		struct {
			T x, y;
		};
	};

	explicit vec2(const T x, const T y);
	explicit vec2(const T array[2]);
	explicit vec2();

	T dot(const vec2<T> v) const;
	T cross(const vec2<T> v) const;
	T norm() const;
	T sqnorm() const;
	vec2<T> normalized() const;
	vec2<T> rotated() const;
	mat2<T> tensorproduct(const vec2<T> v) const;

	T& operator()(const uint32_t idx) &;
	const T& operator()(const uint32_t idx) const&;
	const T operator()(const uint32_t idx) &&;

	vec2<T>& operator=(const vec2<T> v) &;
	vec2<T>& operator+=(const vec2<T> v) &;
	vec2<T>& operator-=(const vec2<T> v) &;
	vec2<T>& operator*=(const T s) &;
	vec2<T>& operator/=(const T s) &;

	template <class U>
	operator vec2<U>() const;

	inline static vec2<T> zero()
	{
		return vec2<T>(0.0, 0.0);
	}
};

template <class T>
const vec2<T> operator+(const vec2<T> a, const vec2<T> b);
template <class T>
const vec2<T> operator-(const vec2<T> a, const vec2<T> b);
template <class T>
const vec2<T> operator-(const vec2<T> a);
template <class T>
const vec2<T> operator*(const T a, const vec2<T> b);
template <class T>
const vec2<T> operator*(const vec2<T> a, const T b);
template <class T>
const vec2<T> operator/(const vec2<T> a, const T b);

template <class T>
std::ostream& operator<<(std::ostream& os, const vec2<T> vec);

////////////////////

template <class T>
class vec3 {
    public:
	union {
		T cmp[3];
		struct {
			T x, y, z;
		};
	};

	explicit vec3(const T x, const T y, const T z);
	explicit vec3(const T array[3]);
	explicit vec3();

	T dot(const vec3<T> v) const;
	vec3<T> cross(const vec3<T> v) const;
	T norm() const;
	T sqnorm() const;
	vec3<T> normalized() const;
	mat3<T> torotation() const;
	mat3<T> skew() const;
	mat3<T> tensorproduct(const vec3<T> v) const;

	T& operator()(const uint32_t idx) &;
	const T& operator()(const uint32_t idx) const&;
	const T operator()(const uint32_t idx) &&;

	vec3<T>& operator=(const vec3<T> v) &;
	vec3<T>& operator+=(const vec3<T> v) &;
	vec3<T>& operator-=(const vec3<T> v) &;
	vec3<T>& operator*=(const T s) &;
	vec3<T>& operator/=(const T s) &;

	template <class U>
	operator vec3<U>() const;

	inline static vec3<T> zero()
	{
		return vec3<T>(0.0, 0.0, 0.0);
	}
};

template <class T>
const vec3<T> operator+(const vec3<T> a, const vec3<T> b);
template <class T>
const vec3<T> operator-(const vec3<T> a, const vec3<T> b);
template <class T>
const vec3<T> operator-(const vec3<T> a);
template <class T>
const vec3<T> operator*(const T a, const vec3<T> b);
template <class T>
const vec3<T> operator*(const vec3<T> a, const T b);
template <class T>
const vec3<T> operator/(const vec3<T> a, const T b);

template <class T>
std::ostream& operator<<(std::ostream& os, const vec3<T> vec);

////////////////////

template <class T>
class vec4 {
    public:
	union {
		T cmp[4];
		struct {
			T x, y, z, w;
		};
	};

	explicit vec4(const T x, const T y, const T z, const T w);
	explicit vec4(const T array[4]);
	explicit vec4();

	T dot(const vec4<T> v) const;
	T norm() const;
	T sqnorm() const;
	vec4<T> normalized() const;

	T& operator()(const uint32_t idx) &;
	const T& operator()(const uint32_t idx) const&;
	const T operator()(const uint32_t idx) &&;

	vec4<T>& operator=(const vec4<T> v) &;
	vec4<T>& operator+=(const vec4<T> v) &;
	vec4<T>& operator-=(const vec4<T> v) &;
	vec4<T>& operator*=(const T s) &;
	vec4<T>& operator/=(const T s) &;

	template <class U>
	operator vec4<U>() const;

	inline static vec4<T> zero()
	{
		return vec4<T>(0.0, 0.0, 0.0, 0.0);
	}
};

template <class T>
const vec4<T> operator+(const vec4<T> a, const vec4<T> b);
template <class T>
const vec4<T> operator-(const vec4<T> a, const vec4<T> b);
template <class T>
const vec4<T> operator-(const vec4<T> a);
template <class T>
const vec4<T> operator*(const T a, const vec4<T> b);
template <class T>
const vec4<T> operator*(const vec4<T> a, const T b);
template <class T>
const vec4<T> operator/(const vec4<T> a, const T b);

template <class T>
std::ostream& operator<<(std::ostream& os, const vec4<T> vec);

////////////////////

template <class T>
class quaternion : public vec4<T> {
    public:
	// q = w + xi + yj + zk
	explicit quaternion(const T x, const T y, const T z, const T w);
	explicit quaternion(const T array[4]);
	explicit quaternion(const T s, const vec3<T> v);
	explicit quaternion() { }
	explicit quaternion(const vec3<T> v);

	T gets() const;
	vec3<T> getv() const;
	quaternion<T> conjugate() const;
	vec3<T> rotatevector(const vec3<T> x) const;

	// quaternion to rotation vector
	inline static vec3<T> log(const quaternion<T> q)
	{
		return std::acos(q.gets()) * q.getv().normalized();
	}

	// rotation vector to quaternion
	inline static quaternion<T> exp(const vec3<T> w)
	{
		T halftheta = w.norm() * 0.5;
		T sin	    = std::sin(halftheta);
		T cos	    = std::cos(halftheta);
		vec3<T> n   = w.normalized();
		return quaternion<T>(n.x * sin, n.y * sin, n.z * sin, cos);
	}

	inline static quaternion<T> slerp(const quaternion<T> q0, quaternion<T> q1, T t)
	{
		if (q0.dot(q1) < 0.0)
			q1 *= -1.0;
		quaternion<T> delta = q1 * q0.conjugate();
		T halfTheta	    = std::acos(delta.w);
		return (std::sin(t * halfTheta) / std::sin(halfTheta)) * q0 + (std::sin(halfTheta - t * halfTheta) / std::sin(halfTheta)) * q1;
	}

	inline static quaternion<T> unit()
	{
		return quaternion<T>(0.0, 0.0, 0.0, 1.0);
	}
};

template <class T>
const quaternion<T> operator+(const quaternion<T> a, const quaternion<T> b);
template <class T>
const quaternion<T> operator-(const quaternion<T> a, const quaternion<T> b);
template <class T>
const quaternion<T> operator-(const quaternion<T> a);
template <class T>
const quaternion<T> operator*(const quaternion<T> a, const quaternion<T> b);
template <class T>
const quaternion<T> operator*(const T a, const quaternion<T> b);
template <class T>
const quaternion<T> operator*(const quaternion<T> a, const T b);
template <class T>
const quaternion<T> operator/(const quaternion<T> a, const T b);

template <class T>
std::ostream& operator<<(std::ostream& os, const quaternion<T> vec);

////////////////////

template <class T>
class mat2 {
    public:
	T cmp[4];
	//collum major
	//OpenGLと同じ
	//
	// 0 2
	// 1 3

	explicit mat2(const T m00, const T m01, const T m10, const T m11);
	explicit mat2(const vec2<T> row0, const vec2<T> row1);
	explicit mat2(const T array[4]);
	explicit mat2();

	T dot(const mat2<T> m) const;
	T norm() const;
	T sqnorm() const;
	mat2<T> transpose() const;
	T trace() const;
	T determinant() const;
	mat2<T> adjugate() const;
	mat2<T> inverse() const;

	T& operator()(const uint32_t idxr, const uint32_t idxc) &;
	const T& operator()(const uint32_t idxr, const uint32_t idxc) const&;
	const T operator()(const uint32_t idxr, const uint32_t idxc) &&;

	mat2<T>& operator=(const mat2<T> m) &;
	mat2<T>& operator+=(const mat2<T> m) &;
	mat2<T>& operator-=(const mat2<T> m) &;
	mat2<T>& operator*=(const T s) &;
	mat2<T>& operator/=(const T s) &;

	template <class U>
	operator mat2<U>() const;

	inline static mat2<T> identity()
	{
		return mat2<T>(1.0, 0.0, 0.0, 1.0);
	}
	inline static mat2<T> zero()
	{
		return mat2<T>(0.0, 0.0, 0.0, 0.0);
	}
};

template <class T>
const mat2<T> operator+(const mat2<T> a, const mat2<T> b);
template <class T>
const mat2<T> operator-(const mat2<T> a, const mat2<T> b);
template <class T>
const mat2<T> operator-(const mat2<T> a);
template <class T>
const mat2<T> operator*(const T a, const mat2<T> b);
template <class T>
const mat2<T> operator*(const mat2<T> a, const T b);
template <class T>
const mat2<T> operator*(const mat2<T> a, const mat2<T> b);
template <class T>
const mat2<T> operator*(const mat2<T> a, const vec2<T> b);
template <class T>
const mat2<T> operator/(const mat2<T> a, const T b);

template <class T>
std::ostream& operator<<(std::ostream& os, const mat2<T> mat);

////////////////////

template <class T>
class mat3 {
    public:
	T cmp[9];
	//collum major
	//OpenGLと同じ
	//
	// 0 3 6
	// 1 4 7
	// 2 5 8

	//explicit mat3(const std::initializer_list<std::initializer_list<T>>& ilist);
	//explicit mat3(const std::initializer_list<T>& ilist);
	explicit mat3(const T m00, const T m01, const T m02, const T m10, const T m11, const T m12, const T m20, const T m21, const T m22);
	explicit mat3(const T array[9]);
	explicit mat3(const vec3<T> row0, const vec3<T> row1, const vec3<T> row2);
	explicit mat3();

	T dot(const mat3<T> m) const;
	T norm() const;
	T sqnorm() const;
	mat3<T> transpose() const;
	T trace() const;
	T determinant() const;
	mat3<T> adjugate() const;
	mat3<T> inverse() const;

	T& operator()(const uint32_t idxr, const uint32_t idxc) &;
	const T& operator()(const uint32_t idxr, const uint32_t idxc) const&;
	const T operator()(const uint32_t idxr, const uint32_t idxc) &&;

	mat3<T>& operator=(const mat3<T> m) &;
	mat3<T>& operator+=(const mat3<T> m) &;
	mat3<T>& operator-=(const mat3<T> m) &;
	mat3<T>& operator*=(const T s) &;
	mat3<T>& operator/=(const T s) &;

	template <class U>
	operator mat3<U>() const;

	inline static mat3<T> identity()
	{
		return mat3<T>(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
	}
	inline static mat3<T> zero()
	{
		return mat3<T>(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
	}
};

template <class T>
const mat3<T> operator+(const mat3<T> a, const mat3<T> b);
template <class T>
const mat3<T> operator-(const mat3<T> a, const mat3<T> b);
template <class T>
const mat3<T> operator-(const mat3<T> a);
template <class T>
const mat3<T> operator*(const T a, const mat3<T> b);
template <class T>
const mat3<T> operator*(const mat3<T> a, const T b);
template <class T>
const mat3<T> operator*(const mat3<T> a, const mat3<T> b);
template <class T>
const mat3<T> operator*(const mat3<T> a, const vec3<T> b);
template <class T>
const mat3<T> operator/(const mat3<T> a, const T b);

template <class T>
std::ostream& operator<<(std::ostream& os, const mat3<T> mat);

////////////////////

template <class T>
class mat4 {
    public:
	T cmp[16];
	//collum major
	//OpenGLと同じ
	//
	// 0  4  8 12
	// 1  5  9 13
	// 2  6 10 14
	// 3  7 11 15

	explicit mat4(const mat3<T> mat, const vec3<T> vec);

	mat4<T> transpose() const;

	T& operator()(const uint32_t idxr, const uint32_t idxc) &;
	const T& operator()(const uint32_t idxr, const uint32_t idxc) const&;
	const T operator()(const uint32_t idxr, const uint32_t idxc) &&;

	mat4<T>& operator=(const mat4<T> m) &;
	mat4<T>& operator+=(const mat4<T> m) &;
	mat4<T>& operator-=(const mat4<T> m) &;
	mat4<T>& operator*=(const T s) &;
	mat4<T>& operator/=(const T s) &;

	template <class U>
	operator mat4<U>() const;

	inline static mat4<T> identity()
	{
		return mat4<T>(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0);
	}
	inline static mat4<T> zero()
	{
		return mat4<T>(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
	}
};

template <class T>
const mat4<T> operator+(const mat4<T> a, const mat4<T> b);
template <class T>
const mat4<T> operator-(const mat4<T> a, const mat4<T> b);
template <class T>
const mat4<T> operator-(const mat4<T> a);
template <class T>
const mat4<T> operator*(const T a, const mat4<T> b);
template <class T>
const mat4<T> operator*(const mat4<T> a, const T b);
template <class T>
const mat4<T> operator*(const mat4<T> a, const mat4<T> b);
template <class T>
const mat4<T> operator*(const mat4<T> a, const vec4<T> b);
template <class T>
const mat4<T> operator/(const mat4<T> a, const T b);

template <class T>
std::ostream& operator<<(std::ostream& os, const mat4<T> mat);

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

template <class T>
inline vec2<T>::vec2(const T x, const T y)
    : x(x)
    , y(y)
{
}
template <class T>
inline vec2<T>::vec2(const T array[2])
    : x(array[0])
    , y(array[1])
{
}
template <class T>
inline vec2<T>::vec2()
{
}

template <class T>
inline T vec2<T>::dot(const vec2<T> v) const
{
	return x * v.x + y * v.y;
}
template <class T>
inline T vec2<T>::cross(const vec2<T> v) const
{
	return x * v.y - y * v.x;
}
template <class T>
inline T vec2<T>::sqnorm() const
{
	return x * x + y * y;
}
template <class T>
inline T vec2<T>::norm() const
{
	T t = x * x + y * y;
	if (t < 0.0000001)
		return 0.0;
	else
		return std::sqrt(t);
}
template <class T>
inline vec2<T> vec2<T>::normalized() const
{
	T norm = this->norm();
	if (norm < 0.0000001)
		return vec2<T>::zero();
	else
		return (*this) / norm;
}
template <class T>
inline vec2<T> vec2<T>::rotated() const
{
	return vec2<T>(-y, x);
}

template <class T>
inline T& vec2<T>::operator()(const uint32_t idx) &
{
	return cmp[idx];
}
template <class T>
inline const T& vec2<T>::operator()(const uint32_t idx) const&
{
	return cmp[idx];
}
template <class T>
inline const T vec2<T>::operator()(const uint32_t idx) &&
{
	return cmp[idx];
}

template <class T>
inline vec2<T>& vec2<T>::operator=(const vec2<T> v) &
{
	x = v.x;
	y = v.y;
	return *this;
}
template <class T>
inline vec2<T>& vec2<T>::operator+=(const vec2<T> v) &
{
	x += v.x;
	y += v.y;
	return *this;
}
template <class T>
inline vec2<T>& vec2<T>::operator-=(const vec2<T> v) &
{
	x -= v.x;
	y -= v.y;
	return *this;
}
template <class T>
inline vec2<T>& vec2<T>::operator*=(const T s) &
{
	x *= s;
	y *= s;
	return *this;
}
template <class T>
inline vec2<T>& vec2<T>::operator/=(const T s) &
{
	x /= s;
	y /= s;
	return *this;
}

template <class T>
template <class U>
inline vec2<T>::operator vec2<U>() const
{
	return vec2<U>(
	    static_cast<U>(x),
	    static_cast<U>(y));
}

template <class T>
inline const vec2<T> operator+(const vec2<T> a, const vec2<T> b)
{
	return vec2<T>(
	    a.x + b.x,
	    a.y + b.y);
}
template <class T>
inline const vec2<T> operator-(const vec2<T> a, const vec2<T> b)
{
	return vec2<T>(
	    a.x - b.x,
	    a.y - b.y);
}
template <class T>
inline const vec2<T> operator-(const vec2<T> a)
{
	return vec2<T>(
	    -a.x,
	    -a.y);
}
template <class T>
inline const vec2<T> operator*(const T a, const vec2<T> b)
{
	return vec2<T>(
	    a * b.x,
	    a * b.y);
}
template <class T>
inline const vec2<T> operator*(const vec2<T> a, const T b)
{
	return vec2<T>(
	    b * a.x,
	    b * a.y);
}
template <class T>
inline const vec2<T> operator/(const vec2<T> a, const T b)
{
	return vec2<T>(
	    a.x / b,
	    a.y / b);
}

template <class T>
inline std::ostream& operator<<(std::ostream& os, const vec2<T> vec)
{
	os << vec.x << " " << vec.y;
	return os;
}

////////////////////

template <class T>
inline vec3<T>::vec3(const T x, const T y, const T z)
    : x(x)
    , y(y)
    , z(z)
{
}
template <class T>
inline vec3<T>::vec3(const T array[3])
    : x(array[0])
    , y(array[1])
    , z(array[2])
{
}
template <class T>
inline vec3<T>::vec3()
{
}

template <class T>
inline T vec3<T>::dot(const vec3<T> v) const
{
	return x * v.x + y * v.y + z * v.z;
}
template <class T>
inline vec3<T> vec3<T>::cross(const vec3<T> v) const
{
	return vec3<T>(
	    y * v.z - z * v.y,
	    z * v.x - x * v.z,
	    x * v.y - y * v.x);
}
template <class T>
inline T vec3<T>::sqnorm() const
{
	return x * x + y * y + z * z;
}
template <class T>
inline T vec3<T>::norm() const
{
	T t = x * x + y * y + z * z;
	if (t < 0.0000001)
		return 0.0;
	else
		return std::sqrt(t);
}
template <class T>
inline vec3<T> vec3<T>::normalized() const
{
	T norm = this->norm();
	if (norm < 0.0000001)
		return vec3<T>::zero();
	else
		return (*this) / norm;
}
template <class T>
inline mat3<T> vec3<T>::torotation() const
{
	T omega	 = this->norm();
	T omega2 = this->sqnorm();
	if (omega2 < 0.0000001)
		return mat3<T>::identity();
	T cos = std::cos(omega);
	T sin = std::sin(omega);

	return cos * mat3<T>::identity() + ((1 - cos) / omega2) * this->tensorproduct(*this) + (sin / omega) * this->skew();
}
template <class T>
inline mat3<T> vec3<T>::skew() const
{
	return mat3<T>(
	    vec3(0.0, -z, y),
	    vec3(z, 0.0, -x),
	    vec3(-y, x, 0.0));
}
template <class T>
inline mat3<T> vec3<T>::tensorproduct(const vec3<T> v) const
{
	return mat3<T>(
	    vec3(x * v.x, x * v.y, x * v.z),
	    vec3(y * v.x, y * v.y, y * v.z),
	    vec3(z * v.x, z * v.y, z * v.z));
}

template <class T>
inline T& vec3<T>::operator()(const uint32_t idx) &
{
	return cmp[idx];
}
template <class T>
inline const T& vec3<T>::operator()(const uint32_t idx) const&
{
	return cmp[idx];
}
template <class T>
inline const T vec3<T>::operator()(const uint32_t idx) &&
{
	return cmp[idx];
}

template <class T>
inline vec3<T>& vec3<T>::operator=(const vec3<T> v) &
{
	x = v.x;
	y = v.y;
	z = v.z;
	return *this;
}
template <class T>
inline vec3<T>& vec3<T>::operator+=(const vec3<T> v) &
{
	x += v.x;
	y += v.y;
	z += v.z;
	return *this;
}
template <class T>
inline vec3<T>& vec3<T>::operator-=(const vec3<T> v) &
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	return *this;
}
template <class T>
inline vec3<T>& vec3<T>::operator*=(const T s) &
{
	x *= s;
	y *= s;
	z *= s;
	return *this;
}
template <class T>
inline vec3<T>& vec3<T>::operator/=(const T s) &
{
	x /= s;
	y /= s;
	z /= s;
	return *this;
}

template <class T>
template <class U>
inline vec3<T>::operator vec3<U>() const
{
	return vec3<U>(
	    static_cast<U>(x),
	    static_cast<U>(y),
	    static_cast<U>(z));
}

template <class T>
inline const vec3<T> operator+(const vec3<T> a, const vec3<T> b)
{
	return vec3<T>(
	    a.x + b.x,
	    a.y + b.y,
	    a.z + b.z);
}
template <class T>
inline const vec3<T> operator-(const vec3<T> a, const vec3<T> b)
{
	return vec3<T>(
	    a.x - b.x,
	    a.y - b.y,
	    a.z - b.z);
}
template <class T>
inline const vec3<T> operator-(const vec3<T> a)
{
	return vec3<T>(
	    -a.x,
	    -a.y,
	    -a.z);
}
template <class T>
inline const vec3<T> operator*(const T a, const vec3<T> b)
{
	return vec3<T>(
	    a * b.x,
	    a * b.y,
	    a * b.z);
}
template <class T>
inline const vec3<T> operator*(const vec3<T> a, const T b)
{
	return vec3<T>(
	    b * a.x,
	    b * a.y,
	    b * a.z);
}
template <class T>
inline const vec3<T> operator/(const vec3<T> a, const T b)
{
	return vec3<T>(
	    a.x / b,
	    a.y / b,
	    a.z / b);
}

template <class T>
inline std::ostream& operator<<(std::ostream& os, const vec3<T> vec)
{
	os << vec.x << " " << vec.y << " " << vec.z;
	return os;
}

////////////////////
//
template <class T>
inline vec4<T>::vec4(const T x, const T y, const T z, const T w)
    : x(x)
    , y(y)
    , z(z)
    , w(w)
{
}
template <class T>
inline vec4<T>::vec4(const T array[4])
    : x(array[0])
    , y(array[1])
    , z(array[2])
    , w(array[3])
{
}
template <class T>
inline vec4<T>::vec4()
{
}

template <class T>
inline T vec4<T>::dot(const vec4<T> v) const
{
	return x * v.x + y * v.y + z * v.z + w * v.w;
}
template <class T>
inline T vec4<T>::sqnorm() const
{
	return x * x + y * y + z * z + w * w;
}
template <class T>
inline T vec4<T>::norm() const
{
	T t = x * x + y * y + z * z + w * w;
	if (t < 0.0000001)
		return 0.0;
	else
		return std::sqrt(t);
}
template <class T>
inline vec4<T> vec4<T>::normalized() const
{
	T norm = this->norm();
	if (norm < 0.0000001)
		return vec4<T>::zero();
	else
		return (*this) / norm;
}

template <class T>
inline T& vec4<T>::operator()(const uint32_t idx) &
{
	return cmp[idx];
}
template <class T>
inline const T& vec4<T>::operator()(const uint32_t idx) const&
{
	return cmp[idx];
}
template <class T>
inline const T vec4<T>::operator()(const uint32_t idx) &&
{
	return cmp[idx];
}

template <class T>
inline vec4<T>& vec4<T>::operator=(const vec4<T> v) &
{
	x = v.x;
	y = v.y;
	z = v.z;
	w = v.w;
	return *this;
}
template <class T>
inline vec4<T>& vec4<T>::operator+=(const vec4<T> v) &
{
	x += v.x;
	y += v.y;
	z += v.z;
	w += v.w;
	return *this;
}
template <class T>
inline vec4<T>& vec4<T>::operator-=(const vec4<T> v) &
{
	x -= v.x;
	y -= v.y;
	z -= v.z;
	w -= v.w;
	return *this;
}
template <class T>
inline vec4<T>& vec4<T>::operator*=(const T s) &
{
	x *= s;
	y *= s;
	z *= s;
	w *= s;
	return *this;
}
template <class T>
inline vec4<T>& vec4<T>::operator/=(const T s) &
{
	x /= s;
	y /= s;
	z /= s;
	w /= s;
	return *this;
}

template <class T>
template <class U>
inline vec4<T>::operator vec4<U>() const
{
	return vec4<U>(
	    static_cast<U>(x),
	    static_cast<U>(y),
	    static_cast<U>(z),
	    static_cast<U>(w));
}

template <class T>
inline const vec4<T> operator+(const vec4<T> a, const vec4<T> b)
{
	return vec4<T>(
	    a.x + b.x,
	    a.y + b.y,
	    a.z + b.z,
	    a.w + b.w);
}
template <class T>
inline const vec4<T> operator-(const vec4<T> a, const vec4<T> b)
{
	return vec4<T>(
	    a.x - b.x,
	    a.y - b.y,
	    a.z - b.z,
	    a.w - b.w);
}
template <class T>
inline const vec4<T> operator-(const vec4<T> a)
{
	return vec4<T>(
	    -a.x,
	    -a.y,
	    -a.z,
	    -a.w);
}
template <class T>
inline const vec4<T> operator*(const T a, const vec4<T> b)
{
	return vec4<T>(
	    a * b.x,
	    a * b.y,
	    a * b.z,
	    a * b.w);
}
template <class T>
inline const vec4<T> operator*(const vec4<T> a, const T b)
{
	return vec4<T>(
	    b * a.x,
	    b * a.y,
	    b * a.z,
	    b * a.w);
}
template <class T>
inline const vec4<T> operator/(const vec4<T> a, const T b)
{
	return vec4<T>(
	    a.x / b,
	    a.y / b,
	    a.z / b,
	    a.w / b);
}

template <class T>
inline std::ostream& operator<<(std::ostream& os, const vec4<T> vec)
{
	os << vec.x << " " << vec.y << " " << vec.z << " " << vec.w;
	return os;
}

////////////////////

template <class T>
inline quaternion<T>::quaternion(const T x, const T y, const T z, const T w)
    : vec4<T>(x, y, z, w)
{
}
template <class T>
inline quaternion<T>::quaternion(const T array[4])
    : vec4<T>(array)
{
}
template <class T>
inline quaternion<T>::quaternion(const T s, const vec3<T> v)
    : vec4<T>(v.x, v.y, v.z, s)
{
}
template <class T>
inline quaternion<T>::quaternion(const vec3<T> v)
{
	*this = quaternion<T>::exp(v);
}

template <class T>
inline T quaternion<T>::gets() const
{
	return this->w;
}

template <class T>
inline vec3<T> quaternion<T>::getv() const
{
	return vec3<T>(this->x, this->y, this->z);
}

template <class T>
inline quaternion<T> quaternion<T>::conjugate() const
{
	return quaternion<T>(-this->x, -this->y, -this->z, this->w);
}

template <class T>
inline vec3<T> quaternion<T>::rotatevector(const vec3<T> x) const
{
	const vec3<T> v = getv();
	const T s	= gets();
	return 2.0 * v.dot(x) * v + s * s * x + 2.0 * s * v.cross(x) - this->norm() * x;
}

template <class T>
inline const quaternion<T> operator+(const quaternion<T> a, const quaternion<T> b)
{
	return quaternion<T>(
	    a.x + b.x,
	    a.y + b.y,
	    a.z + b.z,
	    a.w + b.w);
}
template <class T>
inline const quaternion<T> operator-(const quaternion<T> a, const quaternion<T> b)
{
	return quaternion<T>(
	    a.x - b.x,
	    a.y - b.y,
	    a.z - b.z,
	    a.w - b.w);
}
template <class T>
inline const quaternion<T> operator-(const quaternion<T> a)
{
	return quaternion<T>(
	    -a.x,
	    -a.y,
	    -a.z,
	    -a.w);
}
template <class T>
inline const quaternion<T> operator*(const quaternion<T> a, const quaternion<T> b)
{
	T s0	   = a.gets();
	T s1	   = b.gets();
	vec3<T> v0 = a.getv();
	vec3<T> v1 = a.getv();
	T s	   = s0 * s1 - v0.dot(v1);
	vec3<T> v  = s0 * v1 + s1 * v0 + v0.cross(v1);
	return quaternion<T>(s, v);
}
template <class T>
inline const quaternion<T> operator*(const T a, const quaternion<T> b)
{
	return quaternion<T>(
	    a * b.x,
	    a * b.y,
	    a * b.z,
	    a * b.w);
}
template <class T>
inline const quaternion<T> operator*(const quaternion<T> a, const T b)
{
	return quaternion<T>(
	    b * a.x,
	    b * a.y,
	    b * a.z,
	    b * a.w);
}
template <class T>
inline const quaternion<T> operator/(const quaternion<T> a, const T b)
{
	return quaternion<T>(
	    a.x / b,
	    a.y / b,
	    a.z / b,
	    a.w / b);
}

template <class T>
inline std::ostream& operator<<(std::ostream& os, const quaternion<T> vec)
{
	os << vec.x << " " << vec.y << " " << vec.z << " " << vec.w;
	return os;
}

////////////////////

template <class T>
inline mat2<T>::mat2(const T m00, const T m01, const T m10, const T m11)
    : cmp { m00, m01, m10, m11 }
{
}
template <class T>
inline mat2<T>::mat2(const T(array)[4])
    : cmp { array[0], array[1], array[2], array[3] }
{
}
template <class T>
inline mat2<T>::mat2(const vec2<T> row0, const vec2<T> row1)
    : cmp {
	    row0.cmp[0],
	    row1.cmp[0],
	    row0.cmp[1],
	    row1.cmp[1]
    }
{
}
template <class T>
inline mat2<T>::mat2()
{
}
template <class T>
inline T mat2<T>::dot(const mat2<T> m) const
{
	return cmp[0] * m.cmp[0] + cmp[1] * m.cmp[1] + cmp[2] * m.cmp[2] + cmp[3] * m.cmp[3];
}
template <class T>
inline T mat2<T>::sqnorm() const
{
	return this->dot(*this);
}
template <class T>
inline T mat2<T>::norm() const
{
	T t = this->sqnorm();
	if (t < 0.0000000001)
		return 0.0;
	else
		return std::sqrt(t);
}
template <class T>
inline mat2<T> mat2<T>::transpose() const
{
	return mat2<T>(cmp[0], cmp[2], cmp[1], cmp[3]);
}
template <class T>
inline T mat2<T>::trace() const
{
	return cmp[0] + cmp[3];
}
template <class T>
inline T mat2<T>::determinant() const
{
	return cmp[0] * cmp[3] - cmp[1] * cmp[2];
}
template <class T>
inline mat2<T> mat2<T>::adjugate() const
{
	return mat2<T>(
	    cmp[3], -cmp[1],
	    -cmp[2], cmp[0]);
}
template <class T>
inline mat2<T> mat2<T>::inverse() const
{
	T det = this->determinant();
	//assert(det >= 0.0000000001 && "singular matrix");
	return (this->adjugate()) / det;
}

template <class T>
inline T& mat2<T>::operator()(const uint32_t idxr, const uint32_t idxc) &
{
	return cmp[idxr + idxc * 2];
}
template <class T>
inline const T& mat2<T>::operator()(const uint32_t idxr, const uint32_t idxc) const&
{
	return cmp[idxr + idxc * 2];
}
template <class T>
inline const T mat2<T>::operator()(const uint32_t idxr, const uint32_t idxc) &&
{
	return cmp[idxr + idxc * 2];
}

template <class T>
inline mat2<T>& mat2<T>::operator=(const mat2<T> m) &
{
	for (uint32_t i = 0; i < 4; i++) {
		this->cmp[i] = m.cmp[i];
	}
	return *this;
}
template <class T>
inline mat2<T>& mat2<T>::operator+=(const mat2<T> m) &
{
	for (uint32_t i = 0; i < 4; i++) {
		this->cmp[i] += m.cmp[i];
	}
	return *this;
}
template <class T>
inline mat2<T>& mat2<T>::operator-=(const mat2<T> m) &
{
	for (uint32_t i = 0; i < 4; i++) {
		this->cmp[i] -= m.cmp[i];
	}
	return *this;
}
template <class T>
inline mat2<T>& mat2<T>::operator*=(const T s) &
{
	for (uint32_t i = 0; i < 4; i++) {
		this->cmp[i] *= s;
	}
	return *this;
}
template <class T>
inline mat2<T>& mat2<T>::operator/=(const T s) &
{
	for (uint32_t i = 0; i < 4; i++) {
		this->cmp[i] /= s;
	}
	return *this;
}

template <class T>
template <class U>
inline mat2<T>::operator mat2<U>() const
{
	return mat2<U>(
	    static_cast<U>(cmp[0]),
	    static_cast<U>(cmp[1]),
	    static_cast<U>(cmp[2]),
	    static_cast<U>(cmp[3]));
}

template <class T>
inline const mat2<T> operator+(const mat2<T> a, const mat2<T> b)
{
	return mat2<T>(
	    a.cmp[0] + b.cmp[0],
	    a.cmp[1] + b.cmp[1],
	    a.cmp[2] + b.cmp[2],
	    a.cmp[3] + b.cmp[3]);
}
template <class T>
inline const mat2<T> operator-(const mat2<T> a, const mat2<T> b)
{
	return mat2<T>(
	    a.cmp[0] - b.cmp[0],
	    a.cmp[1] - b.cmp[1],
	    a.cmp[2] - b.cmp[2],
	    a.cmp[3] - b.cmp[3]);
}
template <class T>
inline const mat2<T> operator-(const mat2<T> a)
{
	return mat2<T>(
	    -a.cmp[0],
	    -a.cmp[1],
	    -a.cmp[2],
	    -a.cmp[3]);
}
template <class T>
inline const mat2<T> operator*(const T a, const mat2<T> b)
{
	return mat2<T>(
	    a * b.cmp[0],
	    a * b.cmp[1],
	    a * b.cmp[2],
	    a * b.cmp[3]);
}
template <class T>
inline const mat2<T> operator*(const mat2<T> a, const T b)
{
	return mat2<T>(
	    b * a.cmp[0],
	    b * a.cmp[1],
	    b * a.cmp[2],
	    b * a.cmp[3]);
}
template <class T>
inline const mat2<T> operator*(const mat2<T> a, const mat2<T> b)
{
	return mat2<T>(
	    a.cmp[0] * b.cmp[0] + a.cmp[2] * b.cmp[1],
	    a.cmp[0] * b.cmp[2] + a.cmp[2] * b.cmp[3],
	    a.cmp[1] * b.cmp[0] + a.cmp[3] * b.cmp[1],
	    a.cmp[1] * b.cmp[2] + a.cmp[3] * b.cmp[3]);
}
template <class T>
inline const vec2<T> operator*(const mat2<T> a, const vec2<T> b)
{
	return vec2<T>(
	    a.cmp[0] * b.x + a.cmp[2] * b.y,
	    a.cmp[1] * b.x + a.cmp[3] * b.y);
}
template <class T>
inline const mat2<T> operator/(const mat2<T> a, const T b)
{
	return mat2<T>(
	    a.cmp[0] / b,
	    a.cmp[1] / b,
	    a.cmp[2] / b,
	    a.cmp[3] / b);
}

template <class T>
inline std::ostream& operator<<(std::ostream& os, const mat2<T> mat)
{
	for (int32_t i = 0; i < 2; i++) {
		for (int32_t j = 0; j < 2; j++) {
			os << mat(i, j) << " ";
		}
		os << std::endl;
	}
	return os;
}

////////////////////

template <class T>
inline mat3<T>::mat3(const T m00, const T m01, const T m02, const T m10, const T m11, const T m12, const T m20, const T m21, const T m22)
    : cmp {
	    m00,
	    m01,
	    m02,
	    m10,
	    m11,
	    m12,
	    m20,
	    m21,
	    m22
    }
{
}
template <class T>
inline mat3<T>::mat3(const T array[9])
    : cmp {
	    array[0],
	    array[1],
	    array[2],
	    array[3],
	    array[4],
	    array[5],
	    array[6],
	    array[7],
	    array[8]
    }
{
}
template <class T>
inline mat3<T>::mat3(const vec3<T> row0, const vec3<T> row1, const vec3<T> row2)
    : cmp {
	    row0.cmp[0],
	    row1.cmp[0],
	    row2.cmp[0],
	    row0.cmp[1],
	    row1.cmp[1],
	    row2.cmp[1],
	    row0.cmp[2],
	    row1.cmp[2],
	    row2.cmp[2]
    }
{
}
template <class T>
inline mat3<T>::mat3()
{
}

template <class T>
inline T mat3<T>::dot(const mat3<T> m) const
{
	return cmp[0] * m.cmp[0] + cmp[1] * m.cmp[1] + cmp[2] * m.cmp[2] + cmp[3] * m.cmp[3] + cmp[4] * m.cmp[4] + cmp[5] * m.cmp[5] + cmp[6] * m.cmp[6] + cmp[7] * m.cmp[7] + cmp[8] * m.cmp[8];
}
template <class T>
inline T mat3<T>::sqnorm() const
{
	return this->dot(*this);
}
template <class T>
inline T mat3<T>::norm() const
{
	T t = this->sqnorm();
	if (t < 0.0000000001)
		return 0.0;
	else
		return std::sqrt(t);
}
template <class T>
inline mat3<T> mat3<T>::transpose() const
{
	return mat3<T>(
	    cmp[0],
	    cmp[3],
	    cmp[6],
	    cmp[1],
	    cmp[4],
	    cmp[7],
	    cmp[2],
	    cmp[5],
	    cmp[8]);
}
template <class T>
inline T mat3<T>::trace() const
{
	return cmp[0] + cmp[4] + cmp[8];
}
template <class T>
inline T mat3<T>::determinant() const
{
	return cmp[6] * (cmp[1] * cmp[5] - cmp[2] * cmp[4]) + cmp[7] * (cmp[2] * cmp[3] - cmp[0] * cmp[5]) + cmp[8] * (cmp[0] * cmp[4] - cmp[1] * cmp[3]);
}
template <class T>
inline mat3<T> mat3<T>::adjugate() const
{
	return mat3<T>(
	    (cmp[4] * cmp[8] - cmp[5] * cmp[7]),
	    -(cmp[1] * cmp[8] - cmp[2] * cmp[7]),
	    (cmp[1] * cmp[5] - cmp[2] * cmp[4]),
	    -(cmp[3] * cmp[8] - cmp[5] * cmp[6]),
	    (cmp[0] * cmp[8] - cmp[2] * cmp[6]),
	    -(cmp[0] * cmp[3] - cmp[2] * cmp[3]),
	    (cmp[3] * cmp[7] - cmp[4] * cmp[6]),
	    -(cmp[0] * cmp[7] - cmp[1] * cmp[6]),
	    (cmp[0] * cmp[4] - cmp[1] * cmp[3]));
}
template <class T>
inline mat3<T> mat3<T>::inverse() const
{
	T det = this->determinant();
	//assert(det >= 0.0000000001 && "singular matrix");
	return (this->adjugate()) / det;
}

template <class T>
inline T& mat3<T>::operator()(const uint32_t idxr, const uint32_t idxc) &
{
	return cmp[idxr + idxc * 3];
}
template <class T>
inline const T& mat3<T>::operator()(const uint32_t idxr, const uint32_t idxc) const&
{
	return cmp[idxr + idxc * 3];
}
template <class T>
inline const T mat3<T>::operator()(const uint32_t idxr, const uint32_t idxc) &&
{
	return cmp[idxr + idxc * 3];
}

template <class T>
inline mat3<T>& mat3<T>::operator=(const mat3<T> m) &
{
	for (uint32_t i = 0; i < 9; i++) {
		this->cmp[i] = m.cmp[i];
	}
	return *this;
}
template <class T>
inline mat3<T>& mat3<T>::operator+=(const mat3<T> m) &
{
	for (uint32_t i = 0; i < 9; i++) {
		this->cmp[i] += m.cmp[i];
	}
	return *this;
}
template <class T>
inline mat3<T>& mat3<T>::operator-=(const mat3<T> m) &
{
	for (uint32_t i = 0; i < 9; i++) {
		this->cmp[i] -= m.cmp[i];
	}
	return *this;
}
template <class T>
inline mat3<T>& mat3<T>::operator*=(const T s) &
{
	for (uint32_t i = 0; i < 9; i++) {
		this->cmp[i] *= s;
	}
	return *this;
}
template <class T>
inline mat3<T>& mat3<T>::operator/=(const T s) &
{
	for (uint32_t i = 0; i < 9; i++) {
		this->cmp[i] /= s;
	}
	return *this;
}

template <class T>
template <class U>
inline mat3<T>::operator mat3<U>() const
{
	return mat3<U>(
	    static_cast<U>(cmp[0]),
	    static_cast<U>(cmp[1]),
	    static_cast<U>(cmp[2]),
	    static_cast<U>(cmp[3]),
	    static_cast<U>(cmp[4]),
	    static_cast<U>(cmp[5]),
	    static_cast<U>(cmp[6]),
	    static_cast<U>(cmp[7]),
	    static_cast<U>(cmp[8]));
}

template <class T>
inline const mat3<T> operator+(const mat3<T> a, const mat3<T> b)
{
	return mat3<T>(
	    a.cmp[0] + b.cmp[0],
	    a.cmp[1] + b.cmp[1],
	    a.cmp[2] + b.cmp[2],
	    a.cmp[3] + b.cmp[3],
	    a.cmp[4] + b.cmp[4],
	    a.cmp[5] + b.cmp[5],
	    a.cmp[6] + b.cmp[6],
	    a.cmp[7] + b.cmp[7],
	    a.cmp[8] + b.cmp[8]);
}
template <class T>
inline const mat3<T> operator-(const mat3<T> a, const mat3<T> b)
{
	return mat3<T>(
	    a.cmp[0] - b.cmp[0],
	    a.cmp[1] - b.cmp[1],
	    a.cmp[2] - b.cmp[2],
	    a.cmp[3] - b.cmp[3],
	    a.cmp[4] - b.cmp[4],
	    a.cmp[5] - b.cmp[5],
	    a.cmp[6] - b.cmp[6],
	    a.cmp[7] - b.cmp[7],
	    a.cmp[8] - b.cmp[8]);
}
template <class T>
inline const mat3<T> operator-(const mat3<T> a)
{
	return mat3<T>(
	    -a.cmp[0],
	    -a.cmp[1],
	    -a.cmp[2],
	    -a.cmp[3],
	    -a.cmp[4],
	    -a.cmp[5],
	    -a.cmp[6],
	    -a.cmp[7],
	    -a.cmp[8]);
}
template <class T>
inline const mat3<T> operator*(const T a, const mat3<T> b)
{
	return mat3<T>(
	    a * b.cmp[0],
	    a * b.cmp[1],
	    a * b.cmp[2],
	    a * b.cmp[3],
	    a * b.cmp[4],
	    a * b.cmp[5],
	    a * b.cmp[6],
	    a * b.cmp[7],
	    a * b.cmp[8]);
}
template <class T>
inline const mat3<T> operator*(const mat3<T> a, const T b)
{
	return mat3<T>(
	    b * a.cmp[0],
	    b * a.cmp[1],
	    b * a.cmp[2],
	    b * a.cmp[3],
	    b * a.cmp[4],
	    b * a.cmp[5],
	    b * a.cmp[6],
	    b * a.cmp[7],
	    b * a.cmp[8]);
}
template <class T>
inline const mat3<T> operator*(const mat3<T> a, const mat3<T> b)
{
	return mat3<T>(
	    a.cmp[0] * b.cmp[0] + a.cmp[3] * b.cmp[1] + a.cmp[6] * b.cmp[2],
	    a.cmp[1] * b.cmp[0] + a.cmp[4] * b.cmp[1] + a.cmp[7] * b.cmp[2],
	    a.cmp[2] * b.cmp[0] + a.cmp[5] * b.cmp[1] + a.cmp[8] * b.cmp[2],
	    a.cmp[0] * b.cmp[3] + a.cmp[3] * b.cmp[4] + a.cmp[6] * b.cmp[5],
	    a.cmp[1] * b.cmp[3] + a.cmp[4] * b.cmp[4] + a.cmp[7] * b.cmp[5],
	    a.cmp[2] * b.cmp[3] + a.cmp[5] * b.cmp[4] + a.cmp[8] * b.cmp[5],
	    a.cmp[0] * b.cmp[6] + a.cmp[3] * b.cmp[7] + a.cmp[6] * b.cmp[8],
	    a.cmp[1] * b.cmp[6] + a.cmp[4] * b.cmp[7] + a.cmp[7] * b.cmp[8],
	    a.cmp[2] * b.cmp[6] + a.cmp[5] * b.cmp[7] + a.cmp[8] * b.cmp[8]);
}
template <class T>
inline const vec3<T> operator*(const mat3<T> a, const vec3<T> b)
{
	return vec3<T>(
	    a.cmp[0] * b.x + a.cmp[3] * b.y + a.cmp[6] * b.z,
	    a.cmp[1] * b.x + a.cmp[4] * b.y + a.cmp[7] * b.z,
	    a.cmp[2] * b.x + a.cmp[5] * b.y + a.cmp[8] * b.z);
}
template <class T>
inline const mat3<T> operator/(const mat3<T> a, const T b)
{
	return mat3<T>(
	    a.cmp[0] / b,
	    a.cmp[1] / b,
	    a.cmp[2] / b,
	    a.cmp[3] / b,
	    a.cmp[4] / b,
	    a.cmp[5] / b,
	    a.cmp[6] / b,
	    a.cmp[7] / b,
	    a.cmp[8] / b);
}

template <class T>
inline std::ostream& operator<<(std::ostream& os, const mat3<T> mat)
{
	for (int32_t i = 0; i < 3; i++) {
		for (int32_t j = 0; j < 3; j++) {
			os << mat(i, j) << " ";
		}
		os << std::endl;
	}
	return os;
}

////////////////////

template <class T>
inline mat4<T>::mat4(const mat3<T> mat, const vec3<T> vec)
    : cmp {
	    mat.cmp[0], mat.cmp[1], mat.cmp[2], 0.0,
	    mat.cmp[3], mat.cmp[4], mat.cmp[5], 0.0,
	    mat.cmp[6], mat.cmp[7], mat.cmp[8], 0.0,
	    vec.cmp[0], vec.cmp[1], vec.cmp[2], 1.0
    }
{
}

template <class T>
inline mat4<T> mat4<T>::transpose() const
{
	return mat4<T>(
	    cmp[0], cmp[4], cmp[8], cmp[12],
	    cmp[1], cmp[5], cmp[9], cmp[13],
	    cmp[2], cmp[6], cmp[10], cmp[14],
	    cmp[3], cmp[7], cmp[11], cmp[15]);
}

template <class T>
inline T& mat4<T>::operator()(const uint32_t idxr, const uint32_t idxc) &
{
	return cmp[idxr + idxc * 4];
}
template <class T>
inline const T& mat4<T>::operator()(const uint32_t idxr, const uint32_t idxc) const&
{
	return cmp[idxr + idxc * 4];
}
template <class T>
inline const T mat4<T>::operator()(const uint32_t idxr, const uint32_t idxc) &&
{
	return cmp[idxr + idxc * 4];
}

template <class T>
inline mat4<T>& mat4<T>::operator=(const mat4<T> m) &
{
	for (uint32_t i = 0; i < 16; i++) {
		this->cmp[i] = m.cmp[i];
	}
	return *this;
}
template <class T>
inline mat4<T>& mat4<T>::operator+=(const mat4<T> m) &
{
	for (uint32_t i = 0; i < 16; i++) {
		this->cmp[i] += m.cmp[i];
	}
	return *this;
}
template <class T>
inline mat4<T>& mat4<T>::operator-=(const mat4<T> m) &
{
	for (uint32_t i = 0; i < 16; i++) {
		this->cmp[i] -= m.cmp[i];
	}
	return *this;
}
template <class T>
inline mat4<T>& mat4<T>::operator*=(const T s) &
{
	for (uint32_t i = 0; i < 16; i++) {
		this->cmp[i] *= s;
	}
	return *this;
}
template <class T>
inline mat4<T>& mat4<T>::operator/=(const T s) &
{
	for (uint32_t i = 0; i < 16; i++) {
		this->cmp[i] /= s;
	}
	return *this;
}

template <class T>
template <class U>
inline mat4<T>::operator mat4<U>() const
{
	return mat4<U>(
	    static_cast<U>(cmp[0]),
	    static_cast<U>(cmp[1]),
	    static_cast<U>(cmp[2]),
	    static_cast<U>(cmp[3]),
	    static_cast<U>(cmp[4]),
	    static_cast<U>(cmp[5]),
	    static_cast<U>(cmp[6]),
	    static_cast<U>(cmp[7]),
	    static_cast<U>(cmp[8]),
	    static_cast<U>(cmp[9]),
	    static_cast<U>(cmp[10]),
	    static_cast<U>(cmp[11]),
	    static_cast<U>(cmp[12]),
	    static_cast<U>(cmp[13]),
	    static_cast<U>(cmp[14]),
	    static_cast<U>(cmp[15]));
}

template <class T>
inline const mat4<T> operator+(const mat4<T> a, const mat4<T> b)
{
	return mat4<T>(
	    a.cmp[0] + b.cmp[0],
	    a.cmp[1] + b.cmp[1],
	    a.cmp[2] + b.cmp[2],
	    a.cmp[3] + b.cmp[3],
	    a.cmp[4] + b.cmp[4],
	    a.cmp[5] + b.cmp[5],
	    a.cmp[6] + b.cmp[6],
	    a.cmp[7] + b.cmp[7],
	    a.cmp[8] + b.cmp[8],
	    a.cmp[9] + b.cmp[9],
	    a.cmp[10] + b.cmp[10],
	    a.cmp[11] + b.cmp[11],
	    a.cmp[12] + b.cmp[12],
	    a.cmp[13] + b.cmp[13],
	    a.cmp[14] + b.cmp[14],
	    a.cmp[15] + b.cmp[15]);
}
template <class T>
inline const mat4<T> operator-(const mat4<T> a, const mat4<T> b)
{
	return mat4<T>(
	    a.cmp[0] - b.cmp[0],
	    a.cmp[1] - b.cmp[1],
	    a.cmp[2] - b.cmp[2],
	    a.cmp[3] - b.cmp[3],
	    a.cmp[4] - b.cmp[4],
	    a.cmp[5] - b.cmp[5],
	    a.cmp[6] - b.cmp[6],
	    a.cmp[7] - b.cmp[7],
	    a.cmp[8] - b.cmp[8],
	    a.cmp[9] - b.cmp[9],
	    a.cmp[10] - b.cmp[10],
	    a.cmp[11] - b.cmp[11],
	    a.cmp[12] - b.cmp[12],
	    a.cmp[13] - b.cmp[13],
	    a.cmp[14] - b.cmp[14],
	    a.cmp[15] - b.cmp[15]);
}
template <class T>
inline const mat4<T> operator-(const mat4<T> a)
{
	return mat4<T>(
	    -a.cmp[0],
	    -a.cmp[1],
	    -a.cmp[2],
	    -a.cmp[3],
	    -a.cmp[4],
	    -a.cmp[5],
	    -a.cmp[6],
	    -a.cmp[7],
	    -a.cmp[8],
	    -a.cmp[9],
	    -a.cmp[10],
	    -a.cmp[11],
	    -a.cmp[12],
	    -a.cmp[13],
	    -a.cmp[14],
	    -a.cmp[15]);
}
template <class T>
inline const mat4<T> operator*(const T a, const mat4<T> b)
{
	return mat4<T>(
	    a * b.cmp[0],
	    a * b.cmp[1],
	    a * b.cmp[2],
	    a * b.cmp[3],
	    a * b.cmp[4],
	    a * b.cmp[5],
	    a * b.cmp[6],
	    a * b.cmp[7],
	    a * b.cmp[8],
	    a * b.cmp[9],
	    a * b.cmp[10],
	    a * b.cmp[11],
	    a * b.cmp[12],
	    a * b.cmp[13],
	    a * b.cmp[14],
	    a * b.cmp[15]);
}
template <class T>
inline const mat4<T> operator*(const mat4<T> a, const T b)
{
	return mat4<T>(
	    b * a.cmp[0],
	    b * a.cmp[1],
	    b * a.cmp[2],
	    b * a.cmp[3],
	    b * a.cmp[4],
	    b * a.cmp[5],
	    b * a.cmp[6],
	    b * a.cmp[7],
	    b * a.cmp[8],
	    b * a.cmp[9],
	    b * a.cmp[10],
	    b * a.cmp[11],
	    b * a.cmp[12],
	    b * a.cmp[13],
	    b * a.cmp[14],
	    b * a.cmp[15]);
}
template <class T>
inline const mat4<T> operator*(const mat4<T> a, const mat4<T> b)
{
	return mat4<T>(
	    a.cmp[0] * b.cmp[0] + a.cmp[3] * b.cmp[1] + a.cmp[6] * b.cmp[2],
	    a.cmp[1] * b.cmp[0] + a.cmp[4] * b.cmp[1] + a.cmp[7] * b.cmp[2],
	    a.cmp[2] * b.cmp[0] + a.cmp[5] * b.cmp[1] + a.cmp[8] * b.cmp[2],
	    a.cmp[0] * b.cmp[3] + a.cmp[3] * b.cmp[4] + a.cmp[6] * b.cmp[5],
	    a.cmp[1] * b.cmp[3] + a.cmp[4] * b.cmp[4] + a.cmp[7] * b.cmp[5],
	    a.cmp[2] * b.cmp[3] + a.cmp[5] * b.cmp[4] + a.cmp[8] * b.cmp[5],
	    a.cmp[0] * b.cmp[6] + a.cmp[3] * b.cmp[7] + a.cmp[6] * b.cmp[8],
	    a.cmp[1] * b.cmp[6] + a.cmp[4] * b.cmp[7] + a.cmp[7] * b.cmp[8],
	    a.cmp[2] * b.cmp[6] + a.cmp[5] * b.cmp[7] + a.cmp[8] * b.cmp[8]);
}
template <class T>
inline const vec4<T> operator*(const mat4<T> a, const vec4<T> b)
{
	return vec4<T>(
	    a.cmp[0] * b.x + a.cmp[4] * b.y + a.cmp[8] * b.z + a.cmp[12] * b.w,
	    a.cmp[1] * b.x + a.cmp[5] * b.y + a.cmp[9] * b.z + a.cmp[13] * b.w,
	    a.cmp[2] * b.x + a.cmp[6] * b.y + a.cmp[10] * b.z + a.cmp[14] * b.w,
	    a.cmp[3] * b.x + a.cmp[7] * b.y + a.cmp[11] * b.z + a.cmp[15] * b.w);
}
template <class T>
inline const mat4<T> operator/(const mat4<T> a, const T b)
{
	return mat4<T>(
	    a.cmp[0] / b,
	    a.cmp[1] / b,
	    a.cmp[2] / b,
	    a.cmp[3] / b,
	    a.cmp[4] / b,
	    a.cmp[5] / b,
	    a.cmp[6] / b,
	    a.cmp[7] / b,
	    a.cmp[8] / b,
	    a.cmp[9] / b,
	    a.cmp[10] / b,
	    a.cmp[11] / b,
	    a.cmp[12] / b,
	    a.cmp[13] / b,
	    a.cmp[14] / b,
	    a.cmp[15] / b);
}

template <class T>
inline std::ostream& operator<<(std::ostream& os, const mat4<T> mat)
{
	for (int32_t i = 0; i < 4; i++) {
		for (int32_t j = 0; j < 4; j++) {
			os << mat(i, j) << " ";
		}
		os << std::endl;
	}
	return os;
}
