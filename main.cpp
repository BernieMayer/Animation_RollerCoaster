// ==========================================================================
// Barebones OpenGL Core Profile Boilerplate
//    using the GLFW windowing system (http://www.glfw.org)
//
// Loosely based on
//  - Chris Wellons' example (https://github.com/skeeto/opengl-demo) and
//  - Camilla Berglund's example (http://www.glfw.org/docs/latest/quick.html)
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================

#include <iostream>
#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include <vector>
#include <cstdlib>

#include "glm/glm.hpp"
#include <glm/gtc/type_ptr.hpp>
#include "glm/gtc/matrix_transform.hpp"

// specify that we want the OpenGL core profile before including GLFW headers
#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include "camera.h"

#include "Vec3f.h"
#include "Vec3f_FileIO.h"


#define PI 3.14159265359

using namespace std;
using namespace glm;

//Forward definitions
bool CheckGLErrors(string location);
void QueryGLVersion();
string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);
void loadVec3fFromFile( VectorContainerVec3f & vecs, std::string const & fileName );

double calculate_x(vec3 pos_prev, vec3 pos_current,vec3 pos_next);
double calculate_c(vec3 pos_past, vec3 pos_future);


vec2 mousePos;
bool leftmousePressed = false;
bool rightmousePressed = false;

Camera* activeCamera;

GLFWwindow* window = 0;

mat4 winRatio = mat4(1.f);

const vec3 GRAVITY = vec3(0, 9.81, 0);
bool isFirstPerson = false;


const int NUMCARTS = 10;
// --------------------------------------------------------------------------
// GLFW callback functions

// reports GLFW errors
void ErrorCallback(int error, const char* description)
{
    cout << "GLFW ERROR " << error << ":" << endl;
    cout << description << endl;
}

// handles keyboard input events
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    else if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
      std::cout << "Now printing to the camera location (" << activeCamera->pos.x << "," << activeCamera->pos.y << "," << activeCamera->pos.z << ")" <<"\n";

      printf("Now printing the camerea dir( %f, %f, %f) \n", activeCamera->dir.x, activeCamera->dir.y, activeCamera->dir.z );
      printf("Now printing the camera right ( %f, %f, %f) \n", activeCamera->right.x,activeCamera->right.y,activeCamera->right.z );
      printf("Now printing the camera up (%f, %f, %f) \n", activeCamera->up.x , activeCamera->right.y, activeCamera->right.z);
      //std::cout << "Now "
    } else if (key == GLFW_KEY_F && action == GLFW_PRESS)
    {
      if (isFirstPerson)
      {
        isFirstPerson = false;
        //Camera cam = Camera(vec3(0, 0, -1), vec3(0.31649,-0.564746,4.26627));

        activeCamera->pos = vec3(0.613712,2.25309,10.3123);
        activeCamera->dir = vec3( -0.058043, -0.213090, -0.975307);
        activeCamera->right = vec3(1,0,0);
        //activeCamera->right = vec3( 0.998291, -0.005760, -0.058152);
        //activeCamera->up = vec3(-0.006774, -0.005760, -0.058152);
      } else {
        isFirstPerson = true;
      }
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if( (action == GLFW_PRESS) || (action == GLFW_RELEASE) ){
		if(button == GLFW_MOUSE_BUTTON_LEFT)
			leftmousePressed = !leftmousePressed;
		else if(button == GLFW_MOUSE_BUTTON_RIGHT)
			rightmousePressed = !rightmousePressed;
	}
}

void mousePosCallback(GLFWwindow* window, double xpos, double ypos)
{
	int vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);

	vec2 newPos = vec2(xpos/(double)vp[2], -ypos/(double)vp[3])*2.f - vec2(1.f);

	vec2 diff = newPos - mousePos;
	if(leftmousePressed){
		activeCamera->trackballRight(-diff.x);
		activeCamera->trackballUp(-diff.y);
	}
	else if(rightmousePressed){
		float zoomBase = (diff.y > 0) ? 1.f/2.f : 2.f;

		activeCamera->zoom(pow(zoomBase, abs(diff.y)));
	}

	mousePos = newPos;
}

void resizeCallback(GLFWwindow* window, int width, int height)
{
	int vp[4];
	glGetIntegerv(GL_VIEWPORT, vp);

	glViewport(0, 0, width, height);

	float minDim = float(std::min(width, height));

	winRatio[0][0] = minDim/float(width);
	winRatio[1][1] = minDim/float(height);
}





//==========================================================================
// TUTORIAL STUFF


//vec2 and vec3 are part of the glm math library.
//Include in your own project by putting the glm directory in your project,
//and including glm/glm.hpp as I have at the top of the file.
//"using namespace glm;" will allow you to avoid writing everyting as glm::vec2

struct VertexBuffers{
	enum{ VERTICES=0, NORMALS, INDICES, COUNT};

	GLuint id[COUNT];
};

