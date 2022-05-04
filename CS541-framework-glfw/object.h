////////////////////////////////////////////////////////////////////////
// A lightweight class representing an instance of an object that can
// be drawn onscreen.  An Object consists of a shape (batch of
// triangles), and various transformation, color and texture
// parameters.  It also contains a list of child objects to be drawn
// in a hierarchical fashion under the control of parent's
// transformations.
//
// Methods consist of a constructor, and a Draw procedure, and an
// append for building hierarchies of objects.

#ifndef _OBJECT
#define _OBJECT

#include "shapes.h"
#include "texture.h"
#include <utility>              // for pair<Object*,glm::mat4>

class Shader;
class Object;

typedef std::pair<Object*,glm::mat4> INSTANCE;

// Object:: A shape, and its transformations, colors, and textures and sub-objects.
class Object
{
 public:
    Shape* shape;               // Polygons 
    glm::mat4 animTr;                // This model's animation transformation
    int objectId;               // Object id to be sent to the shader
    bool drawMe;                // Toggle specifies if this object (and children) are drawn.

    glm::vec3 diffuseColor;          // Diffuse color of object
    glm::vec3 specularColor;         // Specular color of object
    float shininess;            // Surface roughness value
	bool reflective;			//To determine if the object is reflective

    std::vector<INSTANCE> instances; // Pairs of sub-objects and transformations 

    int textureId;
    int textureUnit;
    
    int nmapId;
    int nmapUnit;

    glm::vec3 brightness;

    Object(Shape* _shape, const int objectId,
           const glm::vec3 _d=glm::vec3(), const glm::vec3 _s=glm::vec3(), const float _n=1,
		   const bool _reflective=false, const int _texId=-1, const int texUnit = -1, 
           const int _nmapId=-1, const int _nmapUnit=-1, glm::vec3 &_brightness=glm::vec3(0.0, 0.0, 0.0));

    // If this object is to be drawn with a texture, this is a good
    // place to store the texture id (a small positive integer).  The
    // texture id should be set in Scene::InitializeScene and used in
    // Object::Draw.
    
    void Draw(ShaderProgram* program, glm::mat4& objectTr);

    void add(Object* m, glm::mat4 tr=glm::mat4()) { instances.push_back(std::make_pair(m,tr)); }
};

#endif
