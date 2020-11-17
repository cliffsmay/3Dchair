/*
 * 3DChair.cpp
 *
 *  Created on: Aug 11, 2020
 *      Author: Clifford Smay
 *      Course: CS-330
 */

/* Header Inclusions */
#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>

// GLM Math Header Inclusions
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// SOIL Image Loader Inclusion
#include "SOIL2/SOIL2.h"

// Standard namespace
using namespace std;

// Macro for window title
#define WINDOW_TITLE "3D Chair"

//Vertex and Fragment Shader Source Macro
#ifndef GLSL
#define GLSL (Version, Source) "#version " #Version "\n" #Source
#endif


/* Variable Declarations */

// Window Size
GLint windowWidth = 800;
GLint windowHeight = 600;

// Shader Program ID
GLint chairShaderProgram;
GLint keyLightShaderProgram;
GLint fillLightShaderProgram;

// Vertex Array & Buffer Objects
GLuint chairVBO;
GLuint lightVBO;
GLuint chairVAO;
GLuint keyLightVAO;
GLuint fillLightVAO;
GLuint texture;
GLfloat degrees = glm::radians(-45.0f);

//Subject position and scale
glm::vec3 chairPosition(0.0f, 0.0f, 0.0f);
glm::vec3 chairScale(1.0f);

// Light color
glm::vec3 keyLightColor(0.0f, 1.0f, 0.0f);		// Green Light
glm::vec3 fillLightColor(1.0f, 1.0f, 1.0f);		// White Light

// Movement speed per frame
GLfloat cameraSpeed = 0.0055f;

// Stores key pressed
GLchar keymod;

GLfloat checkMotion;
GLfloat checkZoom;

// Locks mouse cursor at the center of the screen
GLfloat lastMouseX = 400, lastMouseY = 300;
GLfloat scale_by_x = 400, scale_by_y = 300, scale_by_z = 300;
// Mouse offset, yaw, and pitch variables
GLfloat mouseXOffset, mouseYOffset, yaw = 0.0f, pitch = 0.0f;
// Used for mouse / camera rotation sensitivity
GLfloat sensitivity = 0.005f;


// GLOBAL VECTOR DECLARATIONS
// Initial camera position
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);
// Temporary y unit vector
glm::vec3 CameraUpY = glm::vec3(0.0f, 1.0f, 0.0f);
// Temporary z unit vector
glm::vec3 CameraForwardZ = glm::vec3(0.0f, 0.0f, -1.0f);
// Temporary z unit vector for mouse
glm::vec3 front;

// Light position and scale
glm::vec3 keyLightPosition(-1.0f, 1.0f, -6.0f);
glm::vec3 fillLightPosition(3.0f, 0.0f, 0.0f);
glm::vec3 lightScale(0.3f);

//Camera rotation
float cameraRotation = glm::radians(-25.0f);

/* USER-DEFINED FUNCTION DECLARATIONS */
void CheckStatus(GLuint, bool);
void AttachShader(GLuint, GLenum, const char*);
void UResizeWindow(int, int);
void URenderGraphics(void);
void UKeyboard(unsigned char key, int x, int y);
void UCreateShader(void);
void UCreateBuffers(void);
void UGenerateTexture(void);
void UMouseMove(int x, int y);
void onMotion(int curr_x, int curr_y);
void OnMouseClicks(int button, int state, int x, int y);


/* CHAIR VERTEX SHADER SOURCE CODE
 *  Take the position vertices, normals & texture coordinates from the buffer
 *  Create the uniform global variables
 *  Send the Normal calculation, Fragment Position
 *  and mobile Texture Coordinates to the Fragment Shader
 */
const char * chairVertexShaderSource =

		 "#version 330\n"
		 "layout(location=0) in vec3 position;\n"
		 "layout(location=1) in vec3 normal; \n"
		 "layout(location=2) in vec2 textureCoordinate;\n"

		 "out vec3 Normal;\n"
		 "out vec3 FragmentPos;\n"
	     "out vec2 mobileTextureCoordinate;\n"

		 "uniform mat4 model;\n"
		 "uniform mat4 view;\n"
		 "uniform mat4 projection;\n"

		 "void main() \n"
		 "{ \n"
				   "gl_Position = projection * view * model * vec4(position, 1.0f);\n"
				   "FragmentPos = vec3(model * vec4(position, 1.0f));\n"
			       "Normal = mat3(transpose(inverse(model))) * normal;\n"
				   "mobileTextureCoordinate = vec2(textureCoordinate.x, 1.0f - textureCoordinate.y);\n"
	"} \n";


/* CHAIR FRAGMENT SHADER SOURCE CODE
 * Takes the Normal matrix, Fragment Position & mobile texture coordinates from the Vertex Shader
 * Sends the pyramid Color to the GPU
 * Creates the uniform global values for determining lighting and texture
 * Uses the Phong method to determine lighting by calculating:
 * Ambient Lighting, Diffuse Lighting & Specular Component
 * which is then multiplied with the texture on the pyramid
 */
