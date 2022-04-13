////////////////////////////////////////////////////////////////////////
// The scene class contains all the parameters needed to define and
// draw a simple scene, including:
//   * Geometry
//   * Light parameters
//   * Material properties
//   * viewport size parameters
//   * Viewing transformation values
//   * others ...
//
// Some of these parameters are set when the scene is built, and
// others are set by the framework in response to user mouse/keyboard
// interactions.  All of them can be used to draw the scene.

const bool fullPolyCount = true; // Use false when emulating the graphics pipeline in software

#include "math.h"
#include <iostream>
#include <stdlib.h>

#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
using namespace gl;

#include <glu.h>                // For gluErrorString

#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/ext.hpp>          // For printing GLM objects with to_string

#include "framework.h"
#include "shapes.h"
#include "object.h"
#include "texture.h"
#include "transform.h"

const float PI = 3.14159f;
const float rad = PI/180.0f;    // Convert degrees to radians

glm::mat4 Identity;

const float grndSize = 100.0;    // Island radius;  Minimum about 20;  Maximum 1000 or so
const float grndOctaves = 4.0;  // Number of levels of detail to compute
const float grndFreq = 0.03;    // Number of hills per (approx) 50m
const float grndPersistence = 0.03; // Terrain roughness: Slight:0.01  rough:0.05
const float grndLow = -3.0;         // Lowest extent below sea level
const float grndHigh = 5.0;        // Highest extent above sea level

////////////////////////////////////////////////////////////////////////
// This macro makes it easy to sprinkle checks for OpenGL errors
// throughout your code.  Most OpenGL calls can record errors, and a
// careful programmer will check the error status *often*, perhaps as
// often as after every OpenGL call.  At the very least, once per
// refresh will tell you if something is going wrong.
#define CHECKERROR {GLenum err = glGetError(); if (err != GL_NO_ERROR) { fprintf(stderr, "OpenGL error (at line scene.cpp:%d): %s\n", __LINE__, gluErrorString(err)); exit(-1);} }

// Create an RGB color from human friendly parameters: hue, saturation, value
glm::vec3 HSV2RGB(const float h, const float s, const float v)
{
    if (s == 0.0)
        return glm::vec3(v,v,v);

    int i = (int)(h*6.0) % 6;
    float f = (h*6.0f) - i;
    float p = v*(1.0f - s);
    float q = v*(1.0f - s*f);
    float t = v*(1.0f - s*(1.0f-f));
    if      (i == 0)     return glm::vec3(v,t,p);
    else if (i == 1)  return glm::vec3(q,v,p);
    else if (i == 2)  return glm::vec3(p,v,t);
    else if (i == 3)  return glm::vec3(p,q,v);
    else if (i == 4)  return glm::vec3(t,p,v);
    else   /*i == 5*/ return glm::vec3(v,p,q);
}

////////////////////////////////////////////////////////////////////////
// Constructs a hemisphere of spheres of varying hues
Object* SphereOfSpheres(Shape* SpherePolygons)
{
    Object* ob = new Object(NULL, nullId);
    
    for (float angle=0.0;  angle<360.0;  angle+= 18.0)
        for (float row=0.075;  row<PI/2.0;  row += PI/2.0/6.0) {   
            glm::vec3 hue = HSV2RGB(angle/360.0, 1.0f-2.0f*row/PI, 1.0f);

            Object* sp = new Object(SpherePolygons, spheresId,
                                    hue, glm::vec3(1.0, 1.0, 1.0), 0.128); //phong alpha = 120
            float s = sin(row);
            float c = cos(row);
            ob->add(sp, Rotate(2,angle)*Translate(c,0,s)*Scale(0.075*c,0.075*c,0.075*c));
        }
    return ob;
}

////////////////////////////////////////////////////////////////////////
// Constructs a -1...+1  quad (canvas) framed by four (elongated) boxes
Object* FramedPicture(const glm::mat4& modelTr, const int objectId, 
                      Shape* BoxPolygons, Shape* QuadPolygons, int textureId, 
                      int textureUnit)
{
    // This draws the frame as four (elongated) boxes of size +-1.0
    float w = 0.05;             // Width of frame boards.
    
    Object* frame = new Object(NULL, nullId);
    Object* ob;
    
    glm::vec3 woodColor(87.0/255.0,51.0/255.0,35.0/255.0);
    ob = new Object(BoxPolygons, frameId,
                    woodColor, glm::vec3(0.02, 0.02, 0.02), 0.408); //phong alpha is 10
    frame->add(ob, Translate(0.0, 0.0, 1.0+w)*Scale(1.0, w, w));
    frame->add(ob, Translate(0.0, 0.0, -1.0-w)*Scale(1.0, w, w));
    frame->add(ob, Translate(1.0+w, 0.0, 0.0)*Scale(w, w, 1.0+2*w));
    frame->add(ob, Translate(-1.0-w, 0.0, 0.0)*Scale(w, w, 1.0+2*w));

    ob = new Object(QuadPolygons, objectId,
                    woodColor, glm::vec3(0.0, 0.0, 0.0), 0.408, false, textureId, textureUnit); //phong alpha is 10
    frame->add(ob, Rotate(0,90));

    return frame;
}

