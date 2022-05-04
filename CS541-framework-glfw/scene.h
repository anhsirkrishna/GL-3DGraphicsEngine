////////////////////////////////////////////////////////////////////////
// The scene class contains all the parameters needed to define and
// draw a simple scene, including:
//   * Geometry
//   * Light parameters
//   * Material properties
//   * Viewport size parameters
//   * Viewing transformation values
//   * others ...
//
// Some of these parameters are set when the scene is built, and
// others are set by the framework in response to user mouse/keyboard
// interactions.  All of them can be used to draw the scene.

#include "shapes.h"
#include "object.h"
#include "texture.h"
#include "fbo.h"

enum ObjectIds {
    nullId = 0,
    skyId = 1,
    seaId = 2,
    groundId = 3,
    roomId = 4,
    boxId = 5,
    frameId = 6,
    lPicId = 7,
    rPicId = 8,
    teapotId = 9,
    spheresId = 10,
    floorId = 11
};

class Shader;


struct HBlock {
    float N;
    float hammersly[2 * 40];
};

class Scene
{
public:
    GLFWwindow* window;

    // @@ Declare interactive viewing variables here. (spin, tilt, ry, front back, ...)
    float spin, tilt, ry, tx, ty, zoom, front, back;


    const float speed = 20.0; //Keeps track of how fast we are in gamelike mode
    glm::vec3 eye; //Vector3 which keeps track of the position of the eye

    bool w_down, a_down, s_down, d_down; //Bool to check if the WASD keys are down
    bool gamelike_mode = false; //Bool to keep track of which mode we are using gamelike vs worldview

    float time_at_prev_frame = 0.0f;//Float to keep track of the time at the previous frame.

    // Light parameters
    float lightSpin, lightTilt, lightDist;
    glm::vec3 lightPos;
    // @@ Perhaps declare additional scene lighting values here. (lightVal, lightAmb)
    glm::vec3 light, ambient;

    glm::vec3 reflectionEye;

    int lightingMode = 3;
    int reflectionMode = 3;
    int mode; // Extra mode indicator hooked up to number keys and sent to shader

    int debug_mode = 0;

    // Viewport
    int width, height;

    // Transformations
    glm::mat4 WorldProj, WorldView, WorldInverse, LightView, LightProj;

    // All objects in the scene are children of this single root object.
    Object* objectRoot;
    Object* central, * anim, * room, * floor, * teapot, * podium, * sky,
        * ground, * sea, * spheres, * leftFrame, * rightFrame;
    Object* small_sphere;
    Object* small_sphere_2;
    Object* small_sphere_3;
    Object* small_sphere_4;

    std::vector<Object*> animated;
    ProceduralGround* proceduralground;
    std::vector<Object*> LocalLights;
    std::vector<glm::vec3> local_light_positions;
    std::vector<float> local_light_radii;
    float local_light_range;

    // Shader programs
    ShaderProgram* lightingProgram;
    ShaderProgram* shadowProgram;
    ShaderProgram* reflectionProgram;
    ShaderProgram* gbufferProgram;
    ShaderProgram* localLightsProgram;
    ShaderProgram* shadowBlur_H_Program;
    ShaderProgram* shadowBlur_V_Program;
    ShaderProgram* AOProgram;
    ShaderProgram* bilinear_H_Program;
    ShaderProgram* bilinear_V_Program;
    ShaderProgram* postProcessing_Program;
    ShaderProgram* postProcessing_Compute;
    // @@ Declare additional shaders if necessary

    //FBO decleration
    FBO shadowPassRenderTarget, upperReflectionRenderTarget, lowerReflectionRenderTarget, gbufferRenderTarget;
    FBO AORenderTarget;
    int fbo_width, fbo_height;

    glm::mat4 BMatrix, ShadowMatrix;

    Texture* p_sky_dome;
    Texture* p_sky_dome_cage;
    Texture* p_sky_dome_night;
    Texture* p_barca_sky;
    Texture* p_barca_irr_map;
    Texture* p_mon_valley_sky;
    Texture* p_mon_valley_irr_map;

    int sky_dome_mode = 2;
    int texture_mode = 1;
    int draw_fbo = 14;
    int local_lights_on = 0;

    int ao_enabled = 1;
    int tone_map_mode = 1;
    // Options menu stuff
    bool show_demo_window;

    int ao_sample_count = 20;
    float ao_range = 1.0f;
    float ao_scale = 1;
    float ao_contrast = 1;

    //Deferred shading reqs
    GLuint screen_quad_vao;

    int shadow_blur_kernel_width;
    GLuint blur_kernel_block_id;
    GLuint bilinear_kernel_block_id;
    FBO shadowBlurOutput;
    FBO bilinearFilterOutput;
    FBO bilinearFilterOutput_2;
    FBO postProcessingBuffer;
    int kernel_width = 3;
    int bilinear_kernel_width = 3;
    int bloom_kernerl_width = 10;
    float exposure = 8.0f;
    float gamma = 2.2f;

    float bloom_threshold = 0.6f;
    int bloom_pass_count = 20;
    int bloom_enabled = 1;

    std::vector<float> kernel_vals;
    std::vector<float> bilinear_kernel_vals;

    HBlock h_block;
    int sampling_count = 20;
    GLuint h_block_id;

    int sky_dome_width;
    int sky_dome_height;

    void InitializeScene();
    void BuildTransforms();
    void DrawMenu();
    void DrawScene();
    void CreateFullScreenQuad();
    void DrawFullScreenQuad();
    void CreateLocalLights(Shape* SpherePolygons);
    void DrawLocalLights(ShaderProgram* program);
    void RebuildGbuffer(int w, int h);
    void RecalculateKernel();
    void RecalculateBloomKernel();
    void RecalculateBilinearKernel();
    void RecalculateHBlock();
};