const char* chairFragmentShaderSource =
		 "#version 330 \n"
		 "in vec3 Normal;\n"
		 "in vec3 FragmentPos;\n"
		 "in vec2 mobileTextureCoordinate;\n"

		 "out vec4 chairColor;\n"

		 "uniform vec3 keyLightColor;\n"
		 "uniform vec3 fillLightColor;\n"
		 "uniform vec3 keyLightPos;\n"
		 "uniform vec3 fillLightPos;\n"
		 "uniform vec3 viewPosition;\n"
		 "uniform sampler2D uTexture;\n"

		 "void main() \n"
		 "{ \n"

		 		  "float ambientStrength = 0.1f;\n"
				  "vec3 keyAmbient = ambientStrength * keyLightColor;\n"
		 		  "vec3 fillAmbient = ambientStrength * fillLightColor;\n"

		 		  "vec3 norm = normalize(Normal);\n"
		 		  "vec3 keyLightDirection = normalize(keyLightPos - FragmentPos);\n"
		 		  "vec3 fillLightDirection = normalize(fillLightPos - FragmentPos);\n"
		 		  "float keyImpact = max(dot(norm, keyLightDirection), 0.0);\n"
		 		  "float fillImpact = max(dot(norm, fillLightDirection), 0.0);\n"
		 		  "vec3 keyDiffuse = keyImpact * keyLightColor;\n"
				  "vec3 fillDiffuse = fillImpact * fillLightColor;\n"

		 		  "float keySpecularIntensity = 1.0f;\n"
		 		  "float fillSpecularIntensity = 0.1f;\n"
		 		  "float highlightSize = 16.0f;\n"
		 		  "vec3 viewDir = normalize(viewPosition - FragmentPos);\n"
		 		  "vec3 keyReflectDir = reflect(-keyLightDirection, norm);\n"
		 		  "vec3 fillReflectDir = reflect(-fillLightDirection, norm);\n"
		 		  "float keySpecularComponent = pow(max(dot(viewDir, keyReflectDir), 0.0), highlightSize);\n"
		 		  "float fillSpecularComponent = pow(max(dot(viewDir, fillReflectDir), 0.0), highlightSize);\n"
				  "vec3 keySpecular = keySpecularIntensity * keySpecularComponent * keyLightColor;\n"
				  "vec3 fillSpecular = fillSpecularIntensity * fillSpecularComponent * fillLightColor;\n"

		 		  "vec3 objectColor = texture(uTexture, mobileTextureCoordinate).xyz;\n"
		 		  "vec3 keyPhong = (keyAmbient + keyDiffuse + keySpecular) * objectColor;\n"
		 		  "vec3 fillPhong = (fillAmbient + fillDiffuse + fillSpecular) * objectColor;\n"
		 		  "vec3 phong = keyPhong + fillPhong;\n"
		 		  "chairColor = vec4(phong, 1.0f);\n"

		"} \n";

/* KEY LIGHT VERTEX SHADER SOURCE CODE
 * Takes the position vertices from the buffer
 * Creates the uniform global variables
 */
const char * keyLightVertexShaderSource =
		 "#version 330 \n"
		 "layout(location=0) in vec3 position;\n"

		 "uniform mat4 model;\n"
		 "uniform mat4 view;\n"
		 "uniform mat4 projection;\n"

		 "void main() \n"
		 "{ \n"

                  "gl_Position = projection * view * model * vec4(position, 1.0f);\n"

		"} \n";

/* KEY LIGHT FRAGMENT SHADER SOURCE CODE
 * Sends the color to the GPU
 */
const char* keyLightFragmentShaderSource =
		  "#version 330 \n"
		  "out vec4 color;\n"

		  "void main() \n"
		  "{ \n"
		           "color = vec4(0.8f, 1.0f, 0.8f, 1.0f);\n"
		"} \n";

/* FILL LIGHT VERTEX SHADER SOURCE CODE
 * Takes the position vertices from the buffer
 * Creates the uniform global variables
 */
const char * fillLightVertexShaderSource =
		   "#version 330 \n"
		   "layout(location=0) in vec3 position;\n"

		   "uniform mat4 model;\n"
		   "uniform mat4 view;\n"
		   "uniform mat4 projection;\n"

		   "void main() \n"
		   "{ \n"
					 "gl_Position = projection * view * model * vec4(position, 1.0f);\n"

		"} \n";

/* FILL LIGHT FRAGMENT SHADER SOURCE CODE
 * Sends the color to the gpu
 */
const char* fillLightFragmentShaderSource =
		 "#version 330 \n"

		 "out vec4 color;\n"

		 "void main() \n"
		 "{ \n"
				   "color = vec4(1.0f);\n"
		"} \n";



