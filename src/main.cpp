//cpsc589 assignment 2 qiyue zhang 10131658


#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <math.h>
#include <algorithm>
#define _USE_MATH_DEFINES
#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "glm/gtc/matrix_transform.hpp"
#include <iomanip>
#include <ctime>
#include <chrono>
#include <glm/gtc/type_ptr.hpp>

using std::string;
using std::vector;
using std::cout;
using std::cin;
using std::cerr;
using std::endl;

float PI = glm::pi<float>();

int windowWidth;
int windowHeight;

bool closeWindow = false;
double dotX, dotY;
vector<float> controlPoints = {0,0};
float scroll = 1.f;
double offsetX=0, offsetY = 0;
// object transformation controls
float rollAngle = 0, pitchAngle = 0, yawAngle = 0;
float scale = 1;
int dots = 0;

// curve parameters
// k = order of B-spline
// m = number of control points
// E = coefficient vector (control points)
// U = knot sequence
// u = fixed parameter value

float m = 5, k = 3;
// float u;
vector<float> U = {0, 0, 0, 0.25, 0.5, 0.75, 1, 1, 1};
//vector<float> E = {0.1, 0.1, 0.5, 0.5, 0.3, 0.3, 0.7, 0.7, 0.0, 0.0, 0.2, 0.2};//{0.1, 0.1, 0.2, 0.2, 0.3, 0.3, 0.4, 0.4, 0.5, 0.5, 0.6, 0.6}; //, 2, 3, 4, 5, 6}; // knot sequence + control points
float inc = 0.001; //inc
// for transformations
glm::mat4 mvp;

struct Point {
	float x;
	float y;
}E1 = {0.2, 0.2}, E2 = {0.4, 0.4}, E3 = {0.6, 0.4}, E4 = {0.8, 0.2}, E5 = {0.6, 0.00}, E6 = {0.4, 0.00};
vector<Point> E = {E1, E2, E3, E4, E5, E6};

// taken from boilerplate code from CPSC453
class Program {
	GLuint vertex_shader;
	GLuint fragment_shader;
public:
	GLuint id;
	Program() {
		vertex_shader = 0;
		fragment_shader = 0;
		id = 0;
	}
	Program(string vertex_path, string fragment_path) {
		init(vertex_path, fragment_path);
	}
	void init(string vertex_path, string fragment_path) {
		id = glCreateProgram();
		vertex_shader = addShader(vertex_path, GL_VERTEX_SHADER);
		fragment_shader = addShader(fragment_path, GL_FRAGMENT_SHADER);
		if (vertex_shader)
			glAttachShader(id, vertex_shader);
		if (fragment_shader)
			glAttachShader(id, fragment_shader);

		glLinkProgram(id);
	}
	GLuint addShader(string path, GLuint type) {
		std::ifstream in(path);
		string buffer = [&in] {
			std::ostringstream ss {};
			ss << in.rdbuf();
			return ss.str();
		}();
		const char *buffer_array[] = { buffer.c_str() };

		GLuint shader = glCreateShader(type);

		glShaderSource(shader, 1, buffer_array, 0);
		glCompileShader(shader);

		// Compile results
		GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE) {
			GLint length;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
			string info(length, ' ');
			glGetShaderInfoLog(shader, info.length(), &length, &info[0]);
			cerr << "ERROR compiling shader:" << endl << endl;
			cerr << info << endl;
		}
		return shader;
	}
	~Program() {
		glUseProgram(0);
		glDeleteProgram(id);
		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);
	}
};

// taken from boilerplate code from CPSC453
class VertexArray {
	std::map<string, GLuint> buffers;
	std::map<string, int> indices;
public:
	GLuint id;
	unsigned int count;
	VertexArray(int c) {
		glGenVertexArrays(1, &id);
		count = c;
	}

