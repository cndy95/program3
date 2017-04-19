#include "vgl.h"
#include <vector>
using namespace std;

#include "LoadShaders.h"
#include "loadObj.h"
#include <SOIL.h>

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

#ifdef __APPLE__
#include <cmath>
#endif

const float PI  = 3.141592653589793238463f;
float aspectRadio = .8f;
int PolygonMode = 0, shaderMode = 0, actionListener = 0, lightCount = 0, objectCount = 0;
char fileNames[9][128];
char imageNames[9][128];

int shadermode[9];

vector<GLuint> programList;
GLuint program, program1;
vec3 initialEye, initialCenter, initialViewUp;
vec3 eye, center , viewup;
vector<objInfo> masterList;
vector<int> shaderList;
vector<char*> pathList;

mat4 viewingMatrix, projectionMatrix;
mat4 modelingMatrix[9];

lightProperties lights[4];
void readFile(char *filename) {
	FILE * inputfile = fopen(filename, "r");
	if (inputfile == NULL) {
		cerr << "ERROR:cannot open file " << filename  << endl;
		return;
	}

	char buff[128];
	lightCount = 0;

	while ( fscanf(inputfile, "%s", buff) != EOF) {
		vec3 newvec;
		mat4 newMatrix;
		if ( strcmp("view", buff) == 0) {
			char charEye[128], charCenter[128], charViewup[128];
			fscanf(inputfile, "%s%f%f%f %s%f%f%f %s%f%f%f",
			       charEye, &eye.x, &eye.y, &eye.z,
			       charCenter, &center.x, &center.y, &center.z,
			       charViewup, &viewup.x, &viewup.y, &viewup.z);

		} else if ( strcmp("light", buff) == 0 && lightCount < 4) {
			char charType[128], type[128], value[128];

			fscanf(inputfile, "%s%s", charType, type);
			vec3 ambient, color, position;

			fscanf(inputfile, "%s", value);
			if ( strcmp("ambient", value) == 0) {
				fscanf(inputfile, "%f%f%f",  &ambient.x, &ambient.y, &ambient.z);
				lights[lightCount].ambient = ambient;
			}
			fscanf(inputfile, "%s", value);
			if ( strcmp("color", value) == 0) {
				fscanf(inputfile, "%f%f%f",  &color.x, &color.y, &color.z);
				lights[lightCount].color = color;
			}
			fscanf(inputfile, "%s", value);
			if ( strcmp("position", value) == 0) {
				fscanf(inputfile, "%f%f%f",  &position.x, &position.y, &position.z);
				lights[lightCount].position = position;
			}

			if ( strcmp("directional", type) == 0) {
				lights[lightCount].isEnabled = true;
				lights[lightCount].isLocal = false;
				lights[lightCount].isSpot = false;
			}
			else if ( (strcmp("local", type) == 0) || ( strcmp("spot", type) == 0)) {
				float temp;
				vec3 coneDirection;
				char current[128];
				fscanf(inputfile, "%s", current);
				if ( strcmp("constAtt", current) == 0) {
					fscanf(inputfile, "%f",  &temp);
					lights[lightCount].constantAttenuation = temp;
				}
				fscanf(inputfile, "%s", current);
				if ( strcmp("linearAtt", current) == 0) {
					fscanf(inputfile, "%f",  &temp);
					lights[lightCount].linearAttenuation = temp;
				}
				fscanf(inputfile, "%s", current);
				if ( strcmp("quadAtt", current) == 0) {
					fscanf(inputfile, "%f",  &temp);
					lights[lightCount].quadraticAttenuation = temp;
				}
				lights[lightCount].isEnabled = true;
				lights[lightCount].isLocal = true;
				lights[lightCount].isSpot = false;

				if ( strcmp("spot", type) == 0) {

					fscanf(inputfile, "%s", type);
					if ( strcmp("coneDirection", type) == 0) {
						fscanf(inputfile, "%f%f%f",  &coneDirection.x, &coneDirection.y, &coneDirection.z);
						lights[lightCount].coneDirection = coneDirection;
					}
					fscanf(inputfile, "%s", type);
					if ( strcmp("spotCosCutoff", type) == 0) {
						fscanf(inputfile, "%f",  &temp);
						lights[lightCount].spotCosCutoff = temp;
					}
					fscanf(inputfile, "%s", type);
					if ( strcmp("spotExponent", type) == 0) {
						fscanf(inputfile, "%f",  &temp);
						lights[lightCount].spotExponent = temp;
					}
					lights[lightCount].isSpot = true;
				}
			}
			lightCount = lightCount + 1;

		}  else if ( strcmp("object", buff) == 0) {
			char fileName[128];
			fscanf(inputfile, "%s", fileName);
			strcpy(fileNames[objectCount] , fileName);
			modelingMatrix[objectCount] = glm::mat4(1.0f);
			objectCount = objectCount + 1;
		} else if ( strcmp("s", buff) == 0) {
			fscanf(inputfile, "%f%f%f", &newvec.x, &newvec.y, &newvec.z);
			newMatrix = glm::scale(glm::mat4(1.0f), newvec);
			modelingMatrix[objectCount - 1] = newMatrix * modelingMatrix[objectCount - 1] ;
		} else if ( strcmp("t", buff) == 0) {
			fscanf(inputfile, "%f%f%f", &newvec.x, &newvec.y, &newvec.z);
			newMatrix = glm::translate(glm::mat4(1.0f), newvec);
			modelingMatrix[objectCount - 1] = newMatrix * modelingMatrix[objectCount - 1];

		} else if ( strcmp("rx", buff) == 0) {
			float rx;
			fscanf(inputfile, "%f", &rx);
			newMatrix = glm::rotate(glm::mat4(1.0f), rx * PI / 180.0f, glm::vec3(1, 0, 0));
			modelingMatrix[objectCount - 1] = newMatrix * modelingMatrix[objectCount - 1];
		} else if ( strcmp("ry", buff) == 0) {
			float ry;
			fscanf(inputfile, "%f", &ry);
			newMatrix = glm::rotate(glm::mat4(1.0f), ry * PI / 180.0f, glm::vec3(0, 1, 0));
			modelingMatrix[objectCount - 1] = newMatrix * modelingMatrix[objectCount - 1];
		} else if ( strcmp("rz", buff) == 0) {
			float rz;
			fscanf(inputfile, "%f", &rz);
			newMatrix = glm::rotate(glm::mat4(1.0f), rz * PI / 180.0f, glm::vec3(0, 0, 1));
			modelingMatrix[objectCount - 1] = newMatrix * modelingMatrix[objectCount - 1];
		} else if ( strcmp("shader", buff) == 0) {
			char fn[128];
			fscanf(inputfile, "%s", fn);

			if (strcmp("TOON", fn) == 0)
				shadermode[objectCount - 1] = 1;
			else if (strcmp("BLINN_PHONG", fn) == 0)
				shadermode[objectCount - 1] = 0;
		}
	}
}

