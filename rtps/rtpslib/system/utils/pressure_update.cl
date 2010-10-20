#ifndef _PRESSURE_UPDATE_CL_
#define _PRESSURE_UPDATE_CL_

	// gradient
	float dWijdr = Wspiky_dr(rlen, sphp->smoothing_distance, sphp);

	float4 di = density(index_i);  // should not repeat di=
	float4 dj = density(index_j);

	//form simple SPH in Krog's thesis

	float rest_density = 1000.f;
	float Pi = sphp->K*(di.x - rest_density);
	float Pj = sphp->K*(dj.x - rest_density);

	float kern = -dWijdr * (Pi + Pj)*0.5;
	float4 stress = kern*r; // correct version

	float4 veli = veleval(index_i); // sorted
	float4 velj = veleval(index_j);

	// Add viscous forces

	#if 0
	float vvisc = 0.001f; // SHOULD BE SET IN GE_SPH.cpp
	float dWijlapl = Wvisc_lapl(rlen, sphp->smoothing_distance, sphp);
	stress += vvisc * (velj-veli) * dWijlapl;
	#endif

	stress *=  sphp->mass/(di.x*dj.x);  // original

	#if 1
	// Add XSPH stabilization term
	float Wijpol6 = Wpoly6(rlen, sphp->smoothing_distance, sphp);
	pt->xsph +=  (2.f * sphp->mass * (velj-veli)/(di.x+dj.x) * Wijpol6);
	pt->xsph.w = 0.f;
	#endif

	pt->force += stress;

#endif