////////////////////////////////////////////////////////////////////////
// InitializeScene is called once during setup to create all the
// textures, shape VAOs, and shader programs as well as setting a
// number of other parameters.
void Scene::InitializeScene()
{
    debug_mode = 0;

    glEnable(GL_DEPTH_TEST);
    CHECKERROR;

    // @@ Initialize interactive viewing variables here. (spin, tilt, ry, front back, ...)
    spin = 21.0;
    tilt = 30.0;
    ry = 0.4;
    tx = ty = 2.0;
    zoom = -40.0;
    front = 0.5;
    back = 5000.0;
    eye = glm::vec3(0.0, 0.0, 0.0);
    // Set initial light parameters
    lightSpin = 150.0;
    lightTilt = -45.0;
    lightDist = 100.0;
    // @@ Perhaps initialize additional scene lighting values here. (lightVal, lightAmb)
    light = glm::vec3(3.0, 3.0, 3.0);
    ambient = glm::vec3(0.2, 0.2, 0.2);

    CHECKERROR;
    objectRoot = new Object(NULL, nullId);

    BMatrix = Translate(0.5, 0.5, 0.5) * Scale(0.5, 0.5, 0.5);

    // Enable OpenGL depth-testing
    glEnable(GL_DEPTH_TEST);

    // Create the lighting shader program from source code files.
    // @@ Initialize additional shaders if necessary
    lightingProgram = new ShaderProgram();
    lightingProgram->AddShader("final.vert", GL_VERTEX_SHADER);
	lightingProgram->AddShader("final.frag", GL_FRAGMENT_SHADER);

    glBindAttribLocation(lightingProgram->programId, 0, "vertex");
    glBindAttribLocation(lightingProgram->programId, 1, "vertexNormal");
    glBindAttribLocation(lightingProgram->programId, 2, "vertexTexture");
    glBindAttribLocation(lightingProgram->programId, 3, "vertexTangent");
    lightingProgram->LinkProgram();

    // Create the shadow shader program from source code files.
    shadowProgram = new ShaderProgram();
    shadowProgram->AddShader("shadow.vert", GL_VERTEX_SHADER);
    shadowProgram->AddShader("shadow.frag", GL_FRAGMENT_SHADER);

    glBindAttribLocation(shadowProgram->programId, 0, "vertex");
    glBindAttribLocation(shadowProgram->programId, 1, "vertexNormal");
    glBindAttribLocation(shadowProgram->programId, 2, "vertexTexture");
    glBindAttribLocation(shadowProgram->programId, 3, "vertexTangent");
    shadowProgram->LinkProgram();

    //Create the FBO as the render target for the shadow pass
    fbo_width = 1024;
    fbo_height = 1024;
    shadowPassRenderTarget.CreateFBO(fbo_width, fbo_height);

	// Create the reflection shader program 
	reflectionProgram = new ShaderProgram();
	reflectionProgram->AddShader("reflection.vert", GL_VERTEX_SHADER);
	reflectionProgram->AddShader("reflection.frag", GL_FRAGMENT_SHADER);
	reflectionProgram->AddShader("lighting.vert", GL_VERTEX_SHADER);
	reflectionProgram->AddShader("lighting.frag", GL_FRAGMENT_SHADER);

	glBindAttribLocation(reflectionProgram->programId, 0, "vertex");
	glBindAttribLocation(reflectionProgram->programId, 1, "vertexNormal");
	glBindAttribLocation(reflectionProgram->programId, 2, "vertexTexture");
	glBindAttribLocation(reflectionProgram->programId, 3, "vertexTangent");
	reflectionProgram->LinkProgram();
	reflectionProgram->isReflectionShader = true;

	//Create the two FBOs require for reflection, one for each parabaloid
	upperReflectionRenderTarget.CreateFBO(fbo_width, fbo_height);
	lowerReflectionRenderTarget.CreateFBO(fbo_width, fbo_height);

    // Create the shader program for deferred shading pass
    gbufferProgram = new ShaderProgram();
    gbufferProgram->AddShader("gbuffer.vert", GL_VERTEX_SHADER);
    gbufferProgram->AddShader("gbuffer.frag", GL_FRAGMENT_SHADER);

    glBindAttribLocation(gbufferProgram->programId, 0, "vertex");
    glBindAttribLocation(gbufferProgram->programId, 1, "vertexNormal");
    glBindAttribLocation(gbufferProgram->programId, 2, "vertexTexture");
    glBindAttribLocation(gbufferProgram->programId, 3, "vertexTangent");
    gbufferProgram->LinkProgram();

    // Create the compute shader program for shadow map blur
    shadowBlur_H_Program = new ShaderProgram();
    shadowBlur_H_Program->AddShader("shadow_horizontal.comp", GL_COMPUTE_SHADER);
    shadowBlur_H_Program->LinkProgram();

    shadowBlur_V_Program = new ShaderProgram();
    shadowBlur_V_Program->AddShader("shadow_vertical.comp", GL_COMPUTE_SHADER);
    shadowBlur_V_Program->LinkProgram();

    glGenBuffers(1, &blur_kernel_block_id); // Generates block 
    int bindpoint = 0; // Start at zero, increment for other blocks

    glBindBuffer(GL_UNIFORM_BUFFER, blur_kernel_block_id);
    glBindBufferBase(GL_UNIFORM_BUFFER, bindpoint, blur_kernel_block_id);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(float) * 101, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    GLuint loc = glGetUniformBlockIndex(shadowBlur_H_Program->programId, "blurKernel");
    glUniformBlockBinding(shadowBlur_H_Program->programId, loc, bindpoint);

    loc = glGetUniformBlockIndex(shadowBlur_V_Program->programId, "blurKernel");
    glUniformBlockBinding(shadowBlur_V_Program->programId, loc, bindpoint);

    //Create the FBO as the output texture for the shadow blue
    shadowBlurOutput.CreateFBO(fbo_width, fbo_height);

    //Create the FBO for the gbuffer used in deferred shading
    gbufferRenderTarget.CreateFBO(750, 750, 4);

    //Create a uniform block for the hammersly block
    glGenBuffers(1, &h_block_id);
    glBindBuffer(GL_UNIFORM_BUFFER, h_block_id);
    bindpoint += 1;
    glBindBufferBase(GL_UNIFORM_BUFFER, bindpoint, h_block_id);
    size_t h_block_size = sizeof(h_block);
    glBufferData(GL_UNIFORM_BUFFER, h_block_size, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    loc = glGetUniformBlockIndex(lightingProgram->programId, "HammersleyBlock");
    glUniformBlockBinding(lightingProgram->programId, loc, bindpoint);

    sampling_count = 20;
    h_block.N = 20;
    RecalculateHBlock();

    glBindBuffer(GL_UNIFORM_BUFFER, h_block_id);
    h_block_size = sizeof(h_block);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, h_block_size, &h_block);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    CHECKERROR;

    //Create a compute shader for AO pass
    AOProgram = new ShaderProgram();
    AOProgram->AddShader("ao.comp", GL_COMPUTE_SHADER);
    AOProgram->LinkProgram();

    AORenderTarget.CreateFBO(750, 750);

    // Create the compute shader program for bilinear filter
    bilinear_H_Program = new ShaderProgram();
    bilinear_H_Program->AddShader("bilinear_filter_horizontal.comp", GL_COMPUTE_SHADER);
    bilinear_H_Program->LinkProgram();

    bilinear_V_Program = new ShaderProgram();
    bilinear_V_Program->AddShader("bilinear_filter_vertical.comp", GL_COMPUTE_SHADER);
    bilinear_V_Program->LinkProgram();

    glGenBuffers(1, &bilinear_kernel_block_id); // Generates block 
    bindpoint++; // Start at zero, increment for other blocks

    glBindBuffer(GL_UNIFORM_BUFFER, bilinear_kernel_block_id);
    glBindBufferBase(GL_UNIFORM_BUFFER, bindpoint, bilinear_kernel_block_id);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(float) * 101, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    loc = glGetUniformBlockIndex(bilinear_H_Program->programId, "blurKernel");
    glUniformBlockBinding(bilinear_H_Program->programId, loc, bindpoint);

    loc = glGetUniformBlockIndex(bilinear_V_Program->programId, "blurKernel");
    glUniformBlockBinding(bilinear_V_Program->programId, loc, bindpoint);

    //Create the FBO as the output texture for the shadow blue
    bilinearFilterOutput.CreateFBO(750, 750);
    bilinearFilterOutput_2.CreateFBO(750, 750);

    // Create the shader program for Local lights pass
    localLightsProgram = new ShaderProgram();
    localLightsProgram->AddShader("local_lights.vert", GL_VERTEX_SHADER);
    localLightsProgram->AddShader("local_lights.frag", GL_FRAGMENT_SHADER);

    glBindAttribLocation(localLightsProgram->programId, 0, "vertex");
    glBindAttribLocation(localLightsProgram->programId, 1, "vertexNormal");
    glBindAttribLocation(localLightsProgram->programId, 2, "vertexTexture");
    glBindAttribLocation(localLightsProgram->programId, 3, "vertexTangent");
    localLightsProgram->LinkProgram();

    // Create all the Polygon shapes
    proceduralground = new ProceduralGround(grndSize, 400,
                                     grndOctaves, grndFreq, grndPersistence,
                                     grndLow, grndHigh);
    
    Shape* TeapotPolygons =  new Teapot(fullPolyCount?12:2);
    Shape* BoxPolygons = new Box();
    Shape* SpherePolygons = new Sphere(32);
    Shape* RoomPolygons = new Ply("room.ply");
    Shape* FloorPolygons = new Plane(10.0, 10);
    Shape* QuadPolygons = new Quad();
    Shape* SeaPolygons = new Plane(2000.0, 50);
    Shape* GroundPolygons = proceduralground;

    // Various colors used in the subsequent models
    glm::vec3 woodColor(87.0/255.0, 51.0/255.0, 35.0/255.0);
    glm::vec3 brickColor(134.0/255.0, 60.0/255.0, 56.0/255.0);
    glm::vec3 floorColor(6*16/255.0, 5.5*16/255.0, 3*16/255.0);
    glm::vec3 brassColor(0.5, 0.5, 0.1);
    glm::vec3 grassColor(62.0/255.0, 102.0/255.0, 38.0/255.0);
    glm::vec3 waterColor(0.3, 0.3, 1.0);
    glm::vec3 skyColor(133.0 / 255.0, 222.0 / 255.0, 255.0 / 255.0);
    glm::vec3 greymetalColor(70.0 / 255.0, 71.0 / 255.0, 62.0 / 255.0);

    glm::vec3 black(0.0, 0.0, 0.0);
    glm::vec3 brightSpec(0.05, 0.05, 0.05);
    glm::vec3 polishedSpec(0.03, 0.03, 0.03);
    glm::vec3 brushedSpec1(0.1, 0.1, 0.1);
    glm::vec3 brushedSpec2(0.2, 0.2, 0.2);
    glm::vec3 brushedSpec3(0.8, 0.8, 0.8);
    glm::vec3 brushedSpec4(1.0, 1.0, 1.0);
 
    // Creates all the models from which the scene is composed.  Each
    // is created with a polygon shape (possibly NULL), a
    // transformation, and the surface lighting parameters Kd, Ks, and
    // alpha.

    // @@ This is where you could read in all the textures and
    // associate them with the various objects being created in the
    // next dozen lines of code.

    // @@ To change an object's surface parameters (Kd, Ks, or alpha),
    // modify the following lines.
    
    // Grass texture from https://opengameart.org/content/tileable-dirt-textures
    Texture grassTexture(".\\textures\\Dirt_01.jpg", true);
    Texture grassNMap(".\\textures\\Dirt_01_Nrm.jpg", true);

    //Crate texture from https://opengameart.org/content/3-crate-textures-w-bump-normal
    Texture crateTexture(".\\textures\\crate1_diffuse.jpg", false);
    Texture crateNMap(".\\textures\\crate1_normal.jpg", false);
    
    //Floor texture from https://opengameart.org/content/117-stone-wall-tilable-textures-in-8-themes
    Texture floorTexture(".\\textures\\6670-diffuse.jpg", true);
    Texture floorNMap(".\\textures\\6670-normal.jpg", true);

    //Wall texture from https://opengameart.org/content/pixars-textures
    Texture wallTexture(".\\textures\\Black_glazed_tile_pxr128.jpg");
    Texture wallNMap(".\\textures\\Black_glazed_tile_pxr128_normal.jpg");

    //Teapot texture from https://opengameart.org/content/rusted-metal-texture-pack
    Texture teapotTexture(".\\textures\\metall010-new-tileable-bar.jpg");
    Texture teapotNMap(".\\textures\\metall010-new-tileable-bar-nm.jpg");

    Texture gooseTexture(".\\textures\\goose.jpg");

    Texture waterNMap(".\\textures\\ripples_normalmap.jpg");
    

    central    = new Object(NULL, nullId);
    anim       = new Object(NULL, nullId);
    room       = new Object(RoomPolygons, roomId, brickColor, black, 0.817, false, wallTexture.textureId, 0, wallNMap.textureId, 1); //phong alpha = 1
    floor      = new Object(FloorPolygons, floorId, floorColor, black, 0.817, false, floorTexture.textureId, 2, floorNMap.textureId, 3); //phong alpha = 1
    teapot     = new Object(TeapotPolygons, teapotId, brassColor, brightSpec, 0.128, true, teapotTexture.textureId, 4, teapotNMap.textureId, 5); //phong alpha = 120 | Reflective set to true
	reflectionEye = glm::vec3(0, 0, 1.5);
    podium     = new Object(BoxPolygons, boxId, glm::vec3(woodColor), polishedSpec, 0.408, false, crateTexture.textureId, 6, crateNMap.textureId, 7); //phong alpha = 10 
    small_sphere = new Object(SpherePolygons, spheresId, black, brushedSpec4, 0.8); //phong alpha = 10
    small_sphere_2 = new Object(SpherePolygons, spheresId, black, brushedSpec4, 0.5); //phong alpha = 10
    small_sphere_3 = new Object(SpherePolygons, spheresId, black, brushedSpec4, 0.1); //phong alpha = 10
    small_sphere_4 = new Object(SpherePolygons, spheresId, black, brushedSpec4, 0); //phong alpha = 10
    sky        = new Object(SpherePolygons, skyId, black, black, 1); //phong alpha = 0
    ground     = new Object(GroundPolygons, groundId, grassColor, black, 0.817, false, grassTexture.textureId, 8, grassNMap.textureId, 9); //phong alpha = 1
    sea        = new Object(SeaPolygons, seaId, waterColor, brightSpec, 0.128, false, -1, -1, waterNMap.textureId, 10); //phong alpha = 120
    leftFrame  = FramedPicture(Identity, lPicId, BoxPolygons, QuadPolygons, gooseTexture.textureId, 11);
    rightFrame = FramedPicture(Identity, rPicId, BoxPolygons, QuadPolygons, gooseTexture.textureId, 12);
    spheres    = SphereOfSpheres(SpherePolygons);
#ifdef REFL
    spheres->drawMe = true;
#else
    spheres->drawMe = false;
#endif


    // @@ To change the scene hierarchy, examine the hierarchy created
    // by the following object->add() calls and adjust as you wish.
    // The objects being manipulated and their polygon shapes are
    // created above here.

    // Scene is composed of sky, ground, sea, room and some central models
    if (fullPolyCount) {
        objectRoot->add(sky, Scale(2000.0, 2000.0, 2000.0));
        objectRoot->add(sea); 
        objectRoot->add(ground); }
    objectRoot->add(central);
#ifndef REFL
    objectRoot->add(room,  Translate(0.0, 0.0, 0.02));
#endif
    objectRoot->add(floor, Translate(0.0, 0.0, 0.02));

    // Central model has a rudimentary animation (constant rotation on Z)
    animated.push_back(anim);

    // Central contains a teapot on a podium and an external sphere of spheres
    central->add(podium, Translate(0.0, 0,0));
    central->add(anim, Translate(0.0, 0,0));

    central->add(small_sphere, Translate(2.0, 2.0, 2) * Scale(0.5, 0.5, 0.5));
    central->add(small_sphere_2, Translate(2.0, -2.0, 2) * Scale(0.5, 0.5, 0.5));
    central->add(small_sphere_3, Translate(-2.0, 2.0, 2) * Scale(0.5, 0.5, 0.5));
    central->add(small_sphere_4, Translate(-2.0, -2.0, 2) * Scale(0.5, 0.5, 0.5));

    anim->add(teapot, Translate(0.1, 0.0, 1.5)*TeapotPolygons->modelTr);
    if (fullPolyCount)
        anim->add(spheres, Translate(0.0, 0.0, 0.0)*Scale(16, 16, 16));
    
    // Room contains two framed pictures
    if (fullPolyCount) {
        room->add(leftFrame, Translate(-1.5, 9.85, 1.)*Scale(0.8, 0.8, 0.8));
        room->add(rightFrame, Translate( 1.5, 9.85, 1.)*Scale(0.8, 0.8, 0.8)); }

    CHECKERROR;

    // Options menu stuff
    show_demo_window = false;

    p_sky_dome_cage = new Texture(".\\textures\\cages.jpg");
    //Skydome texture from https://vwartclub.com/?section=xfree3d&category=hdri&article=xfree3d-hdri-shop-s84-low-cloudy-1836
    p_sky_dome = new Texture(".\\textures\\Sky.jpg");
    p_barca_sky = new Texture(".\\textures\\Barce_Rooftop_C_3k.hdr", true);
    p_irr_map = new Texture(".\\textures\\Barce_Rooftop_C_3k.irr.hdr", true);
    //p_barca_sky = new Texture(".\\textures\\MonValley_A_LookoutPoint_2k.hdr");
    //p_irr_map = new Texture(".\\textures\\MonValley_A_LookoutPoint_2k.irr.hdr");
    //Create a full screen quad to render for the deferred shading pass.
    CreateFullScreenQuad();
    CreateLocalLights(SpherePolygons);
}

