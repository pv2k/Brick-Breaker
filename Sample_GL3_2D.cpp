#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>
#include <map>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

struct Color {
	float r;
	float g;
	float b;
};
typedef struct Color Color;

struct Point {
    float x;
	float y;
};
typedef struct Point Point;

struct gameObjects {
	string name;
	VAO *object;
	Color color;
	int status=0;
	int flag = 0;
	float x;
	float y;
	float speed;
	float len,breadth,radius;
	float angle=0;
};
typedef struct gameObjects gameObjects;

map <string,gameObjects> Gun;
map <string,gameObjects> Brick;
map <string,gameObjects> Basket;
map <string,gameObjects> Laser;
map <string,gameObjects> Mirror;
map <string,gameObjects> Line;

GLuint programID;

int fbwidth=600,fbheight=600;
float red_basket_trans_dir;
float green_basket_trans_dir;
bool red_basket_trans_status = false;
bool green_basket_trans_status = false;
float gun_trans_dir;
bool gun_trans_status = false;
float gun_rot_dir;
bool gun_rot_status = false;
float brick_speed = 0.05;
int laser_trans_status = 0;
float camera_rotation_angle = 90;
float triangle_rotation = 0;
float red_basket_rotation = 0;
float red_basket_translation = -2.0f;
float green_basket_rotation = 0;
float green_basket_translation = 2.0f;
float gun_rotation = 0;
float gun_translation = 0;
float laser_translation = -3.6;
float click_time;
int laser_count=1;
float x_intersection,y_intersection;
double last_update_time, current_time;
int brick_cnt = 0;
int m_flag0=0,m_flag1=0,m_flag2=0,m_flag3=0,zp_flag=0;
double mouse_x,mouse_y,m_click_x;
int zoom=0;
float pan=0;
int points = 0,misfire=0;

void createRectangle (string comp, string body, Color Color, float l, float b, float x, float y,float angle);

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
//    exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
     // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE) {
        switch (key) {
			case GLFW_KEY_S:
				gun_trans_status = false;
				break;
			case GLFW_KEY_F:
				gun_trans_status = false;
				break;
			case GLFW_KEY_A:
				gun_rot_status = false;
				break;
			case GLFW_KEY_D:
				gun_rot_status = false;
				break;
			case GLFW_KEY_LEFT_CONTROL:
				red_basket_trans_status = false;
				break;
			case GLFW_KEY_RIGHT_CONTROL:
				red_basket_trans_status = false;
				break;
			case GLFW_KEY_RIGHT_ALT:
				green_basket_trans_status = false;
				break;
			case GLFW_KEY_LEFT_ALT:
				green_basket_trans_status = false;
				break;
			case GLFW_KEY_SPACE:
				// click_time = glfwGetTime();
				laser_trans_status = 1;
				break;
            case GLFW_KEY_X:
                // do something ..
                break;
            default:
                break;
        }
    }
    else if (action == GLFW_PRESS) {
		zp_flag = 0;
        switch (key) {
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
			case GLFW_KEY_S:
				gun_trans_status = true;
				gun_trans_dir = 0.1;
				break;
			case GLFW_KEY_F:
				gun_trans_status = true;
				gun_trans_dir = -0.1;
				break;
			case GLFW_KEY_A:
				gun_rot_dir = 0.5;
				gun_rot_status = true;
				break;
			case GLFW_KEY_D:
				gun_rot_dir = -0.5;
				gun_rot_status = true;
				break;
			case GLFW_KEY_LEFT_CONTROL:
				red_basket_trans_status = true;
				red_basket_trans_dir = -0.08;
				break;
			case GLFW_KEY_RIGHT_CONTROL:
				red_basket_trans_status = true;
				red_basket_trans_dir = 0.08;
				break;
			case GLFW_KEY_LEFT_ALT:
				green_basket_trans_status = true;
				green_basket_trans_dir = -0.08;
				break;
			case GLFW_KEY_RIGHT_ALT:
				green_basket_trans_status = true;
				green_basket_trans_dir = 0.08;
				break;
			case GLFW_KEY_N:
				if(brick_speed<=0.11)
					brick_speed+=0.02;
				break;
			case GLFW_KEY_M:
				if(brick_speed>0.03)
					brick_speed-=0.02;
				break;
			case GLFW_KEY_UP:
				if(zoom<3){
					zoom++;
					zp_flag = 1;
				}
				break;
			case GLFW_KEY_DOWN:
				if(zoom){
					zoom--;
					zp_flag = 1;
				}
				break;
			case GLFW_KEY_LEFT:
				if(zoom){
					pan++;
					zp_flag = 1;
				}
				break;
			case GLFW_KEY_RIGHT:
				if(zoom){
					pan--;
					zp_flag = 1;
				}
				break;
			default:
                break;
        }
    }
	if(zp_flag){
	if(pan>zoom)
		pan=zoom;
	if(pan<-zoom)
		pan=-zoom;
	Matrices.projection = glm::ortho(-4.0f+zoom-pan, 4.0f-zoom-pan, -4.0f+zoom, 4.0f-zoom, 0.1f, 500.0f);
}
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		default:
			break;
	}
}

