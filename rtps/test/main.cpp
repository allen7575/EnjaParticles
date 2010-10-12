#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#include "timers.h"


#include <timege.h>

//#include <utils.h>
//#include <string.h>
//#include <string>
#include <sstream>
#include <iomanip>

#include <GL/glew.h>
#if defined __APPLE__ || defined(MACOSX)
    #include <GLUT/glut.h>
#else
    #include <GL/glut.h>
    //OpenCL stuff
#endif

#include "RTPS.h"
//#include "timege.h"

// GORDON APP TEST
#include "datastructures.h"

int window_width = 800;
int window_height = 600;
int glutWindowHandle = 0;
float translate_x = -400.f;
float translate_y = -300.f;
float translate_z = 350.f;

// mouse controls
int mouse_old_x, mouse_old_y;
int mouse_buttons = 0;
float rotate_x = 0.0, rotate_y = 0.0;
//std::vector<Triangle> triangles;
//std::vector<Box> boxes;

// offsets into the triangle list. tri_offsets[i] corresponds to the 
// triangle list for box[i]. Number of triangles for triangles[i] is
//    tri_offsets[i+1]-tri_offsets[i]
// Add one more offset so that the number of triangles in 
//   boxes[boxes.size()-1] is tri_offsets[boxes.size()]-tri_offsets[boxes.size()-1]
std::vector<int> tri_offsets;


void init_gl();

void appKeyboard(unsigned char key, int x, int y);
void appRender();
void appDestroy();

void appMouse(int button, int state, int x, int y);
void appMotion(int x, int y);

void timerCB(int ms);

void drawString(const char *str, int x, int y, float color[4], void *font);
void showFPS(float fps, std::string *report);
void *font = GLUT_BITMAP_8_BY_13;

rtps::RTPS* ps;

//timers
//GE::Time *ts[3];

//================
#include "materials_lights.h"

//----------------------------------------------------------------------
#if 0
float rand_float(float mn, float mx)
{
	float r = random() / (float) RAND_MAX;
	return mn + (mx-mn)*r;
}
#endif
//----------------------------------------------------------------------
void testGE_SPH()
{
	#if 0
	rtps::DataStructures ds(ps);

	ds.setupArrays();
	ds.ts_cl[rtps::DataStructures::TI_HASH] = new GE::Time("hash", 5);
	ds.ts_cl[rtps::DataStructures::TI_SORT] = new GE::Time("sort", 5);
	ds.ts_cl[rtps::DataStructures::TI_BUILD] = new GE::Time("build", 5);
	ds.ts_cl[rtps::DataStructures::TI_NEIGH] = new GE::Time("neigh", 5);
	ds.ts_cl[rtps::DataStructures::TI_DENS] = new GE::Time("density", 5);
	ds.ts_cl[rtps::DataStructures::TI_PRES] = new GE::Time("pressure", 5);
	ds.ts_cl[rtps::DataStructures::TI_EULER] = new GE::Time("euler", 5);
	ps->setTimers(ds.ts_cl);
	#endif


	//GE::Time::printAll();
}
//----------------------------------------------------------------------
void testGordonApp()
{
#if 0
// I removed ge_datastructures from Cmakefile

	rtps::DataStructures ds(ps);

	ds.setupArrays();
	ds.ts_cl[rtps::DataStructures::TI_HASH] = new GE::Time("hash", 5);
	ds.ts_cl[rtps::DataStructures::TI_SORT] = new GE::Time("sort", 5);
	ds.ts_cl[rtps::DataStructures::TI_BUILD] = new GE::Time("build", 5);
	ds.ts_cl[rtps::DataStructures::TI_NEIGH] = new GE::Time("neigh", 5);
	ds.ts_cl[rtps::DataStructures::TI_DENS] = new GE::Time("density", 5);
	ds.ts_cl[rtps::DataStructures::TI_PRES] = new GE::Time("pressure", 5);
	ds.ts_cl[rtps::DataStructures::TI_EULER] = new GE::Time("euler", 5);

	for (int i=0; i < 30; i++) {
	printf("==========================================\n");
	printf("ITERATION: %d\n", i);
	ds.hash();
	ds.sort();
	ds.buildDataStructures();
	ds.neighbor_search();
	}

	GE::Time::printAll();
#endif
}
//----------------------------------------------------------------------
int main(int argc, char** argv)
{
    //initialize glut
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(window_width, window_height);
    glutInitWindowPosition (glutGet(GLUT_SCREEN_WIDTH)/2 - window_width/2, 
                            glutGet(GLUT_SCREEN_HEIGHT)/2 - window_height/2);

    
    std::stringstream ss;
    ss << "Real-Time Particle System (get nb particles from elsewhere)" << std::endl;
    glutWindowHandle = glutCreateWindow(ss.str().c_str());

    glutDisplayFunc(appRender); //main rendering function
    glutTimerFunc(30, timerCB, 30); //determin a minimum time between frames
    glutKeyboardFunc(appKeyboard);
    glutMouseFunc(appMouse);
    glutMotionFunc(appMotion);

	define_lights_and_materials();

    // initialize necessary OpenGL extensions
    glewInit();
    GLboolean bGLEW = glewIsSupported("GL_VERSION_2_0 GL_ARB_pixel_buffer_object"); 
    printf("GLEW supported?: %d\n", bGLEW);

    //initialize the OpenGL scene for rendering
    init_gl();

    printf("before we call enjas functions\n");
        
    //default constructor
    ps = new rtps::RTPS();

	testGE_SPH();

	// TEST of my datastructures, sort, hash, build, step1
	//testGordonApp();
	//exit(0);

    glutMainLoop();
    return 0;
}

