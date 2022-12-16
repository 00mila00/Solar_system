#include "imgui.h"
#include "imgui_impl/imgui_impl_glfw.h"
#include "imgui_impl/imgui_impl_opengl3.h"
#include <stdio.h>

#define IMGUI_IMPL_OPENGL_LOADER_GLAD

// You may use another loader/header of your choice (glext, glLoadGen, etc.), or chose to manually implement your own.
#if defined(IMGUI_IMPL_OPENGL_LOADER_GL3W)
#include <GL/gl3w.h>    // Initialize with gl3wInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLEW)
#include <GL/glew.h>    // Initialize with glewInit()
#elif defined(IMGUI_IMPL_OPENGL_LOADER_GLAD)
#include <glad/glad.h>  // Initialize with gladLoadGL()
#else
#include IMGUI_IMPL_OPENGL_LOADER_CUSTOM
#endif

#include <GLFW/glfw3.h> // Include glfw3.h after our OpenGL definitions
#include <spdlog/spdlog.h>

#include "..\..\src\Mesh.h"
#include "..\..\src\Shader.h"
#include "..\..\src\Model.h"
#include "..\..\src\Camera.h"

#define PI 3.14159265
#define Cos(th) cos(PI/180*(th))
#define Sin(th) sin(PI/180*(th))

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);

// settings
const unsigned int SCR_WIDTH = 1400;
const unsigned int SCR_HEIGHT = 900;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float atime = 0.0f;

// VARIABLES
float x_rotation = 0.25;
float y_rotation = 0.0;
float z_rotation = 0.0;

bool wireframe_mode = false;
float speed = 0.02f;
int sideDegree = 50;
float transparency = 0.5f;

unsigned int VAO;
unsigned int VBO;

std::vector <glm::mat4> models;
std::vector <glm::vec3> vertices;
std::vector <glm::vec3> orbit_vertices;
std::vector <glm::vec3> stars;

// METHODS
void generateTexture(GLuint tex, const char* filename);
void createCone(int sides, float height);

