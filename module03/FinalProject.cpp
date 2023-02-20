/*
* FinalProject.cpp

  Date: February 17, 2023
  Author: Andrew Robinson
*/

#include <iostream>         // cout, cerr
#include <random>
#include <vector>
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#include <learnOpengl/camera.h> // Camera class
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h> // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
    const char* const WINDOW_TITLE = "Final Project"; // Macro for window title

    // Variables for window width and height
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;
    const int SECTOR_COUNT = 100;

    // Stores the GL data relative to a given mesh
    struct GLMesh
    {
        GLuint vao;         // Handle for the vertex array object
        GLuint vbos[2];     // Handles for the vertex buffer objects
        GLuint nIndices;    // Number of indices of the mesh
    };

    // Main GLFW window
    GLFWwindow* gWindow = nullptr;

    // Milk Carton mesh data
    GLMesh cartonMesh;
    GLMesh cartonCapMesh;    
    GLMesh gMesh;

    // Shader program
    GLuint gProgramId;
    GLuint gLampProgramId;

    // Texture
    GLuint gTextureId;
    glm::vec2 gUVScale(1.0f, 1.0f);
    GLint gTexWrapMode = GL_REPEAT;

    // camera
    Camera gCamera(glm::vec3(-1.0f, 2.4f, 3.0f));
    glm::vec3 cameraPerspective = glm::vec3(-1.0f, 2.4f, 3.0f);
    glm::vec3 cameraOrtho = glm::vec3(0.0f, 10.4f, 7.0f);
    float gLastX = WINDOW_WIDTH / 2.0f;
    float gLastY = WINDOW_HEIGHT / 2.0f;
    bool gFirstMouse = true;

    // timing
    float gDeltaTime = 0.0f; // time between current frame and last frame
    float gLastFrame = 0.0f;

    // Subject position and scale
    glm::vec3 gCubePosition(0.0f, 0.0f, 0.0f);
    glm::vec3 gCubeScale(7.0f);

    // Pyramid and light color
    glm::vec3 gObjectColor(1.0f, 0.2f, 0.0f);
    glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);

    //Light position and scale
    glm::vec3 gLightPosition(1.5f, 0.5f, 3.0f);
    glm::vec3 gLightScale(0.3f);

    // Lamp animation
    bool gIsLampOrbiting = true;
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void UCreateCartonMesh(GLMesh& mesh, vector<GLfloat>& verts, vector<GLushort>& indices);
void UCreateMesh(GLMesh& mesh);
void UDestroyMesh(GLMesh& mesh);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);

vector <GLfloat> GenCylinderVerts(float radius, float zPos);
vector <GLushort> GenCylinderIndices();
float RandomFloat();

/* Vertex Shader Source Code*/
const GLchar * vertexShaderSource = GLSL(440, 
    layout(location = 0) in vec3 position;// Vertex data Vertex Attrib Pointer 0
    layout(location = 1) in vec3 normal;
    layout(location = 2) in vec2 textureCoordinate;  // Color data from Vertex Attrib Pointer 1

    out vec3 vertexNormal;
    out vec3 vertexFragmentPos;
    out vec2 vertexTextureCoordinate; 

    //Global variables for the  transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main()
    {
        gl_Position = projection * view * model * vec4(position, 1.0f); // transforms vertices to clip coordinates

        vertexFragmentPos = vec3(model * vec4(position, 1.0f));

        vertexNormal = mat3(transpose(inverse(model))) * normal;

        vertexTextureCoordinate = textureCoordinate; // references incoming color data
    }
);

