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
vector<float> controlPoints;
float scroll = 1.f;
double offsetX=0, offsetY = 0;
// // object transformation controls
// float rollAngle = 0, pitchAngle = 0, yawAngle = 0;
// float scale = 1;
int dots = 0;
bool mouseButtonPressed = false;

//for surface
float roll = 0, pitch = 0, yaw = 0;
float scale = 0.8;
float deltaX = 0.0, deltaY = 0.0;
// curve parameters
// k = order of B-spline
// m = number of control points
// E = coefficient vector (control points)
// U = knot sequence
// u = fixed parameter value

// for transformations
glm::mat4 surfaceMvp;
bool showSurface = false;

class BSpline {
public:
	float k, m, inc;
	vector<float> knots;
	vector<glm::vec3> controlPoints;
	vector<float> weights;

	BSpline() {
		k = 2;
		m = 0; //num control pt
		inc = 0.01;
		knots = {0, 0.5, 1};
		weights = {};
		controlPoints = {};
		cout<<"wtf"<<endl;
		init();
	}
	void init() {

	}

	void incK(int increase) {
		k += (increase == 0 || k+1 > m+1) ? 0 : 1; 	// inc k
		k -= (increase == 1 || k-1 < 2) ? 0 : 1; 	// dec k
	}
	// void decK() {
	// 	k -= (k-1 < 2) ? 0 : 1;
	// }
	void incDetail(int increase) {
		inc -= (increase == 1 && (inc -= inc/2 < 0.00001)) ? 0 : inc/2;	// inc detail
		inc += (increase == 0 && (inc += inc/2 > 0.01)) ? 0 : inc/2;		// dec detail
	}
	void decDetail() {
		inc += (inc += inc/2 > 0.01) ? 0 : inc/2;
	}

	// calculating delta index from u
	int delta(float u) {
		int ret = -1;
		for(int i = 0; i < m + k; i++) {
			if(u >= knots[i] && u < knots[i+1]) ret = i;
		}
		// cout<< "i = " <<ret<<endl;
		return ret;
	}

	void updateKnots() {
		knots.clear();
		for(int i = 0; i < k; i++) {
			knots.push_back(0);
		}
		float knotInc = 1/(m-k+2);
		for(int i = 0; i < m - k + 1; i++) { //CHANGE: index used to be k to m inclusive
			knots.push_back(knotInc*(1+i));
		}
		for(int i = m+1; i < m + k + 1; i++) {
			knots.push_back(1);
		}
	}

	// computing E_delta_1 from the set of input Ei
	glm::vec3 E_delta_1(float u) {

		if(u == 0 || u == 1) return (u == 0) ? controlPoints[0] : controlPoints[controlPoints.size() -1 ];
		int d = delta(u);
		// CHANGE: comment out line below
		// if(d == -1 ) return (u < 1) ? controlPoints[0] : controlPoints[controlPoints.size()-1]; // if its not in any interval then its prob the first point?? (or last)
		// cout<<"@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"<<endl;
		vector<glm::vec3> c;
		for(int i = 0; i < k; i++) {
			c.push_back(controlPoints[d-i]);
		}
		for( int r = k; r > 1; r--) {
			int i = d;
			for(int s = 0; s < r-1; s++) {
				float omega = (u - knots[i])/(knots[i+r-1] - knots[i]);
				c[s].x = omega*c[s].x + (1-omega)*c[s+1].x;
				c[s].y = omega*c[s].y + (1-omega)*c[s+1].y;
				c[s].z = 0.0;
				i--;
			}
		}
		return c[0];
	}

	void addControlPoint(float x, float y) {
		controlPoints.push_back(glm::vec3(x, y, 0.0f));
		m = controlPoints.size() == 0 ? 0 : controlPoints.size() - 1;
	}
	int selectControlPoint(float x, float y) {
		for(int i = 0; i < (int)controlPoints.size(); i++) {
			if(abs(controlPoints[i].x - x) < 10/(float)windowWidth && abs(controlPoints[i].y - y) < 10/(float)windowHeight)
				return i;
		}
		return -1;
	}
	void deleteControlPoint(int i) {
		controlPoints.erase(controlPoints.begin() + i);
		m = controlPoints.size() == 0 ? 0 : controlPoints.size() - 1;
	}

	vector<float> getCurve() {
		updateKnots();
		vector<float> curve;
		// cout<<"Printing curve..."<<endl;

		for(float u = 0; u < 1 +inc; u += inc) {
			glm::vec3 c = E_delta_1(u);
			curve.push_back(c.x);
			curve.push_back(c.y);
			curve.push_back(c.z);
			// cout<<"u = "<<u <<"\tPoint = ("<<c.x<<" , "<<c.y<<")"<<endl;
		}
		return curve;
	}

	vector<float> getSurface() {
		vector<float> c = getCurve();
		vector<float> s;
		for(int i = 0; i < c.size(); i+=3) {
			for(float j = 0; j < 2*PI + inc; j += 0.01) {

				s.push_back(c[i] * glm::cos(j));
				s.push_back(c[i+1]);
				s.push_back(c[i] * glm::sin(j));

				s.push_back(c[i+3] * glm::cos(j));
				s.push_back(c[i+4]);
				s.push_back(c[i+3] * glm::sin(j));
			}
		}
		return s;
	}