int main()
{
    // glfw: initialize and configure
    const char* glsl_version = "#version 430";
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Solar System", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    GLFWimage images[1];
    images[0].pixels = stbi_load("../../Textures/sattelite_icon.png", &images[0].width, &images[0].height, 0, 4); //rgba channels 
    glfwSetWindowIcon(window, 1, images);
    stbi_image_free(images[0].pixels);

    // tell stb_image.h to flip loaded texture's on the y-axis (before loading model).
    //stbi_set_flip_vertically_on_load(true);

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders
    Shader ourShader("shader.vert", "shader.frag");

    // load models
    Model planet("../../res/models/sphere.obj");
    Model sattelite("../../res/models/Sattelite.obj");
    Model orbit("../../res/models/orbit.obj");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImGui::StyleColorsClassic();

    for (int i = 0; i < 250; i++) {
        stars.push_back(glm::vec3((rand() % 50 + 1) - 20, (rand() % 50 + 1) - 25, (rand() % 100 + 1) - 90));
    }

    //create cone and buffers for cone data
    createCone(sideDegree, 2.0);
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) * vertices.size(), &vertices.front(), GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    GLuint texture[3];
    glGenTextures(3, texture);
    generateTexture(texture[0], "../../res/models/Earth.jpg");
    generateTexture(texture[1], "../../Textures/Sun.jpg");
    generateTexture(texture[2], "../../Textures/pink.jpg");

    GLint textureLocation = glGetUniformLocation(ourShader.ID, "texture");

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        glm::vec4 planet1_color = glm::vec4(1.0f, 1.0f, 1.0f, transparency);
        glm::vec4 planet2_color = glm::vec4(1.0f, 0.0f, 1.0f, transparency);
        glm::vec4 planet3_color = glm::vec4(0.0f, 1.0f, 1.0f, transparency);
        glm::vec4 planet4_color = glm::vec4(1.0f, 0.0f, 0.0f, transparency);
        glm::vec4 planet5_color = glm::vec4(0.0f, 0.0f, 1.0f, transparency);
        
        // per-frame time logic
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        processInput(window);

        // render
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // don't forget to enable shader before setting uniforms
        ourShader.use();
        // view/projection transformations
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // SCENE GRAPH
        glm::mat4 model = glm::mat4(1.0f);
        // SUN
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[1]);
        glUniform1i(textureLocation, 0);
        model = scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
        model = glm::rotate(model, x_rotation, glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, y_rotation, glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, z_rotation, glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, atime/4 * glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ourShader.setInt("is_texture", 1);
        ourShader.setVec4("ourColor", 1.0f, 1.0f, 0.0f, 1.0f);
        ourShader.setMat4("model", model);
        planet.Draw(ourShader);
        ourShader.setInt("is_texture", 0);
        
        // orbit 1 
        model = scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
        ourShader.setVec4("ourColor", planet1_color);
        ourShader.setMat4("model", model);
        orbit.Draw(ourShader);
       
        // orbit 2
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[2]);
        glUniform1i(textureLocation, 0);
        ourShader.setInt("is_texture", 1);
        model = scale(model, 2.0f * glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setVec4("ourColor", planet2_color);
        ourShader.setMat4("model", model);  
        orbit.Draw(ourShader);
        
        // orbit 3
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[0]);
        glUniform1i(textureLocation, 0);
        model = scale(model, 1.6f * glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setVec4("ourColor", planet3_color);
        ourShader.setMat4("model", model);
        orbit.Draw(ourShader);
        ourShader.setInt("is_texture", 0);

        // orbit 4
        model = scale(model, 1.6f * glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setVec4("ourColor", planet4_color);
        ourShader.setMat4("model", model);
        orbit.Draw(ourShader);
        models.push_back(model);
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // PLANET 1
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[0]);
        glUniform1i(textureLocation, 0);
        model = glm::rotate(model, atime / 4 * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));	        //orbital rotation
        model = translate(model, glm::vec3(19.0f, 0.0f, 0.0f));									            //orbital position
        model = scale(model, glm::vec3(1.2f, 1.2f, 1.2f));										            //size
        model = glm::rotate(model, glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f));			            //axis tilt
        model = glm::rotate(model, atime / 4 * glm::radians(130.0f), glm::vec3(0.0f, 1.0f, 0.0f));          //axis rotation
        ourShader.setVec4("ourColor", 1.0f, 1.0f, 1.0f, 1.0f);									            //color
        ourShader.setMat4("model", model);
        planet.Draw(ourShader);
        ourShader.setInt("is_texture", 0);
        
        // orbit 1
        model = scale(model, 0.03f * glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setVec4("ourColor", 0.8f, 0.8f, 1.0f, transparency);
        ourShader.setMat4("model", model);
        orbit.Draw(ourShader);

        // orbit 2
        model = scale(model, 1.6f * glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setVec4("ourColor", 0.8f, 0.8f, 1.0f, transparency);
        ourShader.setMat4("model", model);
        orbit.Draw(ourShader);
        models.push_back(model);

        //----------------------------------------------------------------------------------------------------------------
        // MOON 1			
        model = glm::rotate(model, atime/8 * glm::radians(130.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = translate(model, glm::vec3(100.0f, 0.0f, 0.0f));
        model = scale(model, 15.0f * glm::vec3(1.0f, 1.0f, 1.0f));

        ourShader.setVec4("ourColor", 0.8f, 0.8f, 1.0f, 1.0f);
        ourShader.setMat4("model", model);

        createCone(sideDegree, 2.0);
        glBindVertexArray(VAO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) * vertices.size(), &vertices.front(), GL_STATIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());
        model = models.at(models.size() - 1);
        
        // MOON 2			
        model = glm::rotate(model, atime / 8 * glm::radians(100.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = translate(model, glm::vec3(60.0f, 0.0f, 0.0f));
        model = scale(model, 10.0f * glm::vec3(1.0f, 1.0f, 1.0f));
        model = glm::rotate(model, glm::radians(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));			            //axis tilt
 
        ourShader.setVec4("ourColor", 0.8f, 0.6f, 1.0f, 1.0f);
        ourShader.setMat4("model", model);

        createCone(sideDegree, 2.0);
        glBindVertexArray(VAO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices)* vertices.size(), &vertices.front(), GL_STATIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());

        models.pop_back();
        model = models.at(models.size() - 1);
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // PLANET 2
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[2]);
        glUniform1i(textureLocation, 0);
        models.push_back(model);
        model = glm::rotate(model, atime / 4 * glm::radians(-60.0f), glm::vec3(0.0f, 1.0f, 0.0f));	        //orbital rotation
        model = translate(model, glm::vec3(38.0f, 0.0f, 0.0f));									            //orbital position
        model = scale(model, glm::vec3(0.9f, 0.9f, 0.9f));										            //size
        model = glm::rotate(model, glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f));			            //axis tilt
        model = glm::rotate(model, atime / 4 * glm::radians(130.0f), glm::vec3(0.0f, 1.0f, 0.0f));          //axis rotation
     
        ourShader.setInt("is_texture", 1);
        ourShader.setVec4("ourColor", 1.0f, 0.0f, 1.0f, 1.0f);									            //color
        ourShader.setMat4("model", model);
        sattelite.Draw(ourShader);
        model = models.at(models.size() - 1);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // PLANET 3
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture[0]);
        glUniform1i(textureLocation, 0);
        models.push_back(model);
        model = glm::rotate(model, atime / 4 * glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));	        //orbital rotation
        model = translate(model, glm::vec3(62.0f, 0.0f, 0.0f));									            //orbital position
        model = scale(model, glm::vec3(2.5f, 2.5f, 2.5f));										            //size
        model = glm::rotate(model, glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f));			            //axis tilt
        model = glm::rotate(model, atime / 4 * glm::radians(130.0f), glm::vec3(0.0f, 1.0f, 0.0f));          //axis rotation
        ourShader.setInt("is_texture", 1);
        ourShader.setVec4("ourColor", 1.0f, 1.0f, 1.0f, 1.0f);									            //color
        ourShader.setMat4("model", model);
        planet.Draw(ourShader);
        ourShader.setInt("is_texture", 0);
        models.push_back(model);

        // orbit 1
        model = scale(model, 0.03f * glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setVec4("ourColor", 0.8f, 0.8f, 1.0f, transparency);
        ourShader.setMat4("model", model);
        orbit.Draw(ourShader);
       
        // orbit 2
        model = scale(model, 2.0f * glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setVec4("ourColor", 0.8f, 0.8f, 1.0f, transparency);
        ourShader.setMat4("model", model);
        orbit.Draw(ourShader);
        //----------------------------------------------------------------------------------------------------------------
        // MOON 1			
        model = glm::rotate(model, atime / 4 * glm::radians(40.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = translate(model, glm::vec3(100.0f, 0.0f, 0.0f));
        model = scale(model, 5.0f * glm::vec3(0.9f, 0.9f, 0.9f));
        model = glm::rotate(model, glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f));			            //axis tilt
        ourShader.setVec4("ourColor", 1.0f, 1.0f, 1.0f, 1.0);
        ourShader.setMat4("model", model);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());
        model = models.at(models.size() - 1);

        // MOON 2
        model = glm::rotate(model, atime / 4 * glm::radians(10.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = translate(model, glm::vec3(3.2f, 0.0f, 0.0f));
        model = scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
        ourShader.setVec4("ourColor", 0.5, 0.5, 0.5, 1.0);
        ourShader.setMat4("model", model);
        sattelite.Draw(ourShader);
        models.pop_back();
        model = models.at(models.size() - 1);
        //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // PLANET 4
        models.push_back(model);
        model = glm::rotate(model, atime / 4  *  glm::radians(25.0f), glm::vec3(0.0f, 1.0f, 0.0f));	//orbital rotation
        model = translate(model, glm::vec3(100.0f, 0.0f, 0.0f));									        //orbital position
        model = scale(model, glm::vec3(2.5f, 2.5f, 2.5f));										            //size
        model = glm::rotate(model, glm::radians(-10.0f), glm::vec3(1.0f, 0.0f, 0.0f));			            //axis tilt
        model = glm::rotate(model, atime / 4 * glm::radians(130.0f), glm::vec3(0.0f, 1.0f, 0.0f));          //axis rotation
        ourShader.setVec4("ourColor", 1.0f, 0.0f, 0.0f,1.0f);									            //color
        ourShader.setMat4("model", model);
        planet.Draw(ourShader);
        models.push_back(model);

        // orbit 1
        model = scale(model, 0.03f * glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setVec4("ourColor", 0.8f, 0.8f, 1.0f, transparency);
        ourShader.setMat4("model", model);
        orbit.Draw(ourShader);

        // orbit 2
        model = scale(model, 1.5f * glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setVec4("ourColor", 0.8f, 0.8f, 1.0f, transparency);
        ourShader.setMat4("model", model);
        orbit.Draw(ourShader);

        // orbit 3
        model = scale(model, 1.5f * glm::vec3(1.0f, 1.0f, 1.0f));
        ourShader.setVec4("ourColor", 0.8f, 0.8f, 1.0f, transparency);
        ourShader.setMat4("model", model);
        orbit.Draw(ourShader);
        //----------------------------------------------------------------------------------------------------------------
        // MOON 1
        model = glm::rotate(model, atime / 4 * glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = translate(model, glm::vec3(8.2f, 0.0f, 0.0f));
        model =  scale(model, 0.4f * glm::vec3(0.4f, 0.4f, 0.4f));
        model = glm::rotate(model, glm::radians(2.0f), glm::vec3(1.0f, 0.0f, 0.0f));			            
        ourShader.setVec4("ourColor", 0.5, 0.5, 0.5, 1.0);
        ourShader.setMat4("model", model);
        sattelite.Draw(ourShader);
        model = models.at(models.size() - 1);

        // MOON 2
        model = glm::rotate(model, atime / 4 * glm::radians(20.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = translate(model, glm::vec3(3.2f, 0.0f, 0.0f));
        model = scale(model, 0.5f * glm::vec3(0.5f, 0.5f, 0.5f));
        model = glm::rotate(model, glm::radians(1.0f), glm::vec3(1.0f, 0.0f, 0.0f));			            
        ourShader.setVec4("ourColor", 0.4, 0.4, 0.4, 1.0);
        ourShader.setMat4("model", model);
        sattelite.Draw(ourShader);
        model = models.at(models.size() - 1);

        // MOON 3
        model = glm::rotate(model, atime / 4 * glm::radians(60.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = translate(model, glm::vec3(4.2f, 0.0f, 0.0f));
        model = scale(model, 0.5f * glm::vec3(0.6f, 0.6f, 0.6f));
        model = glm::rotate(model, glm::radians(-2.0f), glm::vec3(1.0f, 0.0f, 0.0f));			            
        ourShader.setVec4("ourColor", 0.3, 0.3, 0.3, 1.0);
        ourShader.setMat4("model", model);
        sattelite.Draw(ourShader);
        model = models.at(models.size() - 1);

        // MOON 4
        model = glm::rotate(model, atime / 8 * glm::radians(160.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = translate(model, glm::vec3(6.8f, 0.0f, 0.0f));
        model = scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
        model = glm::rotate(model, glm::radians(-1.0f), glm::vec3(1.0f, 0.0f, 0.0f));			            
        ourShader.setVec4("ourColor", 0.5, 0.2, 0.5, 1.0);
        ourShader.setMat4("model", model);
        sattelite.Draw(ourShader);

        models.pop_back();
        model = models.at(models.size() - 1);

        // stars
        ourShader.setVec4("ourColor", 1.0, 1.0, 1.0, 1.0);
        glBindVertexArray(VAO);
        for (unsigned int i = 0; i < 250; i++)
        {
            // calculate the model matrix for each object and pass it to shader before drawing
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, stars[i]);
            
                if ((int)(atime * 10) % 4 == 0) {
                    model = glm::scale(model, 0.3f * glm::vec3(0.1, 0.1, 0.1));
                }
                else {
                    model = glm::scale(model, 0.5f * glm::vec3(0.1, 0.1, 0.1));
                }
            float angle = 20.0f * i;
            model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
            ourShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, vertices.size());
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::Begin("View settings");

            ImGui::SliderFloat("X Rotation", &x_rotation, -PI, PI);
            ImGui::SliderFloat("Y Rotation", &y_rotation, -PI, PI);
            ImGui::SliderFloat("Z Rotation", &z_rotation, -PI, PI);

            ImGui::SliderFloat("Speed", &speed, 0.0f, 0.1f);
            ImGui::SliderFloat("Transparency", &transparency, 0.0f, 1.0f);

            ImGui::Spacing();
            ImGui::Spacing();
            if (ImGui::Button("Wireframe mode"))
            {
                if (wireframe_mode)
                {
                    wireframe_mode = false;
                }
                else
                {
                    wireframe_mode = true;
                }
            }

            if (wireframe_mode)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            }
            else
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }

            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::SliderInt("Degrees step", &sideDegree, 1, 60);

            ImGui::End();
        }

        atime += speed/2;

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

void createCone(int details, float height)
{
    vertices.clear();

    //sides
    for (int k = 0; k <= 360; k += details)
    {
        vertices.push_back(glm::vec3(0.0, 0.0, height));
        vertices.push_back(glm::vec3(Cos(k), Sin(k), 0.0));
        vertices.push_back(glm::vec3(Cos(k + details), Sin(k + details), 0.0));
    }

    //base
   
    for (int k = 0; k <= 360; k += details)
    {
        vertices.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
        vertices.push_back(glm::vec3(Cos(k), Sin(k), 0.0f));
        vertices.push_back(glm::vec3(Cos(k + details), Sin(k + details), 0.0f));
    }
}
void generateTexture(GLuint texture, const char* filename) {
    int w, h, n;
    glBindTexture(GL_TEXTURE_2D, texture);
    unsigned char* data = stbi_load(filename, &w, &h, &n, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    stbi_image_free(data);

}