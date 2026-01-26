#include "data_base.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <GLFW/glfw3.h>

void DataBase::loadIcones(std::string filename) {
	int w;
	int h;
	int comp;

	unsigned char* image = stbi_load(filename.c_str(), &w, &h, &comp, STBI_rgb);

	if (image == nullptr)
		throw(std::string("Failed to load texture"));

	GLuint m_texture;
	glGenTextures(1, &m_texture);
	textureId = m_texture;
	textureSize.x = w;
	textureSize.y = h;

	glBindTexture(GL_TEXTURE_2D, m_texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if (comp == 3)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	else if (comp == 4)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(image);
}

DataBase::~DataBase() {
	GLuint m_texture = textureId;
	glDeleteTextures(1, &m_texture);
}