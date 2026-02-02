#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <array>

class Shader;
class EXR;
class Cubemap;

class eqr2cmap {
public:
	explicit eqr2cmap(int cubemapSize = 512);
	~eqr2cmap();
	void convert(const EXR& src, Cubemap& dst);

	void renderCube();

private:
	Shader* shader = nullptr;
	GLuint fbo = 0;
	GLuint rbo = 0;
	GLuint cubeVAO = 0;
	GLuint cubeVBO = 0;
	std::array<glm::mat4, 6> views;
	glm::mat4 projection;
	int size = 512;
	void initFramebuffer();
	void initMatrices();
};