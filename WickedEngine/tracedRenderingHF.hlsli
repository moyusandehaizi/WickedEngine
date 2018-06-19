#ifndef _TRACEDRENDERING_HF_
#define _TRACEDRENDERING_HF_
#include "lightingHF.hlsli"

#define NOSUN
#include "skyHF.hlsli"

static const float INFINITE_RAYHIT = 1000000;
static const float EPSILON = 0.0001f;


struct Material
{
	float4		baseColor;
	float4		texMulAdd;
	float		roughness;
	float		reflectance;
	float		metalness;
	float		emissive;
	float		refractionIndex;
	float		subsurfaceScattering;
	float		normalMapStrength;
	float		parallaxOcclusionMapping;
};

//struct Sphere
//{
//	float3 position;
//	float radius;
//	float3 albedo;
//	float3 specular;
//	float emission;
//};

struct Ray
{
	float3 origin;
	float3 direction;
	float3 direction_inverse;
	float3 energy;
	uint primitiveID;
	float2 bary;
};

inline xTracedRenderingStoredRay CreateStoredRay(in Ray ray, in uint pixelID)
{
	xTracedRenderingStoredRay storedray;

	storedray.pixelID = pixelID;
	storedray.origin = ray.origin;
	storedray.direction_energy = f32tof16(ray.direction) | (f32tof16(ray.energy) << 16);
	storedray.primitiveID = ray.primitiveID;
	storedray.bary = ray.bary;

	return storedray;
}
inline void LoadRay(in xTracedRenderingStoredRay storedray, out Ray ray, out uint pixelID)
{
	pixelID = storedray.pixelID;

	ray.origin = storedray.origin;
	ray.direction = asfloat(f16tof32(storedray.direction_energy));
	ray.direction_inverse = rcp(ray.direction);
	ray.energy = asfloat(f16tof32(storedray.direction_energy >> 16));
	ray.primitiveID = storedray.primitiveID;
	ray.bary = storedray.bary;
}

inline Ray CreateRay(float3 origin, float3 direction)
{
	Ray ray;
	ray.origin = origin;
	ray.direction = direction;
	ray.direction_inverse = rcp(ray.direction);
	ray.energy = float3(1, 1, 1);
	ray.primitiveID = 0xFFFFFFFF;
	ray.bary = 0;
	return ray;
}

inline Ray CreateCameraRay(float2 uv)
{
	// Invert the perspective projection of the view-space position
	float3 direction = mul(float4(uv, 0.0f, 1.0f), g_xFrame_MainCamera_InvP).xyz;
	// Transform the direction from camera to world space and normalize
	direction = mul(float4(direction, 0.0f), g_xFrame_MainCamera_InvV).xyz;
	direction = normalize(direction);

	return CreateRay(g_xFrame_MainCamera_CamPos, direction);
}

struct RayHit
{
	float distance;
	float3 position;
	uint primitiveID;
	float2 bary;
};

inline RayHit CreateRayHit()
{
	RayHit hit;
	hit.distance = INFINITE_RAYHIT;
	hit.position = float3(0.0f, 0.0f, 0.0f);
	hit.primitiveID = 0xFFFFFFFF;
	hit.bary = 0;
	//hit.normal = float3(0.0f, 0.0f, 0.0f);
	//hit.materialIndex = 0;
	//hit.texCoords = 0;

	return hit;
}

//inline void IntersectGroundPlane(Ray ray, inout RayHit bestHit)
//{
//	// Calculate distance along the ray where the ground plane is intersected
//	float t = -ray.origin.y / ray.direction.y;
//	if (t > 0 && t < bestHit.distance)
//	{
//		bestHit.distance = t;
//		bestHit.position = ray.origin + t * ray.direction;
//		bestHit.normal = float3(0.0f, 1.0f, 0.0f);
//	}
//}
//
//inline void IntersectSphere(Ray ray, inout RayHit bestHit, Sphere sphere)
//{
//	// Calculate distance along the ray where the sphere is intersected
//	float3 d = ray.origin - sphere.position;
//	float p1 = -dot(ray.direction, d);
//	float p2sqr = p1 * p1 - dot(d, d) + sphere.radius * sphere.radius;
//	if (p2sqr < 0)
//		return;
//	float p2 = sqrt(p2sqr);
//	float t = p1 - p2 > 0 ? p1 - p2 : p1 + p2;
//	if (t > 0 && t < bestHit.distance)
//	{
//		bestHit.distance = t;
//		bestHit.position = ray.origin + t * ray.direction;
//		bestHit.normal = normalize(bestHit.position - sphere.position);
//	}
//}