//Describe the setup of the Vertex Array Object
bool initVAO(GLuint vao, const VertexBuffers& vbo)
{
	glBindVertexArray(vao);		//Set the active Vertex Array

	glEnableVertexAttribArray(0);		//Tell opengl you're using layout attribute 0 (For shader input)
	glBindBuffer( GL_ARRAY_BUFFER, vbo.id[VertexBuffers::VERTICES] );		//Set the active Vertex Buffer
	glVertexAttribPointer(
		0,				//Attribute
		3,				//Size # Components
		GL_FLOAT,	//Type
		GL_FALSE, 	//Normalized?
		sizeof(vec3),	//Stride
		(void*)0			//Offset
		);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, vbo.id[VertexBuffers::NORMALS]);
	glVertexAttribPointer(
		1,				//Attribute
		3,				//Size # Components
		GL_FLOAT,	//Type
		GL_FALSE, 	//Normalized?
		sizeof(vec3),	//Stride
		(void*)0			//Offset
		);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.id[VertexBuffers::INDICES]);

	return !CheckGLErrors("initVAO");		//Check for errors in initialize
}


//Loads buffers with data
bool loadBuffer(const VertexBuffers& vbo,
				const vector<vec3>& points,
				const vector<vec3> normals,
				const vector<unsigned int>& indices)
{
	glBindBuffer(GL_ARRAY_BUFFER, vbo.id[VertexBuffers::VERTICES]);
	glBufferData(
		GL_ARRAY_BUFFER,				//Which buffer you're loading too
		sizeof(vec3)*points.size(),		//Size of data in array (in bytes)
		&points[0],						//Start of array (&points[0] will give you pointer to start of vector)
		GL_STATIC_DRAW					//GL_DYNAMIC_DRAW if you're changing the data often
										//GL_STATIC_DRAW if you're changing seldomly
		);

	glBindBuffer(GL_ARRAY_BUFFER, vbo.id[VertexBuffers::NORMALS]);
	glBufferData(
		GL_ARRAY_BUFFER,				//Which buffer you're loading too
		sizeof(vec3)*normals.size(),	//Size of data in array (in bytes)
		&normals[0],					//Start of array (&points[0] will give you pointer to start of vector)
		GL_STATIC_DRAW					//GL_DYNAMIC_DRAW if you're changing the data often
										//GL_STATIC_DRAW if you're changing seldomly
		);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.id[VertexBuffers::INDICES]);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER,
		sizeof(unsigned int)*indices.size(),
		&indices[0],
		GL_STATIC_DRAW
		);

	return !CheckGLErrors("loadBuffer");
}

bool loadCurveBuffer(const VertexBuffers& vbo,
				const vector<vec3>& points,
				const vector<vec3> normals)
{
  glBindBuffer(GL_ARRAY_BUFFER, vbo.id[VertexBuffers::VERTICES]);
  glBufferData(
    GL_ARRAY_BUFFER,				//Which buffer you're loading too
    sizeof(vec3)*points.size(),		//Size of data in array (in bytes)
    &points[0],						//Start of array (&points[0] will give you pointer to start of vector)
    GL_STATIC_DRAW					//GL_DYNAMIC_DRAW if you're changing the data often
                    //GL_STATIC_DRAW if you're changing seldomly
    );

  glBindBuffer(GL_ARRAY_BUFFER, vbo.id[VertexBuffers::NORMALS]);
  glBufferData(
    GL_ARRAY_BUFFER,				//Which buffer you're loading too
    sizeof(vec3)*normals.size(),	//Size of data in array (in bytes)
    &normals[0],					//Start of array (&points[0] will give you pointer to start of vector)
    GL_STATIC_DRAW					//GL_DYNAMIC_DRAW if you're changing the data often
                    //GL_STATIC_DRAW if you're changing seldomly
    );

    return !CheckGLErrors("loadBuffer");
}

//Compile and link shaders, storing the program ID in shader array
GLuint initShader(string vertexName, string fragmentName)
{
	string vertexSource = LoadSource(vertexName);		//Put vertex file text into string
	string fragmentSource = LoadSource(fragmentName);		//Put fragment file text into string

	GLuint vertexID = CompileShader(GL_VERTEX_SHADER, vertexSource);
	GLuint fragmentID = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

	return LinkProgram(vertexID, fragmentID);	//Link and store program ID in shader array
}

//Initialization
void initGL()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glClearColor(0.f, 0.f, 0.f, 0.f);		//Color to clear the screen with (R, G, B, Alpha)
}

bool loadUniforms(GLuint program, mat4 perspective, mat4 modelview)
{
	glUseProgram(program);

	glUniformMatrix4fv(glGetUniformLocation(program, "modelviewMatrix"),
						1,
						false,
						&modelview[0][0]);

	glUniformMatrix4fv(glGetUniformLocation(program, "perspectiveMatrix"),
						1,
						false,
						&perspective[0][0]);

	return !CheckGLErrors("loadUniforms");
}