void modifyViewing() {
	vec3 fVector;
	vec3 uvector;
	mat4 newMatrix;

	switch (actionListener) {
	case 1:
		fVector = center - eye;
		eye = eye + .01f * fVector;
		center = center + .01f * fVector;
		break;
	case 2:
		fVector = center - eye;
		eye = eye - .01f * fVector;
		center = center - .01f * fVector;
		break;

	case 3:
		fVector = eye - center ;
		uvector = glm::cross(viewup, fVector);
		eye = eye + .01f * uvector;
		center = center + .01f * uvector;
		break;
	case 4:
		fVector = eye - center ;
		uvector = glm::cross(viewup, fVector);
		eye = eye - 0.01f * uvector;
		center = center - .01f * uvector;
		break;

	case 5:
		eye = eye + 2.f * viewup;
		center = center + 2.f * viewup;
		break;
	case 6:
		eye = eye - 2.f * viewup;
		center = center - 2.f * viewup;
		break;

	case 7:
		newMatrix = glm::rotate(mat4(1.0),  PI / 180.0f, viewup);
		eye = vec3(newMatrix * vec4(eye, 1.0f));
		center = vec3(newMatrix * vec4(center, 1.0f));
		break;
	case 8:
		newMatrix = glm::rotate(mat4(1.0), - PI / 180.0f, viewup);
		eye = vec3(newMatrix * vec4(eye, 1.0f));
		center = vec3(newMatrix * vec4(center, 1.0f));
		break;

	case 9:
		newMatrix = glm::rotate(mat4(1.0),  PI / 180.0f, center - eye);
		viewup = vec3(newMatrix * vec4(viewup, 1.0f));
		break;
	case 10:
		newMatrix = glm::rotate(mat4(1.0), - PI / 180.0f, center - eye);
		viewup = vec3(newMatrix * vec4(viewup, 1.0f));
		break;
	}
	viewingMatrix = glm::lookAt(eye, center, viewup);
	actionListener = 0;
}