	VertexArray(const VertexArray &v) {
		glGenVertexArrays(1, &id);

		// Copy data from the old object
		this->indices = std::map<string, int>(v.indices);
		count = v.count;

		vector<GLuint> temp_buffers(v.buffers.size());

		// Allocate some temporary buffer object handles
		glGenBuffers(v.buffers.size(), &temp_buffers[0]);

		// Copy each old VBO into a new VBO
		int i = 0;
		for (auto &ent : v.buffers) {
			int size = 0;
			glBindBuffer(GL_ARRAY_BUFFER, ent.second);
			glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

			glBindBuffer(GL_COPY_READ_BUFFER, temp_buffers[i]);
			glBufferData(GL_COPY_READ_BUFFER, size, NULL, GL_STATIC_COPY);

			glCopyBufferSubData(GL_ARRAY_BUFFER, GL_COPY_READ_BUFFER, 0, 0,
					size);
			i++;
		}

		// Copy those temporary buffer objects into our VBOs

		i = 0;
		for (auto &ent : v.buffers) {
			GLuint buffer_id;
			int size = 0;
			int index = indices[ent.first];

			glGenBuffers(1, &buffer_id);

			glBindVertexArray(this->id);
			glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
			glBindBuffer(GL_COPY_READ_BUFFER, temp_buffers[i]);
			glGetBufferParameteriv(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &size);

			// Allocate VBO memory and copy
			glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
			glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_ARRAY_BUFFER, 0, 0,
					size);
			string indexs = ent.first;

			buffers[ent.first] = buffer_id;
			indices[ent.first] = index;

			// Setup the attributes
			size = size / (sizeof(float) * this->count);
			glVertexAttribPointer(index, size, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(index);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
			i++;
		}

		// Delete temporary buffers
		glDeleteBuffers(v.buffers.size(), &temp_buffers[0]);
	}

