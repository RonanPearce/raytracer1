#ifndef CAMERAH
#define CAMERAH

#include "ray.h"
#include "utils.h"

class camera {
public:
	camera(vec3 lookfrom, vec3 lookat, vec3 vup, float vfov, float aspect, float aperture, float focus_dist, float t0, float t1) {
		setup(lookfrom, lookat, vup, vfov, aspect, aperture, focus_dist, t0, t1);
	}

	void setup(vec3 lookfrom, vec3 lookat, vec3 vup, float vfov, float aspect, float aperture, float focus_dist, float t0, float t1) {
		time0 = t0;
		time1 = t1;
		lens_radius = aperture / 2.0f;
		float theta = vfov * (float)M_PI / 180.0f;
		float half_height = tan(theta / 2.0f);
		float half_width = aspect * half_height;
		origin = lookfrom;
		w = unit_vector(lookfrom - lookat);
		u = unit_vector(cross(vup, w));
		v = cross(w, u);
		lower_left_corner = origin - half_width * focus_dist * u - half_height * focus_dist * v - focus_dist * w;
		horizontal = 2.0f * half_width * focus_dist * u;
		vertical = 2.0f * half_height * focus_dist * v;
	}

	inline ray get_ray(float s, float t) {
		vec3 rd = lens_radius * random_in_unit_disc();
		vec3 offset = u * rd.x() + v * rd.y();
		float time = time0 + float(drand48() * (time1 - time0));
		return ray(origin + offset, lower_left_corner + s * horizontal + t * vertical - origin - offset, time);
	}

	vec3 origin;
	vec3 lower_left_corner;
	vec3 horizontal;
	vec3 vertical;
	vec3 u, v, w;
	float time0, time1;
	float lens_radius;
};

#endif
