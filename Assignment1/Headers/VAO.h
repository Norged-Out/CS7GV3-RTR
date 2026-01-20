#pragma once

#include<glad/glad.h>
class VBO;

class VAO
{
public:
	// ID reference for the Vertex Array Object
	GLuint ID;
	// Constructor that generates a VAO ID
	VAO();

	// Links a VBO to the VAO using a certain layout for float attributes
	void LinkVBO(VBO& VBO, GLuint layout, GLint numComponents, GLsizei stride, const void* offset);

	// Binds the VAO
	void Bind();
	// Unbinds the VAO
	void Unbind();
	// Deletes the VAO
	void Delete();
};