#pragma once

#include "src/utils/mathfunc/mathfunc.hpp"

inline fmat4 makeProjectionMatrix(const float& near, const float& far, const float& right, const float& left, const float& top,
	const float& bottom)
{
	fmat4 ret = fmat4::zero();
	ret.cmp[0] = (2 * near) / (right - left);
	ret.cmp[2] = (right + left) / (right - left);
	ret.cmp[5] = (2 * near) / (top - bottom);
	ret.cmp[6] = (top + bottom) / (top - bottom);
	ret.cmp[10] = -(far + near) / (far - near);
	ret.cmp[11] = -(2 * near * far) / (far - near);
	ret.cmp[14] = -1.0f;
	return ret;
}

// パラメータはすべて正
inline fmat4 makeProjectionMatrixVk(const float& near, const float& far, const float& right, const float& left, const float& top,
	const float& bottom)
{
	fmat4 ret = fmat4::zero();
	ret.cmp[0] = (2 * near) / (right - left);
	ret.cmp[2] = (right + left) / (right - left);
	ret.cmp[5] = -(2 * near) / (top - bottom);
	ret.cmp[6] = -(top + bottom) / (top - bottom);
	ret.cmp[10] = (near) / (far - near);
	ret.cmp[11] = (near * far) / (far - near);
	ret.cmp[14] = -1.0f;
	return ret;
}

// パラメータはすべて正
inline fmat4 makeProjectionMatrixVk(const float& near, const float& far, const float& fovY, const float& aspect)
{
	const float top = near * tanf(fovY * 0.5f);
	const float bottom = -top;
	const float right = top * aspect;
	const float left = -right;
	return makeProjectionMatrixVk(near, far, right, left, top, bottom);
}

inline fmat4 makeCameraMatrix(const fvec3& eye, const fvec3& center, const fvec3& up)
{
	fvec3 z = -(eye - center).normalized();
	fvec3 x = up.cross(z).normalized();
	fvec3 y = z.cross(x);
	return fmat4(
		x.x, x.y, x.z, -center.dot(x),
		y.x, y.y, y.z, -center.dot(y),
		z.x, z.y, z.z, -center.dot(z),
		0.0f, 0.0f, 0.0f, 1.0f);
}