/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,

    in vec3 vertexNormal;
    in vec3 vertexFragmentPos;
    in vec2 vertexTextureCoordinate; // Variable to hold incoming color data from vertex shader

    out vec4 fragmentColor;

    uniform vec3 objectColor;
    uniform vec3 lightColor;
    uniform vec3 lightPos;
    uniform vec3 viewPosition;
    uniform sampler2D uTexture;
    uniform vec2 uvScale;

    void main()
    {
        // Phong lighting model calculations to generate ambient, diffuse, and specular components

        //Calculate Ambient lighting
        float ambientStrength = 1.0f; // Set ambient or global lighting strength
        vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

        // Calculate Diffuse lighting
        vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
        vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube

        float impact = max(dot(norm, lightDirection), 0.0); // Calculate diffuse impact by generating dot product of normal and light

        vec3 diffuse = impact * lightColor; // Generate diffuse light color

        // Calculate Specular lighting
        float specularIntensity = 1.0f; // Set specular light strength
        float highlightSize = 16.0f; // Set specular highlight size
        vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
        vec3 reflectDir = reflect(-lightDirection, norm); // Calculate reflection vector

        // Calculate specular component
        float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
        vec3 specular = specularIntensity * specularComponent * lightColor;

        // Texture holds the color to be used for all three components
        vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

        // Calculate phong result
        vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

        fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
        
    }
);

const GLchar* lampVertexShaderSource = GLSL(440,

    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

    // Uniform / Global variables for the transform matrices
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main() {

        gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
    }
);

const GLchar* lampFragmentShaderSource = GLSL(440,

    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

    void main() {

        fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
    }
);

/* Vectors to hold shape vertices and indices */
vector <GLfloat> cartonVerts = {
    // Vertex positions // Colors (r,g,b,a)

    // Pyramid vertices
     0.6f, -0.5f, -1.1f,  1.0f, 0.0f, 0.0f, 1.0f, // Back side base vertex 0
     0.6f, -0.5f,  0.1f,  0.0f, 1.0f, 0.0f, 1.0f, // Front right base vertex 1
    -0.6f, -0.5f, -1.1f,  0.0f, 0.0f, 1.0f, 1.0f, // Back-left base vertex 2
    -0.6f, -0.5f,  0.1f,  0.5f, 0.2f, 0.0f, 1.0f, // Front-left base vertex3
     0.0f, -0.2f, -0.5f,  0.2f, 0.3f, 1.0f, 1.0f, // Top vertex 4

     // Cube vertices - share indices 0, 1, 2, and 3 with pyramid
      0.6f, -2.5f, -1.1f,   0.5f, 0.5f, 0.8f, 1.0f, // Back-right base vertex 5
      0.6f, -2.5f,  0.1f,   0.9f, 1.0f, 0.8f, 1.0f, // Front-right base vertex 6
     -0.6f, -2.5f, -1.1f,   0.6f, 0.9f, 0.0f, 1.0f, // Back-left base vertex 7
     -0.6f, -2.5f,  0.2f,   1.0f, 0.8f, 0.2f, 1.0f, // Front-left base vertex 8

     // Shared vertex between both planes
      0.5f, -0.2f, -0.5f,   0.2f, 0.3f, 0.0f, 1.0f, // Top-right vertex 9
     -0.5f, -0.2f, -0.5f,   0.0f, 0.2f, 0.3f, 1.0f, // Top-left vertex 10

     // Final vertexes
      0.5f, -0.02f, -0.5f,  0.8f, 0.4f, 0.2f, 1.0f, // Top-right vertex 11
     -0.5f, -0.02f, -0.5f,  0.2f, 1.0f, 1.0f, 1.0f // Top-left vertex 12
};

vector <GLushort> cartonIndices = {
    //Pyramid
    0, 1, 2,
    2, 3, 1,
    1, 4, 3,
    0, 1, 4,
    0, 2, 4,
    2, 3, 4,

    //Cube
    8, 6, 3,
    3, 1, 6,
    5, 6, 1,
    1, 0, 5,
    7, 5, 0,
    0, 2, 7,
    8, 7, 2,
    2, 3, 8,
    8, 6, 5,
    5, 7, 8,

    //Planes
    1, 4, 9,
    3, 4, 10,
    0, 4, 9,
    2, 4, 10,

    //Final
    9, 11, 12,
    10, 12, 9
};

vector <GLfloat> cartonCapVerts = GenCylinderVerts(0.2f, 0.2f);
vector <GLushort> cartonCapIndices = GenCylinderIndices();

