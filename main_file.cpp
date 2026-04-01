#define GLM_FORCE_RADIANS

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdlib.h>
#include <stdio.h>
#include "shaderprogram.h"
#include "fileexplorer.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "objmodel.h"
Models::ObjModel rat;
FileExplorer explorer;

//rat
GLuint ratBaseTex;
GLuint ratAtlasTex;
//
struct Rat
{
	glm::vec3 pos = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec2 atlasOffset = glm::vec2(0.0f, 0.0f);
	float angle = 0.0f;
	float speed = 2.0f;
	float turnTimer = 0.0f;
	float turnInterval = 2.0f;
	float lastTime = 0.0f;
};



std::vector<Rat> rats;

//camera :3
float camYaw = 0.0f;
float camPitch = 0.0f;
float ltX = 400.0f;
float ltY = 300.0f;
float startMouse = true;

//floor :3
GLuint floorVAO;

//panel
GLuint panelVAO;

//shadow map
GLuint shadowFBO;
GLuint shadowTex;
const int SHADOW_SIZE = 2048;

//skybox
GLuint skyboxVAO;
GLuint skyboxTex;
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

void initShadowMap() {
    glGenTextures(1, &shadowTex);
    glBindTexture(GL_TEXTURE_2D, shadowTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 SHADOW_SIZE, SHADOW_SIZE, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float border[] = {1,1,1,1};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    glGenFramebuffers(1, &shadowFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

unsigned int loadSkybox() {
	unsigned int texID;
	
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texID);

	// stbi_set_flip_vertically_on_load(false);

	std::vector<std::string> textures = {
		"textures/x_pos.png",
		"textures/x_neg.png",
		"textures/z_pos.png",
		"textures/z_neg.png",
		"textures/y_pos.png",
		"textures/y_neg.png"
	};

	int w, h, nC;
	for (unsigned int i = 0; i < textures.size(); i++) {
		unsigned char* data = stbi_load(textures[i].c_str(), &w, &h, &nC, 3);
		if (data) {
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, w, h, 0,
				GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		} else {
			fprintf(stderr, "Failed to load cubemap face: %s\n  stbi: %s\n",
				textures[i].c_str(), stbi_failure_reason());
			stbi_image_free(data);
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return texID;
}
void geneRatE(int num_rats){
	Rat r;
	for(int i=0; i<num_rats;i++){
		r.pos.x= -20 + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/40));
		r.pos.z= -20 + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/40));
		r.atlasOffset.x= static_cast <float> (rand()) /static_cast <float> (RAND_MAX);
		r.atlasOffset.y= static_cast <float> (rand()) /static_cast <float> (RAND_MAX);
		rats.push_back(r);
	}
}

