#include "structs.h"
#include <math.h>

namespace rtps
{

float magnitude(float4 vec)
{
    return sqrt(vec.x*vec.x + vec.y*vec.y + vec.z*vec.z);
}
float dist_squared(float4 vec)
{
    return vec.x*vec.x + vec.y*vec.y + vec.z*vec.z;
}
float distance(float4 p1, float4 p2)
{
    return sqrt((p1.x - p2.x)*(p1.x - p2.x) + (p1.y-p2.y)*(p1.y-p2.y) + (p1.z-p2.z)*(p1.z-p2.z));
}

float length(float4 v)
{
    return sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
}

//normalized vector pointing from p1 to p2
float4 norm_dir(float4 p1, float4 p2)
{
    float4 dir = float4(p2.x - p1.x, p2.y - p1.y, p2.z - p1.z, 0.0f);
    float norm = length(dir);
    if(norm > 0)
    {
        dir.x /= norm;
        dir.y /= norm;
        dir.z /= norm;
    }
    return dir;
}

float4 predator_prey(float4 p)
{
    float4 v = float4(0,0,0,0);
    int a1 = 2;
    int a2 = 2;
    int b1 = 1;
    int b2 = 1;
    v.x = a1*p.x - b1*p.x*p.y;
    v.y = -a2*p.y + b2*p.y*p.x;
    //v.x = a1 - b1*p.y;
    //v.y = -a2 + b2*p.x;
    return v;
}

float4 runge_kutta(float4 yn, float h)
{
    float4 k1 = predator_prey(yn); 
    float4 k2 = predator_prey(yn + .5f*h*k1);
    float4 k3 = predator_prey(yn + .5f*h*k2);
    float4 k4 = predator_prey(yn + h*k3);

    float4 vn = (k1 + 2.f*k2 + 2.f*k3 + k4);
    return (1./6.)*vn;
    
    /*
    yn[i].x += h*(vn[i].x);
    yn[i].y += h*(vn[i].y);
    yn[i].z += h*(vn[i].z);
    //yn[i] += h*vn[i]; //this would work with float3
    */
}

float4 force_field(float4 p, float4 ff, float dist, float max_force)
{
    float d = distance(p, ff);
    //printf("distance: %f, dist %f\n", d, dist);
    if(d < dist)
    {
        float4 dir = norm_dir(p, ff);
        float mag = max_force * (dist - d)/dist;
        dir.x *= mag;
        dir.y *= mag;
        dir.z *= mag;
        printf("forcefield: %f, %f, %f\n", dir.x, dir.y, dir.z);
        return dir;
    }
    return float4(0, 0, 0, 0);
}


}