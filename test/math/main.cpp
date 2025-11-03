#include <cstdint>
#include <iostream>

#include "src/utils/mathfunc/mathfunc.hpp"

template class vec3<float>;
template class vec3<double>;

int main(int argc, char const* argv[])
{

	vec3<float> v0(2.0, 0.5, 2.0);
	vec3<float> v1(1.0, 0.0, 2.0);

	std::cout << v0 + v1 << std::endl;

	std::cout << v0 - v1 << std::endl;

	vec3<float> v3;

	v3 = v0;
	std::cout << v3 << std::endl;
	v3 += v0;
	std::cout << v3 << std::endl;

	std::cout << std::endl;

	fmat3 m(0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);

	std::cout << m << std::endl;

	std::cout << m.transpose() << std::endl;

	fmat3 m2(0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0);

	std::cout << m2 << std::endl;

	fmat3 m3(0.0, 1.0, 2.0, 1.0, 0.0, 0.0, 6.0, 7.0, 8.0);

	std::cout << m3 << std::endl;

	std::cout << m3.adjugate() * m3 << std::endl;
	std::cout << m3.determinant() << std::endl;

	fquaternion q0(0.0, 0.0, 0.0, 1.0);
	fquaternion q1(vec3(0.0, 1.0, 0.0));

	auto q2 = fquaternion::slerp(q0, q1, 0.5);

	return 0;
}
