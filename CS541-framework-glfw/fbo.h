///////////////////////////////////////////////////////////////////////
// A slight encapsulation of a Frame Buffer Object (i'e' Render
// Target) and its associated texture.  When the FBO is "Bound", the
// output of the graphics pipeline is captured into the texture.  When
// it is "Unbound", the texture is available for use as any normal
// texture.
////////////////////////////////////////////////////////////////////////

class FBO {
public:
    unsigned int fboID=0;
    unsigned int textureID[4] = {0, 0, 0, 0};
    int width, height;  // Size of the texture.
    unsigned int color_attachment_count;
    void CreateFBO(const int w, const int h, const int _color_attachment_count=1);
    void Bind();
    void Unbind();
    void BindTexture(const int program_id, const int texture_unit, const char* var_name, const int color_attachment=0);
    void UnbindTexture(const int texture_unit);
    void Resize(const int w, const int h);
    void DeleteFBO();
};