void Scene::DrawMenu()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (ImGui::BeginMainMenuBar()) {
        // This menu demonstrates how to provide the user a list of toggleable settings.
        if (ImGui::BeginMenu("Objects")) {
            if (ImGui::MenuItem("Draw spheres", "", spheres->drawMe))  {spheres->drawMe ^= true; }
            if (ImGui::MenuItem("Draw walls", "", room->drawMe))       {room->drawMe ^= true; }
            if (ImGui::MenuItem("Draw ground", "", ground->drawMe)){ground->drawMe ^= true;}
            if (ImGui::MenuItem("Draw sea", "", sea->drawMe)) { sea->drawMe ^= true; }
            ImGui::EndMenu(); }
        
        if (ImGui::BeginMenu("Lighting ")) {
            if (ImGui::MenuItem("Phong", "", lightingMode == 0)) { lightingMode = 0; }
            if (ImGui::MenuItem("BRDF", "", lightingMode == 1)) { lightingMode = 1; }
            if (ImGui::MenuItem("GGX", "", lightingMode == 2)) { lightingMode = 2; }
            if (ImGui::MenuItem("IBL", "", lightingMode == 3)) { lightingMode = 3; }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Local Lights ")) {
            if (ImGui::MenuItem("On", "", local_lights_on == 1)) { local_lights_on = 1; }
            if (ImGui::MenuItem("Off", "", local_lights_on == 0)) { local_lights_on = 0; }
            ImGui::EndMenu();
        }

		if (ImGui::BeginMenu("Reflection ")) {
			if (ImGui::MenuItem("Mirror", "", reflectionMode == 0)) { reflectionMode = 0; }
			if (ImGui::MenuItem("Mixed Color BRDF", "", reflectionMode == 1)) { reflectionMode = 1; }
            if (ImGui::MenuItem("Mixed Color Simple", "", reflectionMode == 2)) { reflectionMode = 2; }
			if (ImGui::MenuItem("Off", "", reflectionMode == 3)) { reflectionMode = 3; }
			ImGui::EndMenu();
		}

        if (ImGui::BeginMenu("Skydome ")) {
            if (ImGui::MenuItem("Sky", "", sky_dome_mode == 0)) { sky_dome_mode = 0; }
            if (ImGui::MenuItem("Cage", "", sky_dome_mode == 1)) { sky_dome_mode = 1; }
            if (ImGui::MenuItem("HDR Sky", "", sky_dome_mode == 2)) { sky_dome_mode = 2; }
            ImGui::EndMenu();
        }

        // This menu demonstrates how to provide the user a choice
        // among a set of choices.  The current choice is stored in a
        // variable named "mode" in the application, and sent to the
        // shader to be used as you wish.
        if (ImGui::BeginMenu("Textures ")) {
            if (ImGui::MenuItem("Off", "", texture_mode == 0)) { texture_mode = 0; }
            if (ImGui::MenuItem("On", "", texture_mode == 1)) { texture_mode = 1; }
            if (ImGui::MenuItem("On without Nmaps", "", texture_mode == 2)) { texture_mode = 2; }
            ImGui::EndMenu(); }

        if (ImGui::BeginMenu("AO ")) {
            if (ImGui::MenuItem("Enabled", "", ao_enabled == 1)) { ao_enabled = 1; }
            if (ImGui::MenuItem("Disabled", "", ao_enabled == 0)) { ao_enabled = 0; }
            ImGui::SliderInt("AO sample count", &ao_sample_count, 10, 20);
            ImGui::SliderFloat("AO range", &ao_range, 0, 1);
            ImGui::SliderFloat("AO scale", &ao_scale, 0, 10);
            ImGui::SliderFloat("AO contrast", &ao_contrast, 0, 10);
            ImGui::SliderInt("AO Kernel width ", &bilinear_kernel_width, 2, 50);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Draw FBOs")) {
            if (ImGui::MenuItem("Draw Shadow Map", "", draw_fbo == 0)) { draw_fbo = 0; }
            if (ImGui::MenuItem("Draw Shadow Map squared", "", draw_fbo == 1)) { draw_fbo = 1; }
            if (ImGui::MenuItem("Draw Shadow Map cubed", "", draw_fbo == 2)) { draw_fbo = 2; }
            if (ImGui::MenuItem("Draw Shadow Map pow 4", "", draw_fbo == 3)) { draw_fbo = 3; }
            if (ImGui::MenuItem("Draw Upper Reflection Map ", "", draw_fbo == 4)) { draw_fbo = 4; }
            if (ImGui::MenuItem("Draw Lower Reflection Map", "", draw_fbo == 5)) { draw_fbo = 5; }
            if (ImGui::MenuItem("Draw World Position Map", "", draw_fbo == 6)) { draw_fbo = 6; }
            if (ImGui::MenuItem("Draw Normal Map", "", draw_fbo == 7)) { draw_fbo = 7; }
            if (ImGui::MenuItem("Draw Diffuse color Map", "", draw_fbo == 8)) { draw_fbo = 8; }
            if (ImGui::MenuItem("Draw Specular color Map", "", draw_fbo == 9)) { draw_fbo = 9; }
            if (ImGui::MenuItem("Draw AO Map", "", draw_fbo == 10)) { draw_fbo = 10; }
            if (ImGui::MenuItem("Draw AO Map Blur1", "", draw_fbo == 11)) { draw_fbo = 11; }
            if (ImGui::MenuItem("Draw AO Map Blur2", "", draw_fbo == 12)) { draw_fbo = 12; }
            if (ImGui::MenuItem("Disable", "", draw_fbo == 13)) { draw_fbo = 13; }
            ImGui::EndMenu();
        }

    ImGui::EndMainMenuBar(); 
    }
    if (lightingMode != 3) {
        ImGui::Begin("Shadow Kernel Control");
        ImGui::SliderInt("Shadow Blur Kernel", &kernel_width, 2, 50);
        ImGui::Text("LightDist : %f", lightDist);
        ImGui::End();
    }
    else {
        ImGui::Begin("Exposure Control");
        ImGui::SliderInt("Exposure ", &exposure, 2, 20);
        ImGui::SliderInt("Sampling count", &sampling_count, 20, 40);
        ImGui::End();
    }
    
    if (gamelike_mode == true) {
        const float step = speed * (glfwGetTime() - time_at_prev_frame);
        if (w_down)
            eye += step * glm::vec3(sin((spin * PI) / 180), cos((spin * PI) / 180), 0.0);
        if (s_down)
            eye -= step * glm::vec3(sin((spin * PI) / 180), cos((spin * PI) / 180), 0.0);
        if (d_down)
            eye += step * glm::vec3(cos((spin * PI) / 180), -sin((spin * PI) / 180), 0.0);
        if (a_down)
            eye -= step * glm::vec3(cos((spin * PI) / 180), -sin((spin * PI) / 180), 0.0);
        eye.z = proceduralground->HeightAt(eye.x, eye.y) + 2.0;
    }

    time_at_prev_frame = glfwGetTime();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Scene::CreateFullScreenQuad() {
    //Create a VAO and put the ID in vao_id
    glGenVertexArrays(1, &screen_quad_vao);
    //Use the same VAO for all the following operations
    glBindVertexArray(screen_quad_vao);

    //Put a vertex consisting of 3 float coordinates x,y,z into the list of all vertices
    std::vector<float> vertices = {
        -1,  1, 0,
        -1, -1, 0,
         1, -1, 0,
         1,  1, 0
    };


    //Create a continguous buffer for all the vertices/points
    GLuint point_buffer;
    glGenBuffers(1, &point_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, point_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    CHECKERROR;

    //IBO data
    GLuint indexData[] = { 0, 1, 2, 3 };
    //Create IBO
    GLuint indeces_buffer;
    glGenBuffers(1, &indeces_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indeces_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), indexData, GL_STATIC_DRAW);
    CHECKERROR;
    glBindVertexArray(0);
}

