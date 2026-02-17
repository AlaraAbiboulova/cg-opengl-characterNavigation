#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Camera.h"
#include "Model.h"//loading the models

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool Init();
bool InitGL();
void Render();
void Shutdown();

GLuint CreateCubeVAO(float width, GLuint& outVBO);
void DrawCube(GLuint cubeVAO);

void OnFramebufferSize(GLFWwindow* window, int width, int height);
void OnMouseMove(GLFWwindow* window, double xpos, double ypos);
void OnScroll(GLFWwindow* window, double xoffset, double yoffset);
void OnMouseButton(GLFWwindow* window, int button, int action, int mods);

void UpdateMovementAndCollision(GLFWwindow* window);

bool LoadTexture2D(const char* filename, GLuint& outTexID);

//window
GLFWwindow* window = nullptr;
int fbWidth = 960;
int fbHeight = 540;

float dt = 0.0f;//current frame
float tPrev = 0.0f;

//shaders
Shader cubeShader;
Shader lightShader;
Shader modelShader;

GLuint cubeVAO = 0;
GLuint cubeVBO = 0;

//room
const float ROOM_W  = 10.0f;
const float ROOM_L  = 14.0f;
const float ROOM_H  = 3.0f;
//thickness
const float WALL_T  = 0.25f;
const float FLOOR_T = 0.20f;
const float CEIL_T  = 0.20f;

//prozorec
static const float WIN_W = 3.4f;
static const float WIN_H = 1.7f;
static const float WIN_Y = 1.35f;
static const float WIN_INSET = 0.01f;//push inside ->inset
static const float WIN_Z_CENTER = 0.25f;

//light from windwo
static const glm::vec3 WIN_LIGHT_COLOR(1.00f, 0.98f, 0.92f);
static const float     WIN_LIGHT_INTENSITY = 1.15f;
static const float     WIN_LIGHT_RANGE     = 12.0f;
static const float     WIN_LIGHT_INNER_DEG = 38.0f;
static const float     WIN_LIGHT_OUTER_DEG = 62.0f;

//textures
GLuint texFloor = 0;
GLuint texWall  = 0;
//switchvane mejdu 2 texturi
GLuint texCouchA = 0;
GLuint texCouchB = 0;
bool useCouchB = false;
bool cWasDown = false;

GLuint texTableBase = 0;

//windwo textures
GLuint dayTexture   = 0;
GLuint nightTexture = 0;
bool isDay = true;
bool nWasDown = false;

//models
Model couch;
Model table;
Model boy;
Model dog;
Model lamp; // NEW

//model paths
static const char* PATH_COUCH = "./models/couchV3.obj";
static const char* PATH_TABLE = "./models/table 02 obj.obj";
static const char* PATH_BOY   = "./models/boy.obj";
static const char* PATH_DOG   = "./models/uploads-files-5014764-Dog_Quad.obj";
static const char* PATH_TABLE_BASECOLOR = "./models/tableTex/Wood03_BaseColor_2K.jpg";
static const char* PATH_LAMP  = "./models/lamp.obj";

// Characters, YOFF offset, so character doesnt float/sink
static const float BOY_SCALE = 0.6f;
static const float BOY_YOFF  = -0.01f;
static const float DOG_SCALE = 0.55f;
static const float DOG_YOFF  = 0.0f;

// Lamp
static const float LAMP_SCALE = 0.012f;
static glm::vec3 lampPos(-3.0f, 0.0f, -2.50f);
static float lampYawDeg = -90.0f;//rotation aroung Y

// Lamp collision
static const float LAMP_HALF_X = 0.35f;
static const float LAMP_HALF_Z = 0.35f;

//Light coming from the lamp
static const glm::vec3 lampLightColor(1.00f, 0.85f, 0.65f);
static const float lampIntensity = 2.2f;   // stable constant
static bool lampOn = false;
static bool lWasDown = false;//key handling

//camera
static const float CAM_HEIGHT  = 1.7f;
static const float LOOK_HEIGHT = 1.4f;
static const float CAM_RADIUS = 0.25f;//clamping

//lightning
glm::vec3 lightPos(0.0f, 2.6f, 0.0f);
glm::vec3 lightColorDay(1.2f, 1.2f, 1.2f);
glm::vec3 lightColorNight(0.15f, 0.15f, 0.35f);

//camera pos so it orbits the characters
Camera cam(glm::vec3(0.0f, 1.6f, 6.0f));
float camYawDeg   = 180.0f;
float camPitchDeg = -12.0f;//rotation X
float camDist     = 4.0f;

bool firstMouse = true;
float lastMouseX = 0.0f;
float lastMouseY = 0.0f;

//char position at beginning
glm::vec3 boyPos(0.0f, 0.0f, 3.0f);
float boyYawDeg = 180.0f;

glm::vec3 dogPos(2.0f, 0.0f, 2.0f);
float dogYawDeg = 180.0f;

float boyRadius = 0.52f;
float dogRadius = 0.38f;