static void cursor_position(GLFWwindow* window, double xpos, double ypos)
{
	mouse_x= ((8*xpos)/fbwidth)-4;
	mouse_y=-((8*ypos)/fbheight)+4;;
	if(m_flag0==1){
		if(mouse_x>3.4f)
		  mouse_x = 3.4f;
		else if(mouse_x < -3.4f)
		  mouse_x = -3.4f;
		red_basket_translation = mouse_x;
	}
	if(m_flag1==1){
		if(mouse_x>=3.4f)
		  mouse_x = 3.4f;
		else if(mouse_x <= -3.4f)
		  mouse_x = -3.4f;
		green_basket_translation = mouse_x;
	}
	if(m_flag2==1){
		if(mouse_y > 3.44f)
		  mouse_y = 3.44f;
		if(mouse_y < -1.64f)
		  mouse_y = -1.64f;
		gun_translation = mouse_y;
	}
	// trans_cons();
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
	Color red = {1,0,0};
	if(action == GLFW_RELEASE){
		if(button == GLFW_MOUSE_BUTTON_LEFT){
			m_flag0=0;
			m_flag1=0;
			m_flag2=0;
		}
		if(button == GLFW_MOUSE_BUTTON_RIGHT)
			m_flag3=0;
	}

	else if(action == GLFW_PRESS){
		if(button == GLFW_MOUSE_BUTTON_LEFT){
			if(mouse_x>=-4 && mouse_x<=-3.0 && mouse_y>=gun_translation-0.2 && mouse_y<=gun_translation+0.2){
				m_flag2=1;
			}
			else if(mouse_x>=-0.3+red_basket_translation && mouse_x<=0.3+red_basket_translation && mouse_y>=-3.25 && mouse_y<=-2.75){
				m_flag0=1;}
			else if(mouse_x>=-0.3+green_basket_translation && mouse_x<=0.3+green_basket_translation && mouse_y>=-3.25 && mouse_y<=-2.75)
				m_flag1=1;
			else if(mouse_x>-3.63){
				float slope = (mouse_y-gun_translation)/(mouse_x+3.63);
				float anglee= (atan(slope)*180.0)/M_PI;
				if(anglee>=-60 && anglee<=60){
					gun_rotation = anglee;
					string laser = "laser" + to_string(laser_count);
					if(Laser[laser].status == 0){
						Laser[laser].angle = anglee;
						Laser[laser].status = 1;
						click_time=glfwGetTime();
					}
					// createRectangle(laser,"Laser",red,0.15,0.04,-3.6,gun_translation,anglee);
					// laser_count++;
				}
			}
		}
		if(button == GLFW_MOUSE_BUTTON_RIGHT){
			if(m_flag3==0)
				m_click_x = mouse_x;
			m_flag3=1;
		}
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	zoom += yoffset;
	if(zoom>=4)
		zoom=3;
	if(zoom<0)
		zoom=0;
	if(pan>zoom)
		pan=zoom;
	if(pan<-zoom)
		pan=-zoom;
	Matrices.projection = glm::ortho(-4.0f+zoom-pan, 4.0f-zoom-pan, -4.0f+zoom, 4.0f-zoom, 0.1f, 500.0f);
}

/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
}