//Draws buffers to screen
void render(GLuint vao, int startElement, int numElements)
{
	glBindVertexArray(vao);		//Use the LINES vertex array

	glDrawElements(
			GL_TRIANGLES,		//What shape we're drawing	- GL_TRIANGLES, GL_LINES, GL_POINTS, GL_QUADS, GL_TRIANGLE_STRIP
			numElements,		//How many indices
			GL_UNSIGNED_INT,	//Type
			(void*)0			//Offset
			);

	CheckGLErrors("render");
}

void renderCurve(GLuint vao, int numPoints)
{
  glBindVertexArray(vao);

  glDrawArrays( GL_LINE_LOOP, 0, numPoints);

  CheckGLErrors("renderCurve");
  //glBindVertexArray(0);
}

void renderBead(GLuint programBead, vec3 beadPosition, mat4 perspective, mat4 modelview)
{

  glUseProgram(programBead);


  glUniformMatrix4fv(glGetUniformLocation(programBead, "modelviewMatrix"),
            1,
            GL_FALSE,
            glm::value_ptr(modelview));

  glUniformMatrix4fv(glGetUniformLocation(programBead, "perspectiveMatrix"),
            1,
            GL_FALSE,
            glm::value_ptr(perspective));

  glUniform3fv(glGetUniformLocation(programBead, "beadPosition"), 1, glm::value_ptr(beadPosition) );

  glPointSize(10.0f);
  glDrawArrays(GL_POINTS, 0, 1);

  CheckGLErrors("renderBead");

}




void generateSquare(vector<vec3>* vertices, vector<vec3>* normals,
					vector<unsigned int>* indices, float width)
{
	vertices->push_back(vec3(-width*0.5f, -width*0.5f, 0.f));
	vertices->push_back(vec3(width*0.5f, -width*0.5f, 0.f));
	vertices->push_back(vec3(width*0.5f, width*0.5f, 0.f));
	vertices->push_back(vec3(-width*0.5f, width*0.5f, 0.f));

	normals->push_back(vec3(0.f, 0.f, 1.f));
	normals->push_back(vec3(0.f, 0.f, 1.f));
	normals->push_back(vec3(0.f, 0.f, 1.f));
	normals->push_back(vec3(0.f, 0.f, 1.f));

	//First triangle
	indices->push_back(0);
	indices->push_back(1);
	indices->push_back(2);
	//Second triangle
	indices->push_back(2);
	indices->push_back(3);
	indices->push_back(0);
}

void generatePlane(vector<vec3>* vertices, vector<vec3>* normals,
            vector<unsigned int>* indices, float width)
{
  vertices->push_back(vec3(-width*0.5f,  -1.f, -width*0.5f));
  vertices->push_back(vec3(width*0.5f,  -1.f , -width*0.5f));
  vertices->push_back(vec3(width*0.5f,  -1.f , width*0.5f));
  vertices->push_back(vec3(-width*0.5f,  -1.f, width*0.5f));

  normals->push_back(vec3(1.f, 0.f, 0.f));
  normals->push_back(vec3(1.f, 0.f, 0.f));
  normals->push_back(vec3(1.f, 0.f, 0.f));
  normals->push_back(vec3(1.f, 0.f, 0.f));

  //First triangle
  indices->push_back(0);
  indices->push_back(1);
  indices->push_back(2);
  //Second triangle
  indices->push_back(2);
  indices->push_back(3);
  indices->push_back(0);
}

void generateCart(vector<vec3>* vertices, std::vector<vec3>* normals,
   vector<unsigned int>* indices, float width)
{

  vertices->push_back(vec3(0,0,0));
  vertices->push_back(vec3(width * 1,0,0));
  vertices->push_back(vec3(width * 1,width * 1,0));
  vertices->push_back(vec3(0,width * 1,0));

  vertices->push_back(vec3(0,0,width * 1));
  vertices->push_back(vec3(width * 1,0,width * 1));
  vertices->push_back(vec3(width * 1, width * 1, width * 1));
  vertices->push_back(vec3(0,width * 1, width * 1));

  indices->push_back(0);
  indices->push_back(1);
  indices->push_back(2);

  indices->push_back(2);
	indices->push_back(3);
	indices->push_back(0);

  indices->push_back(0);
  indices->push_back(4);
  indices->push_back(7);

  indices->push_back(7);
  indices->push_back(0);
  indices->push_back(3);

  indices->push_back(1);
  indices->push_back(2);
  indices->push_back(5);

  indices->push_back(6);
  indices->push_back(5);
  indices->push_back(2);

  indices->push_back(5);
  indices->push_back(6);
  indices->push_back(4);

  indices->push_back(4);
  indices->push_back(7);
  indices->push_back(6);

  indices->push_back(0);
  indices->push_back(1);
  indices->push_back(5);

  indices->push_back(5);
  indices->push_back(4);
  indices->push_back(0);

  int half = vertices->size()/ 2;
  for (int i = 0; i < half; i++)
  {
    normals->push_back(vec3(0.f, 0.f, 1.f));
  }

  for (int i = half; i < vertices->size(); i++)
  {
    normals->push_back(vec3(0.0f, 1.0f, 0.f));
  }




}


