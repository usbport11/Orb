#include "stdafx.h"
#include "classes/system/Shader.h"
#include "classes/system/Scene.h"
#include "classes/system/FPSController.h"
#include "classes/image/TextureLoader.h"
#include "classes/buffers/TextureQuadBuffer.h"

bool Pause;
bool keys[1024] = {0};
int WindowWidth = 800, WindowHeight = 600;
bool EnableVsync = 1;
GLFWwindow* window;
stFPSController FPSController;

MShader Shader;
MScene Scene;
MTextureLoader TL;
MTextureQuadBuffer TQB;

int ManaLevel = 256;
unsigned int One;
stTexture* txOrb = NULL;
stTextureQuad txqGlass;
stTextureQuad txqStencil;
stTextureQuad txqMana;

static void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

static void mousepos_callback(GLFWwindow* window, double x, double y) {
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
		return;
	}
	if(key == GLFW_KEY_UP && action == GLFW_PRESS) {
		ManaLevel += 10;
		if(ManaLevel >= 256) ManaLevel = 256;
		txqMana.v[2].y = txqMana.v[3].y = ManaLevel;
		TQB.UpdateQuad(&txqMana);
		return;
	}
	if(key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
		ManaLevel -= 10;
		if(ManaLevel <= 0) ManaLevel = 0;
		txqMana.v[2].y = txqMana.v[3].y = ManaLevel;
		TQB.UpdateQuad(&txqMana);
		return;
	}
}

bool InitApp() {
	LogFile<<"Starting application"<<endl;    
    glfwSetErrorCallback(error_callback);
    
    if(!glfwInit()) return false;
    window = glfwCreateWindow(WindowWidth, WindowHeight, "TestApp", NULL, NULL);
    if(!window) {
        glfwTerminate();
        return false;
    }
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mousepos_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwMakeContextCurrent(window);
    if(glfwExtensionSupported("WGL_EXT_swap_control")) {
    	LogFile<<"Window: V-Sync supported. V-Sync: "<<EnableVsync<<endl;
		glfwSwapInterval(EnableVsync);//0 - disable, 1 - enable
	}
	else LogFile<<"Window: V-Sync not supported"<<endl;
    LogFile<<"Window created: width: "<<WindowWidth<<" height: "<<WindowHeight<<endl;

	//glew
	GLenum Error = glewInit();
	if(GLEW_OK != Error) {
		LogFile<<"Window: GLEW Loader error: "<<glewGetErrorString(Error)<<endl;
		return false;
	}
	LogFile<<"GLEW initialized"<<endl;
	
	if(!CheckOpenglSupport()) return false;
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//shaders
	if(!Shader.CreateShaderProgram("shaders/main.vertexshader.glsl", "shaders/main.fragmentshader.glsl")) return false;
	if(!Shader.AddUnifrom("MVP", "MVP")) return false;
	LogFile<<"Shaders loaded"<<endl;

	//scene
	if(!Scene.Initialize(&WindowWidth, &WindowHeight)) return false;
	LogFile<<"Scene initialized"<<endl;

	//randomize
    srand(time(NULL));
    LogFile<<"Randomized"<<endl;
    
    //other initializations
    txOrb = TL.LoadTexture("textures/orb.png", 1, 1, 0, One, GL_NEAREST, GL_REPEAT);
    if(!txOrb) return false;
    LogFile<<"Texture loaded"<<endl;

    if(!TQB.Initialize(GL_STATIC_DRAW, txOrb->Id)) return false;
    txqGlass = stTextureQuad(glm::vec2(0, 0), glm::vec2(256, 256), glm::vec2(0, 0.5), glm::vec2(0.5, 0.5));
    txqMana = stTextureQuad(glm::vec2(0, 0), glm::vec2(256, ManaLevel), glm::vec2(0.5, 0.5), glm::vec2(0.5, 0.5));
    txqStencil = stTextureQuad(glm::vec2(0, 0), glm::vec2(256, 256), glm::vec2(0, 0), glm::vec2(0.5, 0.5));
    if(!TQB.AddQuad(&txqGlass)) return false;
    if(!TQB.AddQuad(&txqMana)) return false;
    if(!TQB.AddQuad(&txqStencil)) return false;
    TQB.Relocate();
		
	//turn off pause
	Pause = false;
    
    return true;
}

void RenderStep() {
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	glLoadIdentity();
	
    glUseProgram(Shader.ProgramId);
	glUniformMatrix4fv(Shader.Uniforms["MVP"], 1, GL_FALSE, Scene.GetDynamicMVP());
	
	//draw functions
	TQB.Begin();
	glEnable(GL_STENCIL_TEST);
	glEnable(GL_ALPHA_TEST);//
	glColorMask(false, false, false, false);
	glStencilFunc(GL_ALWAYS, 1, 1);
	glAlphaFunc(GL_GREATER, 0.5);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
		TQB.DrawQuad(&txqStencil);
	glColorMask(true, true, true, true);
	glStencilFunc(GL_EQUAL, 1, 1);
	glAlphaFunc(GL_GREATER, 0.5);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glEnable(GL_BLEND);
		TQB.DrawQuad(&txqMana);
		TQB.DrawQuad(&txqGlass);
		glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);//
	glDisable(GL_STENCIL_TEST);
	TQB.End();
}

void ClearApp() {
	//clear funstions
	TQB.Close();
	TL.DeleteTexture(txOrb, One);
	TL.Close();
	
	memset(keys, 0, 1024);
	Shader.Close();
	LogFile<<"Application: closed"<<endl;
}

int main(int argc, char** argv) {
	LogFile<<"Application: started"<<endl;
	if(!InitApp()) {
		ClearApp();
		glfwTerminate();
		LogFile.close();
		return 0;
	}
	FPSController.Initialize(glfwGetTime());
	while(!glfwWindowShouldClose(window)) {
		FPSController.FrameStep(glfwGetTime());
    	FPSController.FrameCheck();
		RenderStep();
        glfwSwapBuffers(window);
        glfwPollEvents();
	}
	ClearApp();
    glfwTerminate();
    LogFile.close();
}