VAO *triangle, *circle, *rectangle;

// Creates the triangle object used in this sample code
// void createTriangle ()
// {
//   /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */
//
//   /* Define vertex array as used in glBegin (GL_TRIANGLES) */
//   static const GLfloat vertex_buffer_data [] = {
//     0, 1,0, // vertex 0
//     -1,-1,0, // vertex 1
//     1,-1,0, // vertex 2
//   };
//
//   static const GLfloat color_buffer_data [] = {
//     1,0,0, // color 0
//     0,1,0, // color 1
//     0,0,1, // color 2
//   };
//
//   // create3DObject creates and returns a handle to a VAO that can be used later
//   triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
// }

// Creates the rectangle object used in this sample code
void createRectangle (string comp, string body, Color Color, float l, float b, float x, float y,float angle)
{
	const GLfloat vertex_buffer_data [] = {
		-l,-b,0,
		l,-b,0,
		l,b,0,

		l,b,0,
		-l,b,0,
		-l,-b,0
	};

	const GLfloat color_buffer_data [] = {
	  Color.r,Color.g,Color.b, // color 1
	  Color.r,Color.g,Color.b, // color 2
	  Color.r,Color.g,Color.b, // color 3

	  Color.r,Color.g,Color.b, // color 3
	  Color.r,Color.g,Color.b, // color 4
	  Color.r,Color.g,Color.b  // color 1
	};

	rectangle = create3DObject(GL_TRIANGLES,6,vertex_buffer_data, color_buffer_data, GL_FILL);

	gameObjects gameObject = {};
	gameObject.name = comp;
	gameObject.object = rectangle;
	gameObject.x = x;
	gameObject.y = y;
	gameObject.len = 2*l;
	gameObject.breadth = 2*b;
	gameObject.speed = 0;
	gameObject.color =  Color;
	gameObject.angle = angle;

	if(body == "Basket")
		Basket[comp] = gameObject;
	else if(body == "Brick")
		Brick[comp] = gameObject;
	else if(body == "Gun")
		Gun[comp] = gameObject;
	else if(body == "Laser"){
		gameObject.speed = 0.11;
		Laser[comp] = gameObject;
	}
	else if(body == "Mirror")
		Mirror[comp] = gameObject;
	else if(body== "Line")
		Line[comp] = gameObject;

}

void createCircle (string comp, string body, Color Color, float radius, float x, float y,float parts)
{
	GLfloat vertex_buffer_data[360*9];
	GLfloat color_buffer_data[360*9];
	for(int i=0;i<360;i++){
		vertex_buffer_data[9*i]=0;
		vertex_buffer_data[9*i+1]=0;
		vertex_buffer_data[9*i+2]=0;
		vertex_buffer_data[9*i+3]=radius*cos(i*M_PI/180);
		vertex_buffer_data[9*i+4]=radius*sin(i*M_PI/180);
		vertex_buffer_data[9*i+5]=0;
		vertex_buffer_data[9*i+6]=radius*cos((i+1)*M_PI/180);
		vertex_buffer_data[9*i+7]=radius*sin((i+1)*M_PI/180);
		vertex_buffer_data[9*i+8]=0;
	}
	for (int i = 0; i<360*9; i+=3){
		color_buffer_data[i]=Color.r;
		color_buffer_data[i+1]=Color.g;
		color_buffer_data[i+2]=Color.b;
	}
	circle = create3DObject(GL_TRIANGLES, (360*3)*parts, vertex_buffer_data, color_buffer_data, GL_FILL);

	gameObjects gameObject = {};
	gameObject.name = comp;
	gameObject.object = circle;
	gameObject.x = x;
	gameObject.y = y;
	gameObject.radius = radius;
	gameObject.speed = 0;
	gameObject.color =  Color;
	if(body == "Gun")
		Gun[comp] = gameObject;
	else if (body == "Basket")
		Basket[comp] = gameObject;
}