vec3 arcLengthParameterization(vec3 bead_pos, int& i, vector<vec3> points, double deltaS)
{

  vec3 newBeadPos;
  int size = points.size();
  //assume points is at least i + 1 in size

  //case 1, the distance between the next point is greater than deltaS
  if (abs(length((points.at((i +1) % size) - bead_pos))) > deltaS)
  {
    newBeadPos = bead_pos + (points.at((i + 1) % size) - bead_pos) * (float)((deltaS / abs((length(points.at((i + 1)%size) - bead_pos)))));
    return newBeadPos;
  } else
  {
    //The distance between the bead postion and the next point is less than or
    //equal to deltaS

    double s_prime = abs(length((points.at(i % size) - bead_pos)));
    i +=1;

    while (( s_prime + abs(length(points.at((i + 1) % size) - points.at(i % size) ))  ) < deltaS)
    {
      s_prime = s_prime + abs((length(points.at((i + 1) % size)  - points.at(i % size))));
      i += 1;
    }

    newBeadPos = bead_pos + (points.at((i + 1)%size) - points.at(i % size)) * (float)(abs((deltaS - s_prime))/(abs((length(points.at( (i+ 1) % size) - points.at(i % size))))));

    return newBeadPos;
  }
}

mat4 makeFresnetFrame(vec3 bead_pos, vector<vec3> curve_points, double velocity,  int i)
{

  double vs = velocity * 1.0f/60.0f;

  vec3 beadPos_tmp = arcLengthParameterization(beadPos_tmp, i, curve_points, vs);

  vec3 beadPos_prev = curve_points.at( (i - 10) % curve_points.size());
  vec3 beadPos_future = curve_points.at( (i + 10) % curve_points.size());
  double x = calculate_x(beadPos_prev, beadPos_tmp ,beadPos_future);
  double c = calculate_c(beadPos_prev, beadPos_future);

  double r = (pow(x, 2) + pow(c, 2))/(2 * x);
  double k = 1.0f/r;

  vec3 n = 1.0f/(length(beadPos_future - 2.0f * beadPos_tmp + beadPos_prev))
             *  (beadPos_future - 2.0f * beadPos_tmp + beadPos_prev);

  vec3 acc_perpendicular = (float) k * n;

  vec3 Normal_cart = +(acc_perpendicular + GRAVITY);

  vec3 Tangent = beadPos_future - beadPos_prev;
  vec3 T_tmp = normalize(Tangent);

  vec3 normalizd_Normal_cart = normalize(Normal_cart);

  vec3 B = cross(T_tmp, normalizd_Normal_cart);
  vec3 normalized_B = normalize(B);

  vec3 T = cross(normalizd_Normal_cart, normalized_B);
  vec3 T_hat = normalize(T);
  mat4 ModelMatrix = mat4(vec4(normalized_B,0), vec4(normalizd_Normal_cart,0),  vec4(T_hat,0), vec4(beadPos_tmp, 1));
  return ModelMatrix;
}

int highestPoint(std::vector<vec3> points)
{
  //assume points is at least one element

  int index = 0;
  vec3 highestPoint = points.at(0);

  for (int i = 0; i < points.size(); i++)
  {
    if ( points.at(i).y > highestPoint.y)
    {
      index = i;
      highestPoint = points.at(i);
    }
  }

  return index;
}


void generateSecondLineForTrack(vector<vec3> current_Points,
                                vector<vec3>* newPoints1,
                                std::vector<vec3>* newPoints2,
                                vector<vec3>* normals)
{
    for (int i = 0; i < current_Points.size(); i++)
    {
      vec3 posFuture = current_Points.at( (i + 1)%current_Points.size());
      vec3 posPast = current_Points.at( (i - 1)%current_Points.size());
      vec3 posCurrent = current_Points.at(i);

      double x = calculate_x(posPast, posCurrent, posFuture);
      double c = calculate_c(posPast, posFuture);

      vec3 acc_perpendicular = (float)(1.0f/(pow(x,2) + pow(c,2)))
                          * (posFuture - 2.0f * posCurrent + posPast);
      vec3 normal = acc_perpendicular + GRAVITY;

      vec3 Tangent = posFuture - posCurrent;
      vec3 normalized_Tangent = normalize(Tangent);

      vec3 normalized_normal = normalize(normal);

      vec3 B = cross(normalized_Tangent, normalized_normal);
      vec3 B_hat = normalize(B);

      vec3 newPoint1 = posCurrent + 0.3f * B_hat;
      vec3 newPoint2 = posCurrent - 0.3f * B_hat;
      newPoints1->push_back(newPoint1);
      newPoints2->push_back(newPoint2);
    }
    //newPoints1->push_back(newPoints->at(0));

    for (int j = 0; j < newPoints1->size(); j++)
    {
      normals->push_back(vec3(0,1,0));
    }
}