void loadTexture(char* textureFileName) {

	int width;
	int height;
	int channels;

	unsigned char* image = SOIL_load_image(textureFileName, &width, &height, &channels, SOIL_LOAD_AUTO);//use SOIL to read image file

	if (image == NULL)
	{
		cerr << "Unable to load image: " << textureFileName << endl;
		exit(1);
	}
	for (int i = 0; i * 2 < height; ++i)
	{
		int a = i * width * channels;
		int b = (height - 1 - i) * width * channels;
		for (int j = 0; j < width * channels; j++) {
			unsigned char temp = image[a];
			image[a] = image[b];
			image[b] = temp;
			++a;
			++b;
		}
	}
	GLuint tex2;
	glGenTextures(1, &tex2);
	glBindTexture(GL_TEXTURE_2D, tex2);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	if (channels == 3)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
		             GL_RGB, GL_UNSIGNED_BYTE, image);
	else if (channels == 4)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
		             GL_RGBA, GL_UNSIGNED_BYTE, image);

	SOIL_free_image_data(image);	// free up memory allocated by SOIL_load_image

}

void init() {


	for (int i = 0; i < objectCount; i++) {
		objInfo *theseObjects;

		char* fileName = fileNames[i];

		int objCount;
		loadObjFile( fileName, &theseObjects, &objCount);

		string fileNameinString = string(fileName);

		char* imageName;

		for (int j = 0; j < objCount; j++) {
			theseObjects[j].modelMatrix = modelingMatrix[i];
			masterList.push_back(theseObjects[j]);
			shaderList.push_back(shadermode[i]);
			pathList.push_back(fileName);
		}
	}

	ShaderInfo shader1[] = {{GL_VERTEX_SHADER, "TOON.vert"},
		{GL_FRAGMENT_SHADER, "TOON.frag"},
		{GL_NONE, NULL}
	};
	ShaderInfo shader2[] = {{GL_VERTEX_SHADER, "BLINN_PHONG.vert"},
		{GL_FRAGMENT_SHADER, "BLINN_PHONG.frag"},
		{GL_NONE, NULL}
	};

	program = LoadShaders(shader1); //TOON = 1
	program1 = LoadShaders(shader2); //BLINN_PHONG = 0


	initialEye = eye;
	initialCenter = center;
	initialViewUp = viewup ;
	viewingMatrix = glm::lookAt(eye, center, viewup);

}


