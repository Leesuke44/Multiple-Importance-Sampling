#pragma once
#include "ray.h"
#include "algebra3.h"
#include <iostream>
using namespace std;

class Rectangle
{
public:
	point3 V[4];
	vec3 Normal;
	point3 RGB;
	float Ambient;
	float	Diffuse;
	float	Specular;
	float	Shininess;
	float	Reflection;

	Rectangle() {}

	Rectangle(point3 v[4], vec3 normal, float material[]) {
		for (int i = 0; i < 4; i++) {
			V[i] = v[i];
		}

		Normal = normal;

		point3 rgb = point3(material[0], material[1], material[2]);
		RGB = rgb;
		Ambient = material[3];
		Diffuse = material[4];
		Specular = material[5];
		Shininess = material[6];
		Reflection = material[7];
	}

	vec3 colorinshadow() {
		return(Ambient * RGB * 255);
	}

	vec3 color() {
		vec3 color = Diffuse * RGB;

		return  color;
	}

	float hit(const ray& r) {

		vec3 v0 = V[0];
		vec3 v1 = V[1];
		vec3 v2 = V[3];

		vec3 ov0 = r.origin() -v0;

		mat3 coe;
		coe.set(v1-v0, v2-v0, -r.direction());

		mat3 inv_coe = coe.inverse();

		vec3 res = inv_coe.transpose() * ov0;

		double s1 = res[0];
		double s2 = res[1];
		double t = res[2];

		if (s1 >= 0.0 && s1 <= 1.0 && s2 >= 0.0 && s2 <= 1.0 && t > 0.0001)  return t;

		return -1;
	}

private:
	
};


