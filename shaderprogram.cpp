#include "shaderprogram.h"
ShaderProgram* spLambert = nullptr;
ShaderProgram* spShadow = nullptr;
ShaderProgram* spSkybox = nullptr;
ShaderProgram* spTexture = nullptr;
ShaderProgram* spTl = nullptr;

void initShaders() {
	spLambert = new ShaderProgram("v_lambert.glsl", NULL, "f_lambert.glsl");
	spShadow  = new ShaderProgram("v_shadow.glsl",  NULL, "f_shadow.glsl");
	spSkybox = new ShaderProgram("v_skybox.glsl", NULL, "f_skybox.glsl");
	spTexture = new ShaderProgram("v_texture.glsl", NULL, "f_texture.glsl");
	spTl = new ShaderProgram("v_texturelambert.glsl", NULL, "f_texturelambert.glsl");
}

void freeShaders() {
	delete spLambert;
	delete spShadow;
	delete spSkybox;
	delete spTexture;
}

char* ShaderProgram::readFile(const char* fileName) {
	int filesize;
	FILE* plik;
	char* result;

	plik = fopen(fileName, "rb");
	if (plik != NULL) {
		fseek(plik, 0, SEEK_END);
		filesize = ftell(plik);
		fseek(plik, 0, SEEK_SET);
		result = new char[filesize + 1];
		int readsize = fread(result, 1, filesize, plik);
		result[filesize] = 0;
		fclose(plik);
		return result;
	}
	return NULL;
}

GLuint ShaderProgram::loadShader(GLenum shaderType, const char* fileName) {
	GLuint shader = glCreateShader(shaderType);
	const GLchar* shaderSource = readFile(fileName);
	glShaderSource(shader, 1, &shaderSource, NULL);
	glCompileShader(shader);
	delete[] shaderSource;

	int infologLength = 0;
	int charsWritten  = 0;
	char* infoLog;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLength);
	if (infologLength > 1) {
		infoLog = new char[infologLength];
		glGetShaderInfoLog(shader, infologLength, &charsWritten, infoLog);
		printf("%s\n", infoLog);
		delete[] infoLog;
	}
	return shader;
}

ShaderProgram::ShaderProgram(const char* vertexShaderFile, const char* geometryShaderFile, const char* fragmentShaderFile) {
	printf("Loading vertex shader...\n");
	vertexShader = loadShader(GL_VERTEX_SHADER, vertexShaderFile);

	if (geometryShaderFile != NULL) {
		printf("Loading geometry shader...\n");
		geometryShader = loadShader(GL_GEOMETRY_SHADER, geometryShaderFile);
	} else {
		geometryShader = 0;
	}

	printf("Loading fragment shader...\n");
	fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentShaderFile);

	shaderProgram = glCreateProgram();

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	if (geometryShaderFile != NULL) glAttachShader(shaderProgram, geometryShader);
	glLinkProgram(shaderProgram);

	int infologLength = 0;
	int charsWritten  = 0;
	char* infoLog;

	glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &infologLength);
	if (infologLength > 1) {
		infoLog = new char[infologLength];
		glGetProgramInfoLog(shaderProgram, infologLength, &charsWritten, infoLog);
		printf("%s\n", infoLog);
		delete[] infoLog;
	}

	printf("Shader program created\n");
}

ShaderProgram::~ShaderProgram() {
	glDetachShader(shaderProgram, vertexShader);
	if (geometryShader != 0) glDetachShader(shaderProgram, geometryShader);
	glDetachShader(shaderProgram, fragmentShader);

	glDeleteShader(vertexShader);
	if (geometryShader != 0) glDeleteShader(geometryShader);
	glDeleteShader(fragmentShader);

	glDeleteProgram(shaderProgram);
}

void ShaderProgram::use() {
	glUseProgram(shaderProgram);
}

GLuint ShaderProgram::u(const char* variableName) {
	return glGetUniformLocation(shaderProgram, variableName);
}

GLuint ShaderProgram::a(const char* variableName) {
	return glGetAttribLocation(shaderProgram, variableName);
}