// Generate vector of cylinder vertices
vector <GLfloat> GenCylinderVerts(float radius, float zPos) {

    //initialize x and y
    float initX = 0;
    float initY = -radius;
    float angle;
    vector <GLfloat> cylinderVerts;

    for (int i = 0; i < 2; ++i) {
        float h = -zPos / 2.0f + i * zPos; // Height -> h / 2 or -h / 2
        float red = RandomFloat();
        float green = RandomFloat();
        float blue = RandomFloat();

        // Set origins position and color
        cylinderVerts.push_back(0.0f);
        cylinderVerts.push_back(0.0f);
        cylinderVerts.push_back(h);

        cylinderVerts.push_back(0.8f); // r
        cylinderVerts.push_back(0.9f); // g
        cylinderVerts.push_back(0.8f); // b
        cylinderVerts.push_back(1.0f);

        // Set intial x and y position and color 
        cylinderVerts.push_back(initX);
        cylinderVerts.push_back(initY);
        cylinderVerts.push_back(h);

        cylinderVerts.push_back(red); // r
        cylinderVerts.push_back(green); // g
        cylinderVerts.push_back(blue); // b
        cylinderVerts.push_back(1.0f);

        // Calculate new x and y for each sector
        for (int i = 0; i < SECTOR_COUNT; ++i) {
            angle = 3.1415926 * 2.0f * i / SECTOR_COUNT;
            float newX = radius * sin(angle);
            float newY = -radius * cos(angle);

            cylinderVerts.push_back(newX);
            cylinderVerts.push_back(newY);
            cylinderVerts.push_back(h);

            // Color
            cylinderVerts.push_back(red);
            cylinderVerts.push_back(green);
            cylinderVerts.push_back(blue);
            cylinderVerts.push_back(1.0f);
        }

    }

    return cylinderVerts;
}

// Generates vector of cylinder indices
vector <GLushort> GenCylinderIndices() {
    vector <GLushort> cylinderIndices;

    // Top circle cap
    for (GLushort i = 1; i < SECTOR_COUNT; ++i) {
        cylinderIndices.push_back(0);
        cylinderIndices.push_back(i);

        if (i == SECTOR_COUNT) {
            cylinderIndices.push_back(1);
        }
        else {
            cylinderIndices.push_back(i + 1);
        }
    }

    // Base circle
    for (GLushort i = SECTOR_COUNT + 2; i <= SECTOR_COUNT * 2 + 1; ++i) {
        cylinderIndices.push_back(SECTOR_COUNT + 1);
        cylinderIndices.push_back(i);

        if (i == SECTOR_COUNT * 2 + 1) {
            cylinderIndices.push_back(SECTOR_COUNT + 2);
        }
        else {
            cylinderIndices.push_back(i + 1);
        }
    }

    // Sides
    GLushort k1 = 1;
    GLushort k2 = SECTOR_COUNT + 2;

    for (int i = 0; i <= SECTOR_COUNT; ++i, ++k1, ++k2) {
        cylinderIndices.push_back(k1);
        cylinderIndices.push_back(k1 + 1);
        cylinderIndices.push_back(k2);
        cylinderIndices.push_back(k2);
        cylinderIndices.push_back(k2 + 1);
        cylinderIndices.push_back(k1);
    }

    return cylinderIndices;
}

// Generates random floats between 0.0 - 1.0 for coloring cylinder vertices
float RandomFloat() {
    const float FLOAT_MIN = 0.0f;
    const float FLOAT_MAX = 1.0f;

    random_device rd;
    default_random_engine eng(rd());
    uniform_real_distribution<float>distr(FLOAT_MIN, FLOAT_MAX);

    return distr(eng);
}

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j) {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;
        for (int i = width * channels; i > 0; --i) {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}

int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the mesh
    UCreateCartonMesh(cartonMesh, cartonVerts, cartonIndices);
    UCreateCartonMesh(cartonCapMesh, cartonCapVerts, cartonCapIndices);
    UCreateMesh(gMesh);

    // Create the shader program
    if (!UCreateShaderProgram(vertexShaderSource, fragmentShaderSource, gProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lampVertexShaderSource, lampFragmentShaderSource, gLampProgramId))
        return EXIT_FAILURE;

    // Load texture
    const char* texFilename = "C:/Users/ar274/Desktop/Final/Module Four Milestone/resources/textures/Milk.jpg";

    if (!UCreateTexture(texFilename, gTextureId)) {
        cout << "Failed to load texture " << texFilename << endl;
        return EXIT_FAILURE;
    }

    glUseProgram(gProgramId); // tell opengl for each sampler to which texture unit it belongs to
    
    glUniform1i(glGetUniformLocation(gProgramId, "uTexture"), 0); // We set the texture as texture unit 0

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    // Release mesh data
    UDestroyMesh(cartonMesh);
    UDestroyMesh(cartonCapMesh);
    UDestroyMesh(gMesh);

    // Release texture
    UDestroyTexture(gTextureId);

    // Release shader program
    UDestroyShaderProgram(gProgramId);
    UDestroyShaderProgram(gLampProgramId);

    exit(EXIT_SUCCESS); // Terminates the program successfully
}

// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);
    glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    // Camera Speed    
    static float cameraSpeed = 2.5f * gDeltaTime;

    // Escape Key Pressed - Escape the window
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }

    // Escape Key Pressed - Move camera forward
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
        cout << "W key pressed" << endl;
    }

    // S Key Pressed - Move camera backward
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
        cout << "S key pressed" << endl;
    }

    // A Key Pressed - Move camera left
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
        cout << "A key pressed" << endl;
    }

    // D Key Pressed - Move camera right
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
        cout << "D key pressed" << endl;
    }

    // S Key Pressed - Move camera backward
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        gCamera.ProcessKeyboard(UP, gDeltaTime);
        cout << "Q key pressed" << endl;
    }

    // S Key Pressed - Move camera backward
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);
        cout << "E key pressed" << endl;
    }

    // Z Key Pressed - Lower camera speed
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        if (gCamera.MovementSpeed > 0.01f) {
            gCamera.MovementSpeed -= 0.01f;
            cout << "Camera Speed: " << gCamera.MovementSpeed << endl;
        }
        else {
            gCamera.MovementSpeed = 0.01f;
        }
    }

    // C Key Pressed - Higher camera speed
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        if (gCamera.MovementSpeed < 10.0f) {
            gCamera.MovementSpeed += 0.01f;
            cout << "Camera Speed: " << gCamera.MovementSpeed << endl;
        }
        else {
            gCamera.MovementSpeed = 10.0f;
        }
    }

    // X Key Pressed - Reset camera speed
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        gCamera.MovementSpeed = 2.5f;
        cout << "Camera Speed Reset: " << gCamera.MovementSpeed << endl;
    }

    // X Key Pressed - Reset camera speed
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
        // Reset camera  
        gCamera = cameraPerspective;
    }   

    // X Key Pressed - Reset camera speed
    if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS) {
        // Reset camera  
        gCamera = cameraOrtho;
    }

    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && gTexWrapMode != GL_REPEAT)
    {
        glBindTexture(GL_TEXTURE_2D, gTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_REPEAT;

        cout << "Current Texture Wrapping Mode: REPEAT" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && gTexWrapMode != GL_MIRRORED_REPEAT)
    {
        glBindTexture(GL_TEXTURE_2D, gTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_MIRRORED_REPEAT;

        cout << "Current Texture Wrapping Mode: MIRRORED REPEAT" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && gTexWrapMode != GL_CLAMP_TO_EDGE)
    {
        glBindTexture(GL_TEXTURE_2D, gTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_CLAMP_TO_EDGE;

        cout << "Current Texture Wrapping Mode: CLAMP TO EDGE" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && gTexWrapMode != GL_CLAMP_TO_BORDER)
    {
        float color[] = { 1.0f, 0.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, color);

        glBindTexture(GL_TEXTURE_2D, gTextureId);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glBindTexture(GL_TEXTURE_2D, 0);

        gTexWrapMode = GL_CLAMP_TO_BORDER;

        cout << "Current Texture Wrapping Mode: CLAMP TO BORDER" << endl;
    }

    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS)
    {
        gUVScale += 0.1f;
        cout << "Current scale (" << gUVScale[0] << ", " << gUVScale[1] << ")" << endl;
    }
    else if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS)
    {
        gUVScale -= 0.1f;
        cout << "Current scale (" << gUVScale[0] << ", " << gUVScale[1] << ")" << endl;
    }

    // Pause and resume lamp orbiting
    static bool isLKeyDown = false;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !gIsLampOrbiting)
        gIsLampOrbiting = true;
    else if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS && gIsLampOrbiting)
        gIsLampOrbiting = false;
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT: {
        if (action == GLFW_PRESS) {
            cout << "Left mouse button pressed" << endl;
        }
        else {
            cout << "Left mouse button released" << endl;
        }
        break;
    }
    case GLFW_MOUSE_BUTTON_MIDDLE: {
        if (action == GLFW_PRESS) {
            cout << "Middle mouse button pressed" << endl;
        }
        else {
            cout << "Middle mouse button released" << endl;
        }
        break;
    }
    case GLFW_MOUSE_BUTTON_RIGHT: {
        if (action == GLFW_PRESS) {
            cout << "Right mouse button pressed" << endl;
        }
        else {
            cout << "Right mouse button released" << endl;
        }
        break;
    }

    default:
        cout << "Unhandled mouse button event" << endl;
        break;
    }
}