void display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	float background[] = { 0.2, 0.5, 0.5, 0.0 };
	glClearBufferfv(GL_COLOR, 0, background);


	if (PolygonMode)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	projectionMatrix = glm::perspective(45.0f, aspectRadio, 1.0f, 3200.0f);


	for (int i = 0; i < masterList.size(); i++) {
		GLuint currentProgram;
		if (shaderList[i] == 1)
			currentProgram = program;
		else
			currentProgram = program1;

		glUseProgram(currentProgram);

		GLuint loc1 = glGetUniformLocation(currentProgram, "enableTexture");
		glUniform1i(loc1, 1);

		cout<<masterList[i].bumpMap<<endl;

		// if (strlen(masterList[i].diffuseTex) != 0) {
		// 	string pathNameinString = string(pathList[i]);
		// 	pathNameinString = pathNameinString.substr(0, pathNameinString.find_last_of("/\\") + 1);
		// 	char* pathNameinChar = & pathNameinString[0u];

		// 	strcat (pathNameinChar, (char*)masterList[i].diffuseTex);
		// 	loadTexture(pathNameinChar);
		// } else if (strcmp(fileNames[i], "OBJfiles/square.obj") == 0) {
		// 	loadTexture((char*)"OBJfiles/mandrill.png");
		// } else {
		// 	glUniform1i(loc1, 0);
		// }

		mat4 mMatrix = masterList[i].modelMatrix;
		mat4 mvMatrix = viewingMatrix * mMatrix;
		mat3 normalMatrix = mat3(mvMatrix);
		mat4 mvpMatrix = projectionMatrix * viewingMatrix * mMatrix;

		glBindVertexArray(masterList[i].VAO);

		loc1 = glGetUniformLocation(currentProgram, "MVMatrix");
		glUniformMatrix4fv(loc1, 1, false, glm::value_ptr(mvMatrix));

		loc1 = glGetUniformLocation(currentProgram, "NormalMatrix");
		glUniformMatrix3fv(loc1, 1, false, glm::value_ptr(normalMatrix));

		loc1 = glGetUniformLocation(currentProgram, "MVPMatrix");
		glUniformMatrix4fv(loc1, 1, false, glm::value_ptr(mvpMatrix));

		loc1 = glGetUniformLocation(currentProgram, "ambient");
		glUniform3fv(loc1, 1, glm::value_ptr(masterList[i].Ka));

		loc1 = glGetUniformLocation(currentProgram, "diffuse");
		glUniform3fv(loc1, 1, glm::value_ptr(masterList[i].Kd));

		loc1 = glGetUniformLocation(currentProgram, "specular");
		glUniform3fv(loc1, 1, glm::value_ptr(masterList[i].Ks));

		loc1 = glGetUniformLocation(currentProgram, "shininess");
		glUniform1f(loc1, masterList[i].n);


		glDrawArrays(GL_TRIANGLES, 0, masterList[i].VAOsize);

		for (int l = 0; l < lightCount; l++) {
			char uniformName[128];
			lights[l].isEnabled = 1;

			sprintf(uniformName, "Lights[%1d].isEnabled", l);
			GLuint loc = glGetUniformLocation(currentProgram, uniformName);
			glUniform1i(loc, lights[l].isEnabled);

			sprintf(uniformName, "Lights[%1d].isLocal", l);
			loc = glGetUniformLocation(currentProgram, uniformName);
			glUniform1i(loc, lights[l].isLocal);

			sprintf(uniformName, "Lights[%1d].isSpot", l);
			loc = glGetUniformLocation(currentProgram, uniformName);
			glUniform1i(loc, lights[l].isSpot);

			sprintf(uniformName, "Lights[%1d].ambient", l);
			loc = glGetUniformLocation(currentProgram, uniformName);
			glUniform3fv(loc, 1, glm::value_ptr(lights[l].ambient));

			sprintf(uniformName, "Lights[%1d].color", l);
			loc = glGetUniformLocation(currentProgram, uniformName);
			glUniform3fv(loc, 1, glm::value_ptr(lights[l].color));

			sprintf(uniformName, "Lights[%1d].position", l);
			loc = glGetUniformLocation(currentProgram, uniformName);

			vec3 mtx;
			if (lights[l].isLocal)
				mtx = vec3(viewingMatrix * vec4(lights[l].position, 1.0));
			else
				mtx = mat3(viewingMatrix) * lights[l].position;
			glUniform3fv(loc, 1, glm::value_ptr(mtx));

			sprintf(uniformName, "Lights[%1d].halfVector", l);
			loc = glGetUniformLocation(currentProgram, uniformName);
			glUniform3fv(loc, 1, glm::value_ptr(lights[l].halfVector));

			sprintf(uniformName, "Lights[%1d].coneDirection", l);
			loc = glGetUniformLocation(currentProgram, uniformName);

			vec3 temp = mat3(viewingMatrix) * lights[l].coneDirection;
			glUniform3fv(loc, 1, glm::value_ptr(temp));

			sprintf(uniformName, "Lights[%1d].spotCosCutoff", l);
			loc = glGetUniformLocation(currentProgram, uniformName);
			glUniform1f(loc, lights[l].spotCosCutoff);

			sprintf(uniformName, "Lights[%1d].spotExponent", l);
			loc = glGetUniformLocation(currentProgram, uniformName);
			glUniform1f(loc, lights[l].spotExponent);

			sprintf(uniformName, "Lights[%1d].constantAttenuation", l);
			loc = glGetUniformLocation(currentProgram, uniformName);
			glUniform1f(loc, lights[l].constantAttenuation);

			sprintf(uniformName, "Lights[%1d].linearAttenuation", l);
			loc = glGetUniformLocation(currentProgram, uniformName);
			glUniform1f(loc, lights[l].linearAttenuation);

			sprintf(uniformName, "Lights[%1d].quadraticAttenuation", l);
			loc = glGetUniformLocation(currentProgram, uniformName);
			glUniform1f(loc, lights[l].quadraticAttenuation);
		}
	}
}


