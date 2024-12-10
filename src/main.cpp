#include "colorImage.h"
#include "ray.h"
#include "algebra3.h"
#include "sphere.h"
#include "rectangle.h"
#include <iostream>
#include <fstream>
#include <random>
#include <string> 
#include <omp.h>
#include <math.h>
#include <vector>
using namespace std;

struct hitObject {
	point3 hit_position = point3(-1.0);
	Sphere* sphere = NULL;
	Rectangle* rectangle = NULL;
	Rectangle* light = NULL;
};

//camera variable
point3 camera_center;
int img_width, img_height;
vec3 view_direction;
vec3 view_up;
float FOV;

//sphere
Sphere sphere[1];
int sphere_count = 0;

//rectangle
vector<Rectangle> rectangle;
int rec_count = 0;

//light
vector<Rectangle> light;
int light_count = 0;

//probability
float pdf_rr = 0.5;
int N = 4096;
int max_bounces = 3;
float PI = 3.14159265358979323846;


void file_process(char* filename){
	ifstream file;
	char type;
	float x, y, z;
	float tmp_m[8];
	point3 T[4];
	point3 center;
	vec3 normal;
	file.open(filename);

	if (file.fail()) cout << "failed to load file!!" << endl;

	while (!file.eof()) {
		file >> type;
		switch (type) {
			case 'E':
				file >> x >> y >> z;
				camera_center = point3(x, y, z);
				break;

			case 'V':
				file >> x >> y >> z;
				view_direction = vec3(x, y, z);
				file >> x >> y >> z;
				view_up = vec3(x, y, z);
				break;

			case 'F':
				file >> FOV;
				break;

			case 'R':
				file >> img_width >> img_height;
				break;

			case 'S':
				float radius;
				file >> x >> y >> z;
				center = point3(x,y,z);
				file >> radius;
				sphere[sphere_count] = Sphere(center, radius, tmp_m);
				sphere_count++;
				break;

			case 'T':
				for (int i = 0; i < 4; i++) {
					file >> x >> y >> z;
					T[i] = point3(x, y, z);
					file >> type;
				}
				file >> x >> y >> z;
				normal = vec3(x, y, z);
				rectangle.push_back( Rectangle(T, normal, tmp_m));
				rec_count++;
				break;

			case 'M':
				
				for (int i = 0; i < 8; i++) file >> tmp_m[i];
				break;
			
			case 'L':
				for (int i = 0; i < 4; i++) {
					file >> x >> y >> z;
					T[i] = point3(x, y, z);
					file >> type;
				}
				file >> x >> y >> z;
				normal = vec3(x, y, z);
				light.push_back(Rectangle(T, normal, tmp_m));
				light_count++;
				break;
		};
	}
	file.close();

}

hitObject hit_object(const ray& r) {
	hitObject res;
	int sphere_index = -1;
	int rec_index = -1;
	int light_index = -1;
	float st = sphere[0].hit(r);
	float rt = rectangle[0].hit(r);
	float lt = light[0].hit(r);
	for (int i = 0; i < sphere_count; i++) {
		float t = sphere[i].hit(r);
		if (t > 0.0) {
			if (st < 0.0 || t <= st) {
				st = t;
				sphere_index = i;
			}
		}
	}
	
	for (int i = 0; i < rec_count; i++) {
		float t = rectangle[i].hit(r);
		if (t > 0.0) {
			if (rt < 0.0 || t <= rt) {
				rt = t;
				rec_index = i;
			}
		}
	}

	for (int i = 0; i < light_count; i++) {
		float t = light[i].hit(r);
		if (t > 0.0) {
			if (lt < 0.0 || t <= rt) {
				lt = t;
				light_index = i;
			}
		}
	}

	if (st > 0 && (rt == -1 || st < rt) && (lt == -1 || st < lt)) {
		res.sphere = &sphere[sphere_index];
		res.hit_position = r.at(st);
	}
	else if (rt > 0 && (st == -1 || rt < st) && (lt == -1 || rt < lt)) {
		res.rectangle = &rectangle[rec_index];
		res.hit_position = r.at(rt);
	}
	else if (lt > 0 && (st == -1 || lt < st) && (rt == -1 || lt < rt)) {
		res.light = &light[light_index];
		res.hit_position = r.at(lt);
	}

	return res;
}

vec3 random_sample(vec3 dir) {
	vec3 sample;
	int sign;
	float n;

	for (int i = 0; i < 3; i++) {
		sign = rand() % 2;
		n = rand() / (RAND_MAX + 1.0);

		if (sign == 0) sign = -1;
		else sign = 1;

		sample[i] = sign * n;
	}
	
	sample += dir;
	sample.normalize();
	return sample;
}