//debbiging toggles  P M
bool wireframeModels = false;
bool mWasDown = false;
bool showCollisionBoxes = false;
bool pWasDown = false;

//collision - bounding boxove
static const float COUCH_HALF_X = 0.90f;
static const float COUCH_HALF_Z = 2.00f;

static const float TABLE_HALF_X = 0.35f;
static const float TABLE_HALF_Z = 0.55f;

//Ray casting
struct Ray { glm::vec3 origin; glm::vec3 dir; };

enum Selected { SELECT_BOY = 0, SELECT_DOG = 1 };
Selected selected = SELECT_BOY;

bool hasTarget = false;
glm::vec3 targetPos(0.0f);

struct AABB { glm::vec3 min; glm::vec3 max; };//collision walls

static glm::vec3 ForwardFromYaw(float yawDeg)// move in direction ure facing
{
    float r = glm::radians(yawDeg);
    return glm::normalize(glm::vec3(std::sin(r), 0.0f, std::cos(r)));
}

static void ClampInsideRoom(glm::vec3& p, float radius)
{
    float minX = -ROOM_W * 0.5f + WALL_T * 0.5f + radius;
    float maxX =  ROOM_W * 0.5f - WALL_T * 0.5f - radius;

    float minZ = -ROOM_L * 0.5f + WALL_T * 0.5f + radius;
    float maxZ =  ROOM_L * 0.5f - WALL_T * 0.5f - radius;

    p.x = std::clamp(p.x, minX, maxX);
    p.z = std::clamp(p.z, minZ, maxZ);
    p.y = 0.0f;
}

static void ClampCameraInsideRoom(glm::vec3& camPos)
{
    float minX = -ROOM_W * 0.5f + WALL_T * 0.5f + CAM_RADIUS;
    float maxX =  ROOM_W * 0.5f - WALL_T * 0.5f - CAM_RADIUS;

    float minZ = -ROOM_L * 0.5f + WALL_T * 0.5f + CAM_RADIUS;
    float maxZ =  ROOM_L * 0.5f - WALL_T * 0.5f - CAM_RADIUS;

    camPos.x = std::clamp(camPos.x, minX, maxX);
    camPos.z = std::clamp(camPos.z, minZ, maxZ);
    camPos.y = std::clamp(camPos.y, 0.25f, ROOM_H + 1.0f);
}
//COLLISION
static bool SphereHitsAABB_XZ(const glm::vec3& c, float r, const AABB& box)
{
    glm::vec3 closest = glm::clamp(c, box.min, box.max);
    glm::vec3 d = c - closest;
    d.y = 0.0f;
    return (glm::dot(d, d) <= r * r);
}

static bool HitsAnyBox(const glm::vec3& p, float r, const std::vector<AABB>& boxes)
{
    for (const auto& b : boxes)
        if (SphereHitsAABB_XZ(p, r, b))
            return true;
    return false;
}


static glm::vec3 MoveWithCollision(const glm::vec3& cur, const glm::vec3& delta, float r,
                                   const std::vector<AABB>& boxes)
{
    glm::vec3 p = cur;

    // X
    {
        glm::vec3 test = p + glm::vec3(delta.x, 0.0f, 0.0f);
        ClampInsideRoom(test, r);
        if (!HitsAnyBox(test, r, boxes))
            p = test;
    }

    // Z
    {
        glm::vec3 test = p + glm::vec3(0.0f, 0.0f, delta.z);
        ClampInsideRoom(test, r);
        if (!HitsAnyBox(test, r, boxes))
            p = test;
    }

    p.y = 0.0f;
    return p;
}

//charcater collision
static void ResolveBoyDog(glm::vec3& a, float ra, glm::vec3& b, float rb)
{
    glm::vec2 d(a.x - b.x, a.z - b.z);
    float d2 = glm::dot(d, d);
    float r = ra + rb;

    if (d2 >= r * r) return;

    float dist = std::sqrt(std::max(d2, 1e-8f));
    glm::vec2 n = d / dist;

    float push = (r - dist) + 0.01f;//pushing them appart

    a.x += n.x * (push * 0.5f);
    a.z += n.y * (push * 0.5f);

    b.x -= n.x * (push * 0.5f);
    b.z -= n.y * (push * 0.5f);

    a.y = b.y = 0.0f;

    ClampInsideRoom(a, ra);
    ClampInsideRoom(b, rb);
}

static std::vector<AABB> MakeObstacles()
{
    std::vector<AABB> boxes;

    float couchX = -ROOM_W * 0.5f + 1.0f;
    float couchZ = 0.0f;

    boxes.push_back({
        glm::vec3(couchX - COUCH_HALF_X, 0.0f, couchZ - COUCH_HALF_Z),
        glm::vec3(couchX + COUCH_HALF_X, 2.0f, couchZ + COUCH_HALF_Z)
    });

    float tableX = couchX + 1.65f;
    float tableZ = couchZ + 0.85f;

    boxes.push_back({
        glm::vec3(tableX - TABLE_HALF_X, 0.0f, tableZ - TABLE_HALF_Z),
        glm::vec3(tableX + TABLE_HALF_X, 1.2f, tableZ + TABLE_HALF_Z)
    });

    boxes.push_back({
        glm::vec3(lampPos.x - LAMP_HALF_X, 0.0f, lampPos.z - LAMP_HALF_Z),
        glm::vec3(lampPos.x + LAMP_HALF_X, 2.2f, lampPos.z + LAMP_HALF_Z)
    });

    return boxes;
}

