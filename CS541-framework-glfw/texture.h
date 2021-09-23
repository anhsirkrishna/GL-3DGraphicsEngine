///////////////////////////////////////////////////////////////////////
// A slight encapsulation of an OpenGL texture. This contains a method
// to read an image file into a texture, and methods to bind a texture
// to a shader for use, and unbind when done.
////////////////////////////////////////////////////////////////////////

#ifndef _TEXTURE_
#define _TEXTURE_


// This class reads an image from a file, stores it on the graphics
// card as a texture, and stores the (small integer) texture id which
// identifies it.  It also supplies two methods for binding and
// unbinding the texture to/from a shader.

class Texture
{
 public:
    unsigned int textureId;
    int width, height, depth;
    unsigned char* image;
    Texture(const std::string &filename);

    void Bind(const int unit, const int programId, const std::string& name);
    void Unbind();
    glm::vec3 GetTexel(float u, float v);
};

#endif