void brickdraw ()
{
	Color black = {0,0,0};
	Color green = {0,1,0};
	Color red = {1,0,0};
	map<int,Color> colormap;
	colormap[0] = black;
	colormap[1] = red;
	colormap[2] = green;
	string brick = "brick" +  to_string (brick_cnt);
	float x = ((float)rand()/(float)RAND_MAX)*5-2.5;
	int clr = rand()%3;
	createRectangle(brick,"Brick",colormap[clr],0.08,0.18,x,4.0,0);
	brick_cnt++;
}

int intersect_point(Point p1,Point p2,Point p4,Point p5){
  float x0=p1.x, y0=p1.y,x1=p2.x, y1=p2.y;int i;
// Point p3;

    float s1_x, s1_y, s2_x, s2_y, x2=p4.x, y2=p4.y, x3=p5.x, y3=p5.y, q, p, r;

    s1_x = x1 - x0;  current_time = glfwGetTime(); // Time in seconds
  if ((current_time - last_update_time) >= 2.0) { // atleast 0.5s elapsed since last frame
	  brickdraw();
	  last_update_time = current_time;
  }
    s1_y = y1 - y0;
    s2_x = x3 - x2;
    s2_y = y3 - y2;

    r=s1_x*s2_y - s2_x*s1_y;
    if(r==0){
      // p3.va=0;
      return 0;
    }

    p = (s1_x*(y0-y2) - s1_y*(x0-x2))/(r*1.0f);
    q = (s2_x*(y0-y2) - s2_y*(x0-x2))/(r*1.0f);

    if (p>=0 && p<=1 && q>=0 && q<=1)
    {
        x_intersection = x0 + (q * s1_x);
        y_intersection = y0 + (q * s1_y);
        // p3.va=1;
        return 1;
    }
    // p3.va=0;
    return 0;
}

