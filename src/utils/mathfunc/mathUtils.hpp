#pragma once

#include "src/utils/mathfunc/mathfunc.hpp"

inline fmat4 makeProjectionMatrix(const float& near, const float& far, const float& right, const float& left, const float& top,
    const float& bottom)
{
	fmat4 ret = fmat4::zero();
	ret.cmp[0]  = (2 * near) / (right - left);
	ret.cmp[2]  = (right + left) / (right - left);
	ret.cmp[5]  = (2 * near) / (top - bottom);
	ret.cmp[6]  = (top + bottom) / (top - bottom);
	ret.cmp[10] = -(far + near) / (far - near);
	ret.cmp[11] = -(2 * near * far) / (far - near);
	ret.cmp[14] = -1.0f;
	return ret;
}