	~BSpline() {}
}b;


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

	void addBuffer(string name, int index, vector<glm::vec3> temp) {
		vector<float> buffer;
		for(int i = 0; i < (int)temp.size(); i++) {
			buffer.push_back(temp[i].x);
			buffer.push_back(temp[i].y);
			buffer.push_back(temp[i].z);
		}
		addBuffer(name, index, buffer);

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
glm::mat4 getMVP(float scale, float x, float y, float rollAngle, float pitchAngle, float yawAngle) {
	glm::mat4 Scale = glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, scale));
	glm::mat4 RotateX = glm::rotate(glm::mat4(1.0f), rollAngle, glm::vec3(1, 0, 0));
	glm::mat4 RotateY = glm::rotate(glm::mat4(1.0f), pitchAngle, glm::vec3(0, 1, 0));
	glm::mat4 RotateZ = glm::rotate(glm::mat4(1.0f), yawAngle, glm::vec3(0, 0, 1));
	glm::mat4 Translate = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0));
	return Scale*RotateZ*RotateY*RotateX*Translate;
}

void renderCurve(VertexArray &va, GLuint pid) {
	glUseProgram(pid);
	glBindVertexArray(va.id);
	glDrawArrays(GL_LINE_STRIP, 0, va.count);
	glBindVertexArray(0);
	glUseProgram(0);
}
void renderPoints(VertexArray &va, GLuint pid, int size) {
	glPointSize(size);
	glUseProgram(pid);
	glBindVertexArray(va.id);
	glDrawArrays(GL_POINTS, 0, va.count);
	glBindVertexArray(0);
	glUseProgram(0);
}
void renderSurface(VertexArray &va, GLuint pid) {
	glUseProgram(pid);
	glBindVertexArray(va.id);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, va.count);
	glBindVertexArray(0);
	glUseProgram(0);
}



void render(GLuint pid, GLuint qid) {
	if(b.controlPoints.size() > 0) {
		if(b.controlPoints.size() > 3) {
			// render curve

			vector<float> curve = b.getCurve();
			VertexArray vac(curve.size()/3);
			vac.addBuffer("vac", 0, curve);
			renderCurve(vac, pid);

			cout<<"Render curve - \tCurve Size = "<<curve.size()/3
			<<"\nm = "<<b.m<<"\nk = "<<b.k<<"\nNumber of knots = "<<b.knots.size()<<
			"\nPrinting knot sequence..."<<endl;
			for(int i = 0; i < b.knots.size(); i++) {
				cout<<b.knots[i]<<endl;
			}

			int su = showSurface? 1: 0;
			cout<< "showSurface = "<<su<<endl;

			if(showSurface) {

				vector<float> surface = b.getSurface();
				surfaceMvp = getMVP(0.8, -0.3, 0.0, 0.0, 0.0, 0.0);
				glUniformMatrix4fv(glGetUniformLocation(qid, "mvp"), 1, GL_FALSE, &surfaceMvp[0][0]);

				VertexArray vas(surface.size()/3);
				vas.addBuffer("vas", 0, surface);
				renderSurface(vas, qid);
			}
		}
		// render control Points and the lines connecting the points
		VertexArray vap(b.controlPoints.size());
		vap.addBuffer("vap", 0, b.controlPoints);
		renderPoints(vap, pid, 5);
		renderCurve(vap, pid);

	}

}