//----------------------------------------------------------------------
void init_gl()
{
    // default initialization
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glDisable(GL_DEPTH_TEST);

    // viewport
    glViewport(0, 0, window_width, window_height);

    // projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //gluPerspective(60.0, (GLfloat)window_width / (GLfloat) window_height, 0.1, 100.0);
    //glOrtho(0., window_width, 0., window_height, -100., 100.);
    gluPerspective(40.0, (GLfloat)window_width / (GLfloat) window_height, 0.1, 300.0); //for lorentz

    // set view matrix
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    //glRotatef(-90, 1.0, 0.0, 0.0);
    //glTranslatef(translate_x, translate_z, translate_y);
    //glTranslatef(0, translate_z, 0);
    //glRotatef(-90, 1.0, 0.0, 0.0);

    return;

}

void appKeyboard(unsigned char key, int x, int y)
{
    switch(key) 
    {
        case '\033': // escape quits
        case '\015': // Enter quits    
        case 'Q':    // Q quits
        case 'q':    // q (or escape) quits
            // Cleanup up and quit
            appDestroy();
            break;
    }
}

//----------------------------------------------------------------------
void appRender()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//gluLookAt(-4.,-4., 5., 5.,5.,0., 0., 0., 1.);
	gluLookAt(-20.,-80., 20., 0.,0.,0., 0., 0., 1.);

    ps->update();

	//glEnable(GL_DEPTH_TEST);

    ps->render();

    //showFPS(enjas->getFPS(), enjas->getReport());
    glutSwapBuffers();

	//glDisable(GL_DEPTH_TEST);
}

//----------------------------------------------------------------------
void appDestroy()
{

	printf("**** before delete ps\n");
    delete ps;
	printf("**** after delete ps\n");

    if(glutWindowHandle)glutDestroyWindow(glutWindowHandle);
    printf("about to exit!\n");

    exit(0);
}

void timerCB(int ms)
{
    glutTimerFunc(ms, timerCB, ms);
    glutPostRedisplay();
}


void appMouse(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN) {
        mouse_buttons |= 1<<button;
    } else if (state == GLUT_UP) {
        mouse_buttons = 0;
    }

    mouse_old_x = x;
    mouse_old_y = y;

    glutPostRedisplay();
}

void appMotion(int x, int y)
{
    float dx, dy;
    dx = x - mouse_old_x;
    dy = y - mouse_old_y;

    if (mouse_buttons & 1) {
        rotate_x += dy * 0.2;
        rotate_y += dx * 0.2;
    } else if (mouse_buttons & 4) {
        translate_z -= dy * 0.5;
    }

    mouse_old_x = x;
    mouse_old_y = y;

    // set view matrix
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

#if 1
    glRotatef(-90, 1.0, 0.0, 0.0);
    //glTranslatef(translate_x, translate_z, translate_y);
    glTranslatef(0, translate_z, 0);
    glRotatef(rotate_x, 1.0, 0.0, 0.0);
    glRotatef(rotate_y, 0.0, 0.0, 1.0); //we switched around the axis so make this rotate_z
#endif

    glutPostRedisplay();
}


///////////////////////////////////////////////////////////////////////////////
// write 2d text using GLUT
// The projection matrix must be set to orthogonal before call this function.
///////////////////////////////////////////////////////////////////////////////
void drawString(const char *str, int x, int y, float color[4], void *font)
{
    glPushAttrib(GL_LIGHTING_BIT | GL_CURRENT_BIT); // lighting and color mask
    glDisable(GL_LIGHTING);     // need to disable lighting for proper text color

    glColor4fv(color);          // set text color
    glRasterPos2i(x, y);        // place text position

    // loop all characters in the string
    while(*str)
    {
        glutBitmapCharacter(font, *str);
        ++str;
    }

    glEnable(GL_LIGHTING);
    glPopAttrib();
}

///////////////////////////////////////////////////////////////////////////////
// display frame rates
///////////////////////////////////////////////////////////////////////////////
void showFPS(float fps, std::string* report)
{
    static std::stringstream ss;

    // backup current model-view matrix
    glPushMatrix();                     // save current modelview matrix
    glLoadIdentity();                   // reset modelview matrix

    // set to 2D orthogonal projection
    glMatrixMode(GL_PROJECTION);        // switch to projection matrix
    glPushMatrix();                     // save current projection matrix
    glLoadIdentity();                   // reset projection matrix
    gluOrtho2D(0, 400, 0, 300);         // set to orthogonal projection

    float color[4] = {1, 1, 0, 1};

    // update fps every second
    ss.str("");
    ss << std::fixed << std::setprecision(1);
    ss << fps << " FPS" << std::ends; // update fps string
    ss << std::resetiosflags(std::ios_base::fixed | std::ios_base::floatfield);
    drawString(ss.str().c_str(), 15, 286, color, font);
    drawString(report[0].c_str(), 15, 273, color, font);
    drawString(report[1].c_str(), 15, 260, color, font);

    // restore projection matrix
    glPopMatrix();                      // restore to previous projection matrix

    // restore modelview matrix
    glMatrixMode(GL_MODELVIEW);         // switch to modelview matrix
    glPopMatrix();                      // restore to previous modelview matrix
}
//----------------------------------------------------------------------