//This function will make a curve that will be used to make the coaster
void generateCurve(vector<vec3>* points, vector<vec3>* normals)
{

    vector<vec3> controlPoints;
    int subdivisions = 5;

    VectorContainerVec3f vectors;

    std::string file("./Track3.con");
    std::cout<< "Now reading from file named" << file << "\n";
    loadVec3fFromFile(vectors, file);

    for (int i = 0; i < vectors.size(); i++)
    {
      Vec3f vec = vectors.at(i);
      std::cout << vec << std::endl;
      points->push_back(vec3(5 * vec.m_x, 5 * vec.m_z, 5 * vec.m_y));
    }


    //points->at(4).y = 0.2;

    //points->at(5).y = 0.5;

    //Add the control points to the lists
    //These points will determine the curve
    /*
    points->push_back(vec3(0,0,0));
    points->push_back(vec3(1,0,0));
    points->push_back(vec3(2,0,0));
    points->push_back(vec3(3,0.5,2));
    points->push_back(vec3(3, 1,2));
    points->push_back(vec3(3, 0, 5));
    points->push_back(vec3(0, 0, 5));
    points->push_back(vec3(0,0,0));
    */



    int j;
    for (int i = 0; i < subdivisions; i++)
    {
      vector<vec3> Q;
      //Add the mid points of the control points to the set Q
      for ( j = 0; j < (points->size() - 1); j++)
      {
        Q.push_back(points->at(j));
        Q.push_back((points->at(j) + points->at(j + 1)) * 0.5f);
      }
      //Makes sure the curve connects back to the start
      Q.push_back(points->at(j));
      //Q.push_back(points)

      //Clear the points since we will want to make a new
      //set of points
      points->clear();
      int r;
      //Subdivide the curve so that the new set of points will be smoother
      for (r = 0; r < (Q.size() - 1); r++)
      {
        points->push_back((Q.at(r) + Q.at(r +1)) * 0.5f);
      }
      //Add in the first point so that the curve will go back to the start
      points->push_back(points->at(0));
    }



    //Normals is poorly named here should actually be color
    for (int i = 0; i < points->size(); i++)
    {
      normals->push_back(vec3(0.f, 0.f,1.f));
    }


}

//This calculation is used to calculate the x value
//The x value is an important part to calculating the
//radius of the curvature circle of the curve
double calculate_x(vec3 pos_prev, vec3 pos_current,vec3 pos_next)
{

  double x = 1.0f/2.0f * ( length(pos_next - 2.0f * pos_current + pos_prev));
  return x;
}

//C is defined as the half distance between the past point and the future point
double calculate_c(vec3 pos_past, vec3 pos_future)
{
  double c = 1.0f/2.0f * length(pos_future - pos_past);
  return c;
}


GLFWwindow* createGLFWWindow()
{
	// initialize the GLFW windowing system
    if (!glfwInit()) {
        cout << "ERROR: GLFW failed to initialize, TERMINATING" << endl;
        return NULL;
    }
    glfwSetErrorCallback(ErrorCallback);

    // attempt to create a window with an OpenGL 4.1 core profile context
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(512, 512, "OpenGL Example", 0, 0);
    if (!window) {
        cout << "Program failed to create GLFW window, TERMINATING" << endl;
        glfwTerminate();
        return NULL;
    }

    // set keyboard callback function and make our context current (active)
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, mousePosCallback);
    glfwSetWindowSizeCallback(window, resizeCallback);
    glfwMakeContextCurrent(window);

    return window;
}



// ==========================================================================
// PROGRAM ENTRY POINT

