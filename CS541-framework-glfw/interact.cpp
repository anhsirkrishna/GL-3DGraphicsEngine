////////////////////////////////////////////////////////////////////////
// All keyboard, mouse, and other interactions are implemented here.
// The single entry point, InitInteraction, sets up GLFW callbacks for
// various events that an interactive graphics program needs to
// handle.
//

#include "framework.h"

extern Scene scene;       // Declared in framework.cpp, but used here.

// Some globals used for mouse handling.
double mouseX, mouseY;
bool shifted = false;
bool leftDown = false;
bool middleDown = false;
bool rightDown = false;

////////////////////////////////////////////////////////////////////////
// Function called to exit
void Quit(void *clientData)
{
    glfwSetWindowShouldClose(scene.window, 1);
}

std::string ACTION[3] = {"Release", "Press", "Repeat"};

////////////////////////////////////////////////////////////////////////
// Called for keyboard actions.

void Keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (ImGui::GetIO().WantCaptureKeyboard) return;

    if (action == GLFW_REPEAT) return; // Because keyboard autorepeat is evil.
        
    printf("Keyboard %c(%d);  S%d %s M%d\n", key, key, scancode, ACTION[action].c_str(), mods);
    fflush(stdout);
    
    // Track SHIFT/NO-SHIFT transitions. (The mods parameter should do this, but doesn't.)
    if (key==GLFW_KEY_LEFT_SHIFT || key==GLFW_KEY_RIGHT_SHIFT)
        shifted = !shifted;
  
    // @@ Catch any key DOWN-transition you'd like in the following
    // switch statement.  Record any change of state in variables in
    // the scene object.

    if  (action == GLFW_PRESS) {
        switch(key) {
        case GLFW_KEY_0: case GLFW_KEY_1: case GLFW_KEY_2: case GLFW_KEY_3: case GLFW_KEY_4:
        case GLFW_KEY_5: case GLFW_KEY_6: case GLFW_KEY_7: case GLFW_KEY_8: case GLFW_KEY_9:
            scene.mode = key-GLFW_KEY_0;
            break;
        case GLFW_KEY_TAB:
            scene.gamelike_mode = !scene.gamelike_mode;
            if (scene.gamelike_mode == true){
                scene.eye = glm::vec3(0.0, 20.0, 0.0);
                scene.tx = 0;
                scene.ty = 0;
                scene.zoom = 0;
            }
            else {
                scene.eye = glm::vec3(0.0, 0.0, 0.0);
                scene.tx = 2.0;
                scene.ty = 2.0;
                scene.zoom = -40;
            }
            break;
        case GLFW_KEY_W:
            scene.w_down = true;
            break;
        case GLFW_KEY_A:
            scene.a_down = true;
            break;
        case GLFW_KEY_S:
            scene.s_down = true;
            break;
        case GLFW_KEY_D:
            scene.d_down = true;
            break;
        case GLFW_KEY_ESCAPE: case GLFW_KEY_Q: // Escape and 'q' keys quit the application
            exit(0); } }
        
    else if (action == GLFW_RELEASE) {
        switch (key) {
        case GLFW_KEY_W:
            scene.w_down = false;
            break;
        case GLFW_KEY_A:
            scene.a_down = false;
            break;
        case GLFW_KEY_S:
            scene.s_down = false;
            break;
        case GLFW_KEY_D:
            scene.d_down = false;
            break;
        }
        // @@ Catch any key UP-transitions you want here.  Record any
        // change of state in variables in the scene object.
        fflush(stdout); }
}

////////////////////////////////////////////////////////////////////////
// Called when a mouse button changes state.
void MouseButton(GLFWwindow* window, int button, int action, int mods)
{        

    if (ImGui::GetIO().WantCaptureMouse) return;

    glfwGetCursorPos(window, &mouseX, &mouseY);
    printf("MouseButton %d %d %d %f %f\n", button, action, mods, mouseX, mouseY);
    
    // @@ Catch any mouse button UP-or-DOWN-transitions you want here.
    // Record any change of state in variables in the scene object.
    
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        leftDown = (action == GLFW_PRESS); }

    else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        middleDown = (action == GLFW_PRESS);  }

    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        rightDown = (action == GLFW_PRESS); }
}

////////////////////////////////////////////////////////////////////////
// Called by GLFW when a mouse moves (while a button is down)
void MouseMotion(GLFWwindow* window, double x, double y)
{
    if (ImGui::GetIO().WantCaptureMouse) return;
                     
    // @@ Catch any mouse movement that occurs while any button is
    // down.  It is not reported here *which* button is down, but you
    // should have recorded the DOWN-transition in a previous call to
    // MouseButton.
    
    // @@ The x,y parameters to this function record the position of
    // the mouse.  Usually you want to know only how much it has moved
    // since the last call.  Hence the following two lines (and this
    // procedure's last two lines) calculate the change in the mouse
    // position.
    
    // Calculate the change in the mouse position
    int dx = x-mouseX;
    int dy = y-mouseY;

    // @@ Please don't disable this shifted-left-button code. 
    if (leftDown && shifted) {  // Rotate light position
        scene.lightSpin += dx/3.0;
        scene.lightTilt -= dy/3.0; }

    else if (leftDown) {
        scene.spin += dx / 3.0;
        scene.tilt += dy / 3.0;
    }

    if (middleDown) { 
    }

    if (rightDown) {
        if (scene.gamelike_mode == false) {
            scene.tx += dx / 3.0;
            scene.ty -= dy / 3.0;
        }
    }

    // Record this position
    mouseX = x;
    mouseY = y;
}


void Scroll(GLFWwindow* window, double x, double y)
{
    if (ImGui::GetIO().WantCaptureMouse)  return;

    printf("Scroll %f %f\n", x, y);
    
    // Figure out the mouse action, and handle accordingly
    // @@ Please don't disable this shifted-scroll-wheel code. 
    if (y>0.0 && shifted) { // Scroll light in
        scene.lightDist = pow(scene.lightDist, 1.0f/1.02f); }

    // @@ Please don't disable this shifted-scroll-wheel code. 
    else if (y<0.0 && shifted) { // Scroll light out
        scene.lightDist = pow(scene.lightDist, 1.02f); }

        
    else if (y>0.0) {
        scene.zoom += 0.5;
    }

    else if (y<0.0) {
        scene.zoom -= 0.5;
    }
}

void InitInteraction()
{
    glfwSetKeyCallback(scene.window, Keyboard);
    glfwSetMouseButtonCallback(scene.window, MouseButton);
    glfwSetCursorPosCallback(scene.window, MouseMotion);
    glfwSetScrollCallback(scene.window, Scroll);
}