//cameera follow 
static glm::vec3 GetFollowPos()
{
    return (selected == SELECT_DOG) ? dogPos : boyPos;
}

static void BuildViewProj(glm::mat4& view, glm::mat4& proj)
{
    glm::vec3 follow = GetFollowPos();
    glm::vec3 lookAt = follow + glm::vec3(0.0f, LOOK_HEIGHT, 0.0f);

    glm::vec3 flatFwd = ForwardFromYaw(camYawDeg);
    float pitchR = glm::radians(camPitchDeg);

    glm::vec3 camDir = glm::normalize(glm::vec3(
        flatFwd.x * std::cos(pitchR),
        std::sin(pitchR),
        flatFwd.z * std::cos(pitchR)
    ));
//behind char
    glm::vec3 desired = lookAt+ glm::vec3(0.0f, CAM_HEIGHT - LOOK_HEIGHT, 0.0f)- camDir * camDist;

    ClampCameraInsideRoom(desired);

    cam.Position = desired;
    view = glm::lookAt(cam.Position, lookAt, glm::vec3(0, 1, 0));

    float aspect = (fbHeight > 0) ? (float)fbWidth / (float)fbHeight : 4.0f / 3.0f;
    proj = glm::perspective(glm::radians(cam.Zoom), aspect, 0.05f, 100.0f);
}

//ray picking
static Ray MakeRayFromMouse(double mx, double my)
{
    glm::mat4 view, proj;
    BuildViewProj(view, proj);

    float x = (float)mx;
    float y = (float)(fbHeight - my);
    glm::vec4 viewport(0.0f, 0.0f, (float)fbWidth, (float)fbHeight);

    glm::vec3 nearP = glm::unProject(glm::vec3(x, y, 0.0f), view, proj, viewport);
    glm::vec3 farP  = glm::unProject(glm::vec3(x, y, 1.0f), view, proj, viewport);

    Ray r;
    r.origin = nearP;
    r.dir = glm::normalize(farP - nearP);
    return r;
}

static bool RaySphere(const Ray& ray, const glm::vec3& center, float radius, float& outT)
{
    glm::vec3 oc = ray.origin - center;
//ray shere intersection-nearest positive hit->quadratic hit looking for real solutions
    float a = glm::dot(ray.dir, ray.dir);
    float b = 2.0f * glm::dot(oc, ray.dir);
    float c = glm::dot(oc, oc) - radius * radius;

    float disc = b * b - 4.0f * a * c;
    if (disc < 0.0f) return false;

    float s = std::sqrt(disc);
    float t0 = (-b - s) / (2.0f * a);
    float t1 = (-b + s) / (2.0f * a);

    float t = (t0 > 1e-4f) ? t0 : t1;
    if (t <= 1e-4f) return false;

    outT = t;
    return true;
}

static bool RayGroundY0(const Ray& ray, float& outT)//floor no reliable hit still checking
{
    float denom = ray.dir.y;
    if (std::abs(denom) < 1e-6f) return false;

    float t = -ray.origin.y / denom;
    if (t <= 1e-4f) return false;

    outT = t;
    return true;
}

//day night switch
static void SetCommonModelUniforms(const glm::mat4& view, const glm::mat4& proj,
                                  const glm::vec3& winLightPos, const glm::vec3& winLightDir)
{
    const float uDayT = isDay ? 1.0f : 0.0f;
    const glm::vec3 baseLightColor = isDay ? lightColorDay : lightColorNight;

    glUseProgram(modelShader.ID);

    modelShader.setMat4("view", view);
    modelShader.setMat4("proj", proj);

    modelShader.setVec3("lightPos", lightPos);
    modelShader.setVec3("viewPos", cam.Position);
    modelShader.setVec3("lightColor", baseLightColor);
    modelShader.setFloat("uDayT", uDayT);

    // Spotlight over selected
    glm::vec3 selPos = (selected == SELECT_DOG) ? dogPos : boyPos;
    glm::vec3 spotP  = selPos + glm::vec3(0.0f, 2.0f, 0.0f);

    modelShader.setInt("uUseSpot", 1);
    modelShader.setVec3("spotPos", spotP);
    modelShader.setVec3("spotDir", glm::vec3(0.0f, -1.0f, 0.0f));

    modelShader.setVec3("spotColor", isDay ? glm::vec3(1.0f, 1.0f, 1.0f)
                                           : glm::vec3(0.85f, 0.85f, 1.00f));
    modelShader.setFloat("spotIntensity", isDay ? 1.85f : 1.85f);

    float inner = std::cos(glm::radians(18.0f));
    float outer = std::cos(glm::radians(22.0f));
    modelShader.setFloat("spotInnerCos", inner);
    modelShader.setFloat("spotOuterCos", outer);

    // Lamp light
    glm::vec3 lampLightPos = lampPos + glm::vec3(0.0f, 1.65f, 0.0f);
    modelShader.setInt("uUseLamp", lampOn ? 1 : 0);
    modelShader.setVec3("lampPos", lampLightPos);
    modelShader.setVec3("lampColor", lampLightColor);
    modelShader.setFloat("lampIntensity", lampIntensity);

    //daylight 
    modelShader.setInt("uUseWinLight", isDay ? 1 : 0);
    modelShader.setVec3("winLightPos", winLightPos);
    modelShader.setVec3("winLightDir", winLightDir);
    modelShader.setVec3("winLightColor", WIN_LIGHT_COLOR);
    modelShader.setFloat("winLightIntensity", WIN_LIGHT_INTENSITY);
    modelShader.setFloat("winLightRange", WIN_LIGHT_RANGE);
    modelShader.setFloat("winInnerCos", std::cos(glm::radians(WIN_LIGHT_INNER_DEG)));
    modelShader.setFloat("winOuterCos", std::cos(glm::radians(WIN_LIGHT_OUTER_DEG)));

    modelShader.setInt("uUseOverrideDiffuse", 0);
}