int main(int argc, char *argv[])
{
    window = createGLFWWindow();
    if(window == NULL)
    	return -1;

    //Initialize glad
    if (!gladLoadGL())
	{
		cout << "GLAD init failed" << endl;
		return -1;
	}

    // query and print out information about our OpenGL environment
    QueryGLVersion();

	initGL();

	//Initialize shader
	GLuint program = initShader("vertex.glsl", "fragment.glsl");
  GLuint beadProg = initShader("bead.vert", "bead.frag");



	GLuint vao;
	VertexBuffers vbo;

  GLuint vao_plane;
  VertexBuffers  vbo_plane;


  GLuint curve_vao;
  VertexBuffers curve_vbo;

  GLuint curve2_vao;
  VertexBuffers curve2_vbo;

  GLuint curve3_vao;
  VertexBuffers curve3_vbo;

	//Generate object ids
	glGenVertexArrays(1, &vao);
	glGenBuffers(VertexBuffers::COUNT, vbo.id);

  glGenVertexArrays(1, &vao_plane);
  glGenBuffers(VertexBuffers::COUNT, vbo_plane.id);

  //Generate curve ids
  glGenVertexArrays(1, &curve_vao);
  glGenBuffers(VertexBuffers::COUNT, curve_vbo.id);

  glGenVertexArrays(1, &curve2_vao);
  glGenBuffers(VertexBuffers::COUNT, curve2_vbo.id);

  glGenVertexArrays(1, &curve3_vao);
  glGenBuffers(VertexBuffers::COUNT, curve3_vbo.id);

	initVAO(vao, vbo);
  initVAO(vao_plane, vbo_plane);
  initVAO(curve_vao, curve_vbo);
  initVAO(curve2_vao, curve2_vbo);
  initVAO(curve3_vao, curve3_vbo);

	//Geometry information
	vector<vec3> points, normals;
	vector<unsigned int> indices;

	//generateSquare(&points, &normals, &indices, 1.f);
  generateCart(&points, &normals, &indices, 0.4f);

  vector <vec3> plane_points, plane_normals;
  vector <unsigned int> plane_indices;

  generatePlane(&plane_points, &plane_normals, &plane_indices, 40.f);

  vector<vec3> curve_points, curve_normals;
  vector<unsigned int> curve_indices;

  generateCurve(&curve_points, &curve_normals);

  vector<vec3> curve2_points, curve2_normals;
  vector<vec3> curve3_points;
  generateSecondLineForTrack(curve_points, &curve2_points,&curve3_points, &curve2_normals);

  int index_of_highest_point = highestPoint(curve_points);
  vec3 H = curve_points.at(index_of_highest_point);
  cout << "The highestPoint has a y of " <<  H.y << "\n";
  vec3 beadPos = curve_points.at(index_of_highest_point);

  vec3 beadPos_prev = beadPos;  //used to calculate the tangential acceleratiion
  vec3 beadPos_future = beadPos_future; //used to calculate the tangential acceleratiion

	loadBuffer(vbo, points, normals, indices);
  loadBuffer(vbo_plane, plane_points, plane_normals, plane_indices);
  loadCurveBuffer(curve_vbo, curve_points, curve_normals);
  loadCurveBuffer(curve2_vbo, curve2_points, curve2_normals);
  loadCurveBuffer(curve3_vbo, curve3_points, curve2_normals);



	Camera cam = Camera(vec3(0, 0, -1), vec3(0.31649,-0.564746,4.26627));
	activeCamera = &cam;
	//float fovy, float aspect, float zNear, float zFar
	mat4 perspectiveMatrix = perspective(radians(80.f), 1.f, 0.1f, 20.f);
  double t = 0;
  int i = index_of_highest_point;
  bool phases_of_the_coaster = true;

  bool lifting_stage = false;
  bool gravityFreeFall = false;
  bool deacc_Stage = true;
  double v_dec = 0.0f;
  double l_dec = 0.0f;

  vector<mat4> modelMatrices;

  vec3 beadPos_tmp = beadPos;
  int j = index_of_highest_point;
  for (int a = 0; a < NUMCARTS; a++)
  {
    double v_tmp = sqrt( (2.0f * dot(GRAVITY , (H - beadPos_tmp)) + 2.f ));

    double vs = v_tmp * 1.0f/60.0f;

    beadPos_tmp = arcLengthParameterization(beadPos_tmp, j, curve_points,  vs);

    beadPos_prev = curve_points.at( (j - 10) % curve_points.size());
    beadPos_future = curve_points.at( (j + 10) % curve_points.size());
    double x = calculate_x(beadPos_prev, beadPos_tmp ,beadPos_future);
    double c = calculate_c(beadPos_prev, beadPos_future);

    double r = (pow(x, 2) + pow(c, 2))/(2 * x);
    double k = 1.0f/r;

    vec3 n = 1.0f/(length(beadPos_future - 2.0f * beadPos_tmp + beadPos_prev))
               *  (beadPos_future - 2.0f * beadPos_tmp + beadPos_prev);

    vec3 acc_perpendicular = (float) k * n;

    vec3 Normal_cart = +(acc_perpendicular + GRAVITY);

        vec3 Tangent = beadPos_future - beadPos_prev;
        vec3 T_tmp = normalize(Tangent);

        vec3 normalizd_Normal_cart = normalize(Normal_cart);

        vec3 B = cross(T_tmp, normalizd_Normal_cart);
        vec3 normalized_B = normalize(B);

        vec3 T = cross(normalizd_Normal_cart, normalized_B);
        vec3 T_hat = normalize(T);
        mat4 ModelMatrix = mat4(vec4(normalized_B,0), vec4(normalizd_Normal_cart,0),  vec4(T_hat,0), vec4(beadPos_tmp, 1));
        //printf("The modelMatrices at %i and j is %i is (%f, %f, %f) \n", a, j, beadPos_tmp.x, beadPos_tmp.y,
        //beadPos_tmp.z);
        modelMatrices.push_back(ModelMatrix);

    //mat4 matrix = ?
    //modelMatrices.push_back(matrix);
  }

    // run an event-triggered main loop
    while (!glfwWindowShouldClose(window))
    {
    glClearColor(0.7,0.7,0.7, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);		//Clear color and depth buffers (Haven't covered yet)
		glUseProgram(program);

		loadUniforms(program, winRatio*perspectiveMatrix*cam.getMatrix(), mat4(1.f));

        // call function to draw our scene
        //render(vao, 0, indices.size());

        renderCurve(curve_vao, curve_points.size());
        renderCurve(curve2_vao, curve2_points.size());
        renderCurve(curve3_vao, curve3_points.size());


        //loadUniforms(beadProg, winRatio*perspectiveMatrix*cam.getMatrix(), mat4(1.f));








        //note vs = v * delta_t



        if (gravityFreeFall && ( (float) (i % curve_points.size())/ (float) curve_points.size()) > 0.60)
        {
          //std::cout << "Now in deacc_Stage stage \n";
          gravityFreeFall = false;
          deacc_Stage = true;
          lifting_stage = false;
          v_dec = sqrt( (2.0f * dot(GRAVITY , (H - beadPos)) + 1.f ));
          l_dec = length(curve_points.at(i%curve_points.size()) - curve_points.at(curve_points.size() -1));
        } else if (!gravityFreeFall && beadPos.y > (H.y - 0.5))
        {
          //std::cout << "Now in gravity Free fall stage \n";
          gravityFreeFall = true;
          deacc_Stage = false;
          lifting_stage = false;
        } else if (deacc_Stage && (i == (curve_points.size() - 12)))
        {
          //std::cout << "Now in lifting stage \n";
          gravityFreeFall = false;
          deacc_Stage = false;
          lifting_stage = true;
        }


        double v;

        //The velocity will change here..
        if (gravityFreeFall)
        {
        v = sqrt( (2.0f * dot(GRAVITY , (H - beadPos)) + 2.f ));
      } else if (deacc_Stage)  {
        v = v_dec * length(beadPos - curve_points.at(curve_points.size() -1))/(l_dec);
      } else
      {
        v = v_dec;
      }


        double vs = v * 1.0f/60.0f;

        ///
        //vec3 arcLengthParameterization(vec3 bead_pos, int i, vector<vec3> points, double deltaS)
        beadPos = arcLengthParameterization(beadPos,i , curve_points, vs);
        //activeCamera->pos = beadPos + vec3(0,0.1,0);
        beadPos_prev = curve_points.at( (i - 10) % curve_points.size());
        beadPos_future = curve_points.at( (i + 10) % curve_points.size());

        double x = calculate_x(beadPos_prev, beadPos ,beadPos_future);
        double c = calculate_c(beadPos_prev, beadPos_future);

        //Calcualating the radius of the curvature
        double r = (pow(x, 2) + pow(c, 2))/(2 * x);
        double k = 1.0f/r;

        vec3 n = 1.0f/(length(beadPos_future - 2.0f * beadPos + beadPos_prev))
                *  (beadPos_future - 2.0f * beadPos + beadPos_prev);


        //Could change this to be simpler
        vec3 acc_perpendicular = (float) k * n;
        //Gravity is added in this case, since GRAVITY is positive
        vec3 Normal_cart = +(acc_perpendicular + GRAVITY);

        vec3 Tangent = beadPos_future - beadPos_prev;
        vec3 T_tmp = normalize(Tangent);

        vec3 normalized_Normal_cart = normalize(Normal_cart);

        vec3 B = cross(T_tmp, normalized_Normal_cart);
        vec3 normalized_B = normalize(B);

        vec3 T = cross(normalized_Normal_cart, normalized_B);
        vec3 T_hat = normalize(T);
        mat4 ModelMatrix = mat4(vec4(normalized_B,0), vec4(normalized_Normal_cart,0),  vec4(T_hat,0), vec4(beadPos, 1));
        mat4 ModelMatrix2 = mat4(vec4(normalized_B,0),vec4(normalized_Normal_cart,0), vec4(T_hat,0), vec4(beadPos- vec3(0.4, 0,0),1));
        if (isFirstPerson){
        activeCamera->pos = beadPos + 0.75f * normalized_Normal_cart;
        activeCamera->up = normalized_Normal_cart;
        activeCamera->dir = T_tmp;
        activeCamera->right = normalized_B;
        }


        //loadUniforms(program, winRatio*perspectiveMatrix*cam.getMatrix(), ModelMatrix);
        //render(vao, 0, indices.size());

        loadUniforms(program, winRatio * perspectiveMatrix * cam.getMatrix(), ModelMatrix);
        render(vao, 0, indices.size());
        /*
        for (int l = 0; l <NUMCARTS;  l++)
        {
          loadUniforms(program, winRatio*perspectiveMatrix*cam.getMatrix(), modelMatrices.at(0));
          render(vao, 0, indices.size());
        } */

        //loadUniforms(program, winRatio*perspectiveMatrix*cam.getMatrix(), modelMatrices.at(0));
        //render(vao, 0, indices.size());

        modelMatrices.pop_back();
        modelMatrices.push_back(ModelMatrix);
        /*
        loadUniforms(program, winRatio *perspectiveMatrix * cam.getMatrix(), modelMatrices.at(1));
        render(vao, 0, indices.size());
        loadUniforms(program, winRatio *perspectiveMatrix * cam.getMatrix(), modelMatrices.at(2));
        render(vao, 0, indices.size());
        */
        loadUniforms(program, winRatio*perspectiveMatrix*cam.getMatrix(), mat4(1.f));
        render(vao_plane, 0, plane_indices.size());

        //renderBead(beadProg,beadPos, winRatio*perspectiveMatrix*cam.getMatrix(),mat4(1.f));
        //std::cout << "ds is " << vs << "\n";
        i = i % curve_points.size();
        // scene is rendered to the back buffer, so swap to front for display
        glfwSwapBuffers(window);
        glfwSwapInterval(1);

        // sleep until next event before drawing again
        glfwPollEvents();
	}

	// clean up allocated resources before exit
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(VertexBuffers::COUNT, vbo.id);
  glDeleteVertexArrays(1, &curve_vao);
  glDeleteBuffers(VertexBuffers::COUNT, curve_vbo.id);
	glDeleteProgram(program);
  glDeleteProgram(beadProg);


	glfwDestroyWindow(window);
   glfwTerminate();

   return 0;
}