	void addBuffer(string name, int index, vector<float> buffer) {
		GLuint buffer_id;
		glBindVertexArray(id);

		glGenBuffers(1, &buffer_id);
		glBindBuffer(GL_ARRAY_BUFFER, buffer_id);
		glBufferData(GL_ARRAY_BUFFER, buffer.size() * sizeof(float),
				buffer.data(), GL_STATIC_DRAW);
		buffers[name] = buffer_id;
		indices[name] = index;

		int components = buffer.size() / count;
		glVertexAttribPointer(index, components, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(index);

		// unset states
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}

	void updateBuffer(string name, vector<float> buffer) {
		glBindBuffer(GL_ARRAY_BUFFER, buffers[name]);
		glBufferData(GL_ARRAY_BUFFER, buffer.size() * sizeof(float),
				buffer.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	~VertexArray() {
		glBindVertexArray(0);
		glDeleteVertexArrays(1, &id);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		for (auto &ent : buffers)
			glDeleteBuffers(1, &ent.second);
	}
};

// for transformations on static curve
glm::mat4 getMVP() {
	glm::mat4 Scale = glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, scale));
	glm::mat4 RotateX = glm::rotate(glm::mat4(1.0f), rollAngle, glm::vec3(1, 0, 0));
	glm::mat4 RotateY = glm::rotate(glm::mat4(1.0f), pitchAngle, glm::vec3(0, 1, 0));
	glm::mat4 RotateZ = glm::rotate(glm::mat4(1.0f), yawAngle, glm::vec3(0, 0, 1));

	return Scale*RotateZ*RotateY*RotateX;
}


// calculating delta index from u
int delta(float u, float m, float k) {
	int ret = -1;
	for(int i = 0; i < m + k; i++) {
		if(u > U[i] && u < U[i+1]) ret = i;
	}
	// cout<< "i = " <<ret<<endl;
	return ret;
}

// computing E_delta_1 from the set of input Ei
Point E_delta_1(float k, float m, float u) {
	int d = delta(u, m, k);
	if(d == -1 ) return (u < 1) ? E[0] : E[E.size()-1]; // if its not in any interval then its prob the first point?? (or last)
	// cout<<"@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"<<endl;
	vector<Point> c;
	for(int i = 0; i < k; i++) {
		c.push_back(E[d-i]);
	}
	for( int r = k; r > 1; r--) {
		int i = d;
		for(int s = 0; s < r-1; s++) {
			float omega = (u - U[i])/(U[i+r-1] - U[i]);
			c[s].x = omega*c[s].x + (1-omega)*c[s+1].x;
			c[s].y = omega*c[s].y + (1-omega)*c[s+1].y;
			i--;
		}
	}
	return c[0];
}


// draw contents of a va
void render(GLuint pid, VertexArray &va) {
	glUseProgram(pid);
	glBindVertexArray(va.id);
	glDrawArrays(GL_LINE_STRIP, 0, va.count);
	glBindVertexArray(0);
	glUseProgram(0);

	// for(int i = 0; i < controlPoints.size(); i++) {
		VertexArray vac(controlPoints.size()/2);
		vac.addBuffer("vac", 0, controlPoints);
		glPointSize(5);
		glUseProgram(pid);
		glBindVertexArray(vac.id);
		glDrawArrays(GL_POINTS, 0, vac.count);
		glBindVertexArray(0);
		glUseProgram(0);
	// }

}

// static curve set up
void drawCurve(GLuint pid) {

		vector<float> curve;
		for(float u = 0; u < 1 +inc; u += inc) {
			Point c = E_delta_1(k, m, u);
			curve.push_back(c.x);
			curve.push_back(c.y);
			// curve.push_back(u);
			// curve.push_back(E_delta_1(k, m, u));
			// cout<< "(u, S(u)) = (" <<curve[curve.size()-2]<<" , "<<curve[curve.size()-1]<<")"<<endl;

		}

		// cout<<"@@@@@@@@@@@@@@@@@@@@@@@@@@@"<<endl;

          int curveVertices = curve.size()/2;

          VertexArray va(curveVertices);
          va.addBuffer("va", 0, curve);
          render(pid, va);

}

// ask user for params
// void getParams() {
// 	cout<<"Enter the radius of the outer circle: ";
// 	if(!(cin>>R)|| R <= 0){ cout<<"\nERROR: Input must be a positive real number\n"; exit(0);}
// 	cout<<"\nEnter the radius of the inner circle: ";
// 	if(!(cin>>r) || r <= 0 || r >= R){ cout<<"\nERROR: Input must be a positive real number less than R\n"; exit(0);}
// 	cout<<"\nEnter the number of cycles: ";
// 	if(!(cin>>n) || n <= 0){ cout<<"\nERROR: Input must be a positive real number\n"; exit(0);}
// 	cout<<"\nEnter the direction of rotation (1 = CCW, -1 = CW): ";
// 	if(!(cin>>dir)||((dir!=1)&&(dir!=-1))){ cout<<"\nERROR: Input must be 1 or -1\n"; exit(0);}
// 	cout<<endl;
// }

// keyboard controls
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

          if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) closeWindow = closeWindow?false:true;

		// reset to default state
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
			rollAngle = 0; pitchAngle = 0; yawAngle = 0; scale = 1.0;
		}

		// scaling controls
		if (key == GLFW_KEY_KP_ADD && (action == GLFW_REPEAT || action == GLFW_PRESS)) scale *= 1.2;
		if (key == GLFW_KEY_KP_SUBTRACT && (action == GLFW_REPEAT || action == GLFW_PRESS)) scale *= 0.8;
		// intrinsic rotation controls
		if (key == GLFW_KEY_KP_7 && (action == GLFW_REPEAT || action == GLFW_PRESS)) rollAngle -= 0.1;
	    	if (key == GLFW_KEY_KP_9 && (action == GLFW_REPEAT || action == GLFW_PRESS)) rollAngle += 0.1;
	     if (key == GLFW_KEY_KP_4 && (action == GLFW_REPEAT || action == GLFW_PRESS)) pitchAngle -= 0.1;
		if (key == GLFW_KEY_KP_6 && (action == GLFW_REPEAT || action == GLFW_PRESS)) pitchAngle += 0.1;
		if (key == GLFW_KEY_KP_1 && (action == GLFW_REPEAT || action == GLFW_PRESS)) yawAngle -= 0.1;
		if (key == GLFW_KEY_KP_3 && (action == GLFW_REPEAT || action == GLFW_PRESS)) yawAngle += 0.1;
		if (key == GLFW_KEY_KP_5 && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
			rollAngle += 0.1;
			pitchAngle += 0.1;
			yawAngle += 0.1;
		}
		if (key == GLFW_KEY_KP_2 && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
			rollAngle -= 0.1;
			pitchAngle -= 0.1;
			yawAngle -= 0.1;
		}

		// if(key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS) {
		// 	animate = animate ? false: true;
		// 	curve.clear();
		// 	t0 = std::chrono::high_resolution_clock::now();
		// 	t = 0;
		// }
		// if(key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
		// 	animate = false;
		// 	outerCircle.clear();
		// 	innerCircle.clear();
		// 	rollAngle = 0; pitchAngle = 0; yawAngle = 0; scale = 1.0;
		// 	getParams();
		// }
}