void initSkybox() {
	float vertices[] = {
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};
	
	GLuint vbo;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &vbo);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	skyboxTex = loadSkybox();
}
void loadRatTexture(){
	glGenTextures(1, &ratBaseTex);
	glBindTexture(GL_TEXTURE_2D, ratBaseTex);
	int w, h, nC;
	unsigned char* data = stbi_load("textures/texture_core.png", &w, &h, &nC, 3);
		if (data) {
			glTexImage2D(
				GL_TEXTURE_2D,
				0, GL_RGB, w, h, 0,
				GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		} else {
			fprintf(stderr, "Failed to load rat texture stbi: %s\n",
				stbi_failure_reason());
			stbi_image_free(data);
		}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

	glGenTextures(1, &ratAtlasTex);
	glBindTexture(GL_TEXTURE_2D, ratAtlasTex);
	data = stbi_load("textures/atlas.png", &w, &h, &nC, 3);
		if (data) {
			glTexImage2D(
				GL_TEXTURE_2D,
				0, GL_RGB, w, h, 0,
				GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		} else {
			fprintf(stderr, "Failed to load atlas texture stbi: %s\n",
				stbi_failure_reason());
			stbi_image_free(data);
		}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
}

void updateRat(Rat *rat) {
    float time = glfwGetTime();
    float dt = time - rat->lastTime;
    rat->lastTime = time;

    rat->turnTimer -= dt;
    if (rat->turnTimer <= 0.0f) {
        rat->angle = ((float)rand() / RAND_MAX) * 2.0f * glm::pi<float>();
        rat->turnTimer = rat->turnInterval + ((float)rand() / RAND_MAX) * 2.0f;
    }

    rat->pos.x += cos(rat->angle) * rat->speed * dt;
    rat->pos.z += sin(rat->angle) * rat->speed * dt;

    float bound = 18.0f;
    rat->pos.x = glm::clamp(rat->pos.x, -bound, bound);
    rat->pos.z = glm::clamp(rat->pos.z, -bound, bound);
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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        explorer.handleKey(key);
    }
}

void error_callback(int error, const char* description) {
	fputs(description, stderr);
}

void initPanel() {
    float w = 3.0f;
    float h = 2.0f;
    float verts[] = {
        -w/2,  h/2,  0.0f, 1.0f,  0.0f, 0.0f,
         w/2,  h/2,  0.0f, 1.0f,  1.0f, 0.0f,
         w/2, -h/2,  0.0f, 1.0f,  1.0f, 1.0f,
        -w/2,  h/2,  0.0f, 1.0f,  0.0f, 0.0f,
         w/2, -h/2,  0.0f, 1.0f,  1.0f, 1.0f,
        -w/2, -h/2,  0.0f, 1.0f,  0.0f, 1.0f,
    };
    GLuint vbo;
    glGenVertexArrays(1, &panelVAO);
    glBindVertexArray(panelVAO);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, false, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, false, 6 * sizeof(float), (void*)(4 * sizeof(float)));
    glBindVertexArray(0);
}