int main()
{
    if (!Init()) return 1;

    while (!glfwWindowShouldClose(window))
    {
        float tNow = (float)glfwGetTime();
        dt = tNow - tPrev;
        tPrev = tNow;

        UpdateMovementAndCollision(window);
        Render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    Shutdown();
    return 0;
}


bool Init()
{
    if (!glfwInit())
    {
        std::cerr << "GLFW init failed\n";
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(960, 540, "Room Scene", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Window creation failed\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, OnFramebufferSize);
    glfwSetCursorPosCallback(window, OnMouseMove);
    glfwSetScrollCallback(window, OnScroll);
    glfwSetMouseButtonCallback(window, OnMouseButton);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    return InitGL();
}

bool InitGL()
{
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "GLEW init failed\n";
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);

    cubeShader.Load("./shaders/cube.vert", "./shaders/cube.frag");
    lightShader.Load("./shaders/light.vert", "./shaders/light.frag");
    modelShader.Load("./shaders/model.vert", "./shaders/model.frag");

    cubeVAO = CreateCubeVAO(1.0f, cubeVBO);

    if (!LoadTexture2D("laminate_floor_02_diff_4k.jpg", texFloor))
        std::cerr << "Failed to load floor texture\n";
    if (!LoadTexture2D("grey_plaster_diff_4k.jpg", texWall))
        std::cerr << "Failed to load wall texture\n";

    if (!LoadTexture2D("Texturelabs_Fabric_146M.jpg", texCouchA))
        std::cerr << "Failed to load Texturelabs_Fabric_146M.jpg\n";
    if (!LoadTexture2D("Texturelabs_Fabric_183M.jpg", texCouchB))
        std::cerr << "Failed to load Texturelabs_Fabric_183M.jpg\n";

    if (!LoadTexture2D(PATH_TABLE_BASECOLOR, texTableBase))
        std::cerr << "Failed to load table basecolor\n";

    // day/night window photos (next to ./app)
    if (!LoadTexture2D("./day.jpg", dayTexture))
        std::cerr << "Failed to load day.jpg (must be next to ./app)\n";
    if (!LoadTexture2D("./night.jpg", nightTexture))
        std::cerr << "Failed to load night.jpg (must be next to ./app)\n";

    std::cout << "[INFO] Loading couch: " << PATH_COUCH << "\n";
    couch.LoadModel(PATH_COUCH);

    std::cout << "[INFO] Loading table: " << PATH_TABLE << "\n";
    table.LoadModel(PATH_TABLE);

    std::cout << "[INFO] Loading boy: " << PATH_BOY << "\n";
    boy.LoadModel(PATH_BOY);

    std::cout << "[INFO] Loading dog: " << PATH_DOG << "\n";
    dog.LoadModel(PATH_DOG);

    std::cout << "[INFO] Loading lamp: " << PATH_LAMP << "\n";
    lamp.LoadModel(PATH_LAMP);

    // cube shader defaults
    glUseProgram(cubeShader.ID);
    cubeShader.setInt("wallMode", 0);
    cubeShader.setVec2("wallSizeXY", ROOM_W, ROOM_H);
    cubeShader.setVec2("wallSizeZY", ROOM_L, ROOM_H);

    cubeShader.setInt("floorTex", 0);
    cubeShader.setInt("wallTex", 0);
    cubeShader.setInt("useFloorTex", 0);
    cubeShader.setInt("useWallTex", 0);

    cubeShader.setFloat("floorTexScale", 2.0f);
    cubeShader.setFloat("wallTexScale", 2.0f);

    // window samplers fixed units
    cubeShader.setInt("uWindowDayTex", 1);
    cubeShader.setInt("uWindowNightTex", 2);

    // initial collision cleanup
    static std::vector<AABB> boxes = MakeObstacles();

    ClampInsideRoom(boyPos, boyRadius);
    ClampInsideRoom(dogPos, dogRadius);

    if (HitsAnyBox(boyPos, boyRadius, boxes))
        boyPos = glm::vec3(0.0f, 0.0f, 3.0f);
    if (HitsAnyBox(dogPos, dogRadius, boxes))
        dogPos = glm::vec3(2.0f, 0.0f, 2.0f);

    ResolveBoyDog(boyPos, boyRadius, dogPos, dogRadius);

    return true;
}


void Shutdown()
{
    glDeleteProgram(cubeShader.ID);
    glDeleteProgram(lightShader.ID);
    glDeleteProgram(modelShader.ID);

    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);

    if (texFloor) glDeleteTextures(1, &texFloor);
    if (texWall)  glDeleteTextures(1, &texWall);
    if (texCouchA) glDeleteTextures(1, &texCouchA);
    if (texCouchB) glDeleteTextures(1, &texCouchB);
    if (texTableBase) glDeleteTextures(1, &texTableBase);
    if (dayTexture) glDeleteTextures(1, &dayTexture);
    if (nightTexture) glDeleteTextures(1, &nightTexture);

    glfwDestroyWindow(window);
    glfwTerminate();
}


