#include <iostream>

#include "glframework/core.h"
#include "glframework/shader.h"
#include <string>
#include <assert.h>//断言
#include "wrapper/checkError.h"
#include "application/Application.h"
#include "glframework/texture.h"

//引入相机+控制器
#include "application/camera/perspectiveCamera.h"
#include "application/camera/orthographicCamera.h"
#include "application/camera/trackBallCameraControl.h"
#include "application/camera/GameCameraControl.h"


#include "glframework/geometry.h"
#include "glframework/material/phongMaterial.h"
#include "glframework/material/whiteMaterial.h"
#include "glframework/material/depthMaterial.h"
#include "glframework/material/opacityMaskMaterial.h"
#include"glframework/material/phongEnvMaterial.h"
#include "glframework/material/screenMaterial.h"
#include "glframework/material/cubeMaterial.h"
#include"glframework/material/grassPhongInstanceMaterial.h"
#include "glframework/material/phongInstanceMaterial.h"

#include "glframework/mesh/mesh.h"
#include "glframework//mesh/instanceMesh.h"
#include "glframework/renderer/renderer.h"
#include "glframework/light/pointLight.h"
#include "glframework/light/spotLight.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "glframework/scene.h"
#include "application/assimpLoader.h"
#include"application/assimpInstanceLoader.h"

#include "glframework/framebuffer/framebuffer.h"

/*  
*┌────────────────────────────────────────────────┐
*│　目	   标： 立方体贴图
*│　讲    师： 赵新政(Carma Zhao)
*	 拆分目标： 
*				1 制作蒙鼓人（与相机位置相同的box）
*				2 制作CubeMap纹理对象，创建过程加入到Texture类中
*				3 使用box的几何数据作为采样的uvw坐标进行采样	
*				
				A1: 如果先绘制了天空盒，需要关闭深度写入，防止阻挡后面的物体
				A2: 在创建texture的时候， 会发生y轴反转的情况，cubemap是不需要反转
				A3：天空盒必须第一个绘制（当作背景），其他物体随后绘制 

				通用手法：
					将剪裁空间坐标系的zc恒等于wc，输出zndc恒为1
					注意：材质当中的depthFunc应该为gl_lequal
*└───────────────────────────────────────────────┘
*/
Renderer* renderer = nullptr;
Scene* scene = nullptr;

int WIDTH = 1600;
int HEIGHT = 1200;

GrassPhongInstanceMaterial* grassMat;
//灯光们
DirectionalLight* dirLight = nullptr;

AmbientLight* ambLight = nullptr;

Camera* camera = nullptr;
CameraControl* cameraControl = nullptr;
GameCameraControl* Game_cameraControl = nullptr;
TrackBallCameraControl* Track_cameraControl = nullptr;
glm::vec3 clearColor{};

void OnResize(int width, int height) {
	GL_CALL(glViewport(0, 0, width, height));
}

void OnKey(int key, int action, int mods) {
	cameraControl->onKey(key, action, mods);
}

//鼠标按下/抬起
void OnMouse(int button, int action, int mods) {
	double x, y;
	glApp->getCursorPosition(&x, &y);
	cameraControl->onMouse(button, action, x, y);
}

//鼠标移动
void OnCursor(double xpos, double ypos) {
	cameraControl->onCursor(xpos, ypos);
}

//鼠标滚轮
void OnScroll(double offset) {
	cameraControl->onScroll(offset);
}