// Functioned called to render a frame
void URender()
{
    // Lamp orbits around the origin
    const float angularVelocity = glm::radians(45.0f);
    if (gIsLampOrbiting) {
        glm::vec4 newPosition = glm::rotate(angularVelocity * gDeltaTime, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(gLightPosition, 1.0f);
        gLightPosition.x = newPosition.x;
        gLightPosition.y = newPosition.y;
        gLightPosition.z = newPosition.z;
    }

   // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(gMesh.vao);   

    // Set the shader to be used
    glUseProgram(gProgramId);

    // Scale, rotation, and translation for carton model matrix
    // 1. Scales the object by 2
    glm::mat4 cartonScale = glm::scale(glm::vec3(0.7f, 0.7f, 0.7f));
    // 2. Rotates shape by 40 degrees in the x axis
    glm::mat4 cartonRotation = glm::rotate(15.2f, glm::vec3(0.0, 1.0f, 0.0f));
    // 3. Place object at the origin
    glm::mat4 cartonTranslation = glm::translate(glm::vec3(0.5f, 3.0f, -4.0f));

    // Scale, rotation, and translation for cap model matrix
    // 1. Scales the object
    glm::mat4 cartonCapScale = glm::scale(glm::vec3(0.4f, 0.4f, 0.4f));
    // 2. Rotates shape
    glm::mat4 cartonCapRotation = glm::rotate(15.26f, glm::vec3(0.1f, 1.0f, -0.6f));
    // 3. Place object at the origin
    glm::mat4 cartonCapTranslation = glm::translate(glm::vec3(0.21f, 2.75f, -3.5f));
    
    // Scale, rotation, and translation for table triangle 1 model matrix
    // 1. Scales the object by 2
    glm::mat4 tableScale = glm::scale(glm::vec3(575.5f, 35.4f, 20.2f));
    // 2. Rotates shape
    glm::mat4 tableRotation = glm::rotate(1.57f, glm::vec3(1.0, 0.0f, 0.0f));
    // 3. Place object
    glm::mat4 tableTranslation = glm::translate(glm::vec3(-2.7f, -0.77f, -0.75f));

    // Scale, rotation, and translation for table triangle 2 model matrix
    // 1. Scales the object
    glm::mat4 tableScale2 = glm::scale(glm::vec3(575.5f, 35.4f, 20.2f));
    // 2. Rotates shapes
    glm::mat4 tableRotation2 = glm::rotate(-1.57f, glm::vec3(1.0, 0.0f, 0.0f));
    // 3. Place object
    glm::mat4 tableTranslation2 = glm::translate(glm::vec3(-2.7f, 3.25f, -7.9f));

    // Model matrix: transformations are applied right-to-left order
    glm::mat4 cartonModel = cartonTranslation * cartonRotation * cartonScale;
    glm::mat4 cartonCapModel = cartonCapTranslation * cartonCapRotation * cartonCapScale;
    glm::mat4 tableModel = tableTranslation * tableRotation * tableScale;
    glm::mat4 tableModel2 = tableTranslation2 * tableRotation2 * tableScale2;

    // Model matrix: transformations are applied right-to-left order
    glm::mat4 model = glm::translate(gCubePosition) * glm::scale(gCubeScale);

    glm::mat4 view = gCamera.GetViewMatrix();

    // Create a perspective projection
    glm::mat4 projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gProgramId, "view");
    GLint projLoc = glGetUniformLocation(gProgramId, "projection");

    // Draws the carton
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(cartonModel));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection)); 

    // Reference matrix uniforms from the Cube Shader program for the cube color, light color, light position, and camera position
    GLint objectColorLoc = glGetUniformLocation(gProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gProgramId, "viewPosition");

    // Pass color, light, and camera data to the Cube Shader program's corresponding uniforms 
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);

    const glm::vec3 cameraPosition = gCamera.Position;

    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint UVScaleLoc = glGetUniformLocation(gProgramId, "uvScale");

    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, gTextureId);

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(cartonMesh.vao);

    glDrawElements(GL_TRIANGLES, cartonIndices.size(), GL_UNSIGNED_SHORT, NULL); // Draws the triangle

    // Activate the VBOs contained within the mesh's VAO
    glBindVertexArray(cartonCapMesh.vao);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(cartonCapModel));

    glDrawElements(GL_TRIANGLES, cartonCapIndices.size(), GL_UNSIGNED_SHORT, NULL); // Draws the triangle
    
    // Draws the table 1
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(tableModel));

    glDrawElements(GL_TRIANGLES, gMesh.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle

     // Draws the table 2
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(tableModel2));

    glDrawElements(GL_TRIANGLES, gMesh.nIndices, GL_UNSIGNED_SHORT, NULL); // Draws the triangle   

    // Set the shader to be used
    glUseProgram(gLampProgramId);

    // Transform the smaller cube used as a visual que for the light source 
    model = glm::translate(gLightPosition) * glm::scale(gLightScale);

    // Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(gLampProgramId, "model");
    viewLoc = glGetUniformLocation(gLampProgramId, "view");
    projLoc = glGetUniformLocation(gLampProgramId, "projection");

    // Pass matrix data to teh Lamp Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glDrawArrays(GL_TRIANGLES, 0, gMesh.nIndices);

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
    glUseProgram(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}