void Scene::DrawFullScreenQuad() {
    glBindVertexArray(screen_quad_vao);
    CHECKERROR;
    glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, 0);
    CHECKERROR;
    glBindVertexArray(0);
}

void Scene::CreateLocalLights(Shape* SpherePolygons) {
    for (int i = -200; i <= 200; i+=5) {
        for (int j = -200; j <= 200; j+=5) {
            Object* new_light = new Object(SpherePolygons, spheresId, glm::vec3(10.0, 10.0, 10.0), glm::vec3(1.0, 1.0, 1.0), 0.128); //phong alpha = 120
            LocalLights.push_back(new_light);
            local_light_positions.push_back(glm::vec3(i, j, 2));
            local_light_radii.push_back(4.0);
        }
    }

    for (int i = -12; i <= 12; i+=6) {
        for (int j = -12; j <= 12; j+=6) {
            Object* new_light = new Object(SpherePolygons, spheresId, glm::vec3(10.0, 10.0, 10.0), glm::vec3(1.0, 1.0, 1.0), 0.128); //phong alpha = 120
            LocalLights.push_back(new_light);
            local_light_positions.push_back(glm::vec3(i, j, 3));
            local_light_radii.push_back(10.0);
        }
    }
}

void Scene::DrawLocalLights(ShaderProgram* program) {
    int loc;
    glm::vec3 local_light_pos;
    float local_light_radius;
    for (int i = 0; i < LocalLights.size(); i++) {
        local_light_pos = local_light_positions[i];
        local_light_radius = local_light_radii[i];
        CHECKERROR;
        loc = glGetUniformLocation(program->programId, "localLightPos");
        CHECKERROR;
        glUniform3fv(loc, 1, &(local_light_pos[0]));
        CHECKERROR;
        loc = glGetUniformLocation(program->programId, "localLightRadius");
        CHECKERROR;
        glUniform1f(loc, local_light_radius);
        CHECKERROR;
        LocalLights[i]->Draw(program, Translate(local_light_pos.x, local_light_pos.y, local_light_pos.z) * 
                                        Scale(local_light_radius, local_light_radius, local_light_radius));
        CHECKERROR;
    }
}