bool checkintersection (float x1 , float y1,float x2, float y2, float x3, float y3, float x4 , float y4)
{
  bool statement = ( (
                        ((y3-y1)*(x2-x1)-(y2-y1)*(x3-x1))*
                        ((y4-y1)*(x2-x1)-(y2-y1)*(x4-x1)) < 0
                     ) &&
                     (
                       ((y1-y3)*(x4-x3)-(y4-y3)*(x1-x3))*
                       ((y2-y3)*(x4-x3)-(y4-y3)*(x2-x3)) < 0
                     )
                   );
  if (statement)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool brick_coll_basket (string baskt, string brck)
{
	// cout << Basket[baskt].x << endl
	if(Brick[brck].y<=-2.4 && Brick[brck].y>=-2.5 ){
	if( Brick[brck].x >= Basket[baskt].x-(Basket[baskt].len/2 - Brick[brck].len/2) && Brick[brck].x <= Basket[baskt].x+(Basket[baskt].len/2 - Brick[brck].len/2))
		return true;}
	else
		return false;
}

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw ()
{
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;

  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // For each model you render, since the MVP will be different (at least the M part)
  //  Don't change unless you are sure!!
  glm::mat4 MVP;	// MVP = Projection * View * Model

  // Load identity to model matrix
  // Matrices.model = glm::mat4(1.0f);
  //
  // /* Render your scene */
  //
  // glm::mat4 translateTriangle = glm::translate (glm::vec3(-2.0f, 0.0f, 0.0f)); // glTranslatef
  // glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
  // glm::mat4 triangleTransform = translateTriangle * rotateTriangle;
  // Matrices.model *= triangleTransform;
  // MVP = VP * Matrices.model; // MVP = p * V * M
  //
  // //  Don't change unless you are sure!!
  // glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  //
  // // draw3DObject draws the VAO given to it using current MVP matrix
  // draw3DObject(triangle);

  // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
  // glPopMatrix ();
  current_time = glfwGetTime(); // Time in seconds
  if ((current_time - last_update_time) >= 2.0) { // atleast 0.5s elapsed since last frame
	  brickdraw();
	  last_update_time = current_time;
  }

  map<string, gameObjects>::iterator it,xy;
  for(it=Gun.begin();it!=Gun.end();it++) //Gun construction
  {
	  string current = it->first;
	  Gun[current].y = gun_translation;
	  glm::mat4 MVP;
	  Matrices.model = glm::mat4(1.0f);
	  glm::mat4 translateGun = glm::translate (glm::vec3(Gun[current].x, Gun[current].y, 0.0f));
	  glm::mat4 prodGun = translateGun;
	  if(current != "gun1"){
	  	glm::mat4 rotateGun = glm::rotate((float)(gun_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
		prodGun *= rotateGun;
	  }
	  Matrices.model *= prodGun;
	  MVP = VP * Matrices.model;
	  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	  draw3DObject(Gun[current].object);
  }

  for(it=Brick.begin();it!=Brick.end();it++)
  {
  	  string current = it->first;
	  if(Brick[current].flag == -1)
	  	continue;
	  if(Brick[current].y <= -4.18){
	  	points-=2;
		Brick[current].flag = -1;
	  }
	  glm::mat4 MVP;
	  Matrices.model = glm::mat4(1.0f);
	  glm::mat4 translateBrick = glm::translate (glm::vec3(Brick[current].x, Brick[current].y, 0.0f));
	  Matrices.model *= (translateBrick);
	  MVP = VP * Matrices.model;
	  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	  draw3DObject(Brick[current].object);

  }

  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateLine = glm::translate (glm::vec3(Line["line"].x, Line["line"].y, 0.0f));
  Matrices.model *= (translateLine);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(Line["line"].object);

  for(it=Basket.begin();it!=Basket.end();it++)
  {
	  string current = it->first;
	  Basket[current].x = laser_translation;
	  if(current == "redbasket" || current == "redcircle")
	  	Basket[current].x = red_basket_translation;
	  else if((current == "greenbasket") || current == "greencircle")
	  	Basket[current].x = green_basket_translation;
	  glm::mat4 MVP;
	  Matrices.model = glm::mat4(1.0f);
	  glm::mat4 translateBasket = glm::translate (glm::vec3(Basket[current].x, Basket[current].y, 0.0f));
	  glm::mat4 prodBasket = translateBasket;
	  if(current == "redcircle" || current == "greencircle"){
		glm::mat4 rotateBasket = glm::rotate((float)(70*M_PI/180.0f), glm::vec3(1,0,0));
		prodBasket *= rotateBasket;
	  }
	  Matrices.model *= (prodBasket);
	  MVP = VP * Matrices.model;
	  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	  draw3DObject(Basket[current].object);
  }

  for(it=Mirror.begin();it!=Mirror.end();it++)
  {
	  string current = it->first;
	  glm::mat4 MVP;
	  Matrices.model = glm::mat4(1.0f);
	  glm::mat4 translateMirror = glm::translate (glm::vec3(Mirror[current].x, Mirror[current].y, 0.0f));
	  glm::mat4 rotateMirror = glm::rotate((float)(Mirror[current].angle*M_PI/180.0f), glm::vec3(0,0,1));
	  Matrices.model *= (translateMirror*rotateMirror);
	  MVP = VP * Matrices.model;
	  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	  draw3DObject(Mirror[current].object);
  }

  for(it=Laser.begin();it!=Laser.end();it++)
  {
	 string current = it->first;
	//  cout << current << endl;
	 if (Laser[current].flag == -1)
	 	continue;
	 if (Laser[current].status == 0){
	 	Laser[current].status = laser_trans_status;
	 	Laser[current].y = gun_translation;
		click_time = glfwGetTime();
	}
	 glm::mat4 MVP;
	 Matrices.model = glm::mat4(1.0f);
	 glm::mat4 translateLaser = glm::translate (glm::vec3(Laser[current].x, Laser[current].y, -1.0f));
	 glm::mat4 prodLaser = translateLaser;
	 if(Laser[current].status == 0)
	 	 Laser[current].angle = gun_rotation;
	 glm::mat4 rotateLaser = glm::rotate ((float)(Laser[current].angle*M_PI/180.0f), glm::vec3(0,0,1));
	 prodLaser *= rotateLaser;
	 Matrices.model *= (prodLaser);
	 MVP = VP * Matrices.model;
	 glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
	 draw3DObject(Laser[current].object);
	 if(Laser[current].status == 1){
	 Laser[current].x += (Laser[current].speed)*cos((Laser[current].angle*M_PI/180.0f));
	 Laser[current].y += (Laser[current].speed)*sin((Laser[current].angle*M_PI/180.0f));
	}
  }
  laser_trans_status = 0;
  string laser = "laser" + to_string(laser_count);
  if(glfwGetTime()-click_time>=1 && Laser[laser].status==1)
  {
	//   cout << laser_count << ' ' << laser << endl;
	  Color red = {1,0,0};
	  laser = "laser" + to_string(laser_count+1);
	  createRectangle(laser,"Laser",red,0.15,0.04,-3.6,gun_translation,0);
	  laser_count++;
  }

  for(it=Laser.begin();it!=Laser.end();it++)
  {
	  string curr1 = it->first;
	  Point P1,P2,P3,P4;
	  int chk;
	  if(Laser[curr1].status == 0)
	  	continue;
	  for(xy=Mirror.begin();xy!=Mirror.end();xy++)
	  {
		  string curr2 = xy->first;
		  P1.x = Laser[curr1].x + (Laser[curr1].len/2)*cos((Laser[curr1].angle*M_PI/180.0f));
		  P1.y = Laser[curr1].y + (Laser[curr1].len/2)*sin((Laser[curr1].angle*M_PI/180.0f));
		  P2.x = Laser[curr1].x - (Laser[curr1].len/2)*cos((Laser[curr1].angle*M_PI/180.0f));
		  P2.y = Laser[curr1].y - (Laser[curr1].len/2)*sin((Laser[curr1].angle*M_PI/180.0f));
		  P3.x = Mirror[curr2].x + (Mirror[curr2].len/2)*cos((Mirror[curr2].angle*M_PI/180.0f));
		  P3.y = Mirror[curr2].y + (Mirror[curr2].len/2)*sin((Mirror[curr2].angle*M_PI/180.0f));
		  P4.x = Mirror[curr2].x - (Mirror[curr2].len/2)*cos((Mirror[curr2].angle*M_PI/180.0f));
		  P4.y = Mirror[curr2].y - (Mirror[curr2].len/2)*sin((Mirror[curr2].angle*M_PI/180.0f));
		  chk = intersect_point(P1,P2,P3,P4);
		  if(chk == 1)
		  {
			  Laser[curr1].x = x_intersection;
			  Laser[curr1].y = y_intersection;
			  Laser[curr1].angle = (2*Mirror[curr2].angle) - Laser[curr1].angle;
			  Laser[curr1].speed = 0.18;
		  }
	  }
  }

  bool check;
  for(it=Laser.begin();it!=Laser.end();it++)
  {
	  string curr1 = it->first;
	  if(Laser[curr1].status == 0 || Laser[curr1].flag == -1)
	  	continue;
	  float theta = (Laser[curr1].angle*M_PI/180.0f);
	  for(xy=Brick.begin();xy!=Brick.end();xy++)
	  {
		  string curr2 = xy->first;
		  if(Brick[curr2].flag == -1)
		  	continue;
		  check = checkintersection (Laser[curr1].x+(Laser[curr1].len/2)*cos(theta), Laser[curr1].y+(Laser[curr1].len/2)*sin(theta), Laser[curr1].x-(Laser[curr1].len/2)*cos(theta), Laser[curr1].y-(Laser[curr1].len/2)*sin(theta), Brick[curr2].x-Brick[curr2].len/2, Brick[curr2].y+Brick[curr2].breadth/2, Brick[curr2].x-Brick[curr2].len/2 , Brick[curr2].y-Brick[curr2].breadth/2);
		  if(check == true){
		  	Laser[curr1].flag = -1;
			Brick[curr2].flag = -1;
			if(Brick[curr2].color.r==0 && Brick[curr2].color.g==0 && Brick[curr2].color.b==0 ){
				points+=10;
			}
			else{
				points-=2;
				misfire++;
				if(misfire==5){
					cout << points << endl;
					exit(0);
				}
			}
		}
	  }
  }

  for(it=Basket.begin();it!=Basket.end();it++)
  {
	  string curr1 = it->first;
	  Color black = {0,0,0};
	//   cout << curr1 << endl;
	  for(xy=Brick.begin();xy!=Brick.end();xy++)
	  {
		  string curr2 = xy->first;
		  if(Brick[curr2].flag == -1)
			continue;
		  check = brick_coll_basket(curr1,curr2);
		  if(check == true){
			 	Brick[curr2].flag = -1;
				if(Brick[curr2].color.r==0 && Brick[curr2].color.g==0 && Brick[curr2].color.b==0 ){
					cout << points << endl;
					exit(0);
				}
				else if(Brick[curr2].color.r==1 && Brick[curr2].color.g==0 && Brick[curr2].color.b==0 && Basket[curr1].color.r==1 && Basket[curr1].color.g==0 && Basket[curr1].color.b==0)
					points+=5;
				else if(Brick[curr2].color.r==0 && Brick[curr2].color.g==1 && Brick[curr2].color.b==0 && Basket[curr1].color.r==0 && Basket[curr1].color.g==1 && Basket[curr1].color.b==0)
					points+=5;
				else                              //bricks collected in wrong baskets
					points-=2;
			}
	  }
  }
  //drawCircle
  // Matrices.model = glm::mat4(1.0f);
  // glm::mat4 translateCircle = glm::translate (glm::vec3(0.0f, 0.0f, 0.0f));
  // Matrices.model *= (translateCircle);
  // MVP = VP * Matrices.model;
  // glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  //
  // draw3DObject(circle);
  //drawGun
  // Matrices.model = glm::mat4(1.0f);
  //
  // glm::mat4 translateGun = glm::translate (glm::vec3(-3.8f, gun_translation, 0.0f));        // glTranslatef
  // glm::mat4 rotateGun = glm::rotate((float)(gun_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  // Matrices.model *= (translateGun * rotateGun);
  // MVP = VP * Matrices.model;
  // glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  //
  // // draw3DObject draws the VAO given to it using current MVP matrix
  // draw3DObject(gun[0]);
  //
  // Matrices.model = glm::mat4(1.0f);
  //
  // translateGun = glm::translate (glm::vec3(-3.4f, gun_translation, 0.0f));        // glTranslatef
  // rotateGun = glm::rotate((float)(gun_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  // Matrices.model *= (translateGun * rotateGun);
  // MVP = VP * Matrices.model;
  // glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  //
  // // draw3DObject draws the VAO given to it using current MVP matrix
  // draw3DObject(gun[1]);

  // Increment angles
  float increments = 1;

  //camera_rotation_angle++; // Simulating camera rotation
  // triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
  red_basket_translation = red_basket_translation + red_basket_trans_dir*red_basket_trans_status;
  green_basket_translation = green_basket_translation + green_basket_trans_dir*green_basket_trans_status;
  gun_translation = gun_translation + gun_trans_dir*gun_trans_status;
  gun_rotation = gun_rotation + gun_rot_dir*gun_rot_status;
  if(gun_rotation >= 60)
  	gun_rotation = 60;
  else if(gun_rotation <= -60)
  	gun_rotation = -60;
  if(gun_translation > 3.44f)
  	gun_translation = 3.44f;
  else if(gun_translation < -1.64f)
  	gun_translation = -1.64f;
  if(green_basket_translation>3.4f)
  	green_basket_translation = 3.4f;
  else if(green_basket_translation < -3.4f)
  	green_basket_translation = -3.4f;
  if(red_basket_translation > 3.4f)
  	red_basket_translation = 3.4f;
  else if(red_basket_translation < -3.4f)
  	red_basket_translation = -3.4f;
  if(m_flag3 && zoom>0){
		pan -= (m_click_x - mouse_x);
		m_click_x = mouse_x;
		if(pan>zoom)
			pan=zoom;
		if(pan<-zoom)
			pan=-zoom;
		Matrices.projection = glm::ortho(-4.0f+zoom-pan, 4.0f-zoom-pan, -4.0f+zoom, 4.0f-zoom, 0.1f, 500.0f);
   }

}
void brickdown()
{
	map<string,gameObjects>::iterator it;
	for(it=Brick.begin();it!=Brick.end();it++)
	{
		string current = it->first;
		Brick[current].y = Brick[current].y - brick_speed;
	}
}
/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
//        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
        glfwTerminate();
//        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks
	glfwSetCursorPosCallback(window, cursor_position);
	glfwSetScrollCallback(window, scroll_callback);

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
	// Create the models
	// createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
	Color blue = {0,0,1};
	Color green = {0,1,0};
	Color red = {1,0,0};
	Color grey = {168.0/255.0,168.0/255.0,168.0/255.0};
	Color gold = {218.0/255.0,165.0/255.0,32.0/255.0};
	Color yellow = {1,1,0};
	Color skyblue = {132/255.0,217/255.0,245/255.0};
	Color black = {0,0,0};
	Color white = {255/255.0,255/255.0,255/255.0};

	map<int,Color> colormap;
	colormap[0] = black;
	colormap[1] = red;
	colormap[2] = green;
	// createRectangle("gun1","Gun",black,0.35,0.2,-3.8,0,0);
	createCircle("gun1","Gun",black,0.56,-4.0,0,1);
	createRectangle("gun2","Gun",black,0.23,0.10,-3.4,0,0);
	createCircle("gun3","Gun",red,0.09,-3.7f,0.0f,1);
	createRectangle("laser1","Laser",red,0.15,0.04,-3.6,0.0,0);
	createRectangle("redbasket","Basket",red,0.6,0.5,-3.0,-3.0,0);
	createCircle("redcircle","Basket",grey,0.6,0.0,-2.5,1);
	createRectangle("greenbasket","Basket",green,0.6,0.5,3.0,-3.0,0);
	createCircle("greencircle","Basket",grey,0.6,0.0,-2.5,1);
	createRectangle("mirror1","Mirror",white,0.45,0.04,2.8,2.5,120);
	createRectangle("mirror2","Mirror",white,0.45,0.04,-1.4,1.4,70);
	createRectangle("mirror3","Mirror",white,0.45,0.04,0.9,-1.4,60);
	createRectangle("line","Line",black,7.0,0.01,0.0,-2.22,0);
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


	reshapeWindow (window, width, height);

    // Background color of the scene
	glClearColor (0, 0.3f, 0.3f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

    // cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    // cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    // cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    // cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
	int width = 600;
	int height = 600;

    GLFWwindow* window = initGLFW(width, height);

	initGL (window, width, height);

    double last_update_time = glfwGetTime(), current_time;

    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {

        // OpenGL Draw commands
        draw();

        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 0.05) { // atleast 0.5s elapsed since last frame
            brickdown();
            last_update_time = current_time;
        }
    }
    glfwTerminate();
//    exit(EXIT_SUCCESS);
}