void setModelBlend(Object* obj, bool blend, float opacity) {
	if (obj->getType() == ObjectType::Mesh) {
		Mesh* mesh = (Mesh*)obj;
		Material* mat = mesh->mMaterial;
		mat->mBlend = blend;
		mat->mOpacity = opacity;
		mat->mDepthWrite = false;
	}

	auto children = obj->getChildren();
	for (int i = 0; i < children.size(); i++) {
		setModelBlend(children[i], blend, opacity);
	}
}
void setInstanceMatrix(Object* obj, int index, glm::mat4 matrix)
{
	if (obj->getType() == ObjectType::InstanceMesh)
	{
		InstanceMesh* im = (InstanceMesh*)obj;
		im->mInstanceMats[index] = matrix;
	}

	auto children = obj->getChildren();
	for (int i = 0; i < children.size(); i++)
	{
		setInstanceMatrix(children[i], index, matrix);
	}
}
void updateInstanceMatrix(Object* obj)
{
	if (obj->getType() == ObjectType::InstanceMesh)
	{
		InstanceMesh* im = (InstanceMesh*)obj;
		im->updateMatrices();
	}

	auto children = obj->getChildren();
	for (int i = 0; i < children.size(); i++)
	{
		updateInstanceMatrix(children[i]);
	}
}
void setInstanceMat(Object* obj,Material* mat)
{
	if (obj->getType() == ObjectType::InstanceMesh)
	{
		InstanceMesh* im = (InstanceMesh*)obj;
		im->mMaterial=mat;
	}

	auto children = obj->getChildren();
	for (int i = 0; i < children.size(); i++)
	{
		setInstanceMat(children[i],mat);
	}
}
void prepare() {
	renderer = new Renderer();
	scene = new Scene();

	//std::vector<std::string> paths = {
	//	"assets/textures/skybox/right.jpg",
	//	"assets/textures/skybox/left.jpg",
	//	"assets/textures/skybox/top.jpg",
	//	"assets/textures/skybox/bottom.jpg",
	//	"assets/textures/skybox/back.jpg",
	//	"assets/textures/skybox/front.jpg"
	//};
	/*Texture* envTex1 = new Texture(paths, 0);*/
	auto boxGeo = Geometry::createBox(1.0f);
	auto boxMat = new CubeMaterial();
	boxMat->mDiffuse = new Texture("assets/textures/env.png", 0);
	auto boxMesh = new Mesh(boxGeo, boxMat);
	scene->addChild(boxMesh);

	

	///*Texture* envTex = new Texture(paths, 1);*/
	//auto sphereGeo = Geometry::createSphere(4.0f);
	//auto sphereMat = new PhongInstanceMaterial();
	//sphereMat->mDiffuse = new Texture("assets/textures/earth.png", 0);
	//auto sphereMesh = new InstanceMesh(sphereGeo, sphereMat, 3);

	int row = 20;
	int col = 20;
	auto grassModel = AssimpInstanceLoader::load("assets/fbx/grassNew.obj", row*col);
	glm::mat4 translate;
	glm::mat4 rotate;
	glm::mat4 transform;

	srand(glfwGetTime());
	for (int r = 0; r <row; r++)
	{
		for (int c = 0; c < col; c++)
		{
			translate = glm::translate(glm::mat4(1.0), glm::vec3(r * 0.2f, 0.0f, c * 0.2f));
			rotate = glm::rotate(glm::mat4(1.0f), glm::radians((float)(rand() % 90)), glm::vec3(0.0, 1.0, 0.0));
			transform = translate*rotate;
			setInstanceMatrix(grassModel, r * col + c, transform);
		}
	}

	updateInstanceMatrix(grassModel);
	grassMat = new GrassPhongInstanceMaterial();
	grassMat->mDiffuse = new Texture("assets/textures/GRASS.PNG", 0);
	grassMat->mOpacityMask = new Texture("assets/textures/grassMask.png", 1);
	grassMat->mCloudMask = new Texture("assets/textures/CLOUD.PNG", 2);
	grassMat->mBlend = true;
	grassMat->mDepthWrite = false;
	setInstanceMat(grassModel, grassMat);

	scene->addChild(grassModel);

	auto Knight = AssimpLoader::load("assets/fbx/knight.fbx");
	Knight->setScale(glm::vec3(0.01));
	Knight->setPosition(glm::vec3(row * 0.2f / 2.0f, 0.0f, col * 0.2f / 2.0f));
	scene->addChild(Knight);

	/*sphereMat->mEnv = envTex;*/
	/*auto sphereMesh = new InstanceMesh(sphereGeo, sphereMat,2);
	glm::mat4 trans1 = glm::mat4(1.0f);
	glm::mat4 trans2 = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.0f, 0.0f));
	sphereMesh->mInstanceMats[0] = trans1;
	sphereMesh->mInstanceMats[1] = trans2;
	sphereMesh->updateMatrices();
	scene->addChild(sphereMesh);*/
	
	

	
	dirLight = new DirectionalLight();
	dirLight->mDirection = glm::vec3(-1.0f);
	dirLight->mSpecularIntensity = 0.1f;

	ambLight = new AmbientLight();
	ambLight->mColor = glm::vec3(0.1f);
}