void key_callback(GLFWwindow * window, int key, int scancode, int action, int mods) {

	if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE)
		exit(1);

	else if (key == GLFW_KEY_P) {
		eye = initialEye;
		center = initialCenter;
		viewup = initialViewUp;
		viewingMatrix = glm::lookAt(eye, center, viewup);
	}
	else if (key == GLFW_KEY_O && action == GLFW_RELEASE)
		PolygonMode = 1 - PolygonMode;

	else if (key == GLFW_KEY_W || key == GLFW_KEY_UP)
		actionListener = 1;
	else if (key == GLFW_KEY_S || key == GLFW_KEY_DOWN)
		actionListener = 2;

	else if (key == GLFW_KEY_D || key == GLFW_KEY_RIGHT)
		actionListener = 3;
	else if (key == GLFW_KEY_A || key == GLFW_KEY_LEFT)
		actionListener = 4;


	else if (key == GLFW_KEY_R)
		actionListener = 5;
	else if (key == GLFW_KEY_T)
		actionListener = 6;

	else if (key == GLFW_KEY_Z)
		actionListener = 7;
	else if (key == GLFW_KEY_X)
		actionListener = 8;

	else if (key == GLFW_KEY_C)
		actionListener = 9;
	else if (key == GLFW_KEY_V)
		actionListener = 10;

}

void window_callback(GLFWwindow * window, int width, int height) {
	aspectRadio = (float)width / (float)height;
}

//----------------------------------------------------------------------------
//
// main
//

#undef _WIN32
#ifdef _WIN32
int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance,
                     _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
#else
int main(int argc, char **argv)
#endif
{
	if (argc != 2) {
		cerr << "need one input file" << endl;
		exit(1);
	}
	readFile(argv[1]);

	glfwInit();

#ifdef __APPLE__
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // 4.1 latest version of OpenGL for OSX 10.9 Mavericks
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

	GLFWwindow *window = glfwCreateWindow(800, 800, "program3", NULL, NULL);

	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, key_callback);
	glfwSetWindowSizeCallback(window, window_callback);

	gl3wInit();

#define CHECK_VERSION
#ifdef CHECK_VERSION
	// code from OpenGL 4 Shading Language cookbook, second edition
	const GLubyte *renderer = glGetString(GL_RENDERER);
	const GLubyte *vendor = glGetString(GL_VENDOR);
	const GLubyte *version = glGetString(GL_VERSION);
	const GLubyte *glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);
	GLint major, minor;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);

	cout << "GL Vendor            :" << vendor << endl;
	cout << "GL Renderer          :" << renderer << endl;
	cout << "GL Version (string)  :" << version << endl;
	cout << "GL Version (integer) :" << major << " " << minor << endl;
	cout << "GLSL Version         :" << glslVersion << endl;
	cout << "major version: " << major << "  minor version: " << minor << endl;
#endif

	init();

	while (!glfwWindowShouldClose(window)) {
		modifyViewing();
		display();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
}