#define BACKFACE_CULLING
inline void IntersectTriangle(in Ray ray, inout RayHit bestHit, in TracedRenderingMeshTriangle tri, uint primitiveID)
{
	float3 v0v1 = tri.v1 - tri.v0;
	float3 v0v2 = tri.v2 - tri.v0;
	float3 pvec = cross(ray.direction, v0v2);
	float det = dot(v0v1, pvec);
#ifdef BACKFACE_CULLING 
	// if the determinant is negative the triangle is backfacing
	// if the determinant is close to 0, the ray misses the triangle
	if (det < EPSILON)
		return;
#else 
	// ray and triangle are parallel if det is close to 0
	if (abs(det) < EPSILON)
		return;
#endif 
	float invDet = 1 / det;

	float3 tvec = ray.origin - tri.v0;
	float u = dot(tvec, pvec) * invDet;
	if (u < 0 || u > 1)
		return;

	float3 qvec = cross(tvec, v0v1);
	float v = dot(ray.direction, qvec) * invDet;
	if (v < 0 || u + v > 1)
		return;

	float t = dot(v0v2, qvec) * invDet;

	if (t > 0 && t < bestHit.distance)
	{
		bestHit.distance = t;
		bestHit.position = ray.origin + t * ray.direction;
		bestHit.primitiveID = primitiveID;
		bestHit.bary = float2(u, v);

		//float w = 1 - u - v;
		//bestHit.normal = normalize(tri.n0 * w + tri.n1 * u + tri.n2 * v);
		//bestHit.texCoords = tri.t0 * w + tri.t1 * u + tri.t2 * v;
		//
		//bestHit.materialIndex = tri.materialIndex;
	}
}

inline bool IntersectTriangleANY(in Ray ray, in float maxDistance, in TracedRenderingMeshTriangle tri)
{
	float3 v0v1 = tri.v1 - tri.v0;
	float3 v0v2 = tri.v2 - tri.v0;
	float3 pvec = cross(ray.direction, v0v2);
	float det = dot(v0v1, pvec);
#ifdef BACKFACE_CULLING 
	// if the determinant is negative the triangle is backfacing
	// if the determinant is close to 0, the ray misses the triangle
	if (det < EPSILON)
		return false;
#else 
	// ray and triangle are parallel if det is close to 0
	if (abs(det) < EPSILON)
		return false;
#endif 
	float invDet = 1 / det;

	float3 tvec = ray.origin - tri.v0;
	float u = dot(tvec, pvec) * invDet;
	if (u < 0 || u > 1)
		return false;

	float3 qvec = cross(tvec, v0v1);
	float v = dot(ray.direction, qvec) * invDet;
	if (v < 0 || u + v > 1)
		return false;

	float t = dot(v0v2, qvec) * invDet;

	if (t > 0 && t < maxDistance)
	{
		return true;
	}

	return false;
}



//inline float rayBoxIntersect(float3 rpos, float3 rdir, float3 vmin, float3 vmax)
//{
//	float t[10];
//	t[1] = (vmin.x - rpos.x) / rdir.x;
//	t[2] = (vmax.x - rpos.x) / rdir.x;
//	t[3] = (vmin.y - rpos.y) / rdir.y;
//	t[4] = (vmax.y - rpos.y) / rdir.y;
//	t[5] = (vmin.z - rpos.z) / rdir.z;
//	t[6] = (vmax.z - rpos.z) / rdir.z;
//	t[7] = fmax(fmax(fmin(t[1], t[2]), fmin(t[3], t[4])), fmin(t[5], t[6]));
//	t[8] = fmin(fmin(fmax(t[1], t[2]), fmax(t[3], t[4])), fmax(t[5], t[6]));
//	t[9] = (t[8] < 0 || t[7] > t[8]) ? NOHIT : t[7];
//	return t[9];
//}


inline bool IntersectBox(in Ray ray, in TracedRenderingAABB box)
{
	if (ray.origin.x >= box.min.x && ray.origin.x <= box.max.x &&
		ray.origin.y >= box.min.y && ray.origin.y <= box.max.y &&
		ray.origin.z >= box.min.z && ray.origin.z <= box.max.z)
		return true;

	float t[9];
	t[1] = (box.min.x - ray.origin.x) * ray.direction_inverse.x;
	t[2] = (box.max.x - ray.origin.x) * ray.direction_inverse.x;
	t[3] = (box.min.y - ray.origin.y) * ray.direction_inverse.y;
	t[4] = (box.max.y - ray.origin.y) * ray.direction_inverse.y;
	t[5] = (box.min.z - ray.origin.z) * ray.direction_inverse.z;
	t[6] = (box.max.z - ray.origin.z) * ray.direction_inverse.z;
	t[7] = max(max(min(t[1], t[2]), min(t[3], t[4])), min(t[5], t[6]));
	t[8] = min(min(max(t[1], t[2]), max(t[3], t[4])), max(t[5], t[6]));

	return (t[8] < 0 || t[7] > t[8]) ? false : true;
}



inline float rand(inout float seed, in float2 pixel)
{
	float result = frac(sin(seed * dot(pixel, float2(12.9898f, 78.233f))) * 43758.5453f);
	seed += 1.0f;
	return result;
}

inline float3x3 GetTangentSpace(float3 normal)
{
	// Choose a helper vector for the cross product
	float3 helper = abs(normal.x) > 0.99f ? float3(0, 0, 1) : float3(1, 0, 0);

	// Generate vectors
	float3 tangent = normalize(cross(normal, helper));
	float3 binormal = normalize(cross(normal, tangent));
	return float3x3(tangent, binormal, normal);
}

inline float3 SampleHemisphere(float3 normal, float alpha, inout float seed, in float2 pixel)
{
	// Sample the hemisphere, where alpha determines the kind of the sampling
	float cosTheta = pow(rand(seed, pixel), 1.0f / (alpha + 1.0f));
	float sinTheta = sqrt(1.0f - cosTheta * cosTheta);
	float phi = 2 * PI * rand(seed, pixel);
	float3 tangentSpaceDir = float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

	// Transform direction to world space
	return mul(tangentSpaceDir, GetTangentSpace(normal));
}

#endif // _TRACEDRENDERING_HF_