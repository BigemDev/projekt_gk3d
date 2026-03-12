#define GLM_FORCE_RADIANS

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include "shaderprogram.h"

#include "objmodel.h"
Models::ObjModel rat;

//camera :3
float camYaw = 0.0f;
float camPitch = 0.0f;
float ltX = 400.0f;
float ltY = 300.0f;
float startMouse = true;

//floor :3
GLuint floorVAO;

void initFloor() {
	float size = 20.0f;
	float y = -2.0f;
	float vertices[] = {
		-size,  y,  -size, 1.0f,  0.0f, 1.0f, 0.0f, 0.0f,
        size,  y,  -size, 1.0f,  0.0f, 1.0f, 0.0f, 0.0f,
        size,  y,   size, 1.0f,  0.0f, 1.0f, 0.0f, 0.0f,
        -size,  y,  -size, 1.0f,  0.0f, 1.0f, 0.0f, 0.0f,
        size,  y,   size, 1.0f,  0.0f, 1.0f, 0.0f, 0.0f,
        -size,  y,   size, 1.0f,  0.0f, 1.0f, 0.0f, 0.0f,

	};
	
	GLuint vbo;
	glGenVertexArrays(1, &floorVAO);
	glBindVertexArray(floorVAO);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, false, 8 * sizeof(float), (void*)0);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, false, 8 * sizeof(float), (void*)(4 * sizeof(float)));

	glBindVertexArray(0);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
	if (startMouse) {
		ltX = xpos;
		ltY = ypos;
		startMouse = false;
	}

	float xoffset = xpos - ltX;
	float yoffset = ypos - ltY;

	ltX = xpos;
	ltY = ypos;

	float sens = 0.01f;
	xoffset *= sens;
	yoffset *= sens;

	camYaw -= xoffset;
	camPitch -= yoffset;

	if (camPitch > 1.5f)
		camPitch = 1.5f;
	if (camPitch < -1.5f)
		camPitch = -1.5f;
}


void error_callback(int error, const char* description) {
	fputs(description, stderr);
}



void initOpenGLProgram(GLFWwindow* window) {
	initShaders();
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	rat = Models::ObjModel("RAT1.obj");
	initFloor();
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void freeOpenGLProgram(GLFWwindow* window) {
	freeShaders();

}

void drawScene(GLFWwindow* window) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float time = glfwGetTime();

	glm::vec3 front;
	front.x = cos(camPitch) * sin(camYaw);
	front.y = sin(camPitch);
	front.z = cos(camPitch) * cos(camYaw);
	front = glm::normalize(front);

	static glm::vec3 camPos = glm::vec3(0.0f, 2.0f, 10.0f);
	glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));
	float speed = 0.05f;


	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camPos += speed * front;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camPos -= speed * front;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camPos -= speed * right;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camPos += speed * right;

	glm::mat4 P = glm::perspective(glm::radians(50.0f), 800.0f/600.0f, 0.1f, 100.0f);

	//static camera if u want :3
	// glm::mat4 V = glm::lookAt(
    //     glm::vec3(0.0f, 2.0f, 10.0f),
    //     glm::vec3(0.0f, 0.0f,  0.0f),
    //     glm::vec3(0.0f, 1.0f,  0.0f)
    // );

	//dynamic camera :3
	glm::mat4 V = glm::lookAt(
		camPos,
		camPos + front,
		glm::vec3(0.0f, 1.0f, 0.0f)
	);

    glm::mat4 M = glm::mat4(1.0f);
	// M = glm::rotate(M, time, glm::vec3(0.0f, 1.0f, 0.0f));

	//shader
    spLambert->use();
    glUniformMatrix4fv(spLambert->u("P"), 1, false, glm::value_ptr(P));
    glUniformMatrix4fv(spLambert->u("V"), 1, false, glm::value_ptr(V));
	glUniform4f(spLambert->u("lightDir"), 255/255.0f, 240/255.0f, 40/255.0f, 1.0f);
	
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(M));
	//floor
	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(glm::mat4(1.0f)));
	glUniform4f(spLambert->u("color"), 0.3f, 0.3f, 0.3f, 1.0f);
	glBindVertexArray(floorVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);


	glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(M));
    glUniform4f(spLambert->u("color"),    228/255.0f, 0/255.0f, 124/255.0f, 1.0f);

	rat.drawSolid();

	glfwSwapBuffers(window);
}

int main(void) {
	GLFWwindow* window;

	glfwSetErrorCallback(error_callback);

	if (!glfwInit()) {
		fprintf(stderr, "Nie można zainicjować GLFW.\n");
		exit(EXIT_FAILURE);
	}

	window = glfwCreateWindow(1920, 1080, "Dojebany projekt terminalowy", NULL, NULL);

	if (!window) {
		fprintf(stderr, "Nie można utworzyć okna.\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (!gladLoadGL(glfwGetProcAddress)) {
		fprintf(stderr, "Nie można zainicjować GLAD.\n");
		exit(EXIT_FAILURE);
	}

	initOpenGLProgram(window);

	glfwSetTime(0);
	while (!glfwWindowShouldClose(window)) {
		if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);
		drawScene(window);
		glfwPollEvents();
	}

	freeOpenGLProgram(window);

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