void initOpenGLProgram(GLFWwindow* window) {
	initShaders();
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	rat = Models::ObjModel("RAT1.obj");
	loadRatTexture();
	geneRatE(2);
	rats[0].atlasOffset=glm::vec2(0.5,0.5);
	initFloor();
	initShadowMap();
	initPanel();
	explorer.init("JetBrainsMonoNLNerdFontPropo-Bold.ttf", 512, 384);
	initSkybox();
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void freeOpenGLProgram(GLFWwindow* window) {
	freeShaders();

}

void drawScene(GLFWwindow* window) {
    // macierz światła
    float time = glfwGetTime();
	float lightRadius = 8.0f;
	// glm::vec3 lightPos = glm::vec3(
	// 	sin(time) * lightRadius,
	// 	10.0f,
	// 	cos(time) * lightRadius
	// );

	glm::vec3 lightPos = glm::vec3(
		10.0f,
		10.0f,
		10.0f
	);
    glm::mat4 LP = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 1.0f, 50.0f);
    glm::mat4 LV = glm::lookAt(lightPos, glm::vec3(0,0,0), glm::vec3(0,1,0));
    // glm::mat4 M  = glm::mat4(1.0f);
	

    // shadow map
    glViewport(0, 0, SHADOW_SIZE, SHADOW_SIZE);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    spShadow->use();
    glUniformMatrix4fv(spShadow->u("LP"), 1, false, glm::value_ptr(LP));
    glUniformMatrix4fv(spShadow->u("LV"), 1, false, glm::value_ptr(LV));
    //glUniformMatrix4fv(spShadow->u("M"), 1, false, glm::value_ptr(ratM));
    for (auto& r : rats) {
		glm::mat4 ratM = glm::translate(glm::mat4(1.0f), r.pos);
    	ratM = glm::rotate(ratM, r.angle, glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(spShadow->u("M"), 1, false, glm::value_ptr(ratM));
    	rat.drawSolid();
	}

    glUniformMatrix4fv(spShadow->u("M"), 1, false, glm::value_ptr(glm::mat4(1.0f)));
    glBindVertexArray(floorVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// everything else
    
    glViewport(0, 0, 1920, 1080);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

    glm::mat4 P = glm::perspective(glm::radians(50.0f), 1920.0f/1080.0f, 0.1f, 100.0f);
    glm::mat4 V = glm::lookAt(camPos, camPos + front, glm::vec3(0.0f, 1.0f, 0.0f));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadowTex);

    spLambert->use();
    glUniform1i(spLambert->u("shadowMap"), 0);
    glUniformMatrix4fv(spLambert->u("P"),  1, false, glm::value_ptr(P));
    glUniformMatrix4fv(spLambert->u("V"),  1, false, glm::value_ptr(V));
    glUniformMatrix4fv(spLambert->u("LP"), 1, false, glm::value_ptr(LP));
    glUniformMatrix4fv(spLambert->u("LV"), 1, false, glm::value_ptr(LV));
    glUniform4f(spLambert->u("lightDir"),
        lightPos.x, lightPos.y, lightPos.z, 0.0f);

    // floor
    glUniformMatrix4fv(spLambert->u("M"), 1, false, glm::value_ptr(glm::mat4(1.0f)));
    glUniform4f(spLambert->u("color"), 0.3f, 0.3f, 0.3f, 1.0f);
    glBindVertexArray(floorVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // szczur
	spTl->use();
	glUniformMatrix4fv(spTl->u("LP"), 1, false, glm::value_ptr(LP));
    glUniformMatrix4fv(spTl->u("LV"), 1, false, glm::value_ptr(LV));
	glUniformMatrix4fv(spTl->u("P"), 1, false, glm::value_ptr(P));
    glUniformMatrix4fv(spTl->u("V"), 1, false, glm::value_ptr(V));
	glUniform4f(spTl->u("lightDir"),
        lightPos.x, lightPos.y, lightPos.z, 0.0f);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ratBaseTex);
	glActiveTexture(GL_TEXTURE0+1);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glActiveTexture(GL_TEXTURE0+2);
	glBindTexture(GL_TEXTURE_2D, ratAtlasTex);
	glUniform1i(spTl->u("tex"), 0);
	glUniform1i(spTl->u("shadowMap"), 1);
	glUniform1i(spTl->u("atlas"), 2);	
	for (auto& r : rats) {
		glm::mat4 ratM = glm::translate(glm::mat4(1.0f), r.pos);
		ratM = glm::rotate(ratM,r.angle,glm::vec3(0, 1, 0));
		glUniform2fv(spTl->u("texOffset"), 1, glm::value_ptr(r.atlasOffset));
		glUniformMatrix4fv(spTl->u("M"), 1, false, glm::value_ptr(ratM));
    	rat.drawSolid();
	}
	


	//panel
	glm::mat4 panelM = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 1.5f, 2.0f));
	panelM = glm::rotate(panelM,1.0f,glm::vec3(0,1,0));
    spTexture->use();
    glUniformMatrix4fv(spTexture->u("P"), 1, false, glm::value_ptr(P));
    glUniformMatrix4fv(spTexture->u("V"), 1, false, glm::value_ptr(V));
    glUniformMatrix4fv(spTexture->u("M"), 1, false, glm::value_ptr(panelM));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, explorer.getTexture());
    glUniform1i(spTexture->u("tex"), 0);
    glBindVertexArray(panelVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

	// this needs to go at the end
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);

	spSkybox->use();
	glm::mat4 skyV = glm::mat4(glm::mat3(V));
	glUniformMatrix4fv(spSkybox->u("projection"), 1, false, glm::value_ptr(P));
	glUniformMatrix4fv(spSkybox->u("view"),       1, false, glm::value_ptr(skyV));
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTex);
	glUniform1i(spSkybox->u("skybox"), 0);
	glBindVertexArray(skyboxVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);

	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);    

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
		for(auto& r:rats){
			updateRat(&r);
		}
		drawScene(window);
		glfwPollEvents();
	}

	freeOpenGLProgram(window);

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