void prepareCamera() {
	float size = 10.0f;
	//camera = new OrthographicCamera(-size, size, size, -size, size, -size);
	camera = new PerspectiveCamera(
		60.0f, 
		(float)glApp->getWidth() / (float)glApp->getHeight(),
		0.1f,
		1000.0f
	);

	cameraControl = new TrackBallCameraControl();
	if (cameraControl->getCameraControlType() == CameraType::GameCameraControl)
	{
		Game_cameraControl = (GameCameraControl*)cameraControl;
		Game_cameraControl->setSpeed(0.05f);
	}
	else if (cameraControl->getCameraControlType() == CameraType::TrackBallCameralControl)
	{
		Track_cameraControl = (TrackBallCameraControl*)cameraControl;
	}
	else
	{

	}
	//To do list:修改调整相机模式的方式，不在代码中修改而是调整到imgui上供用户选择
	//目前提供了cameraControl的辨别方式
	cameraControl->setCamera(camera);
	cameraControl->setSensitivity(0.4f);
	
}



void initIMGUI() {
	ImGui::CreateContext();//创建imgui上下文
	ImGui::StyleColorsDark(); // 选择一个主题

	// 设置ImGui与GLFW和OpenGL的绑定
	ImGui_ImplGlfw_InitForOpenGL(glApp->getWindow(), true);
	ImGui_ImplOpenGL3_Init("#version 460");
}

void renderIMGUI() {
	//1 开启当前的IMGUI渲染
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	//2 决定当前的GUI上面有哪些控件，从上到下
	ImGui::Begin("grass Mat");
	ImGui::Text("GrassColor");
	ImGui::SliderFloat("grassUVScale", &grassMat->mUVScale, 1.0f, 100.0f);
	ImGui::SliderFloat("BrightNess", &grassMat->mBrightNess, 1.0f, 10.0f);
	ImGui::Text("Wind");
	ImGui::SliderFloat("WindScale", &grassMat->mWindScale, 0.0f, 1.0f);
	ImGui::ColorEdit3("WindDir", (float*)&grassMat->mWindDir);
	ImGui::SliderFloat("WindPhi", &grassMat->mPhiScale, 0.0f, 10.0f);
	ImGui::Text("Cloud");
	ImGui::ColorEdit3("CloudBlack", (float*) & grassMat->mCloudBlackColor);
	ImGui::ColorEdit3("CloudWhite", (float*) & grassMat->mCloudWhiteColor);
	ImGui::SliderFloat("CloudUVScale", &grassMat->mCloudUvScale, 1.0f, 100.0f);
	ImGui::SliderFloat("CloudSpeed", &grassMat->mCloudSpeed, 0.0f, 1.0f);
	ImGui::SliderFloat("ColorMix", &grassMat->mLerp_para, 0.0f, 1.0f);
	ImGui::End();

	//3 执行UI渲染
	ImGui::Render();
	//获取当前窗体的宽高
	int display_w, display_h;
	glfwGetFramebufferSize(glApp->getWindow(), &display_w, &display_h);
	//重置视口大小
	glViewport(0, 0, display_w, display_h);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

int main() {
	if (!glApp->init(WIDTH, HEIGHT)) {
		return -1;
	}

	glApp->setResizeCallback(OnResize);
	glApp->setKeyBoardCallback(OnKey);
	glApp->setMouseCallback(OnMouse);
	glApp->setCursorCallback(OnCursor);
	glApp->setScrollCallback(OnScroll);

	//设置opengl视口以及清理颜色
	GL_CALL(glViewport(0, 0, WIDTH, HEIGHT));
	GL_CALL(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));

	prepareCamera();

	prepare();
	
	initIMGUI();

	while (glApp->update()) {
		cameraControl->update();
		renderer->setClearColor(clearColor);
		
		renderer->render(scene, camera,dirLight, ambLight);

		renderIMGUI();
	}

	glApp->destroy();

	return 0;
}