void Render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    static std::vector<AABB> boxes = MakeObstacles();

    glm::mat4 view, proj;
    BuildViewProj(view, proj);

    const float uDayT = isDay ? 1.0f : 0.0f;
    const glm::vec3 baseLightColor = isDay ? lightColorDay : lightColorNight;

    glm::vec3 lampLightPos = lampPos + glm::vec3(0.0f, 1.65f, 0.0f);

    //window light position/ direction (world)
    float xInsideRightWall = (ROOM_W * 0.5f) - (WALL_T * 0.5f) - WIN_INSET;
    glm::vec3 winLightPos = glm::vec3(xInsideRightWall - 0.03f, WIN_Y, WIN_Z_CENTER);
    glm::vec3 winLightDir = glm::normalize(glm::vec3(-1.0f, -0.06f, 0.0f)); // into room, slightly downward

//room
    glUseProgram(cubeShader.ID);
    cubeShader.setMat4("view", view);
    cubeShader.setMat4("proj", proj);

    cubeShader.setFloat("uDayT", uDayT);

    cubeShader.setVec3("light.diffuse", baseLightColor);
    cubeShader.setVec3("light.position", lightPos);
    cubeShader.setVec3("viewPos", cam.Position);

    // lamp
    cubeShader.setInt("uUseLamp", lampOn ? 1 : 0);
    cubeShader.setVec3("lampPos", lampLightPos);
    cubeShader.setVec3("lampColor", lampLightColor);
    cubeShader.setFloat("lampIntensity", lampIntensity);

 //day light
    cubeShader.setInt("uUseWinLight", isDay ? 1 : 0);
    cubeShader.setVec3("winLightPos", winLightPos);
    cubeShader.setVec3("winLightDir", winLightDir);
    cubeShader.setVec3("winLightColor", WIN_LIGHT_COLOR);
    cubeShader.setFloat("winLightIntensity", WIN_LIGHT_INTENSITY);
    cubeShader.setFloat("winLightRange", WIN_LIGHT_RANGE);
    cubeShader.setFloat("winInnerCos", std::cos(glm::radians(WIN_LIGHT_INNER_DEG)));
    cubeShader.setFloat("winOuterCos", std::cos(glm::radians(WIN_LIGHT_OUTER_DEG)));

    cubeShader.setVec3("material.ambient", 0.90f, 0.90f, 0.90f);
    cubeShader.setVec3("material.diffuse", 0.90f, 0.90f, 0.90f);
    cubeShader.setVec3("material.specular", 0.35f, 0.35f, 0.35f);
    cubeShader.setFloat("material.shininess", 16.0f);
    cubeShader.setFloat("material.ka", 0.33f);
    cubeShader.setFloat("material.kd", 1.00f);
    cubeShader.setFloat("material.ks", 1.00f);

    auto DrawCubeModel = [&](const glm::mat4& modelMat)
    {
        glm::mat3 nMat = glm::transpose(glm::inverse(glm::mat3(modelMat)));
        cubeShader.setMat4("model", modelMat);
        cubeShader.setMat3("normalMat", nMat);
        DrawCube(cubeVAO);
    };

    glm::vec3 center(0.0f);

    // floor
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texFloor);

        cubeShader.setInt("useFloorTex", 1);
        cubeShader.setInt("useWallTex", 0);
        cubeShader.setInt("uUseWindow", 0);

        glm::mat4 m(1.0f);
        m = glm::translate(m, center + glm::vec3(0.0f, -FLOOR_T * 0.5f, 0.0f));
        m = glm::scale(m, glm::vec3(ROOM_W, FLOOR_T, ROOM_L));
        DrawCubeModel(m);

        cubeShader.setInt("useFloorTex", 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // ceiling
    {
        cubeShader.setInt("useFloorTex", 0);
        cubeShader.setInt("useWallTex", 0);
        cubeShader.setInt("uUseWindow", 0);

        glm::mat4 m(1.0f);
        m = glm::translate(m, center + glm::vec3(0.0f, ROOM_H + CEIL_T * 0.5f, 0.0f));
        m = glm::scale(m, glm::vec3(ROOM_W, CEIL_T, ROOM_L));
        DrawCubeModel(m);
    }

    // walls
    auto DrawWall = [&](glm::vec3 pos, glm::vec3 scale, int wallMode)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texWall);

        cubeShader.setInt("useWallTex", 1);
        cubeShader.setInt("useFloorTex", 0);
        cubeShader.setInt("uUseWindow", 0);
        cubeShader.setInt("wallMode", wallMode);

        glm::mat4 m(1.0f);
        m = glm::translate(m, pos);
        m = glm::scale(m, scale);
        DrawCubeModel(m);

        cubeShader.setInt("useWallTex", 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    };

    DrawWall(center + glm::vec3(0.0f, ROOM_H * 0.5f, -ROOM_L * 0.5f), glm::vec3(ROOM_W, ROOM_H, WALL_T), 0);
    DrawWall(center + glm::vec3(0.0f, ROOM_H * 0.5f,  ROOM_L * 0.5f), glm::vec3(ROOM_W, ROOM_H, WALL_T), 0);
    DrawWall(center + glm::vec3(-ROOM_W * 0.5f, ROOM_H * 0.5f, 0.0f), glm::vec3(WALL_T, ROOM_H, ROOM_L), 1);
    DrawWall(center + glm::vec3( ROOM_W * 0.5f, ROOM_H * 0.5f, 0.0f), glm::vec3(WALL_T, ROOM_H, ROOM_L), 1);