// MAIN PROGRAM
int main(int argc, char* argv[])
{
	//Initializes the OpenGL program
	glutInit(&argc, argv);
	glutInitContextVersion(3,3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	// Memory buffer setup for display
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	// Sets the window size
	glutInitWindowSize(windowWidth, windowHeight);
	// Creates window and provides title (from macro above)
	glutCreateWindow(WINDOW_TITLE);
	// Reshape function if user changes window size
	glutReshapeFunc(UResizeWindow);

	// Verify GLEW initialization
	glewExperimental = GL_TRUE;
			if (glewInit() != GLEW_OK)
			{
				std::cout << "Failed to initialize GLEW" << std::endl;
				return -1;
			}

	// Calls function to Create Shaders
	UCreateShader();

	// Calls function to Create Buffers
	UCreateBuffers();

	// Calls function to Generate Textures
	UGenerateTexture();

	// Set background color
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// Display Graphics
	glutDisplayFunc(URenderGraphics);

	// Detects key press
	glutKeyboardFunc(UKeyboard);

	// Detects mouse click
	glutMouseFunc(OnMouseClicks);
	// Detects mouse movement
	glutPassiveMotionFunc(UMouseMove);
	// Detects mouse press and movement
	glutMotionFunc(onMotion);

	int glutGetModifiers(void);

	// Start OpenGL Loop
	glutMainLoop();

	// Destroys Buffer objects once used
	glDeleteVertexArrays(1, &chairVAO);
	glDeleteVertexArrays(1, &keyLightVAO);
	glDeleteVertexArrays(1, &fillLightVAO);
	glDeleteBuffers(1, &chairVBO);
	glDeleteBuffers(1, &lightVBO);

	return 0;

}

void CheckStatus(GLuint obj, bool isShader) {
		GLint status = GL_FALSE, log[1 << 11] = {0};
		(isShader ? glGetShaderiv : glGetProgramiv)(obj, isShader ?
				  GL_COMPILE_STATUS : GL_LINK_STATUS, &status);
		(isShader ? glGetShaderInfoLog : glGetProgramInfoLog)(obj, sizeof(log), NULL,
				  (GLchar*)log);
		if(status == GL_TRUE) {
				   return;
		}

		std::cerr << (GLchar*)log << "\n";
		std::exit(EXIT_FAILURE);
}

// Function to minimize code to create the shaders
void AttachShader(GLuint program, GLenum type, const char*src) {
		  // Create Shader and return shader ID
		  GLuint shader = glCreateShader(type);
		  // Attach Shader to Source Code
		  glShaderSource(shader, 1, &src, NULL);
		  // Compile Shader
		  glCompileShader(shader);
		  // Verify Shader Compiles Correctly
		  CheckStatus(shader, true);
		  // Attach shader to shader program
		  glAttachShader(program, shader);
		  // Delete already used shader
		  glDeleteShader(shader);
}

/* Resizes the window */
void UResizeWindow(int w, int h)
{
	windowWidth = w;
	windowHeight = h;
	// Sets the new window with the new size
	glViewport(0, 0, windowWidth, windowHeight);
}


/* Render graphics */
void URenderGraphics(void)
{

	// Enable z-depth
	glEnable(GL_DEPTH_TEST);

	// Clears the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Location Variables
	GLint modelLoc;
	GLint viewLoc;
	GLint projLoc;
	GLint uTextureLoc;
	GLint keyLightColorLoc;
	GLint fillLightColorLoc;
	GLint keyLightPositionLoc;
	GLint fillLightPositionLoc;
	GLint viewPositionLoc;

	glm::mat4 model(1.0f);
	glm::mat4 view(1.0f);
	glm::mat4 projection;

	/****** USE THE CHAIR SHADER AND ACTIVATE CHAIR VAO FOR RENDERING AND TRANSFORMING ******/
	glUseProgram(chairShaderProgram);
	glBindVertexArray(chairVAO);

	// Transforms the chair
	model = glm::translate(model, chairPosition);
	model = glm::scale(model, chairScale);

	/* Create Movement Logic */
	//Replaces camera forward vector with Radians normalized as a unit vector
	CameraForwardZ = front;

	// Transforms the camera
	view = glm::translate(view, cameraPosition);
	view = glm::rotate(view, cameraRotation, glm::vec3(0.0f, 1.0f, 0.0f));
	view = glm::lookAt(CameraForwardZ, cameraPosition, CameraUpY);

	// Creates a perspective projection
	projection = glm::perspective(45.0f, (GLfloat)windowWidth / (GLfloat)windowHeight, 0.1f, 100.0f);

	// Creates an Orthographic projection
	//projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);

	// Reference matrix uniforms from the chair Shader Program
	modelLoc = glGetUniformLocation(chairShaderProgram, "model");
	viewLoc = glGetUniformLocation(chairShaderProgram, "view");
	projLoc = glGetUniformLocation(chairShaderProgram, "projection");

	// Pass matrix data to the chair Shader Program's matrix uniforms
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// Reference matrix uniforms from the Chair Shader program for:
	// chair color, light color, light position, and camera position
	uTextureLoc = glGetUniformLocation(chairShaderProgram, "uTexture");
	keyLightColorLoc = glGetUniformLocation(chairShaderProgram, "keyLightColor");
	fillLightColorLoc = glGetUniformLocation(chairShaderProgram, "fillLightColor");
	keyLightPositionLoc = glGetUniformLocation(chairShaderProgram, "keyLightPos");
	fillLightPositionLoc = glGetUniformLocation(chairShaderProgram, "fillLightPos");
	viewPositionLoc = glGetUniformLocation(chairShaderProgram, "viewPosition");

	//Pass color, light, and camera data to the Pyramid Shader program's corresponding uniforms
	glUniform1i(uTextureLoc, 0);
	glUniform3f(keyLightColorLoc, keyLightColor.r, keyLightColor.g, keyLightColor.b);
	glUniform3f(fillLightColorLoc, fillLightColor.r, fillLightColor.g, fillLightColor.b);
	glUniform3f(keyLightPositionLoc, keyLightPosition.x, keyLightPosition.y, keyLightPosition.z);
	glUniform3f(fillLightPositionLoc, fillLightPosition.x, fillLightPosition.y, fillLightPosition.z);
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

	//Provide texture to the chair
	glBindTexture(GL_TEXTURE_2D, texture);

	// Draw the chair
	glDrawArrays(GL_TRIANGLES, 0, 162);

	// Deactivate the chair Vertex Array Object
	glBindVertexArray(0);

/****** USE THE KEY LIGHT SHADER AND ACTIVATE LAMP VERTEX ARRAY OBJECT FOR RENDERING AND TRANSFORMING ******/
	glUseProgram(keyLightShaderProgram);
	glBindVertexArray(keyLightVAO);

	// Transform the smaller chair used as a visual que for the light source
	model = glm::translate(model, keyLightPosition);
	model = glm::scale(model, lightScale);

	// Reference matrix uniforms from the lamp Shader program
	modelLoc = glGetUniformLocation(keyLightShaderProgram, "model");
	viewLoc = glGetUniformLocation(keyLightShaderProgram, "view");
	projLoc = glGetUniformLocation(keyLightShaderProgram, "projection");

	// Pass matrix data to the lamp shader program's matrix uniforms
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// Draw the smaller LAMP cube
	glDrawArrays(GL_TRIANGLES, 0, 36);

	// Deactivate the lamp Vertex Array Object
	glBindVertexArray(0);

/****** USE THE FILL LIGHT SHADER AND ACTIVATE LAMP VERTEX ARRAY OBJECT FOR RENDERING AND TRANSFORMING ******/
	glUseProgram(fillLightShaderProgram);
	glBindVertexArray(fillLightVAO);

	// Transform the smaller chair used as a visual que for the light source
	model = glm::translate(model, keyLightPosition);
	model = glm::scale(model, lightScale);

	// Reference matrix uniforms from the lamp Shader program
	modelLoc = glGetUniformLocation(keyLightShaderProgram, "model");
	viewLoc = glGetUniformLocation(keyLightShaderProgram, "view");
	projLoc = glGetUniformLocation(keyLightShaderProgram, "projection");

	// Pass matrix data to the lamp shader program's matrix uniforms
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// Draw the smaller LAMP cube
	glDrawArrays(GL_TRIANGLES, 0, 36);

	// Deactivate the lamp Vertex Array Object
	glBindVertexArray(0);

	// marks current window to be redisplayed
	glutPostRedisplay();
	// Flips the back buffer with the front buffer every frame. Similar to GL Flush
	glutSwapBuffers();

}

// Function that creates shaders
void UCreateShader(void)
{

	// CHAIR SHADERS
	// Creates the shader program and returns an ID for the chair
	chairShaderProgram = glCreateProgram();
	// Attach the chair's vertex shader to the shader program
	AttachShader(chairShaderProgram, GL_VERTEX_SHADER, chairVertexShaderSource);
	// Attach the chair's fragment shader to the shader program
	AttachShader(chairShaderProgram, GL_FRAGMENT_SHADER, chairFragmentShaderSource);
	// Link vertex and fragment shaders to the chair's shader program
	glLinkProgram(chairShaderProgram);
	CheckStatus(chairShaderProgram, false);

	// KEY LAMP SHADERS
	// Creates the shader program and returns an ID for the lamp
	keyLightShaderProgram = glCreateProgram();
	// Attach the lamp's vertex shader to the shader program
	AttachShader(keyLightShaderProgram, GL_VERTEX_SHADER, keyLightVertexShaderSource);
	// Attach the lamp's fragment shader to the shader program
	AttachShader(keyLightShaderProgram, GL_FRAGMENT_SHADER, keyLightFragmentShaderSource);
	// Link vertex and fragment shaders to the lamp's shader program
	glLinkProgram(keyLightShaderProgram);
	CheckStatus(keyLightShaderProgram, false);

	// FILL LAMP SHADERS
	// Creates the shader program and returns an ID for the lamp
	fillLightShaderProgram = glCreateProgram();
	// Attach the lamp's vertex shader to the shader program
	AttachShader(fillLightShaderProgram, GL_VERTEX_SHADER, fillLightVertexShaderSource);
	// Attach the lamp's fragment shader to the shader program
	AttachShader(fillLightShaderProgram, GL_FRAGMENT_SHADER, fillLightFragmentShaderSource);
	// Link vertex and fragment shaders to the lamp's shader program
	glLinkProgram(fillLightShaderProgram);
	CheckStatus(fillLightShaderProgram, false);

}


/* Implements the UKeyboard function */
void UKeyboard(unsigned char key, GLint x, GLint y)
{
	switch(key){

		case GLUT_ACTIVE_ALT:
			keymod = glutGetModifiers();
			cout<<"You pressed ALT!"<<endl;
			break;

		default:
			cout<<"Press a key!"<<endl;
	}
}

/* CREATES THE BUFFER AND ARRAY OBJECTS */
void UCreateBuffers()
{
	GLfloat chairVertices[] = {
							//Positions			   // Normals			// Texture Coordinates

							// CHAIR SEAT
							// Front
						   -2.0f, -0.2f,  2.0f,	   0.0f, 0.0f, 1.0f, 	0.0f, 0.0f,
							2.0f, -0.2f,  2.0f,    0.0f, 0.0f, 1.0f,	1.0f, 0.0f,
							2.0f,  0.2f,  2.0f,    0.0f, 0.0f, 1.0f,	1.0f, 1.0f,
						    2.0f,  0.2f,  2.0f,    0.0f, 0.0f, 1.0f,	1.0f, 1.0f,
						   -2.0f,  0.2f,  2.0f,    0.0f, 0.0f, 1.0f,	0.0f, 1.0f,
						   -2.0f, -0.2f,  2.0f,    0.0f, 0.0f, 1.0f,	0.0f, 0.0f,

						   // Right
						    2.0f, -0.2f, -2.0f,	   1.0f, 0.0f, 0.0f,	0.0f, 0.0f,
						    2.0f,  0.2f, -2.0f,    1.0f, 0.0f, 0.0f,  	1.0f, 0.0f,
							2.0f,  0.2f,  2.0f,    1.0f, 0.0f, 0.0f,  	1.0f, 1.0f,
						    2.0f,  0.2f,  2.0f,    1.0f, 0.0f, 0.0f,  	1.0f, 1.0f,
						    2.0f, -0.2f,  2.0f,    1.0f, 0.0f, 0.0f,  	0.0f, 1.0f,
						    2.0f, -0.2f, -2.0f,    1.0f, 0.0f, 0.0f,  	0.0f, 0.0f,

						   // Back
						   -2.0f, -0.2f, -2.0f,	   0.0f, 0.0f,-1.0f, 	0.0f, 0.0f,
						   -2.0f,  0.2f, -2.0f,    0.0f, 0.0f,-1.0f,    1.0f, 0.0f,
							2.0f,  0.2f, -2.0f,    0.0f, 0.0f,-1.0f,    1.0f, 1.0f,
						    2.0f,  0.2f, -2.0f,    0.0f, 0.0f,-1.0f,    1.0f, 1.0f,
						    2.0f, -0.2f, -2.0f,    0.0f, 0.0f,-1.0f,    0.0f, 1.0f,
						   -2.0f, -0.2f, -2.0f,    0.0f, 0.0f,-1.0f,    0.0f, 0.0f,

						   // Left
						   -2.0f, -0.2f, -2.0f,   -1.0f, 0.0f, 0.0f, 	0.0f, 0.0f,
						   -2.0f, -0.2f,  2.0f,   -1.0f, 0.0f, 0.0f,    1.0f, 0.0f,
						   -2.0f,  0.2f,  2.0f,   -1.0f, 0.0f, 0.0f,    1.0f, 1.0f,
						   -2.0f,  0.2f,  2.0f,   -1.0f, 0.0f, 0.0f,    1.0f, 1.0f,
						   -2.0f,  0.2f, -2.0f,   -1.0f, 0.0f, 0.0f,    0.0f, 1.0f,
						   -2.0f, -0.2f, -2.0f,   -1.0f, 0.0f, 0.0f,    0.0f, 0.0f,

						   // Top
						    2.0f,  0.2f,  2.0f,	   0.0f, 1.0f, 0.0f,  	0.0f, 0.0f,
						   -2.0f,  0.2f,  2.0f,    0.0f, 1.0f, 0.0f,    1.0f, 0.0f,
						   -2.0f,  0.2f, -2.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f,
						   -2.0f,  0.2f, -2.0f,    0.0f, 1.0f, 0.0f,    1.0f, 1.0f,
						    2.0f,  0.2f, -2.0f,    0.0f, 1.0f, 0.0f,    0.0f, 1.0f,
						    2.0f,  0.2f,  2.0f,    0.0f, 1.0f, 0.0f,    0.0f, 0.0f,

						   // Bottom
							2.0f, -0.2f,  2.0f,    0.0f, -1.0f, 0.0f, 	0.0f, 0.0f,
						   -2.0f, -0.2f,  2.0f,    0.0f, -1.0f, 0.0f,	1.0f, 0.0f,
						   -2.0f, -0.2f, -2.0f,    0.0f, -1.0f, 0.0f,	1.0f, 1.0f,
						   -2.0f, -0.2f, -2.0f,    0.0f, -1.0f, 0.0f,	1.0f, 1.0f,
							2.0f, -0.2f, -2.0f,    0.0f, -1.0f, 0.0f,	0.0f, 1.0f,
							2.0f, -0.2f,  2.0f,    0.0f, -1.0f, 0.0f,	0.0f, 0.0f,

							// CHAIR BACK
							// Front
						   -1.8f,  0.2f, -1.8f,    0.0f, 0.0f,-1.0f, 	0.0f, 0.0f,
							1.8f,  0.2f, -1.8f,    0.0f, 0.0f,-1.0f,	1.0f, 0.0f,
							1.8f,  3.5f, -1.8f,    0.0f, 0.0f,-1.0f,	1.0f, 1.0f,
							1.8f,  3.5f, -1.8f,    0.0f, 0.0f,-1.0f,	1.0f, 1.0f,
						   -1.8f,  3.5f, -1.8f,    0.0f, 0.0f,-1.0f,	0.0f, 1.0f,
						   -1.8f,  0.2f, -1.8f,    0.0f, 0.0f,-1.0f,	0.0f, 0.0f,

							// Back
						   -1.8f,  0.2f, -2.0f,    0.0f, 0.0f,-1.0f,	0.0f, 0.0f,
							1.8f,  0.2f, -2.0f,    0.0f, 0.0f,-1.0f,	1.0f, 0.0f,
							1.8f,  3.5f, -2.0f,    0.0f, 0.0f,-1.0f,	1.0f, 1.0f,
							1.8f,  3.5f, -2.0f,    0.0f, 0.0f,-1.0f,	1.0f, 1.0f,
						   -1.8f,  3.5f, -2.0f,    0.0f, 0.0f,-1.0f,	0.0f, 1.0f,
						   -1.8f,  0.2f, -2.0f,    0.0f, 0.0f,-1.0f,	0.0f, 0.0f,

							// Left
						   -1.8f,  0.2f, -2.0f,   -1.0f, 0.0f, 0.0f,	0.0f, 0.0f,
						   -1.8f,  3.5f, -2.0f,   -1.0f, 0.0f, 0.0f,	1.0f, 0.0f,
						   -1.8f,  3.5f, -1.8f,   -1.0f, 0.0f, 0.0f,	1.0f, 1.0f,
						   -1.8f,  3.5f, -1.8f,   -1.0f, 0.0f, 0.0f,	1.0f, 1.0f,
						   -1.8f,  0.2f, -1.8f,   -1.0f, 0.0f, 0.0f,	0.0f, 1.0f,
						   -1.8f,  0.2f, -2.0f,   -1.0f, 0.0f, 0.0f,	0.0f, 0.0f,

							// Right
						    1.8f,  0.2f, -2.0f,    1.0f, 0.0f, 0.0f, 	0.0f, 0.0f,
						    1.8f,  3.5f, -2.0f,    1.0f, 0.0f, 0.0f,	1.0f, 0.0f,
						    1.8f,  3.5f, -1.8f,    1.0f, 0.0f, 0.0f,	1.0f, 1.0f,
						    1.8f,  3.5f, -1.8f,    1.0f, 0.0f, 0.0f,	1.0f, 1.0f,
						    1.8f,  0.2f, -1.8f,    1.0f, 0.0f, 0.0f,	0.0f, 1.0f,
						    1.8f,  0.2f, -2.0f,    1.0f, 0.0f, 0.0f,	0.0f, 0.0f,
							// Top
						   -1.8f,  3.5f, -2.0f,    0.0f, 1.0f, 0.0f,	0.0f, 0.0f,
						   -1.8f,  3.5f, -1.8f,    0.0f, 1.0f, 0.0f,	1.0f, 0.0f,
						    1.8f,  3.5f, -1.8f,    0.0f, 1.0f, 0.0f,	1.0f, 1.0f,
						    1.8f,  3.5f, -1.8f,    0.0f, 1.0f, 0.0f,	1.0f, 1.0f,
						    1.8f,  3.5f, -2.0f,    0.0f, 1.0f, 0.0f,	0.0f, 1.0f,
						    1.8f,  3.5f, -2.0f,    0.0f, 1.0f, 0.0f,	0.0f, 0.0f,

							// CHAIR RIGHT FRONT LEG
							// Front
						    1.8f, -0.2f,  1.6f,	   0.0f, 0.0f, 1.0f,  	0.0f, 0.0f,
							1.4f, -0.2f,  1.6f,    0.0f, 0.0f, 1.0f,    1.0f, 0.0f,
							1.4f, -3.0f,  1.6f,    0.0f, 0.0f, 1.0f,    1.0f, 1.0f,
							1.4f, -3.0f,  1.6f,    0.0f, 0.0f, 1.0f,    1.0f, 1.0f,
						    1.8f, -3.0f,  1.6f,    0.0f, 0.0f, 1.0f,  	0.0f, 1.0f,
						    1.8f, -0.2f,  1.6f,    0.0f, 0.0f, 1.0f,  	0.0f, 0.0f,

							// Back
						    1.8f, -0.2f,  1.2f,    0.0f, 0.0f, -1.0f,   0.0f, 0.0f,
							1.4f, -0.2f,  1.2f,    0.0f, 0.0f, -1.0f,   1.0f, 0.0f,
							1.4f, -3.0f,  1.2f,    0.0f, 0.0f, -1.0f,   1.0f, 1.0f,
							1.4f, -3.0f,  1.2f,    0.0f, 0.0f, -1.0f,   1.0f, 1.0f,
						    1.8f, -3.0f,  1.2f,    0.0f, 0.0f, -1.0f,   0.0f, 1.0f,
						    1.8f, -0.2f,  1.2f,    0.0f, 0.0f, -1.0f,   0.0f, 0.0f,

							// Right
						    1.8f, -0.2f,  1.6f,    1.0f, 0.0f,  0.0f,   0.0f, 0.0f,
							1.8f, -0.2f,  1.2f,    1.0f, 0.0f,  0.0f,   1.0f, 0.0f,
							1.8f, -3.0f,  1.2f,    1.0f, 0.0f,  0.0f,   1.0f, 1.0f,
							1.8f, -3.0f,  1.2f,    1.0f, 0.0f,  0.0f,   1.0f, 1.0f,
						    1.8f, -3.0f,  1.6f,    1.0f, 0.0f,  0.0f,   0.0f, 1.0f,
						    1.8f, -0.2f,  1.6f,    1.0f, 0.0f,  0.0f,   0.0f, 0.0f,

							// Left
						    1.4f, -0.2f,  1.6f,   -1.0f, 0.0f,  0.0f,   0.0f, 0.0f,
							1.4f, -0.2f,  1.2f,   -1.0f, 0.0f,  0.0f,   1.0f, 0.0f,
							1.4f, -3.0f,  1.2f,   -1.0f, 0.0f,  0.0f,   1.0f, 1.0f,
							1.4f, -3.0f,  1.2f,   -1.0f, 0.0f,  0.0f,   1.0f, 1.0f,
						    1.4f, -3.0f,  1.6f,   -1.0f, 0.0f,  0.0f,   0.0f, 1.0f,
						    1.4f, -0.2f,  1.6f,   -1.0f, 0.0f,  0.0f,   0.0f, 0.0f,

							// CHAIR LEFT FRONT LEG
							// Front
						   -1.8f, -0.2f,  1.6f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f,
						   -1.4f, -0.2f,  1.6f,    0.0f, 0.0f, 1.0f,    1.0f, 0.0f,
						   -1.4f, -3.0f,  1.6f,    0.0f, 0.0f, 1.0f,    1.0f, 1.0f,
						   -1.4f, -3.0f,  1.6f,    0.0f, 0.0f, 1.0f,    1.0f, 1.0f,
						   -1.8f, -3.0f,  1.6f,    0.0f, 0.0f, 1.0f,    0.0f, 1.0f,
						   -1.8f, -0.2f,  1.6f,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f,

							// Back
						    -1.8f, -0.2f,  1.2f,   0.0f, 0.0f, -1.0f,   0.0f, 0.0f,
							-1.4f, -0.2f,  1.2f,   0.0f, 0.0f, -1.0f,   1.0f, 0.0f,
							-1.4f, -3.0f,  1.2f,   0.0f, 0.0f, -1.0f,   1.0f, 1.0f,
							-1.4f, -3.0f,  1.2f,   0.0f, 0.0f, -1.0f,   1.0f, 1.0f,
						    -1.8f, -3.0f,  1.2f,   0.0f, 0.0f, -1.0f,   0.0f, 1.0f,
						    -1.8f, -0.2f,  1.2f,   0.0f, 0.0f, -1.0f,   0.0f, 0.0f,

							// Right
						    -1.8f, -0.2f,  1.6f,   1.0f, 0.0f,  0.0f,   0.0f, 0.0f,
							-1.8f, -0.2f,  1.2f,   1.0f, 0.0f,  0.0f,   1.0f, 0.0f,
							-1.8f, -3.0f,  1.2f,   1.0f, 0.0f,  0.0f,   1.0f, 1.0f,
							-1.8f, -3.0f,  1.2f,   1.0f, 0.0f,  0.0f,   1.0f, 1.0f,
						    -1.8f, -3.0f,  1.6f,   1.0f, 0.0f,  0.0f,   0.0f, 1.0f,
						    -1.8f, -0.2f,  1.6f,   1.0f, 0.0f,  0.0f,   0.0f, 0.0f,

							// Left
						    -1.4f, -0.2f,  1.6f,  -1.0f, 0.0f,  0.0f,   0.0f, 0.0f,
							-1.4f, -0.2f,  1.2f,  -1.0f, 0.0f,  0.0f,   1.0f, 0.0f,
							-1.4f, -3.0f,  1.2f,  -1.0f, 0.0f,  0.0f,   1.0f, 1.0f,
							-1.4f, -3.0f,  1.2f,  -1.0f, 0.0f,  0.0f,   1.0f, 1.0f,
						    -1.4f, -3.0f,  1.6f,  -1.0f, 0.0f,  0.0f,   0.0f, 1.0f,
						    -1.4f, -0.2f,  1.6f,  -1.0f, 0.0f,  0.0f,   0.0f, 0.0f,

							// CHAIR RIGHT BACK LEG
							// Front
						    1.8f, -0.2f, -1.2f,	   0.0f, 0.0f, -1.0f,  	0.0f, 0.0f,
							1.4f, -0.2f, -1.2f,    0.0f, 0.0f, -1.0f,   1.0f, 0.0f,
							1.4f, -3.0f, -1.2f,    0.0f, 0.0f, -1.0f,   1.0f, 1.0f,
							1.4f, -3.0f, -1.2f,    0.0f, 0.0f, -1.0f,   1.0f, 1.0f,
						    1.8f, -3.0f, -1.2f,    0.0f, 0.0f, -1.0f,   0.0f, 1.0f,
						    1.8f, -0.2f, -1.2f,    0.0f, 0.0f, -1.0f,   0.0f, 0.0f,

							// Back
						    1.8f, -0.2f, -1.6f,    0.0f, 0.0f, -1.0f,   0.0f, 0.0f,
							1.4f, -0.2f, -1.6f,    0.0f, 0.0f, -1.0f,   1.0f, 0.0f,
							1.4f, -3.0f, -1.6f,    0.0f, 0.0f, -1.0f,   1.0f, 1.0f,
							1.4f, -3.0f, -1.6f,    0.0f, 0.0f, -1.0f,   1.0f, 1.0f,
						    1.8f, -3.0f, -1.6f,    0.0f, 0.0f, -1.0f,   0.0f, 1.0f,
						    1.8f, -0.2f, -1.6f,    0.0f, 0.0f, -1.0f,   0.0f, 0.0f,

							// Right
							1.8f, -0.2f, -1.6f, 	1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
							1.8f, -0.2f, -1.2f, 	1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
							1.8f, -3.0f, -1.2f, 	1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
							1.8f, -3.0f, -1.2f, 	1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
						    1.8f, -3.0f, -1.6f, 	1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
						    1.8f, -0.2f, -1.6f, 	1.0f, 0.0f, 0.0f,   0.0f, 0.0f,

							// Left
						    1.4f, -0.2f, -1.6f, 	1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
							1.4f, -0.2f, -1.2f, 	1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
							1.4f, -3.0f, -1.2f, 	1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
							1.4f, -3.0f, -1.2f, 	1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
						    1.4f, -3.0f, -1.6f, 	1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
						    1.4f, -0.2f, -1.6f, 	1.0f, 0.0f, 0.0f,   0.0f, 0.0f,

							// CHAIR LEFT BACK LEG
							// Front
						   -1.8f, -0.2f, -1.2f, 	0.0f, 0.0f, -1.0f,   0.0f, 0.0f,
						   -1.4f, -0.2f, -1.2f, 	0.0f, 0.0f, -1.0f,   1.0f, 0.0f,
						   -1.4f, -3.0f, -1.2f, 	0.0f, 0.0f, -1.0f,   1.0f, 1.0f,
						   -1.4f, -3.0f, -1.2f, 	0.0f, 0.0f, -1.0f,   1.0f, 1.0f,
						   -1.8f, -3.0f, -1.2f, 	0.0f, 0.0f, -1.0f,   0.0f, 1.0f,
						   -1.8f, -0.2f, -1.2f, 	0.0f, 0.0f, -1.0f,   0.0f, 0.0f,

							// Back
						    -1.8f, -0.2f, -1.6f, 	0.0f, 0.0f, -1.0f,   0.0f, 0.0f,
							-1.4f, -0.2f, -1.6f, 	0.0f, 0.0f, -1.0f,   1.0f, 0.0f,
							-1.4f, -3.0f, -1.6f, 	0.0f, 0.0f, -1.0f,   1.0f, 1.0f,
							-1.4f, -3.0f, -1.6f, 	0.0f, 0.0f, -1.0f,   1.0f, 1.0f,
						    -1.8f, -3.0f, -1.6f, 	0.0f, 0.0f, -1.0f,   0.0f, 1.0f,
						    -1.8f, -0.2f, -1.6f, 	0.0f, 0.0f, -1.0f,   0.0f, 0.0f,

							// Right
						    -1.8f, -0.2f, -1.6f, 	1.0f, 0.0f,  0.0f,   0.0f, 0.0f,
							-1.8f, -0.2f, -1.2f, 	1.0f, 0.0f,  0.0f,   1.0f, 0.0f,
							-1.8f, -3.0f, -1.2f, 	1.0f, 0.0f,  0.0f,   1.0f, 1.0f,
							-1.8f, -3.0f, -1.2f, 	1.0f, 0.0f,  0.0f,   1.0f, 1.0f,
						    -1.8f, -3.0f, -1.6f, 	1.0f, 0.0f,  0.0f,   0.0f, 1.0f,
						    -1.8f, -0.2f, -1.6f, 	1.0f, 0.0f,  0.0f,   0.0f, 0.0f,

							// Left
						    -1.4f, -0.2f, -1.6f, 	-1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
							-1.4f, -0.2f, -1.2f, 	-1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
							-1.4f, -3.0f, -1.2f, 	-1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
							-1.4f, -3.0f, -1.2f, 	-1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
						    -1.4f, -3.0f, -1.6f, 	-1.0f, 0.0f, 0.0f,   0.0f, 1.0f,
						    -1.4f, -0.2f, -1.6f, 	-1.0f, 0.0f, 0.0f,   0.0f, 0.0f,


		};

	// Position and Normal coordinate data
	GLfloat lightVertices[] {

				// Position
				// Back Face
				0.5f,  -0.5f,  -0.5f,
				0.5f,   0.5f,  -0.5f,
				0.5f,   0.5f,  -0.5f,
			   -0.5f,   0.5f,  -0.5f,
			   -0.5f,  -0.5f,  -0.5f,

				// Front Face
			   -0.5f,  -0.5f,   0.5f,
				0.5f,  -0.5f,   0.5f,
				0.5f,   0.5f,   0.5f,
			    0.5f,   0.5f,   0.5f,
			   -0.5f,   0.5f,   0.5f,
			   -0.5f,  -0.5f,   0.5f,

				// Left Face
			   -0.5f,   0.5f,   0.5f,
			   -0.5f,   0.5f,  -0.5f,
			   -0.5f,  -0.5f,  -0.5f,
			   -0.5f,  -0.5f,  -0.5f,
			   -0.5f,  -0.5f,   0.5f,
			   -0.5f,   0.5f,   0.5f,

				// Right Face
			    0.5f,   0.5f,   0.5f,
				0.5f,   0.5f,  -0.5f,
				0.5f,  -0.5f,  -0.5f,
			    0.5f,  -0.5f,  -0.5f,
			    0.5f,  -0.5f,   0.5f,
			    0.5f,   0.5f,   0.5f,

				// Bottom Face
			   -0.5f,  -0.5f,  -0.5f,
				0.5f,  -0.5f,  -0.5f,
				0.5f,  -0.5f,   0.5f,
			    0.5f,  -0.5f,   0.5f,
			   -0.5f,  -0.5f,   0.5f,
			   -0.5f,  -0.5f,  -0.5f,

				// Top Face
			   -0.5f,   0.5f,  -0.5f,
				0.5f,   0.5f,  -0.5f,
				0.5f,   0.5f,   0.5f,
			    0.5f,   0.5f,   0.5f,
			   -0.5f,   0.5f,   0.5f,
			   -0.5f,   0.5f,  -0.5f,
	};

	// Chair
	// Generate buffer IDs for chair
	glGenVertexArrays(1, &chairVAO);
	glGenBuffers(1, &chairVBO);

	// Activate the VAO before binding and setting any VBOs or Attribute Pointers
	glBindVertexArray(chairVAO);

	// Activate the VBO
	glBindBuffer(GL_ARRAY_BUFFER, chairVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(chairVertices), chairVertices, GL_STATIC_DRAW);

	// Set attribute pointer 0 to hold Position data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	//Set attribute pointer 1 to hold normal data
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	//Set attribute pointer 2 to hold texture data
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// Deactivates the VAO which is good practice
	glBindVertexArray(0);

	// KEY LIGHT
	// Generate buffer IDs for light source
	glGenVertexArrays(1, &keyLightVAO);

	// Activate the VAO before binding and setting any VBOs or Attribute Pointers
	glBindVertexArray(keyLightVAO);

	// Activate the light VBO
	glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(lightVertices), lightVertices, GL_STATIC_DRAW);

	// Set attribute pointer 0 to hold Position data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// Deactivates the VAO which is good practice
	glBindVertexArray(0);

	// FILL LIGHT
	// Generate buffer IDs for light source
	glGenVertexArrays(1, &fillLightVAO);

	// Activate the VAO before binding and setting any VBOs or Attribute Pointers
	glBindVertexArray(fillLightVAO);

	// Activate the same light VBO
	glBindBuffer(GL_ARRAY_BUFFER, lightVBO);

	// Set attribute pointer 0 to hold Position data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// Deactivates the VAO which is good practice
	glBindVertexArray(0);

}