// ==========================================================================
// SUPPORT FUNCTION DEFINITIONS

// --------------------------------------------------------------------------
// OpenGL utility functions

void QueryGLVersion()
{
    // query opengl version and renderer information
    string version  = reinterpret_cast<const char *>(glGetString(GL_VERSION));
    string glslver  = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
    string renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

    cout << "OpenGL [ " << version << " ] "
         << "with GLSL [ " << glslver << " ] "
         << "on renderer [ " << renderer << " ]" << endl;
}

bool CheckGLErrors(string location)
{
    bool error = false;
    for (GLenum flag = glGetError(); flag != GL_NO_ERROR; flag = glGetError())
    {
        cout << "OpenGL ERROR:  ";
        switch (flag) {
        case GL_INVALID_ENUM:
            cout << location << ": " << "GL_INVALID_ENUM" << endl; break;
        case GL_INVALID_VALUE:
            cout << location << ": " << "GL_INVALID_VALUE" << endl; break;
        case GL_INVALID_OPERATION:
            cout << location << ": " << "GL_INVALID_OPERATION" << endl; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            cout << location << ": " << "GL_INVALID_FRAMEBUFFER_OPERATION" << endl; break;
        case GL_OUT_OF_MEMORY:
            cout << location << ": " << "GL_OUT_OF_MEMORY" << endl; break;
        default:
            cout << "[unknown error code]" << endl;
        }
        error = true;
    }
    return error;
}

