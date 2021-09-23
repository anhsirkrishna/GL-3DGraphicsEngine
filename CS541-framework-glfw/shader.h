///////////////////////////////////////////////////////////////////////
// A slight encapsulation of a shader program. This contains methods
// to build a shader program from multiples files containing vertex
// and pixel shader code, and a method to link the result.  When
// loaded (method "Use"), its vertex shader and pixel shader will be
// invoked for all geometry passing through the graphics pipeline.
// When done, unload it with method "Unuse".
////////////////////////////////////////////////////////////////////////

class ShaderProgram
{
public:
    int programId;
    
    ShaderProgram();
    void AddShader(const char* fileName, const GLenum type);
    void LinkProgram();
    void Use();
    void Unuse();
};