void mouse_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
 		// mouseButtonPressed = true;
 	} else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
 		// mouseButtonPressed = false;
     } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
		glfwGetCursorPos(window, &dotX, &dotY);
		//if(dotWithinBounds(dotX, dotY)){
			dots ++;
			cout<<"("<<dotX<<" , "<<dotY<<")"<<endl;
			dotX = (float)dotX/windowWidth - 1;
			dotY = 1 - (float)dotY/windowHeight;
			controlPoints.push_back(dotX);
			controlPoints.push_back(dotY);
			cout<<"("<<dotX<<" , "<<dotY<<")"<<endl;
			// controlPoints.push_back((1.0/scroll)*((-1.0+(float)(dotX/400))-(offsetX/400)));
			// controlPoints.push_back((1.0/scroll)*((1.0 - (float)(dotY/400))-(offsetY/400)));
			// cout<<"placing dot =  dotNum: "<<dots<<" at window coordinates: (" <<
			// (1.0/scroll)*((-1.0+(float)(dotX/400))-(offsetX/400))<<", "<<
			// (1.0/scroll)*((1.0 - (float)(dotY/400))-(offsetY/400))<<" )"<<endl;
		//}
	}

}

void cursor_callback(GLFWwindow* window, double xpos, double ypos) {
	// if(mouseButtonPressed) {
     //    	offsetX = offsetX + xpos - prevCursorX;
     //    	offsetY = offsetY - ypos + prevCursorY;
	//
 //   	}
 //  	prevCursorX = xpos;
 //  	prevCursorY = ypos;
	// //cout<<xpos<<"\t"<<ypos<<endl;
}


int main(int argc, char *argv[]) {

		// getParams();

          if (!glfwInit()) {

                    cout << "ERROR: GLFW failed to initialize, TERMINATING" << endl;
                    return -1;

          }

          GLFWwindow *window = 0;
          glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
          glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
          glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
          glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
          glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

          window = glfwCreateWindow(400, 400, "CPSC 589 Assignment 1", 0, 0);

          glfwSetWindowAspectRatio(window, 1, 1);

          if (!window) {
                    cout << "failed to create window, TERMINATING" << endl;
                    glfwTerminate();
                    return -1;
          }

          glfwMakeContextCurrent(window);
          glfwSetKeyCallback(window, key_callback);
		glfwSetMouseButtonCallback(window, mouse_callback);
		glfwSetCursorPosCallback(window, cursor_callback);

		Program p("data/vertex.glsl", "data/fragment.glsl");
		glUseProgram(p.id);
		GLint mvpLoc = glGetUniformLocation(p.id, "mvp");


		while(!glfwWindowShouldClose(window)) {

			glfwGetWindowSize(window, &windowWidth, &windowHeight);
			glClearColor(0, 51.f/255.f, 102.f/255.f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

               glViewport((windowWidth - std::min(windowWidth, windowHeight))/2, (windowHeight - std::min(windowWidth, windowHeight))/2, std::min(windowWidth, windowHeight), std::min(windowWidth, windowHeight));

			glUseProgram(p.id);

			mvp = getMVP();
			glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &mvp[0][0]);
			drawCurve(p.id);

			if(closeWindow) {break;}

               glfwSwapBuffers(window);
               glfwPollEvents();

          }

          glfwDestroyWindow(window);
          glfwTerminate();

          cout << "The end" << endl;
          return 0;

}