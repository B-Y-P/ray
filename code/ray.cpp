
constexpr f32 epsilon = 0.00001f;

function bool RayIntersectSphere(Ray ray, Sphere sphere, f32 *restrict t){
	v3 v = ray.pos - sphere.center;
	f32 two_a = 2.f*Dot(ray.dir, ray.dir);
	f32 b = 2.f*Dot(ray.dir, v);
	f32 c = Dot(v,v) - sphere.radius*sphere.radius;
	f32 discr = b*b - 2.f*two_a*c;

	f32 t_ = -1.f;
	if(discr <= 0.f){ return false; }
	else{
		f32 sqrt_discr = SqRt(discr);
		f32 pt = (-b + sqrt_discr) / (two_a);
		f32 mt = (-b - sqrt_discr) / (two_a);

		if(pt > 0.f && mt > 0.f){ t_ = Min(pt, mt); }
		else if(pt < 0.f && mt < 0.f){ return false; }
		else{ t_ = pt; }
	}

	*t = t_;
	return (t_ > 0.f);
}

function bool RayIntersectSphere(Ray ray, Sphere sphere, f32 *restrict t, v3 *restrict surface_norm){
	bool result = RayIntersectSphere(ray, sphere, t);
	v3 norm = (ray.pos + ray.dir*(*t)) - sphere.center;
	*surface_norm = Normalize(norm);
	return result;
}

function bool RayIntersectPlane(Ray ray, Plane plane, f32 *restrict t){
	f32 den = Dot(plane.n, ray.dir);
	if(Abs(den) <= epsilon){ return false; }
	f32 num = Dot(plane.n, (plane.pos - ray.pos));

	f32 t_ = num / den;
	*t = t_;
	return (t_ > 0.f);
}

function bool RayIntersectPlane(Ray ray, Plane plane, f32 *restrict t, v3 *restrict surface_norm){
	*surface_norm = plane.n;
	return RayIntersectPlane(ray, plane, t);
}


function v3 RayCast(Ray ray,
					f32       *restrict t,
					Scene     *restrict scene,
					Rng_State *restrict rng,
					i32 depth=5,
					f32 refract=1.f)
{
	u32 mat_slot = 0;
	f32 min_len;
	v3 hit_norm;

	if(RayIntersectPlane(ray, scene->ground, &min_len, &hit_norm)){
		mat_slot = 1;
	}else{
		min_len = F32_MAX;
	}

	b32 hit_sphere = 0;
	v3 sphere_center;
	foreach(i, scene->sphere_count){
		Sphere *sphere = scene->spheres + i;
		v3 sphere_norm;
		f32 sphere_t = -1.f;
		if(RayIntersectSphere(ray, *sphere, &sphere_t, &sphere_norm) && sphere_t < min_len){
			mat_slot = sphere->mat_slot;
			hit_norm = sphere_norm;
			min_len = sphere_t;
			hit_sphere = 1;
			sphere_center = sphere->center;
		}
	}

	Material mat = scene->mats[mat_slot];

	if(mat_slot == 0){ return mat.color; }

	*t = min_len;
	v3 color = {};

	v3 hit_pos = ray.pos + min_len*ray.dir;

	f32 bounce_contrib = (1.f / RaysPerBounce);

	foreach(i, scene->light_count){
		Light *light = scene->lights + i;
		f32 light_rad = light->radius;

		v3 light_dir = light->pos - hit_pos;
		f32 light_dist_inv_sq = 1.f / LenSq(light_dir);
		light_dir = Normalize(light_dir);

		f32 diffuse_intensity = Dot(hit_norm, light_dir);
		diffuse_intensity = ClampN(diffuse_intensity);
		diffuse_intensity *= light_dist_inv_sq * light->diffuse_power;
		v3 diffuse = diffuse_intensity*light->diffuse_color;

#if 1
		// Phong
		v3 reflect_vec = 2.f*Dot(light_dir, hit_norm)*(hit_norm - light_dir);
		f32 spec_intensity = Pow(Dot(reflect_vec, ray.dir), 22.f);
#else
		// Blinn-Phong
		v3 half_vec = light_dir + ray.dir;
		f32 spec_intensity = Dot(hit_norm, half_vec);
		spec_intensity = Pow(ClampN(spec_intensity), 20.f);
#endif
		spec_intensity *= light_dist_inv_sq*light->specular_power;
		v3 specular = spec_intensity*light->specular_color;

		v3 inc_color = Hadamard(mat.color, diffuse) + specular;
		v3 color_add = {};

		foreach(j, RaysPerBounce){

#if RaysPerBounce == 1
			v3 shadow_offset = {};
#else
			v3 shadow_offset = {RandBiNorm(rng), RandBiNorm(rng), RandBiNorm(rng)};
			shadow_offset = Normalize(shadow_offset)*light_rad*SqRt(RandNorm(rng));
#endif

			v3 shadow_dir = light->pos + shadow_offset - hit_pos;
			shadow_dir = Normalize(shadow_dir);
			Ray shadow_ray = {hit_pos + epsilon*hit_norm, shadow_dir};

			f32 t_;
			if(RayIntersectPlane(shadow_ray, scene->ground, &t_)){
				goto shadows;
			}

			foreach(k, scene->sphere_count){
				Sphere *sphere = scene->spheres + k;
				if(RayIntersectSphere(shadow_ray, *sphere, &t_)){
					goto shadows;
				}
			}

			color_add += inc_color;

			shadows:;
		}

		v3 l_color = bounce_contrib*color_add;

		if(mat.texture && hit_sphere){
			v3 d = sphere_center - hit_pos;
			d = Normalize(d);
			f32 u = 0.5f + (ATan2(d.x, d.z)) / TAU32;
			f32 v = 0.5f - (ASin(d.y)) / PI32;
			color += Hadamard(l_color, BiSample(mat.texture, u, v));
		}else{
			color += l_color;
		}

		lights:;
	}

	if(depth > 0){
		f32 cos_ray_sq = -Dot(ray.dir, hit_norm);

		if(mat.refract > 1.f){
			f32 refr_ratio = (refract / mat.refract);
			f32 sin_ray_sq = refr_ratio*refr_ratio*(1.f - cos_ray_sq*cos_ray_sq);
			f32 cos_ray = SqRt(1.f - sin_ray_sq);

			v3 refract_dir = refr_ratio*ray.dir + (refr_ratio*cos_ray - cos_ray_sq)*hit_norm;
			refract_dir = Normalize(refract_dir);

			Ray refract_ray = {hit_pos + epsilon*refract_dir, refract_dir};
			f32 t_ = -1.f;
			v3 refr_color = RayCast(refract_ray, &t_, scene, rng, depth-1, mat.refract);
			color += refr_color;
		}

		if(mat.reflect > 0.f){
			v3 reflect_vec = 2.f*cos_ray_sq*hit_norm + ray.dir;
			reflect_vec = Normalize(reflect_vec);

			Ray reflect_ray = {hit_pos + epsilon*reflect_vec, reflect_vec};
			f32 t_ = -1.f;
			v3 refl_color = RayCast(reflect_ray, &t_, scene, rng, depth-1, refract);
			color += refl_color;
		}
	}

	return color;
}