//right wall w window
    {
        glUseProgram(cubeShader.ID);
        cubeShader.setFloat("uDayT", uDayT);
        cubeShader.setInt("uUseWindow", 1);
        cubeShader.setInt("useFloorTex", 0);
        cubeShader.setInt("useWallTex", 0);
        cubeShader.setInt("wallMode", 1);

        cubeShader.setFloat("wallTexScale", 1.0f);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, dayTexture);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, nightTexture);

        glm::mat4 m(1.0f);
        m = glm::translate(m, glm::vec3(xInsideRightWall, WIN_Y, WIN_Z_CENTER));
        m = glm::scale(m, glm::vec3(0.05f, WIN_H, WIN_W));
        DrawCubeModel(m);

        cubeShader.setFloat("wallTexScale", 2.0f);
        cubeShader.setInt("uUseWindow", 0);

        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(GL_TEXTURE0);
    }

//modells
    SetCommonModelUniforms(view, proj, winLightPos, winLightDir);

    if (wireframeModels)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // couch
    {
        glm::mat4 m(1.0f);
        float couchX = -ROOM_W * 0.5f + 1.0f;
        float couchZ = 0.0f;

        m = glm::translate(m, glm::vec3(couchX, 0.0f, couchZ));
        m = glm::rotate(m, glm::radians(90.0f), glm::vec3(0, 1, 0));
        m = glm::scale(m, glm::vec3(1.8f));

        modelShader.setMat4("model", m);

        GLuint couchTex = useCouchB ? texCouchB : texCouchA;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, couchTex);

        modelShader.setInt("uUseOverrideDiffuse", 1);
        modelShader.setInt("uUnlit", 0);

        couch.Draw(modelShader);

        modelShader.setInt("uUseOverrideDiffuse", 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // table
    {
        float couchX = -ROOM_W * 0.5f + 1.0f;
        float couchZ = 0.0f;

        float tableX = couchX + 1.65f;
        float tableZ = couchZ + 0.85f;

        glm::mat4 m(1.0f);
        m = glm::translate(m, glm::vec3(tableX, 0.0f, tableZ));
        m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(0, 1, 0));
        m = glm::scale(m, glm::vec3(1.05f));

        modelShader.setMat4("model", m);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texTableBase);

        modelShader.setInt("uUseOverrideDiffuse", 1);
        modelShader.setInt("uUnlit", 0);

        table.Draw(modelShader);

        modelShader.setInt("uUseOverrideDiffuse", 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // lamp 
    {
        glm::mat4 m(1.0f);
        m = glm::translate(m, lampPos);
        m = glm::rotate(m, glm::radians(lampYawDeg), glm::vec3(0, 1, 0));
        m = glm::scale(m, glm::vec3(LAMP_SCALE));

        modelShader.setMat4("model", m);
        modelShader.setInt("uUseOverrideDiffuse", 0);
        modelShader.setInt("uUnlit", 0);
        lamp.Draw(modelShader);
    }

    // boy
    {
        glm::mat4 m(1.0f);
        m = glm::translate(m, boyPos + glm::vec3(0.0f, BOY_YOFF, 0.0f));
        m = glm::rotate(m, glm::radians(boyYawDeg), glm::vec3(0, 1, 0));
        m = glm::scale(m, glm::vec3(BOY_SCALE));

        modelShader.setMat4("model", m);
        modelShader.setInt("uUseOverrideDiffuse", 0);
        modelShader.setInt("uUnlit", 0);
        boy.Draw(modelShader);
    }

    // dog
    {
        glm::mat4 m(1.0f);
        m = glm::translate(m, dogPos + glm::vec3(0.0f, DOG_YOFF, 0.0f));
        m = glm::rotate(m, glm::radians(dogYawDeg), glm::vec3(0, 1, 0));
        m = glm::scale(m, glm::vec3(DOG_SCALE));

        modelShader.setMat4("model", m);
        modelShader.setInt("uUseOverrideDiffuse", 0);
        modelShader.setInt("uUnlit", 0);
        dog.Draw(modelShader);
    }

    if (wireframeModels)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // collision boxes debug
    if (showCollisionBoxes)
    {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glUseProgram(lightShader.ID);
        lightShader.setMat4("view", view);
        lightShader.setMat4("proj", proj);

        for (const auto& b : boxes)
        {
            glm::vec3 c = 0.5f * (b.min + b.max);
            glm::vec3 s = (b.max - b.min);

            glm::mat4 m(1.0f);
            m = glm::translate(m, c);
            m = glm::scale(m, s);
            lightShader.setMat4("model", m);

            DrawCube(cubeVAO);
        }

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

// keys
void UpdateMovementAndCollision(GLFWwindow* w)
{
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(w, true);

    bool mDown = (glfwGetKey(w, GLFW_KEY_M) == GLFW_PRESS);
    if (mDown && !mWasDown) wireframeModels = !wireframeModels;
    mWasDown = mDown;

    bool pDown = (glfwGetKey(w, GLFW_KEY_P) == GLFW_PRESS);
    if (pDown && !pWasDown) showCollisionBoxes = !showCollisionBoxes;
    pWasDown = pDown;

    bool cDown = (glfwGetKey(w, GLFW_KEY_C) == GLFW_PRESS);
    if (cDown && !cWasDown) useCouchB = !useCouchB;
    cWasDown = cDown;

    // Day/Night N
    {
        bool nDown = (glfwGetKey(w, GLFW_KEY_N) == GLFW_PRESS);
        if (nDown && !nWasDown) isDay = !isDay;
        nWasDown = nDown;
    }

    // Lamp toggle L
    {
        bool lDown = (glfwGetKey(w, GLFW_KEY_L) == GLFW_PRESS);
        if (lDown && !lWasDown) lampOn = !lampOn;
        lWasDown = lDown;
    }

    static std::vector<AABB> boxes = MakeObstacles();

    glm::vec3* pos = (selected == SELECT_DOG) ? &dogPos : &boyPos;
    float* yawDeg  = (selected == SELECT_DOG) ? &dogYawDeg : &boyYawDeg;
    float radius   = (selected == SELECT_DOG) ? dogRadius : boyRadius;
    float speed    = (selected == SELECT_DOG) ? 1.9f : 2.2f;

    glm::vec3 camFwd = ForwardFromYaw(camYawDeg);
    glm::vec3 camRight = glm::normalize(glm::cross(camFwd, glm::vec3(0, 1, 0)));

    glm::vec3 move(0.0f);
    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) move += camFwd;
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) move -= camFwd;
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) move -= camRight;
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) move += camRight;

    bool usingKeyboard = (glm::length(move) > 0.001f);
    if (usingKeyboard) hasTarget = false;

    if (!usingKeyboard && hasTarget)
    {
        glm::vec3 to = targetPos - *pos;
        to.y = 0.0f;

        float dist = glm::length(to);
        if (dist < 0.10f)
            hasTarget = false;
        else
            move = glm::normalize(to);
    }

    if (glm::length(move) > 0.001f)
    {
        move = glm::normalize(move);
        *yawDeg = glm::degrees(std::atan2(move.x, move.z));
    }

    glm::vec3 delta = move * speed * dt;

    glm::vec3 boyPrev = boyPos;
    glm::vec3 dogPrev = dogPos;

    *pos = MoveWithCollision(*pos, delta, radius, boxes);
    ClampInsideRoom(*pos, radius);

    ResolveBoyDog(boyPos, boyRadius, dogPos, dogRadius);

    if (HitsAnyBox(boyPos, boyRadius, boxes))
        boyPos = boyPrev;
    if (HitsAnyBox(dogPos, dogRadius, boxes))
        dogPos = dogPrev;

    ClampInsideRoom(boyPos, boyRadius);
    ClampInsideRoom(dogPos, dogRadius);
}