// keyboard controls
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

          if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) closeWindow = closeWindow?false:true;
		if (key == GLFW_KEY_X && action == GLFW_PRESS && mouseButtonPressed) {
			glfwGetCursorPos(window, &dotX, &dotY);

			dotX = (float)(2*dotX)/windowWidth - 1;
			dotY = 1 - (float)(2*dotY)/windowHeight;
			int idx = b.selectControlPoint(dotX, dotY);
			if(idx != -1) {
				cout<<"select"<<idx<<endl;
				glfwGetCursorPos(window, &dotX, &dotY);
				b.controlPoints.erase(b.controlPoints.begin()+idx);//[idx] = glm::vec3(dotX, dotY);
			}
		}
		if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
			showSurface = showSurface ? false : true;
			if(showSurface == false) {
				roll = pitch = yaw = deltaY = 0;
				deltaX = 0.0;
				scale = 0.8;
			}
		}

		// intrinsic rotation controls
		if (key == GLFW_KEY_KP_7 && (action == GLFW_REPEAT || action == GLFW_PRESS)) roll -= 0.1;
		if (key == GLFW_KEY_KP_9 && (action == GLFW_REPEAT || action == GLFW_PRESS)) roll += 0.1;
		if (key == GLFW_KEY_KP_4 && (action == GLFW_REPEAT || action == GLFW_PRESS)) pitch -= 0.1;
		if (key == GLFW_KEY_KP_6 && (action == GLFW_REPEAT || action == GLFW_PRESS)) pitch += 0.1;
		if (key == GLFW_KEY_KP_1 && (action == GLFW_REPEAT || action == GLFW_PRESS)) yaw -= 0.1;
		if (key == GLFW_KEY_KP_3 && (action == GLFW_REPEAT || action == GLFW_PRESS)) yaw += 0.1;
		if (key == GLFW_KEY_KP_5 && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
			roll += 0.1;
			pitch += 0.1;
			yaw += 0.1;
		}
		if (key == GLFW_KEY_KP_2 && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
			roll -= 0.1;
			pitch -= 0.1;
			yaw -= 0.1;
		}
		if (key == GLFW_KEY_UP && (action == GLFW_REPEAT || action == GLFW_PRESS)) deltaY += 0.1;
		if (key == GLFW_KEY_DOWN && (action == GLFW_REPEAT || action == GLFW_PRESS)) deltaY -= 0.1;
		if (key == GLFW_KEY_LEFT && (action == GLFW_REPEAT || action == GLFW_PRESS)) deltaX -= 0.1;
		if (key == GLFW_KEY_RIGHT && (action == GLFW_REPEAT || action == GLFW_PRESS)) deltaX += 0.1;
		// scaling controls
		if (key == GLFW_KEY_KP_ADD && (action == GLFW_REPEAT || action == GLFW_PRESS)) scale *= 1.2;
		if (key == GLFW_KEY_KP_SUBTRACT && (action == GLFW_REPEAT || action == GLFW_PRESS)) scale *= 0.8;

		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
			roll = pitch = yaw = deltaY = 0;
			deltaX = 0; scale = 0.8;
			// showSurface = false;
		}
		if (key == GLFW_KEY_DELETE && action == GLFW_PRESS) {
			roll = pitch = yaw = deltaY = 0;
			deltaX = 0; scale = 0.8;
			showSurface = false;
			b.controlPoints.clear();

		}


}

void mouse_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		mouseButtonPressed = true;

 	} else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
 		mouseButtonPressed = false;
     } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
		glfwGetCursorPos(window, &dotX, &dotY);
		dots ++;
		cout<<"("<<dotX<<" , "<<dotY<<")"<<endl;
		dotX = (float)(2*dotX)/windowWidth - 1;
		dotY = 1 - (float)(2*dotY)/windowHeight;
		b.addControlPoint(dotX, dotY);
		cout<<"("<<dotX<<" , "<<dotY<<")"<<endl;
	}

}

void cursor_callback(GLFWwindow* window, double xpos, double ypos) {

	if(mouseButtonPressed) {
		// glfwGetCursorPos(window, &dotX, &dotY);
		dotX = (float)(2*xpos)/windowWidth - 1;
		dotY = 1 - (float)(2*ypos)/windowHeight;
		int idx = b.selectControlPoint(dotX, dotY);
		if(idx != -1) {
			cout<<"select"<<idx<<endl;
			dotX = (float)(2*xpos)/windowWidth - 1;
			dotY = 1 - (float)(2*ypos)/windowHeight;

			b.controlPoints[idx] = glm::vec3(dotX, dotY, 0.0);
		}
	}
}

// double xoffset, yoffset;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	int increase = (yoffset ==  1) ? 1 : 0;
	b.incK(increase);
	cout<<"k = "<<b.k<<endl;
	// cout<<"("<<xoffset<<","<<yoffset<<")\n";
}


int main(int argc, char *argv[]) {
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
		glfwSetScrollCallback(window, scroll_callback);

		Program p("data/vertex.glsl", "data/fragment.glsl");
		Program q("data/vertex2.glsl", "data/fragment2.glsl");

		// glUseProgram(p.id);
		// glUseProgram(q.id);
		// GLint pMvpLoc = glGetUniformLocation(p.id, "mvp");
		// GLint qMvpLoc = glGetUniformLocation(q.id, "mvp");

		glUseProgram(q.id);
		GLint qMvpLoc = glGetUniformLocation(q.id, "mvp");
		// surfaceMvp = getMVP(scale, deltaX, deltaY, roll, pitch, yaw);
		while(!glfwWindowShouldClose(window)) {
			// cout<<"While loop "<<endl;

			glfwGetWindowSize(window, &windowWidth, &windowHeight);
			glClearColor(0, 51.f/255.f, 102.f/255.f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

               glViewport((windowWidth - std::min(windowWidth, windowHeight))/2, (windowHeight - std::min(windowWidth, windowHeight))/2, std::min(windowWidth, windowHeight), std::min(windowWidth, windowHeight));

			glUseProgram(p.id);
			glUseProgram(q.id);

			surfaceMvp = getMVP(scale, deltaX, deltaY, roll, pitch, yaw);
			glUniformMatrix4fv(qMvpLoc, 1, GL_FALSE, &surfaceMvp[0][0]);
			render(p.id, q.id);

			if(closeWindow) {break;}

               glfwSwapBuffers(window);
               glfwPollEvents();

          }

          glfwDestroyWindow(window);
          glfwTerminate();

          cout << "The end" << endl;
          return 0;

}
