#ifndef LAMBERTIANH
#define LAMBERTIANH

#include "utils.h"
#include "material.h"
#include "ray.h"
#include "hitable.h"

class lambertian : public material {
public:

	lambertian(const vec3& a) :albedo(a) {}

	virtual bool scatter(const ray& r_in, const hit_record& rec, vec3& attenuation, ray& scattered) const {
		vec3 target = rec.p + rec.normal + random_in_unit_sphere();
	}

	vec3 albedo;
};

#endif
