#ifndef UTILSH
#define UTILSH

#include <cstdio>
#include <cstring>
#include "vec3.h"

inline double drand48() {
	return double(rand()) / double(RAND_MAX);
}

vec3 random_in_unit_sphere() {
	vec3 p;
	do {
		p = 2.0f * vec3(float(drand48()), float(drand48()), float(drand48())) - vec3(1.0f, 1.0f, 1.0f);
	} while (p.squared_length() >= 1.0f);
	return p;
}

#endif