// --------------------------------------------------------------------------
// OpenGL shader support functions

// reads a text file with the given name into a string
string LoadSource(const string &filename)
{
    string source;

    ifstream input(filename.c_str());
    if (input) {
        copy(istreambuf_iterator<char>(input),
             istreambuf_iterator<char>(),
             back_inserter(source));
        input.close();
    }
    else {
        cout << "ERROR: Could not load shader source from file "
             << filename << endl;
    }

    return source;
}

// creates and returns a shader object compiled from the given source
GLuint CompileShader(GLenum shaderType, const string &source)
{
    // allocate shader object name
    GLuint shaderObject = glCreateShader(shaderType);

    // try compiling the source as a shader of the given type
    const GLchar *source_ptr = source.c_str();
    glShaderSource(shaderObject, 1, &source_ptr, 0);
    glCompileShader(shaderObject);

    // retrieve compile status
    GLint status;
    glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint length;
        glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
        string info(length, ' ');
        glGetShaderInfoLog(shaderObject, info.length(), &length, &info[0]);
        cout << "ERROR compiling shader:" << endl << endl;
        cout << source << endl;
        cout << info << endl;
    }

    return shaderObject;
}

// creates and returns a program object linked from vertex and fragment shaders
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader)
{
    // allocate program object name
    GLuint programObject = glCreateProgram();

    // attach provided shader objects to this program
    if (vertexShader)   glAttachShader(programObject, vertexShader);
    if (fragmentShader) glAttachShader(programObject, fragmentShader);

    // try linking the program with given attachments
    glLinkProgram(programObject);

    // retrieve link status
    GLint status;
    glGetProgramiv(programObject, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint length;
        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
        string info(length, ' ');
        glGetProgramInfoLog(programObject, info.length(), &length, &info[0]);
        cout << "ERROR linking shader program:" << endl;
        cout << info << endl;
    }

    return programObject;
}


// ==========================================================================