void Scene::RebuildGbuffer(int w, int h){
    gbufferRenderTarget.DeleteFBO();
    gbufferRenderTarget.CreateFBO(w, h, 4);
    AORenderTarget.DeleteFBO();
    AORenderTarget.CreateFBO(w, h);
    bilinearFilterOutput.DeleteFBO();
    bilinearFilterOutput.CreateFBO(w, h);
    bilinearFilterOutput_2.DeleteFBO();
    bilinearFilterOutput_2.CreateFBO(w, h);
}

void Scene::BuildTransforms()
{
    

    // @@ When you are ready to try interactive viewing, replace the
    // following hard coded values for WorldProj and WorldView with
    // transformation matrices calculated from variables such as spin,
    // tilt, tr, ry, front, and back.
    const float rx = ry * width / height;
    WorldProj = Perspective(rx, ry, front, back);
    LightProj = Perspective(40 / lightDist, 40 / lightDist, front, back);

    WorldView = Translate(tx, ty, zoom) * Rotate(0, tilt - 90) * Rotate(2, spin) * Translate(-1*eye.x, -1*eye.y, -1*eye.z);
    LightView = LookAt(lightPos, glm::vec3(0, 0, 0), glm::vec3(0, 0, 1));

    ShadowMatrix = BMatrix * LightProj * LightView;
    /*
    WorldProj[0][0]=  2.368;
    WorldProj[1][0]= -0.800;
    WorldProj[2][0]=  0.000;
    WorldProj[3][0]=  0.000;
    WorldProj[0][1]=  0.384;
    WorldProj[1][1]=  1.136;
    WorldProj[2][1]=  2.194;
    WorldProj[3][1]=  0.000;
    WorldProj[0][2]=  0.281;
    WorldProj[1][2]=  0.831;
    WorldProj[2][2]= -0.480;
    WorldProj[3][2]= 42.451;
    WorldProj[0][3]=  0.281;
    WorldProj[1][3]=  0.831;
    WorldProj[2][3]= -0.480;
    WorldProj[3][3]= 43.442;

    
    WorldView[3][0]= 0.0;
    WorldView[3][1]= 0.0;
    WorldView[3][2]= 0.0;
    */

    // @@ Print the two matrices (in column-major order) for
    // comparison with the project document.
    //std::cout << "WorldView: " << glm::to_string(WorldView) << std::endl;
    //std::cout << "WorldProj: " << glm::to_string(WorldProj) << std::endl;
}