void OnMouseMove(GLFWwindow* w, double xpos, double ypos)
{
    bool rmb = (glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);

    if (!rmb)
    {
        firstMouse = true;
        return;
    }

    if (firstMouse)
    {
        lastMouseX = (float)xpos;
        lastMouseY = (float)ypos;
        firstMouse = false;
        return;
    }

    float dx = (float)xpos - lastMouseX;
    float dy = lastMouseY - (float)ypos;

    lastMouseX = (float)xpos;
    lastMouseY = (float)ypos;

    float sens = 0.12f;
    camYawDeg += dx * sens;
    camPitchDeg = std::clamp(camPitchDeg + dy * sens, -40.0f, 8.0f);
}

void OnScroll(GLFWwindow*, double, double yoffset)
{
    camDist -= (float)yoffset * 0.35f;
    camDist = std::clamp(camDist, 2.2f, 7.0f);
}

void OnMouseButton(GLFWwindow* w, int button, int action, int)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
        return;

    double mx, my;
    glfwGetCursorPos(w, &mx, &my);

    Ray ray = MakeRayFromMouse(mx, my);

    glm::vec3 boyC = boyPos + glm::vec3(0.0f, boyRadius, 0.0f);
    glm::vec3 dogC = dogPos + glm::vec3(0.0f, dogRadius, 0.0f);

    float tBoy = 0.0f, tDog = 0.0f;
    bool hitBoy = RaySphere(ray, boyC, boyRadius, tBoy);
    bool hitDog = RaySphere(ray, dogC, dogRadius, tDog);

    if (hitBoy || hitDog)
    {
        if (hitBoy && (!hitDog || tBoy < tDog))
            selected = SELECT_BOY;
        else
            selected = SELECT_DOG;

        hasTarget = false;
        return;
    }

    float t;
    if (RayGroundY0(ray, t))
    {
        glm::vec3 p = ray.origin + t * ray.dir;
        p.y = 0.0f;

        float r = (selected == SELECT_DOG) ? dogRadius : boyRadius;
        ClampInsideRoom(p, r);

        targetPos = p;
        hasTarget = true;
    }
}