void UCreateMesh(GLMesh& mesh) {
    GLfloat verts[] = {
        0.6f, -0.5f, -1.1f,  1.0f, 0.0f, 0.0f, 1.0f, // Back side base vertex 0
        0.6f, -0.5f,  0.1f,  0.0f, 1.0f, 0.0f, 1.0f, // Front right base vertex 1
       -0.6f, -0.5f, -1.1f,  0.0f, 0.0f, 1.0f, 1.0f, // Back-left base vertex 2
       -0.6f, -0.5f,  0.1f,  0.5f, 0.2f, 0.0f, 1.0f, // Front-left base vertex3
        0.0f, -0.2f, -0.5f,  0.2f, 0.3f, 1.0f, 1.0f,
    };

    GLushort indices[] = {
        81, 82, 83,
        81, 84, 83,
    };

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerColor = 4;

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
}

// Implements the UCreateMesh function
void UCreateCartonMesh(GLMesh& mesh, vector<GLfloat>& verts, vector <GLushort>& indices) {

    // Create the vertex attribute pointer
    const GLuint floatsPerVertex = 3; // number of coordinates per vertex
    const GLuint floatsPerColor = 4; // (r, g, b, a) 

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(2, mesh.vbos);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * verts.size(), &verts[0], GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLshort) * indices.size(), &indices[0], GL_STATIC_DRAW);

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);
}

void UDestroyMesh(GLMesh& mesh)
{
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(2, mesh.vbos);
}

//Generate and load the texture
bool UCreateTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);

    if (image) {
        flipImageVertically(image, width, height, channels);
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        }
        else if (channels == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        }
        else {
            cout << "Not implemented to handle image with " << channels << "channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);

        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }
    return false; // Error loading the image
}

void UDestroyTexture(GLuint textureId)
{
    glGenTextures(1, &textureId);
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader

    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}

void UDestroyShaderProgram(GLuint programId)
{
    glDeleteProgram(programId);
}