////////////////////////////////////////////////////////////////////////
// Procedure DrawScene is called whenever the scene needs to be
// drawn. (Which is often: 30 to 60 times per second are the common
// goals.)
void Scene::DrawScene()
{
    // Set the viewport
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    CHECKERROR;
    // Calculate the light's position from lightSpin, lightTilt, lightDist
    lightPos = glm::vec3(lightDist*cos(lightSpin*rad)*sin(lightTilt*rad),
                         lightDist*sin(lightSpin*rad)*sin(lightTilt*rad), 
                         lightDist*cos(lightTilt*rad));

    // Update position of any continuously animating objects
    double atime = 360.0*glfwGetTime()/36;
    for (std::vector<Object*>::iterator m=animated.begin();  m<animated.end();  m++)
        (*m)->animTr = Rotate(2, atime);

    BuildTransforms();

    // The lighting algorithm needs the inverse of the WorldView matrix
    WorldInverse = glm::inverse(WorldView);
    

    if (sampling_count != h_block.N) {
        h_block.N = sampling_count;
        RecalculateHBlock();

        glBindBuffer(GL_UNIFORM_BUFFER, h_block_id);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(h_block), &h_block);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        CHECKERROR;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // Anatomy of a pass:
    //   Choose a shader  (create the shader in InitializeScene above)
    //   Choose and FBO/Render-Target (if needed; create the FBO in InitializeScene above)
    //   Set the viewport (to the pixel size of the screen or FBO)
    //   Clear the screen.
    //   Set the uniform variables required by the shader
    //   Draw the geometry
    //   Unset the FBO (if one was used)
    //   Unset the shader
    ////////////////////////////////////////////////////////////////////////////////

    CHECKERROR;
    int loc, programId;

    ////////////////////////////////////////////////////////////////////////////////
    // Deferred Shading pass
    ////////////////////////////////////////////////////////////////////////////////

    // Choose the shadow shader
    gbufferProgram->Use();
    programId = gbufferProgram->programId;

    // Set the viewport, and clear the screen
    glViewport(0, 0, width, height);
    gbufferRenderTarget.Bind();
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CHECKERROR;

    GLenum bufs[4] = { GL_COLOR_ATTACHMENT0_EXT , GL_COLOR_ATTACHMENT1_EXT , GL_COLOR_ATTACHMENT2_EXT , GL_COLOR_ATTACHMENT3_EXT };
    glDrawBuffers(4, bufs);

    //Bind the skydome texture
    switch (sky_dome_mode)
    {
    case 0:
        p_sky_dome->Bind(13, programId, "SkydomeTex");
        sky_dome_width = p_sky_dome->width;
        sky_dome_height = p_sky_dome->height;
        break;
    case 1:
        p_sky_dome_cage->Bind(13, programId, "SkydomeTex");
        sky_dome_width = p_sky_dome_cage->width;
        sky_dome_height = p_sky_dome_cage->height;
        break;
    case 2:
        p_barca_sky->Bind(13, programId, "SkydomeTex");
        sky_dome_width = p_barca_sky->width;
        sky_dome_height = p_barca_sky->height;
        break;
    }
    CHECKERROR;

    // @@ The scene specific parameters (uniform variables) used by
    // the shader are set here.  Object specific parameters are set in
    // the Draw procedure in object.cpp

    loc = glGetUniformLocation(programId, "WorldProj");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldProj));
    loc = glGetUniformLocation(programId, "WorldView");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldView));
    loc = glGetUniformLocation(programId, "WorldInverse");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldInverse));
    loc = glGetUniformLocation(programId, "lightPos");
    glUniform3fv(loc, 1, &(lightPos[0]));
    loc = glGetUniformLocation(programId, "textureMode");
    glUniform1i(loc, texture_mode);
    CHECKERROR;

    // Draw all objects (This recursively traverses the object hierarchy.)
    objectRoot->Draw(gbufferProgram, Identity);
    CHECKERROR;
    gbufferRenderTarget.Unbind();
    CHECKERROR;
    // Turn off the shader
    gbufferProgram->Unuse();
    ////////////////////////////////////////////////////////////////////////////////
    // End of G buffer pass
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    // Shadow pass
    ////////////////////////////////////////////////////////////////////////////////
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    // Choose the shadow shader
    shadowProgram->Use();
    programId = shadowProgram->programId;

    // Set the viewport, and clear the screen
    glViewport(0, 0, fbo_width, fbo_height);
    shadowPassRenderTarget.Bind();
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    CHECKERROR;

    // @@ The scene specific parameters (uniform variables) used by
    // the shader are set here.  Object specific parameters are set in
    // the Draw procedure in object.cpp

    loc = glGetUniformLocation(programId, "WorldProj");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(LightProj));
    loc = glGetUniformLocation(programId, "LightView");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(LightView));
    loc = glGetUniformLocation(programId, "debugMode");
    glUniform1i(loc, debug_mode);

    float min_depth = lightDist - 25;
    float max_depth = lightDist + 25;
    loc = glGetUniformLocation(programId, "min_depth");
    glUniform1f(loc, min_depth);
    loc = glGetUniformLocation(programId, "max_depth");
    glUniform1f(loc, max_depth);
    CHECKERROR;

    // Draw all objects (This recursively traverses the object hierarchy.)
    objectRoot->Draw(shadowProgram, Identity);
    CHECKERROR;
    shadowPassRenderTarget.Unbind();
    CHECKERROR;
    // Turn off the shader
    shadowProgram->Unuse();
    glDisable(GL_CULL_FACE);
    ////////////////////////////////////////////////////////////////////////////////
    // End of Shadow pass
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    // Shadow pass blur
    ////////////////////////////////////////////////////////////////////////////////
    
    shadowBlur_H_Program->Use();

    RecalculateKernel();

    glBindBuffer(GL_ARRAY_BUFFER, blur_kernel_block_id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, kernel_vals.size() * sizeof(float), &kernel_vals[0]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    CHECKERROR;

    loc = glGetUniformLocation(shadowBlur_H_Program->programId, "width");
    glUniform1i(loc, kernel_width);


    GLuint imageUnit = 0 ; // Perhaps 0 for input image and 1 for output image
    loc = glGetUniformLocation(shadowBlur_H_Program->programId, "src");
    glBindImageTexture(imageUnit, shadowPassRenderTarget.textureID[0],
                       0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    glUniform1i(loc, imageUnit);


    imageUnit = 1; // Perhaps 0 for input image and 1 for output image
    loc = glGetUniformLocation(shadowBlur_H_Program->programId, "dst");
    glBindImageTexture(imageUnit, shadowBlurOutput.textureID[0],
                       0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glUniform1i(loc, imageUnit);
    // Tiles WxH image with groups sized 128x1
    glDispatchCompute(glm::ceil(fbo_width / 128.0f), fbo_height, 1);

    shadowBlur_H_Program->Unuse();

    shadowBlur_V_Program->Use();

    loc = glGetUniformLocation(shadowBlur_V_Program->programId, "width");
    glUniform1i(loc, kernel_width);

    imageUnit = 0; // Perhaps 0 for input image and 1 for output image
    loc = glGetUniformLocation(shadowBlur_V_Program->programId, "src");
    glBindImageTexture(imageUnit, shadowBlurOutput.textureID[0],
        0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    glUniform1i(loc, imageUnit);


    imageUnit = 1; // Perhaps 0 for input image and 1 for output image
    loc = glGetUniformLocation(shadowBlur_V_Program->programId, "dst");
    glBindImageTexture(imageUnit, shadowPassRenderTarget.textureID[0],
        0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glUniform1i(loc, imageUnit);
    // Set all uniform and image variables
    // Tiles WxH image with groups sized 128x1
    glDispatchCompute(fbo_width, glm::ceil(fbo_height / 128.0f), 1);

    shadowBlur_V_Program->Unuse();

    ////////////////////////////////////////////////////////////////////////////////
    // End of Shadow pass blur
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    // Ambient Occlusion pass 
    ////////////////////////////////////////////////////////////////////////////////

    AOProgram->Use();


    loc = glGetUniformLocation(AOProgram->programId, "width");
    glUniform1i(loc, width);

    loc = glGetUniformLocation(AOProgram->programId, "height");
    glUniform1i(loc, height);

    loc = glGetUniformLocation(AOProgram->programId, "ao_sample_count");
    glUniform1i(loc, ao_sample_count);

    loc = glGetUniformLocation(AOProgram->programId, "range_of_influence");
    glUniform1f(loc, ao_range);

    loc = glGetUniformLocation(AOProgram->programId, "scale");
    glUniform1f(loc, ao_scale);

    loc = glGetUniformLocation(AOProgram->programId, "contrast");
    glUniform1f(loc, ao_contrast);

    imageUnit = 0; // Perhaps 0 for input image and 1 for output image
    loc = glGetUniformLocation(AOProgram->programId, "gBufferWorldPos");
    glBindImageTexture(imageUnit, gbufferRenderTarget.textureID[0],
        0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    glUniform1i(loc, imageUnit);

    imageUnit = 1; 
    loc = glGetUniformLocation(AOProgram->programId, "gBufferNormalVec");
    glBindImageTexture(imageUnit, gbufferRenderTarget.textureID[1],
        0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    glUniform1i(loc, imageUnit);


    imageUnit = 2; 
    loc = glGetUniformLocation(AOProgram->programId, "dst");
    glBindImageTexture(imageUnit, AORenderTarget.textureID[0],
        0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glUniform1i(loc, imageUnit);

    // Tiles WxH image with groups sized 128x1
    glDispatchCompute(width, height, 1);

    AOProgram->Unuse();

    ////////////////////////////////////////////////////////////////////////////////
    // End of Ambient Occlusion pass 
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    // Bilinear Filter for AO
    ////////////////////////////////////////////////////////////////////////////////

    bilinear_H_Program->Use();

    RecalculateBilinearKernel();

    glBindBuffer(GL_ARRAY_BUFFER, bilinear_kernel_block_id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, bilinear_kernel_vals.size() * sizeof(float), &bilinear_kernel_vals[0]);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    CHECKERROR;

    loc = glGetUniformLocation(bilinear_H_Program->programId, "width");
    glUniform1i(loc, bilinear_kernel_width);


    imageUnit = 0; // Perhaps 0 for input image and 1 for output image
    loc = glGetUniformLocation(bilinear_H_Program->programId, "src");
    glBindImageTexture(imageUnit, AORenderTarget.textureID[0],
        0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    glUniform1i(loc, imageUnit);

    imageUnit = 1; // Perhaps 0 for input image and 1 for output image
    loc = glGetUniformLocation(bilinear_H_Program->programId, "gBufferWorldPos");
    glBindImageTexture(imageUnit, gbufferRenderTarget.textureID[0],
        0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    glUniform1i(loc, imageUnit);

    imageUnit = 2;
    loc = glGetUniformLocation(bilinear_H_Program->programId, "gBufferNormalVec");
    glBindImageTexture(imageUnit, gbufferRenderTarget.textureID[1],
        0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    glUniform1i(loc, imageUnit);

    imageUnit = 3; // Perhaps 0 for input image and 1 for output image
    loc = glGetUniformLocation(bilinear_H_Program->programId, "dst");
    glBindImageTexture(imageUnit, bilinearFilterOutput.textureID[0],
        0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glUniform1i(loc, imageUnit);
    // Tiles WxH image with groups sized 128x1
    glDispatchCompute(glm::ceil(width / 128.0f), height, 1);

    bilinear_H_Program->Unuse();

    bilinear_V_Program->Use();

    loc = glGetUniformLocation(bilinear_V_Program->programId, "width");
    glUniform1i(loc, bilinear_kernel_width);

    imageUnit = 0; // Perhaps 0 for input image and 1 for output image
    loc = glGetUniformLocation(bilinear_V_Program->programId, "src");
    glBindImageTexture(imageUnit, bilinearFilterOutput.textureID[0],
        0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    glUniform1i(loc, imageUnit);

    imageUnit = 1; // Perhaps 0 for input image and 1 for output image
    loc = glGetUniformLocation(bilinear_V_Program->programId, "gBufferWorldPos");
    glBindImageTexture(imageUnit, gbufferRenderTarget.textureID[0],
        0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    glUniform1i(loc, imageUnit);

    imageUnit = 2;
    loc = glGetUniformLocation(bilinear_V_Program->programId, "gBufferNormalVec");
    glBindImageTexture(imageUnit, gbufferRenderTarget.textureID[1],
        0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
    glUniform1i(loc, imageUnit);

    imageUnit = 3; // Perhaps 0 for input image and 1 for output image
    loc = glGetUniformLocation(bilinear_V_Program->programId, "dst");
    glBindImageTexture(imageUnit, bilinearFilterOutput_2.textureID[0],
        0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
    glUniform1i(loc, imageUnit);
    // Set all uniform and image variables
    // Tiles WxH image with groups sized 128x1
    glDispatchCompute(width, glm::ceil(height / 128.0f), 1);

    bilinear_V_Program->Unuse();

    ////////////////////////////////////////////////////////////////////////////////
    // End of Bilinear Filter for AO
    ////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	// Reflection pass 1
	////////////////////////////////////////////////////////////////////////////////
	int hemisphereSign = 1;

	// Choose the reflection shader
	reflectionProgram->Use();
	programId = reflectionProgram->programId;

    //Bind the skydome texture
    switch (sky_dome_mode)
    {
    case 0:
        p_sky_dome->Bind(13, programId, "SkydomeTex");
        sky_dome_width = p_sky_dome->width;
        sky_dome_height = p_sky_dome->height;
        break;
    case 1:
        p_sky_dome_cage->Bind(13, programId, "SkydomeTex");
        sky_dome_width = p_sky_dome_cage->width;
        sky_dome_height = p_sky_dome_cage->height;
        break;
    case 2:
        p_barca_sky->Bind(13, programId, "SkydomeTex");
        sky_dome_width = p_barca_sky->width;
        sky_dome_height = p_barca_sky->height;
        break;
    }
    CHECKERROR;

    p_irr_map->Bind(14, programId, "IrrMapTex");
    CHECKERROR;

    shadowPassRenderTarget.BindTexture(reflectionProgram->programId, 15, "shadowMap");
    CHECKERROR;
	// Set the viewport, and clear the screen
	glViewport(0, 0, fbo_width, fbo_height);
	upperReflectionRenderTarget.Bind();
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


	// @@ The scene specific parameters (uniform variables) used by
	// the shader are set here.  Object specific parameters are set in
	// the Draw procedure in object.cpp

	loc = glGetUniformLocation(programId, "ReflectionEye");
	glUniform3fv(loc, 1, &(reflectionEye[0]));
	loc = glGetUniformLocation(programId, "HemisphereSign");
	glUniform1i(loc, hemisphereSign);
	loc = glGetUniformLocation(programId, "LightView");
	glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(LightView));
	loc = glGetUniformLocation(programId, "ShadowMatrix");
	glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(ShadowMatrix));
	loc = glGetUniformLocation(programId, "lightPos");
	glUniform3fv(loc, 1, &(lightPos[0]));
	loc = glGetUniformLocation(programId, "light");
	glUniform3fv(loc, 1, &(light[0]));
	loc = glGetUniformLocation(programId, "ambient");
	glUniform3fv(loc, 1, &(ambient[0]));
	loc = glGetUniformLocation(programId, "lightingMode");
	glUniform1i(loc, lightingMode);
	loc = glGetUniformLocation(programId, "mode");
	glUniform1i(loc, mode);
    loc = glGetUniformLocation(programId, "textureMode");
    glUniform1i(loc, texture_mode);
	loc = glGetUniformLocation(programId, "width");
	glUniform1i(loc, width);
	loc = glGetUniformLocation(programId, "height");
	glUniform1i(loc, height);
	CHECKERROR;

	// Draw all objects (This recursively traverses the object hierarchy.)
	objectRoot->Draw(reflectionProgram, Identity);
	CHECKERROR;
	upperReflectionRenderTarget.Unbind();
	// Turn off the shader

	////////////////////////////////////////////////////////////////////////////////
	// End of reflection pass 1
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	// Reflection pass 2
	////////////////////////////////////////////////////////////////////////////////
	hemisphereSign = -1;

	// Set the viewport, and clear the screen
	glViewport(0, 0, fbo_width, fbo_height);
	lowerReflectionRenderTarget.Bind();
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	loc = glGetUniformLocation(programId, "HemisphereSign");
	glUniform1i(loc, hemisphereSign);
	CHECKERROR;

	// Draw all objects (This recursively traverses the object hierarchy.)
	objectRoot->Draw(reflectionProgram, Identity);
	CHECKERROR;
	lowerReflectionRenderTarget.Unbind();
    p_sky_dome->Unbind();
    p_irr_map->Unbind();
    CHECKERROR;
	// Turn off the shader
	reflectionProgram->Unuse();
	////////////////////////////////////////////////////////////////////////////////
	// End of Reflection pass 2
	////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////
	// Lighting pass
	////////////////////////////////////////////////////////////////////////////////

    // Choose the lighting shader
    lightingProgram->Use();
    programId = lightingProgram->programId;
    CHECKERROR;

    //Bind the skydome texture
    switch (sky_dome_mode)
    {
    case 0:
        p_sky_dome->Bind(13, programId, "SkydomeTex");
        sky_dome_width = p_sky_dome->width;
        sky_dome_height = p_sky_dome->height;
        break;
    case 1:
        p_sky_dome_cage->Bind(13, programId, "SkydomeTex");
        sky_dome_width = p_sky_dome_cage->width;
        sky_dome_height = p_sky_dome_cage->height;
        break;
    case 2:
        p_barca_sky->Bind(13, programId, "SkydomeTex");
        sky_dome_width = p_barca_sky->width;
        sky_dome_height = p_barca_sky->height;
        break;
    }
    CHECKERROR;

    p_irr_map->Bind(14, programId, "IrrMapTex");
    CHECKERROR;

    loc = glGetUniformLocation(programId, "skydome_width");
    glUniform1i(loc, sky_dome_width);

    loc = glGetUniformLocation(programId, "skydome_height");
    glUniform1i(loc, sky_dome_height);

    shadowPassRenderTarget.BindTexture(lightingProgram->programId, 15, "shadowMap");
    
    upperReflectionRenderTarget.BindTexture(lightingProgram->programId, 16, "upperReflectionMap");
    
    lowerReflectionRenderTarget.BindTexture(lightingProgram->programId, 17, "lowerReflectionMap");

    gbufferRenderTarget.BindTexture(lightingProgram->programId, 18, "gBufferWorldPos", 0);

    gbufferRenderTarget.BindTexture(lightingProgram->programId, 19, "gBufferNormalVec", 1);

    gbufferRenderTarget.BindTexture(lightingProgram->programId, 20, "gBufferDiffuse", 2);
    
    gbufferRenderTarget.BindTexture(lightingProgram->programId, 21, "gBufferSpecular", 3);

    AORenderTarget.BindTexture(lightingProgram->programId, 22, "AOMap");

    bilinearFilterOutput.BindTexture(lightingProgram->programId, 23, "AOMap_1");

    bilinearFilterOutput_2.BindTexture(lightingProgram->programId, 24, "AOMap_2");

    // Set the viewport, and clear the screen
    glViewport(0, 0, width, height);
    glClearColor(0.5, 0.5, 0.5, 1.0);
    glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
    CHECKERROR;

    // @@ The scene specific parameters (uniform variables) used by
    // the shader are set here.  Object specific parameters are set in
    // the Draw procedure in object.cpp
    
    loc = glGetUniformLocation(programId, "WorldProj");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldProj));
    loc = glGetUniformLocation(programId, "WorldView");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldView));
    loc = glGetUniformLocation(programId, "LightView");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(LightView));
    loc = glGetUniformLocation(programId, "ShadowMatrix");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(ShadowMatrix));
    loc = glGetUniformLocation(programId, "WorldInverse");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldInverse));
    loc = glGetUniformLocation(programId, "lightPos");
    glUniform3fv(loc, 1, &(lightPos[0]));
    loc = glGetUniformLocation(programId, "light");
    glUniform3fv(loc, 1, &(light[0]));
    loc = glGetUniformLocation(programId, "ambient");
    glUniform3fv(loc, 1, &(ambient[0]));
    loc = glGetUniformLocation(programId, "lightingMode");
    glUniform1i(loc, lightingMode);
    loc = glGetUniformLocation(programId, "reflectionMode");
    glUniform1i(loc, reflectionMode);
    loc = glGetUniformLocation(programId, "mode");
    glUniform1i(loc, mode);
    loc = glGetUniformLocation(programId, "textureMode");
    glUniform1i(loc, texture_mode);
    loc = glGetUniformLocation(programId, "drawFbo");
    glUniform1i(loc, draw_fbo);
    loc = glGetUniformLocation(programId, "width");
    glUniform1i(loc, width);
    loc = glGetUniformLocation(programId, "height");
    glUniform1i(loc, height);
    loc = glGetUniformLocation(programId, "ao_enabled");
    glUniform1i(loc, ao_enabled);
    CHECKERROR;

    loc = glGetUniformLocation(programId, "min_depth");
    glUniform1f(loc, min_depth);
    loc = glGetUniformLocation(programId, "max_depth");
    glUniform1f(loc, max_depth);

    loc = glGetUniformLocation(programId, "exposure");
    glUniform1i(loc, exposure);

    //Draw a full screen quad to activate every pixel shader
    DrawFullScreenQuad();
    CHECKERROR; 

    p_sky_dome->Unbind();
    // Turn off the shader
    lightingProgram->Unuse();
    ////////////////////////////////////////////////////////////////////////////////
    // End of Lighting pass
    ////////////////////////////////////////////////////////////////////////////////

    if (local_lights_on == 0)
        return; //Skip local lights pass

    ////////////////////////////////////////////////////////////////////////////////
    // Local Lights pass
    ////////////////////////////////////////////////////////////////////////////////

    glDisable(GL_DEPTH_TEST);

    glBlendFunc(GL_ONE, GL_ONE);
    glEnable(GL_BLEND);

    glCullFace(GL_FRONT);
    glEnable(GL_CULL_FACE);

    // Choose the lighting shader
    localLightsProgram->Use();
    programId = localLightsProgram->programId;
    CHECKERROR;

    gbufferRenderTarget.BindTexture(programId, 18, "gBufferWorldPos", 0);

    gbufferRenderTarget.BindTexture(programId, 19, "gBufferNormalVec", 1);

    gbufferRenderTarget.BindTexture(programId, 20, "gBufferDiffuse", 2);

    gbufferRenderTarget.BindTexture(programId, 21, "gBufferSpecular", 3);
    CHECKERROR;

    // @@ The scene specific parameters (uniform variables) used by
    // the shader are set here.  Object specific parameters are set in
    // the Draw procedure in object.cpp

    loc = glGetUniformLocation(programId, "WorldProj");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldProj));
    loc = glGetUniformLocation(programId, "WorldView");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldView));
    loc = glGetUniformLocation(programId, "WorldInverse");
    glUniformMatrix4fv(loc, 1, GL_FALSE, Pntr(WorldInverse));
    loc = glGetUniformLocation(programId, "ambient");
    glUniform3fv(loc, 1, &(ambient[0]));
    loc = glGetUniformLocation(programId, "lightingMode");
    glUniform1i(loc, lightingMode);
    loc = glGetUniformLocation(programId, "width");
    glUniform1i(loc, width);
    loc = glGetUniformLocation(programId, "height");
    glUniform1i(loc, height);
    CHECKERROR;

    //Draw a full screen quad to activate every pixel shader
    DrawLocalLights(localLightsProgram);
    CHECKERROR;

    p_sky_dome->Unbind();
    // Turn off the shader
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    localLightsProgram->Unuse();

    ////////////////////////////////////////////////////////////////////////////////
    // End of Local Lights pass
    ////////////////////////////////////////////////////////////////////////////////
}


void Scene::RecalculateKernel() {
    kernel_vals.clear();
    float exponent;
    float sum = 0;
    for (int i = -kernel_width; i <= kernel_width; ++i) {
        exponent = (pow(i / (kernel_width / 2.0f), 2) * (-1.0f / 2.0f));
        kernel_vals.push_back(pow(glm::e<float>(), exponent));
        sum += kernel_vals.back();
    }
    float beta = 1 / sum;
    for (unsigned int i = 0; i < kernel_vals.size(); ++i) {
        kernel_vals[i] *= beta;
    }
}

void Scene::RecalculateBilinearKernel() {
    bilinear_kernel_vals.clear();
    float exponent;
    for (int i = -bilinear_kernel_width; i <= bilinear_kernel_width; ++i) {
        exponent = (pow(i / (bilinear_kernel_width / 2.0f), 2) * (-1.0f / 2.0f));
        bilinear_kernel_vals.push_back(pow(glm::e<float>(), exponent));
    }
}

void Scene::RecalculateHBlock() {
    int kk;
    int pos = 0;
    float u, v, p;
    for (int k = 0; k < sampling_count; k++) {
        for (p = 0.5f, kk = k, u = 0.5f; kk; p *= 0.5f, kk >>= 1) {
            if (kk & 1)
                u += p;
        }
        v = (k + 0.5f) / sampling_count;
        h_block.hammersly[pos++] = u;
        h_block.hammersly[pos++] = v;
    }
}