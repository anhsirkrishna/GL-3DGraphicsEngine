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
                      Shape* BoxPolygons, Shape* QuadPolygons)
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
                    woodColor, glm::vec3(0.0, 0.0, 0.0), 10.0);
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
	lightingProgram->AddShader("lighting.vert", GL_VERTEX_SHADER);
	lightingProgram->AddShader("lighting.frag", GL_FRAGMENT_SHADER);

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

    glm::vec3 black(0.0, 0.0, 0.0);
    glm::vec3 brightSpec(0.05, 0.05, 0.05);
    glm::vec3 polishedSpec(0.03, 0.03, 0.03);
 
    // Creates all the models from which the scene is composed.  Each
    // is created with a polygon shape (possibly NULL), a
    // transformation, and the surface lighting parameters Kd, Ks, and
    // alpha.

    // @@ This is where you could read in all the textures and
    // associate them with the various objects being created in the
    // next dozen lines of code.

    // @@ To change an object's surface parameters (Kd, Ks, or alpha),
    // modify the following lines.
    
    central    = new Object(NULL, nullId);
    anim       = new Object(NULL, nullId);
    room       = new Object(RoomPolygons, roomId, brickColor, black, 0.817); //phong alpha = 1
	room->drawMe = false;
    floor      = new Object(FloorPolygons, floorId, floorColor, black, 0.817); //phong alpha = 1
    teapot     = new Object(TeapotPolygons, teapotId, brassColor, brightSpec, 0.128, true); //phong alpha = 120 | Reflective set to true
	reflectionEye = glm::vec3(0, 0, 1.5);
    podium     = new Object(BoxPolygons, boxId, glm::vec3(woodColor), polishedSpec, 0.408); //phong alpha = 10 
    sky        = new Object(SpherePolygons, skyId, black, black, 1); //phong alpha = 0
    ground     = new Object(GroundPolygons, groundId, grassColor, black, 0.817); //phong alpha = 1
    sea        = new Object(SeaPolygons, seaId, waterColor, brightSpec, 0.128); //phong alpha = 120
    leftFrame  = FramedPicture(Identity, lPicId, BoxPolygons, QuadPolygons);
    rightFrame = FramedPicture(Identity, rPicId, BoxPolygons, QuadPolygons); 
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
    p_sky_dome = new Texture(".\\textures\\Sky-049.jpg");
    p_sky_dome_night = new Texture(".\\textures\\core.bmp");
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
            if (ImGui::MenuItem("Draw ground/sea", "", ground->drawMe)){ground->drawMe ^= true;}
            ImGui::EndMenu(); }
        
        if (ImGui::BeginMenu("Lighting ")) {
            if (ImGui::MenuItem("Phong", "", lightingMode == 0)) { lightingMode = 0; }
            if (ImGui::MenuItem("BRDF", "", lightingMode == 1)) { lightingMode = 1; }
            if (ImGui::MenuItem("GGX", "", lightingMode == 2)) { lightingMode = 2; }
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
            if (ImGui::MenuItem("Stars", "", sky_dome_mode == 1)) { sky_dome_mode = 1; }
            if (ImGui::MenuItem("Cage", "", sky_dome_mode == 2)) { sky_dome_mode = 2; }
            ImGui::EndMenu();
        }

        // This menu demonstrates how to provide the user a choice
        // among a set of choices.  The current choice is stored in a
        // variable named "mode" in the application, and sent to the
        // shader to be used as you wish.
        if (ImGui::BeginMenu("Menu ")) {
            if (ImGui::MenuItem("<sample menu of choices>", "",	false, false)) {}
            if (ImGui::MenuItem("Do nothing 0", "",		mode==0)) { mode=0; }
            if (ImGui::MenuItem("Do nothing 1", "",		mode==1)) { mode=1; }
            if (ImGui::MenuItem("Do nothing 2", "",		mode==2)) { mode=2; }
            ImGui::EndMenu(); }

        ImGui::EndMainMenuBar(); }

    
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
        p_sky_dome->Bind(0, programId, "SkydomeTex");
        break;
    case 1:
        p_sky_dome_night->Bind(0, programId, "SkydomeTex");
        break;
    case 2:
        p_sky_dome_cage->Bind(0, programId, "SkydomeTex");
        break;
    }
    CHECKERROR;

	glActiveTexture(GL_TEXTURE10); // Activate texture unit 10
	glBindTexture(GL_TEXTURE_2D, shadowPassRenderTarget.textureID); // Load texture into it
	CHECKERROR;
	loc = glGetUniformLocation(reflectionProgram->programId, "shadowMap");
	glUniform1i(loc, 10); // Tell shader texture is in unit 2
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
    case 0 :
        p_sky_dome->Bind(0, programId, "SkydomeTex");
        break;
    case 1:
        p_sky_dome_night->Bind(0, programId, "SkydomeTex");
        break;
    case 2:
        p_sky_dome_cage->Bind(0, programId, "SkydomeTex");
        break;
    }
    CHECKERROR;

    glActiveTexture(GL_TEXTURE10); // Activate texture unit 10
    glBindTexture(GL_TEXTURE_2D, shadowPassRenderTarget.textureID); // Load texture into it
    CHECKERROR;
    loc = glGetUniformLocation(lightingProgram->programId, "shadowMap");
    glUniform1i(loc, 10); // Tell shader texture is in unit 10
    CHECKERROR;
    glActiveTexture(GL_TEXTURE11); // Activate texture unit 11
    glBindTexture(GL_TEXTURE_2D, upperReflectionRenderTarget.textureID); // Load texture into it
    CHECKERROR;
    loc = glGetUniformLocation(lightingProgram->programId, "upperReflectionMap");
    glUniform1i(loc, 11); // Tell shader texture is in unit 11
    CHECKERROR;
    glActiveTexture(GL_TEXTURE12); // Activate texture unit 12
    glBindTexture(GL_TEXTURE_2D, lowerReflectionRenderTarget.textureID); // Load texture into it
    CHECKERROR;
    loc = glGetUniformLocation(lightingProgram->programId, "lowerReflectionMap");
    glUniform1i(loc, 12); // Tell shader texture is in unit 12

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
    loc = glGetUniformLocation(programId, "width");
    glUniform1i(loc, width);
    loc = glGetUniformLocation(programId, "height");
    glUniform1i(loc, height);
    CHECKERROR;

    // Draw all objects (This recursively traverses the object hierarchy.)
    objectRoot->Draw(lightingProgram, Identity);
    CHECKERROR; 

    p_sky_dome->Unbind();
    CHECKERROR;
    // Turn off the shader
    lightingProgram->Unuse();

    ////////////////////////////////////////////////////////////////////////////////
    // End of Lighting pass
    ////////////////////////////////////////////////////////////////////////////////
}