/* Generate and load the texture */
void UGenerateTexture(void) {

		// Local variable declarations
		int width;
		int height;

		// Loads texture file
		unsigned char* image = SOIL_load_image("wood_texture.jpg", &width, &height, 0, SOIL_LOAD_RGB);

		// Create texture
		glGenTextures(1, &texture);
		// Bind to type of texture
		glBindTexture(GL_TEXTURE_2D, texture);

		// Load the texture image
		// Parameters: (1) Texture Target, (2) Level of Detail, (3) Internal Pixel Format
		// (4) Width, (5) Height, (6) "0", (7 & 8) Format of Pixels, (9) Image Array
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// Filter texture with mipmap to provide higher quality and performance
		glGenerateMipmap(GL_TEXTURE_2D);
		// Load configured image texture
		SOIL_free_image_data(image);
		// Deactivate texture after using it
		glBindTexture(GL_TEXTURE_2D, 0);

}

/* Implements the UMouseMove function */
void UMouseMove(int x, int y)
{


		// Orbits around the center
		front.x = 10.0f * cos(yaw);
		front.y = 10.0f * sin(pitch);
		front.z = sin(yaw) * cos(pitch) * 10.0f;
}

void onMotion(int curr_x, int curr_y) {

		// if left alt and mouse down are set
		if(checkMotion) {

			// gets the direction the mouse was moved
			mouseXOffset = curr_x - lastMouseX;
			mouseYOffset = lastMouseY - curr_y;

			// updates the new mouse coordinates
			lastMouseX = curr_x;
			lastMouseY = curr_y;

			// Applies sensitivity to mouse direction
			mouseXOffset *= sensitivity;
			mouseYOffset *= sensitivity;

					// get the direction of the mouse

					// if there is changes in yaw, then it is moving along x
					if(yaw != yaw+mouseXOffset && pitch == pitch+mouseYOffset) {

							// Increment yaw
							yaw += mouseXOffset;

							// else movement in y
					} else if (pitch != pitch+mouseYOffset && yaw == yaw+mouseXOffset) {

							// Increment y to move vertical
							pitch += mouseYOffset;
					}

					front.x = 10.0f * cos(yaw);
					front.y = 10.0f * sin(pitch);
					front.z = sin(yaw) * cos(pitch) * 10.0f;
		}

// check if user is zooming, alt, right mouse button and down

if(checkZoom) {

		// determine the direction, whether up or down
		if(lastMouseY > curr_y) {

							// increment scale values
							scale_by_y += 0.1f;
							scale_by_x += 0.1f;
							scale_by_z += 0.1f;

							//redisplay
							glutPostRedisplay();

		} else {

					// decrement scale values, zoom in

			scale_by_y -= 0.1f;
										scale_by_x -= 0.1f;
										scale_by_z -= 0.1f;

										// control zoom in size
										if(scale_by_y < 0.2f) {

												scale_by_y = 0.2f;
												scale_by_x = 0.2f;
												scale_by_z = 0.2f;
										}

										glutPostRedisplay();

		}

		// update x and y
		lastMouseY = curr_y;
		lastMouseX = curr_x;
}
}

void OnMouseClicks(int button, int state, int x, int y) {

	// checks for modifier keys like alt, shift, and ctrl
	keymod = glutGetModifiers();

	// set checkMotion to false
	checkMotion = false;

	// check if button is left, and mod is alt and state is down, all should be true
	if(button == GLUT_LEFT_BUTTON && keymod == GLUT_ACTIVE_ALT && state == GLUT_DOWN) {

		// if true then set motion true
		checkMotion = true;

		// zooming to be false
		checkZoom = false;

	}else if(button == GLUT_RIGHT_BUTTON && keymod == GLUT_ACTIVE_ALT && state == GLUT_DOWN) {

			// zoom to be true and motion to be false
			checkMotion = false;
			checkZoom = true;
	}
}