void OnFramebufferSize(GLFWwindow*, int width, int height)
{
    fbWidth = width;
    fbHeight = height;
    glViewport(0, 0, width, height);
}


GLuint CreateCubeVAO(float, GLuint& outVBO)
{
    float vertices[] = {
        -0.5f,-0.5f,-0.5f,   0,0,-1,  0.5f,-0.5f,-0.5f,   0,0,-1,  0.5f, 0.5f,-0.5f, 0,0,-1,
         0.5f, 0.5f,-0.5f,   0,0,-1, -0.5f, 0.5f,-0.5f,   0,0,-1, -0.5f,-0.5f,-0.5f, 0,0,-1,

        -0.5f,-0.5f, 0.5f,   0,0, 1,  0.5f,-0.5f, 0.5f,   0,0, 1,  0.5f, 0.5f, 0.5f, 0,0,1,
         0.5f, 0.5f, 0.5f,   0,0, 1, -0.5f, 0.5f, 0.5f,   0,0, 1, -0.5f,-0.5f, 0.5f, 0,0,1,

        -0.5f, 0.5f, 0.5f,  -1,0,0, -0.5f, 0.5f,-0.5f,  -1,0,0, -0.5f,-0.5f,-0.5f,-1,0,0,
        -0.5f,-0.5f,-0.5f,  -1,0,0, -0.5f,-0.5f, 0.5f,  -1,0,0, -0.5f, 0.5f, 0.5f,-1,0,0,

         0.5f, 0.5f, 0.5f,   1,0,0,  0.5f, 0.5f,-0.5f,   1,0,0,  0.5f,-0.5f,-0.5f, 1,0,0,
         0.5f,-0.5f,-0.5f,   1,0,0,  0.5f,-0.5f, 0.5f,   1,0,0,  0.5f, 0.5f, 0.5f, 1,0,0,

        -0.5f,-0.5f,-0.5f,   0,-1,0,  0.5f,-0.5f,-0.5f,  0,-1,0,  0.5f,-0.5f, 0.5f, 0,-1,0,
         0.5f,-0.5f, 0.5f,   0,-1,0, -0.5f,-0.5f, 0.5f,  0,-1,0, -0.5f,-0.5f,-0.5f,0,-1,0,

        -0.5f, 0.5f,-0.5f,   0, 1,0,  0.5f, 0.5f,-0.5f,  0, 1,0,  0.5f, 0.5f, 0.5f, 0,1,0,
         0.5f, 0.5f, 0.5f,   0, 1,0, -0.5f, 0.5f, 0.5f,  0, 1,0, -0.5f, 0.5f,-0.5f,0,1,0
    };

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &outVBO);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, outVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return vao;
}

void DrawCube(GLuint vao)
{
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

//loading texthres
bool LoadTexture2D(const char* filename, GLuint& outTexID)
{
    glGenTextures(1, &outTexID);
    glBindTexture(GL_TEXTURE_2D, outTexID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int w, h, ch;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename, &w, &h, &ch, 0);

    if (!data)
    {
        std::cout << "Failed to load texture: " << filename << "\n";
        glBindTexture(GL_TEXTURE_2D, 0);
        outTexID = 0;
        return false;
    }

    GLenum format = (ch == 4) ? GL_RGBA : GL_RGB;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}
