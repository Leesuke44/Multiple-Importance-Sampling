#pragma once
#include "ray.h"
#include "algebra3.h"
#include <iostream>
using namespace std;

class Sphere
{
public:
	point3 Center;
	double Radius;
	vec3 Normal;
	point3 RGB;
	float Ambient;
	float	Diffuse;
	float	Specular;
	float	Shininess;
	float	Reflection;

	Sphere() {}

	Sphere(point3 &center, double radius, float material[] ){
		Center = center;
		Radius = radius;
		point3 rgb = point3(material[0], material[1], material[2]);
		RGB = rgb;
		Ambient = material[3];
		Diffuse = material[4];
		Specular = material[5];
		Shininess = material[6];
		Reflection = material[7];
	}

	vec3 colorinshadow() {
		return Ambient * RGB * 255;
	}

	vec3 color(vec3 pos, vec3 dir, point3 light_pos) {
		Normal = (pos - Center).normalize();
		vec3 light_dir = (light_pos - pos).normalize();
		vec3 ray_dir = -dir.normalize();
		vec3 light_color = vec3(1.0, 1.0, 1.0);

		//ambient
		vec3 ambient_color = Ambient * RGB;
		//diffuse
		float diffuse = max((light_dir * Normal), (float)0.0) * Diffuse;
		vec3 diffuse_color = diffuse * RGB;
		//specular
		vec3 helf = light_dir + ray_dir;
		helf.normalize();
		float specular = max((helf * Normal), (float)0.0);
		specular = pow(specular, Shininess) * Specular;
		vec3 specular_color = specular * light_color;

		vec3 color = (ambient_color + diffuse_color + specular_color) * 255;
		for (int i = 0; i < 3; i++) {
			if (color[i] > 255)color[i] = 255;
		}
		return  color;
	}

	float hit(const ray &r) {
		float t = -1;
		vec3 oc = r.origin() - Center;
		auto a = r.direction() * r.direction();
		auto b = 2.0 * oc * r.direction();
		auto c = oc * oc - Radius * Radius;

		auto discriminant = b * b - (4 * a * c);

		if (discriminant >= 0) {
			t = (-b - sqrt(discriminant)) / (2.0 * a);
		}

		return t;
	}

private:

};

