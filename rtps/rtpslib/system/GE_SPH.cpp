
#include <GL/glew.h>
#include <math.h>

#include "GE_SPH.h"
#include "../particle/UniformGrid.h"

// GE: need it have access to my datastructure (GE). Will remove 
// eventually. 
//#include "datastructures.h"

namespace rtps {


//----------------------------------------------------------------------
//void GE_SPH::setGEDataStructures(DataStructures* ds)
//{
	//this->ds = ds;
//}
//----------------------------------------------------------------------
GE_SPH::GE_SPH(RTPS *psfr, int n)
{
    //store the particle system framework
    ps = psfr;

	radixSort = 0;

    num = n;
	nb_vars = 4;  // for array structure in OpenCL
	nb_el = n;
	//printf("num_particles= %d\n", num);

    //*** Initialization, TODO: move out of here to the particle directory
    std::vector<float4> colors(num);
    positions.resize(num);
    forces.resize(num);
    velocities.resize(num);
    densities.resize(num);



	double domain_size_x = 10.; // ft
	double domain_volume = pow(domain_size_x, 1.); // ft^3

	// 1 particle per cell
	// grid resolution: res
	int nb_cells_x = 64; // number of cells along x
	// cell volume
	double cell_size = domain_size_x/nb_cells_x; // in ft
	printf("cell_size= %f\n", cell_size);

	double pi = 3.14;
	printf("*** cell_size= %f\n", cell_size);
	double cell_volume = pow(cell_size, 3.); // ft^3

	// Density (mass per unit vol)
	// mass of water: 1 kg / dm^3 (62lb/ft^3)
	// mass in single cell
	double density = 62.; // water: 62 lb/ft^3
	double mass_single_cell = density * cell_volume; // lb (force or mass?)
	//printf("*** mass_single_cell= %f\n", mass_single_cell);


    //for reading back different values from the kernel
    std::vector<float4> error_check(num);
    
    
    //init sph stuff
    sph_settings.rest_density = 1000;
    //sph_settings.simulation_scale = .001; // should not be required if other parameters set correctly
    sph_settings.simulation_scale = 1.0;

    //sph_settings.particle_mass = (128*1024.)/num * .0002;
	//printf("1 mass= %f\n", sph_settings.particle_mass);
	//printf("num= %d\n", num);

    sph_settings.particle_mass = mass_single_cell;
    //printf("*** sph_settings.particle mass: %f\n", sph_settings.particle_mass);

	// Do not know why required
    sph_settings.particle_rest_distance = .87 * pow(sph_settings.particle_mass / sph_settings.rest_density, 1./3.);
    //printf("particle rest distance: %f\n", sph_settings.particle_rest_distance);
   
    sph_settings.smoothing_distance = 2.f * sph_settings.particle_rest_distance; // *2 decreases grid resolution
    sph_settings.boundary_distance = .5f * sph_settings.particle_rest_distance;

	//printf("rest_distance= %f\n", sph_settings.particle_rest_distance);
	//printf("simulation_scale= %f\n", sph_settings.simulation_scale);

    //sph_settings.spacing = sph_settings.particle_rest_distance / sph_settings.simulation_scale;
    sph_settings.spacing = cell_size;

	// particle radius is the radius of the W function
    float particle_radius = sph_settings.spacing;  // one particle fits per cell (PERHAPS CHANGE?)
    //printf("particle radius: %f\n", particle_radius);
	double particle_volume = (4.*pi/3.) * pow(particle_radius, 3.);
	//printf("particle_volume= %f\n", particle_volume);

	//printf("cell_size= %f\n", cell_size);
    sph_settings.smoothing_distance = particle_radius*2;   // CHECK THIS. Width of W function
    sph_settings.particle_radius = particle_radius; 


	// mass of single fluid particle
	double mass_single_particle = particle_volume * density;
	//double mass_one = pow(particle_radius,3.) * density;
	//printf("*** particle_radius= %f\n", particle_radius);
	//printf("*** mass_one= %f\n", mass_single_particle);
    sph_settings.particle_mass = mass_single_particle;
    //printf("*** sph_settings.particle mass: %f\n", sph_settings.particle_mass);


    //grid = UniformGrid(float3(0,0,0), float3(1024, 1024, 1024), sph_settings.smoothing_distance / sph_settings.simulation_scale);

	float sz = domain_size_x;
	int num_old = num;
    grid = UniformGrid(float4(0.,0.,0.,1.), float4(sz, sz, sz, 1.), nb_cells_x); 

	//printf("spacing= %f\n", sph_settings.spacing);
	//printf("cell size: %f\n", cell_size);
	printf("**** particle covers four cells ****\n");

	grid.delta.print("delta");
	grid.res.print("res");
	grid.size.print("size");


    //grid.make_cube(&positions[0], sph_settings.spacing, num);

	float x1 = domain_size_x*.2;
	float x2 = domain_size_x*.7;
	float z1 = domain_size_x*.2;
	float z2 = domain_size_x*.8;
	float  y = domain_size_x*.5;
	float4 pmin(x1, y, z1, 1.);
	float4 pmax(x2, y, z2, 1.);
	grid.makeCube(&positions[0], pmin, pmax, sph_settings.spacing, num);

	if (num != num_old) {
		printf("mismatch of num. Deal with it\n");
		printf("num, num_old= %d, %d\n", num, num_old);
		exit(1);
	}

	for (int i=0; i < num; i++) {
		positions[i].print("pos");
	}
//void UniformGrid::makeCube(float4* position, float4 pmin, float4 pmax, float spacing, int num)

    cl_params = new BufferGE<GE_SPHParams>(ps->cli, 1);
	GE_SPHParams& params = *(cl_params->getHostPtr());
    params.grid_min = grid.getMin();
    params.grid_max = grid.getMax();
    params.mass = sph_settings.particle_mass;
    params.rest_distance = sph_settings.particle_rest_distance;
    params.smoothing_distance = sph_settings.smoothing_distance;
    params.particle_radius = sph_settings.particle_radius;
    params.simulation_scale = sph_settings.simulation_scale;
    params.boundary_stiffness = 10000.0f;
    params.boundary_dampening = 256.0f;
    params.boundary_distance = sph_settings.particle_rest_distance * .5f;
    params.EPSILON = .00001f;
    params.PI = 3.14159265f;
    params.K = 1.5f;
 
	cl_params->copyToDevice();
	cl_params->copyToHost();
	GE_SPHParams& pparams = *(cl_params->getHostPtr());

    std::fill(colors.begin(), colors.end(),float4(1.0f, 0.0f, 0.0f, 0.0f));
    std::fill(forces.begin(), forces.end(),float4(0.0f, 0.0f, 1.0f, 0.0f));
    std::fill(velocities.begin(), velocities.end(),float4(0.0f, 0.0f, 0.0f, 0.0f));

    std::fill(densities.begin(), densities.end(), 0.0f);
    std::fill(error_check.begin(), error_check.end(), float4(0.0f, 0.0f, 0.0f, 0.0f));

    /*
    for(int i = 0; i < 20; i++)
    {
        printf("position[%d] = %f %f %f\n", positions[i].x, positions[i].y, positions[i].z);
    }
    */

    //*** end Initialization

	// Put in setup Arrays
    // VBO creation, TODO: should be abstracted to another class
    managed = true;
    //printf("positions: %d, %d, %d\n", positions.size(), sizeof(float4), positions.size()*sizeof(float4));
    pos_vbo = createVBO(&positions[0], positions.size()*sizeof(float4), GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW);
    //printf("pos vbo: %d\n", pos_vbo);
    col_vbo = createVBO(&colors[0], colors.size()*sizeof(float4), GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW);
    //printf("col vbo: %d\n", col_vbo);
    // end VBO creation

    //vbo buffers
    cl_position = new BufferVBO<float4>(ps->cli, pos_vbo);
    cl_color = new BufferVBO<float4>(ps->cli, col_vbo);

    //pure opencl buffers
    cl_force       = new BufferGE<float4>(ps->cli, &forces[0], forces.size());
    cl_velocity    = new BufferGE<float4>(ps->cli, &velocities[0], velocities.size());
    cl_density     = new BufferGE<float>(ps->cli, &densities[0], densities.size());
    cl_error_check = new BufferGE<float4>(ps->cli, &error_check[0], error_check.size());

    //printf("create collision wall kernel\n");
    loadCollision_wall();

	setupArrays(); // From GE structures

	printf("=========================================\n");
	printf("Spacing =  particle_radius\n");
	printf("Smoothing distance = 2 * particle_radius to make sure we have particle-particle influence\n");
	printf("\n");
	printf("=========================================\n");
	params.print();
	sph_settings.print();
	cl_GridParams->getHostPtr()->print();
	cl_FluidParams->getHostPtr()->print();
	printf("=========================================\n");


	int print_freq = 20000;
	int time_offset = 5;

	ts_cl[TI_HASH]   = new GE::Time("hash",      time_offset, print_freq);
	ts_cl[TI_RADIX_SORT]   = new GE::Time("radix sort",   time_offset, print_freq);
	ts_cl[TI_BITONIC_SORT] = new GE::Time("bitonic sort", time_offset, print_freq);
	ts_cl[TI_BUILD]  = new GE::Time("build",     time_offset, print_freq);
	ts_cl[TI_NEIGH]  = new GE::Time("neigh",     time_offset, print_freq);
	ts_cl[TI_DENS]   = new GE::Time("density",   time_offset, print_freq);
	ts_cl[TI_PRES]   = new GE::Time("pressure",  time_offset, print_freq);
	ts_cl[TI_VISC]   = new GE::Time("viscosity", time_offset, print_freq);
	ts_cl[TI_EULER]  = new GE::Time("euler",     time_offset, print_freq);
	ts_cl[TI_UPDATE] = new GE::Time("update",    time_offset, print_freq);
	//ps->setTimers(ts_cl);

	// copy pos, vel, dens into vars_unsorted()
	// COULD DO THIS ON GPU
	float4* vars = cl_vars_unsorted->getHostPtr();
	BufferGE<float4>& un = *cl_vars_unsorted;
	BufferGE<float4>& so = *cl_vars_sorted;

	for (int i=0; i < nb_el; i++) {
		//vars[i+DENS*num] = densities[i];
		// PROBLEM: density is float, but vars_unsorted is float4
		// HOW TO DEAL WITH THIS WITHOUT DOUBLING MEMORY ACCESS in 
		// buildDataStructures. 

		//printf("%d, %d, %d, %d\n", DENS, POS, VEL, FOR); exit(0);

		un(i+DENS*nb_el).x = densities[i];
		un(i+DENS*nb_el).y = 0.0;
		un(i+DENS*nb_el).z = 0.0;
		un(i+DENS*nb_el).w = 0.0;
		un(i+POS*nb_el) = positions[i];
		un(i+VEL*nb_el) = velocities[i];
		un(i+FOR*nb_el) = forces[i];

		// SHOULD NOT BE REQUIRED
		so(i+DENS*nb_el).x = densities[i];
		so(i+DENS*nb_el).y = 0.0;
		so(i+DENS*nb_el).z = 0.0;
		so(i+DENS*nb_el).w = 0.0;
		so(i+POS*nb_el) = positions[i];
		so(i+VEL*nb_el) = velocities[i];
		so(i+FOR*nb_el) = forces[i];
	}

	cl_vars_unsorted->copyToDevice();
	cl_vars_sorted->copyToDevice(); // shoudl not be required
}

//----------------------------------------------------------------------
GE_SPH::~GE_SPH()
{
	printf("**** inside GE_SPH destructor ****\n");

    if(pos_vbo && managed)
    {
        glBindBuffer(1, pos_vbo);
        glDeleteBuffers(1, (GLuint*)&pos_vbo);
        pos_vbo = 0;
    }
    if(col_vbo && managed)
    {
        glBindBuffer(1, col_vbo);
        glDeleteBuffers(1, (GLuint*)&col_vbo);
        col_vbo = 0;
    }

	#if 1
	delete 	cl_vars_sorted;
	delete 	cl_vars_unsorted;
	delete 	cl_cells; // positions in Ian code
	delete 	cl_cell_indices_start;
	delete 	cl_cell_indices_end;
	delete 	cl_vars_sort_indices;
	delete 	cl_sort_hashes;
	delete 	cl_sort_indices;
	delete 	cl_unsort;
	delete 	cl_sort;
	delete  cl_GridParams;
	delete  cl_FluidParams;
	delete  cl_params;
	delete	clf_debug;  //just for debugging cl files
	delete	cli_debug;  //just for debugging cl files

    delete cl_force;
    delete cl_velocity;
    delete cl_density;
	delete cl_position;
	delete cl_color;
	#endif

	if (radixSort) delete radixSort;

	printf("***** exit GE_SPH destructor **** \n");
}

//----------------------------------------------------------------------
void GE_SPH::update()
{
	static int count=0;

	ts_cl[TI_UPDATE]->start(); // OK

    //call kernels
    //TODO: add timings
#ifdef CPU
	computeOnCPU();
#endif

#ifdef GPU
	computeOnGPU();
	//exit(0);
#endif

    /*
    std::vector<float4> ftest = cl_force->copyToHost(100);
    for(int i = 0; i < 100; i++)
    {
        if(ftest[i].z != 0.0)
            printf("force: %f %f %f  \n", ftest[i].x, ftest[i].y, ftest[i].z);
    }
    printf("execute!\n");
    */

	ts_cl[TI_UPDATE]->end(); // OK

	count++;
	//printf("count= %d\n", count);
	if (count%10 == 0) {
		count = 0;
		GE::Time::printAll();
	}
}
//----------------------------------------------------------------------
void GE_SPH::setupArrays()
{
	// only for my test routines: sort, hash, datastructures
	//printf("setupArrays, nb_el= %d\n", nb_el); exit(0);

	cl_cells = new BufferGE<float4>(ps->cli, nb_el);
	for (int i=0; i < nb_el; i++) {
		(*cl_cells)[i] = positions[i];
	}
	cl_cells->copyToDevice();

// Need an assign operator (no memory allocation)

	printf("allocate BufferGE<GridParams>\n");
	printf("sizeof(GridParams): %d\n", sizeof(GridParams));
	cl_GridParams = new BufferGE<GridParams>(ps->cli, 1); // destroys ...

	GridParams& gp = *(cl_GridParams->getHostPtr());

	gp.grid_min = grid.getMin();
	gp.grid_max = grid.getMax();
	gp.grid_res = grid.getRes();
	gp.grid_size = grid.getSize();
	gp.grid_delta = grid.getDelta();
	gp.numParticles = nb_el;

	gp.grid_inv_delta.x = 1. / gp.grid_delta.x;
	gp.grid_inv_delta.y = 1. / gp.grid_delta.y;
	gp.grid_inv_delta.z = 1. / gp.grid_delta.z;
	gp.grid_inv_delta.w = 1.;
	gp.grid_inv_delta.print("inv delta");

	cl_GridParams->copyToDevice();

	grid_size = (int) (gp.grid_res.x * gp.grid_res.y * gp.grid_res.z);
	printf("grid_size= %d\n", grid_size);
	gp.grid_size.print("grid size (domain dimensions)"); // domain dimensions
	gp.grid_delta.print("grid delta (cell size)"); // cell size
	gp.grid_min.print("grid min");
	gp.grid_max.print("grid max");
	gp.grid_res.print("grid res (nb points)"); // number of points
	gp.grid_delta.print("grid delta");
	gp.grid_inv_delta.print("grid inv delta");


	cl_vars_unsorted = new BufferGE<float4>(ps->cli, nb_el*nb_vars);
	cl_vars_sorted   = new BufferGE<float4>(ps->cli, nb_el*nb_vars);
	cl_sort_indices  = new BufferGE<int>(ps->cli, nb_el);
	cl_sort_hashes   = new BufferGE<int>(ps->cli, nb_el);
	cl_cell_indices_start = new BufferGE<int>(ps->cli, grid_size);
	cl_cell_indices_end   = new BufferGE<int>(ps->cli, grid_size);


	clf_debug = new BufferGE<float4>(ps->cli, nb_el);
	cli_debug = new BufferGE<int4>(ps->cli, nb_el);


	int nb_floats = nb_vars*nb_el;
	// WHY NOT cl_float4 (in CL/cl_platform.h)
	float4 f;
	float4 zero;

	zero.x = zero.y = zero.z = 0.0;
	zero.w = 1.;


	// SETUP FLUID PARAMETERS
	// cell width is one diameter of particle, which imlies 27 neighbor searches
	#if 1
	cl_FluidParams = new BufferGE<FluidParams>(ps->cli, 1);
	FluidParams& fp = *cl_FluidParams->getHostPtr();;
	float radius = gp.grid_delta.x; 
	fp.smoothing_length = radius; // SPH radius
	fp.scale_to_simulation = 1.0; // overall scaling factor
	//fp.mass = 1.0; // mass of single particle (MIGHT HAVE TO BE CHANGED)
	fp.friction_coef = 0.1;
	fp.restitution_coef = 0.9;
	fp.damping = 0.9;
	fp.shear = 0.9;
	fp.attraction = 0.9;
	fp.spring = 0.5;
	fp.gravity = -9.8; // -9.8 m/sec^2
	fp.choice = 0; // compute density
	cl_FluidParams->copyToDevice();
	#endif

    printf("done with setup arrays\n");
}
//----------------------------------------------------------------------
void GE_SPH::checkDensity()
{
#if 0
        //test density
		// Density checks should be in Density.cpp I believe (perhaps not)
        std::vector<float> dens = cl_density->copyToHost(num);
        float dens_sum = 0.0f;
        for(int j = 0; j < num; j++)
        {
            dens_sum += dens[j];
        }
        printf("summed density: %f\n", dens_sum);
        /*
        std::vector<float4> er = cl_error_check->copyToHost(10);
        for(int j = 0; j < 10; j++)
        {
            printf("rrrr[%d]: %f %f %f %f\n", j, er[j].x, er[j].y, er[j].z, er[j].w);
        }
        */
#endif
}
//----------------------------------------------------------------------
void GE_SPH::computeOnGPU()
{
    glFinish();
    cl_position->acquire();
    cl_color->acquire();
    
    //printf("execute!\n");
	// More steps per update for visualization purposes
    //for(int i=0; i < 10; i++)

    for(int i=0; i < 1; i++)
    {
		// ***** Create HASH ****
		hash();

		// **** Sort arrays ****
		//sort();
		bitonic_sort();

		// **** Reorder pos, vel
		buildDataStructures(); 

		// ***** DENSITY UPDATE *****
		neighbor_search(0); //density
		printf("after DENSITY CALCULATION\n");

		#if 1
		// ***** PRESSURE UPDATE *****
		neighbor_search(1); //pressure
		printf("after PRESSURE CALCULATION\n");
		//exit(1);
		#endif

		#if 0
		// ***** VISCOSITY UPDATE *****
        //computeViscosity();

		// ***** WALL COLLISIONS *****
        //k_collision_wall.execute(num);
		#endif

        // ***** EULER UPDATE *****
		printf("before computeEuler\n");
		computeEuler();
		printf("after computeEuler\n");

		#if 1
		//  print out density
		cl_vars_unsorted->copyToHost();
		cl_vars_sorted->copyToHost();

		float4* density = cl_vars_unsorted->getHostPtr() + 0*nb_el;
		float4* pos     = cl_vars_unsorted->getHostPtr() + 1*nb_el;
		float4* vel     = cl_vars_unsorted->getHostPtr() + 2*nb_el;
		float4* force   = cl_vars_unsorted->getHostPtr() + 3*nb_el;

		float4* density1 = cl_vars_sorted->getHostPtr() + 0*nb_el;
		float4* pos1     = cl_vars_sorted->getHostPtr() + 1*nb_el;
		float4* vel1     = cl_vars_sorted->getHostPtr() + 2*nb_el;
		float4* force1   = cl_vars_sorted->getHostPtr() + 3*nb_el;

		for (int i=30; i < 31; i++) {
			printf("==================\n");
			//printf("dens[%d]= %f, sorted den: %f\n", i, density[i].x, density1[i].x);
			pos[i].print("un pos");
			vel[i].print("un vel");
			force[i].print("un force");
			//pos1[i].print("so pos1");
			//vel1[i].print("so vel1");
			//force1[i].print("so force1");
		} 
		//exit(0);
		#endif
	}

	//printf("before release\n");
    cl_position->release();
    cl_color->release();
}
//----------------------------------------------------------------------
void GE_SPH::computeOnCPU()
{
    cpuDensity();
    //cpuPressure();
    //cpuEuler();
}
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------

}