vec3 uniform_sample(vec3 N) {
	std::random_device rd;
	std::default_random_engine generator(rd());
	std::uniform_real_distribution<float> distribution(0.0, 1.0);


	float cosTheta = distribution(generator);
	float sinTheta = sqrtf(1 - cosTheta * cosTheta);
	float Phi = 2 * PI * distribution(generator);
	float cosPhi = cosf(Phi);
	float sinPhi = sin(Phi);

	vec3 local_sample = vec3(sinTheta * cosPhi, cosTheta,  sinTheta * sinPhi);

	vec3 up = fabs(N[1]) < 1.0 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
	vec3 T = up ^ N;
	vec3 B = N ^ T;



	mat3 TBN = mat3(T, N, B);

	vec3 sample = vec3(T[0]*local_sample[0]+N[0]*local_sample[1]+B[0]*local_sample[2],
								T[1] * local_sample[0] + N[1] * local_sample[1] + B[1] * local_sample[2],
								T[2] * local_sample[0] + N[2] * local_sample[1] + B[2] * local_sample[2]);

	sample.normalize();

	return sample;
}

float dot(vec3 normal, vec3 dir) {
	float cos;
	normal.normalize();
	dir.normalize();
	cos = max(normal * dir, (float)0.0);

	return cos;
}

vec3 trace_path(const ray& r, int bounces) {
	vec3 pixel_color = vec3(1.0,1.0,1.0);
	//russian roulette rand() / (RAND_MAX + 1.0) > pdf_rr
	if (bounces>max_bounces)  return vec3(0.0, 0.0, 0.0);

	hitObject hit_result = hit_object(r);
	
	if (hit_result.light != NULL) {
		pixel_color = hit_result.light->RGB;
	}
	else if (hit_result.rectangle != NULL) {
		bounces++;

		//diffuse pdf	
		float pdf = 1 / (2 * PI);
		//Uniform sampling
		point3 sample_center = hit_result.hit_position;
		vec3 sample_dir = uniform_sample(hit_result.rectangle->Normal);
		ray sample_ray = ray(sample_center, sample_dir);

		//intensiity
		float cosine = dot(hit_result.rectangle->Normal, sample_dir);

		vec3 diffuse = trace_path(sample_ray, bounces);
		vec3 color = hit_result.rectangle->color();

		pixel_color = prod(diffuse, color) * cosine / pdf ;
	}
	else pixel_color = vec3(0.0, 0.0, 0.0);

	return pixel_color;
}

int main(int argc, char* argv[])
{
	ColorImage image;
	Pixel p = { 0,0,0 };

	file_process((char*)"src/input.txt");

	//viewport setting
	point3 viewport_center = camera_center + vec3(0, 0, -1);//設camera和viewport的距離為1
	float theta = FOV / 2.0;
	float viewport_helflength = tan(theta); //tan(θ) = 對邊/鄰邊，又鄰邊為1 

	auto viewport_upper_left = viewport_center + vec3(-viewport_helflength, viewport_helflength, 0);
	auto viewport_upper_right = viewport_center + vec3(viewport_helflength, viewport_helflength, 0);
	auto viewport_lower_left = viewport_center + vec3(-viewport_helflength, -viewport_helflength, 0);

	vec3 viewport_u = viewport_upper_right - viewport_upper_left;
	vec3 viewport_v = viewport_lower_left - viewport_upper_left;

	auto pixel_delta_u = viewport_u / img_width;
	auto pixel_delta_v = viewport_v / img_height;


	//set first pixel
	auto pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);

	image.init(img_width, img_height);

	//Render
#pragma omp parallel for
	for (int j = 0; j < img_height; j++) {
		for (int i = 0; i < img_width; i++) {
			point3 pixelLoc = pixel00_loc + i * pixel_delta_u + j * pixel_delta_v ;

			vec3 r_Dir = pixelLoc - camera_center;
			ray r = ray(camera_center, r_Dir);

			point3 pixel_color ;
			for (int t = 0; t < N; t++) {
				pixel_color += (trace_path(r, 0) / N);
			}

			//color correction
			pixel_color = vec3(pixel_color[0] / (pixel_color[0] + 1.0), pixel_color[1] / (pixel_color[1] + 1.0), pixel_color[2] / (pixel_color[2] + 1.0));

			float gamma = 1 / 2.2;
			pixel_color = vec3(pow(pixel_color[0], gamma), pow(pixel_color[1], gamma), pow(pixel_color[2], gamma));

			pixel_color *= 255;

			p.R = int(pixel_color[0]);
			p.G = int(pixel_color[1]);
			p.B = int(pixel_color[2]);
			image.writePixel(i, j, p);
		}
	}
	
	//write to ppm file
	char filename[64] = { 0 };
	const char* N_ = "N=";
	string tmp = to_string(N);
	const char* N_tostr = tmp.c_str();

	strcat(filename, N_);
	strcat(filename, N_tostr);

	const char* MB_ = ",Mb=";
	tmp = to_string(max_bounces);
	const char* mb_tostr = tmp.c_str();

	strcat(filename, MB_);
	strcat(filename, mb_tostr);

	const char* ppm = ".ppm";
	strcat(filename, ppm);

	image.outputPPM((char*)filename);
}
