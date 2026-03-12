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


void error_callback(int error, const char* description) {
	fputs(description, stderr);
}

void initOpenGLProgram(GLFWwindow* window) {
	initShaders();
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	rat = Models::ObjModel("RAT1.obj");
}

void freeOpenGLProgram(GLFWwindow* window) {
	freeShaders();

}

void drawScene(GLFWwindow* window) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	float time = glfwGetTime();

	glm::mat4 P = glm::perspective(glm::radians(50.0f), 800.0f/600.0f, 0.1f, 100.0f);
    glm::mat4 V = glm::lookAt(
        glm::vec3(0.0f, 2.0f, 10.0f),
        glm::vec3(0.0f, 0.0f,  0.0f),
        glm::vec3(0.0f, 1.0f,  0.0f)
    );
    glm::mat4 M = glm::mat4(1.0f);
	M = glm::rotate(M, time, glm::vec3(0.0f, 1.0f, 0.0f));

    spLambert->use();
    glUniformMatrix4fv(spLambert->u("P"), 1, false, glm::value_ptr(P));
    glUniformMatrix4fv(spLambert->u("V"), 1, false, glm::value_ptr(V));
    glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(M));
	glUniform4f(spLambert->u("lightDir"), 1.0f, 1.0f, 1.0f, 0.0f);
    glUniform4f(spLambert->u("color"),    0.8f, 0.6f, 0.4f, 1.0f);

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

	window = glfwCreateWindow(800, 600, "Dojebany projekt terminalowy", NULL, NULL);

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
		drawScene(window);
		glfwPollEvents();
	}

	freeOpenGLProgram(window);

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
