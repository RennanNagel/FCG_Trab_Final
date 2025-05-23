#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <map>
#include <stack>
#include <string>
#include <limits>
#include <fstream>
#include <sstream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "utils.h"
#include "matrices.h"

void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

void DrawCube(GLint render_as_black_uniform, bool draw_axes);
GLuint BuildTriangles();
void LoadShadersFromFiles();
GLuint LoadShader_Vertex(const char* filename);
GLuint LoadShader_Fragment(const char* filename);
void LoadShader(const char* filename, GLuint shader_id);
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id);

void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

struct SceneObject
{
    const char*  name;
    void*        first_index;
    int          num_indices;
    GLenum       rendering_mode;
};

std::map<const char*, SceneObject> g_VirtualScene;
std::stack<glm::mat4>  g_MatrixStack;
float g_ScreenRatio = 1.0f;
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false;
bool g_MiddleMouseButtonPressed = false;

float g_CameraTheta = 0.0f;
float g_CameraPhi = 0.0f;
float g_CameraDistance = 3.5f;

float g_HeadPositionX = 0.0f;
float g_HeadPositionY = 0.0f;
float g_HeadPositionZ = 0.0f;

// Camera target position (where it's trying to get to)
float g_CameraTargetX = 0.0f;
float g_CameraTargetY = 0.0f;
float g_CameraTargetZ = 0.0f;

float g_HeadAngleX = 0.0f;
float g_HeadAngleY = 0.0f;

bool g_UsePerspectiveProjection = true;
bool g_ShowInfoText = true;
GLuint g_GpuProgramID = 0;

int main()
{
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    glfwSetErrorCallback(ErrorCallback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    window = glfwCreateWindow(800, 800, "INF01047 - 00315453 - Lucas Caique dos Santos Nogueira", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetScrollCallback(window, ScrollCallback);

    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetWindowSize(window, 800, 800);

    glfwMakeContextCurrent(window);

    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    LoadShadersFromFiles();

    GLuint vertex_array_object_id = BuildTriangles();

    TextRendering_Init();

    GLint model_uniform           = glGetUniformLocation(g_GpuProgramID, "model");
    GLint view_uniform            = glGetUniformLocation(g_GpuProgramID, "view");
    GLint projection_uniform      = glGetUniformLocation(g_GpuProgramID, "projection");
    GLint render_as_black_uniform = glGetUniformLocation(g_GpuProgramID, "render_as_black");

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    while (!glfwWindowShouldClose(window))
    {
        // Calculate movement vectors
        glm::vec3 forward = glm::vec3(
            -cos(g_CameraPhi) * sin(g_CameraTheta),
            0.0f,
            -cos(g_CameraPhi) * cos(g_CameraTheta)
        );
        forward = glm::normalize(forward);
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

        // Handle movement using direct key state checks
        float move_delta = 0.1f;
        glm::vec3 movement(0.0f);
        
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement += forward;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement -= forward;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement += right;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement -= right;

        // Normalize diagonal movement to maintain consistent speed
        if (glm::length(movement) > 0.0f)
        {
            movement = glm::normalize(movement) * move_delta;
            g_HeadPositionX += movement.x;
            g_HeadPositionZ += movement.z;
        }

        // Update camera target position
        float smoothing = 0.1f;
        g_CameraTargetX += (g_HeadPositionX - g_CameraTargetX) * smoothing;
        g_CameraTargetY += (g_HeadPositionY - g_CameraTargetY) * smoothing;
        g_CameraTargetZ += (g_HeadPositionZ - g_CameraTargetZ) * smoothing;

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(g_GpuProgramID);
        glBindVertexArray(vertex_array_object_id);

        float r = g_CameraDistance;
        float y = r*sin(g_CameraPhi);
        float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

        // Use camera target position instead of head position directly
        glm::vec4 camera_position_c  = glm::vec4(x + g_CameraTargetX, y + g_CameraTargetY, z + g_CameraTargetZ, 1.0f);
        glm::vec4 camera_lookat_l    = glm::vec4(g_CameraTargetX, g_CameraTargetY, g_CameraTargetZ, 1.0f);
        glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c;
        glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f);

        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);

        glm::mat4 projection;

        float nearplane = -0.1f;
        float farplane  = -30.0f;

        if (g_UsePerspectiveProjection)
        {
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        glUniformMatrix4fv(view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        float head_size = 0.4f;
        glm::mat4 model = Matrix_Identity();
        model = model * Matrix_Translate(g_HeadPositionX, head_size, g_HeadPositionZ);  
        model = model * Matrix_Rotate_X(g_HeadAngleX);
        model = model * Matrix_Rotate_Y(g_HeadAngleY);
        glm::mat4 head_model = model * Matrix_Scale(head_size, head_size, head_size);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(head_model));
        glUniform1i(render_as_black_uniform, false);
        DrawCube(render_as_black_uniform, true);  // Draw axes for head

        // Draw static cube
        float static_cube_size = 0.3f;  // Scale factor for static cube
        model = Matrix_Identity();
        model = model * Matrix_Translate(2.0f, static_cube_size, 0.0f);
        model = model * Matrix_Scale(static_cube_size, static_cube_size, static_cube_size);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(render_as_black_uniform, false);
        DrawCube(render_as_black_uniform, true);  // Draw axes for static cube

        // Draw walls
        float wall_distance = 10.0f;  // Distance of walls from center
        float wall_height = 2.0f;    // Height of walls
        
        model = Matrix_Identity();
        model = model * Matrix_Translate(0.0f, wall_height, -wall_distance);  
        model = model * Matrix_Scale(wall_distance*2, wall_height, 0.2f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        DrawCube(render_as_black_uniform, false);  // No axes for walls

        model = Matrix_Identity();
        model = model * Matrix_Translate(0.0f, wall_height, wall_distance);  // Move up by HALF the FINAL height
        model = model * Matrix_Scale(wall_distance*2, wall_height, 0.2f);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        DrawCube(render_as_black_uniform, false);  // No axes for walls

        model = Matrix_Identity();
        model = model * Matrix_Translate(wall_distance, wall_height, 0.0f);  // Move up by HALF the FINAL height
        model = model * Matrix_Scale(0.2f, wall_height, wall_distance*2);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        DrawCube(render_as_black_uniform, false);  // No axes for walls

        model = Matrix_Identity();
        model = model * Matrix_Translate(-wall_distance, wall_height, 0.0f);  // Move up by HALF the FINAL height
        model = model * Matrix_Scale(0.2f, wall_height, wall_distance*2);
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        DrawCube(render_as_black_uniform, false);  // No axes for walls

        // Draw floor - dark gray, covering entire room area
        GLint color_override_uniform = glGetUniformLocation(g_GpuProgramID, "color_override");
        glUniform4f(color_override_uniform, 0.2f, 0.2f, 0.2f, 1.0f);  // Dark gray with alpha=1 for override
        
        model = Matrix_Identity();
        model = model * Matrix_Translate(0.0f, 0.0f, 0.0f);  // At ground level
        model = model * Matrix_Scale(wall_distance*2, 0.1f, wall_distance*2);  // Make it thin but cover whole room
        glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        DrawCube(render_as_black_uniform, false);  // No axes for floor
        
        // Reset color override
        glUniform4f(color_override_uniform, 1.0f, 1.0f, 1.0f, 0.0f);  // alpha=0 means no override

        TextRendering_ShowEulerAngles(window);
        TextRendering_ShowProjection(window);
        TextRendering_ShowFramesPerSecond(window);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}

void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

void DrawCube(GLint render_as_black_uniform, bool draw_axes)
{
    glUniform1i(render_as_black_uniform, false);

    glDrawElements(
        g_VirtualScene["cube_faces"].rendering_mode,
        g_VirtualScene["cube_faces"].num_indices,
        GL_UNSIGNED_INT,
        (void*)g_VirtualScene["cube_faces"].first_index
    );

    if (draw_axes) {
        glLineWidth(4.0f);

        glDrawElements(
            g_VirtualScene["axes"].rendering_mode,
            g_VirtualScene["axes"].num_indices,
            GL_UNSIGNED_INT,
            (void*)g_VirtualScene["axes"].first_index
        );
    }

    glUniform1i(render_as_black_uniform, true);

    glDrawElements(
        g_VirtualScene["cube_edges"].rendering_mode,
        g_VirtualScene["cube_edges"].num_indices,
        GL_UNSIGNED_INT,
        (void*)g_VirtualScene["cube_edges"].first_index
    );
}

GLuint BuildTriangles()
{
    GLfloat model_coefficients[] = {
        -0.5f,  0.0f,  0.5f, 1.0f,
        -0.5f, -1.0f,  0.5f, 1.0f,
         0.5f, -1.0f,  0.5f, 1.0f,
         0.5f,  0.0f,  0.5f, 1.0f,
        -0.5f,  0.0f, -0.5f, 1.0f,
        -0.5f, -1.0f, -0.5f, 1.0f,
         0.5f, -1.0f, -0.5f, 1.0f,
         0.5f,  0.0f, -0.5f, 1.0f,
         0.0f,  0.0f,  0.0f, 1.0f,
         1.0f,  0.0f,  0.0f, 1.0f,
         0.0f,  0.0f,  0.0f, 1.0f,
         0.0f,  1.0f,  0.0f, 1.0f,
         0.0f,  0.0f,  0.0f, 1.0f,
         0.0f,  0.0f,  1.0f, 1.0f,
    };

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);

    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);

    glBindVertexArray(vertex_array_object_id);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);

    glBufferData(GL_ARRAY_BUFFER, sizeof(model_coefficients), NULL, GL_STATIC_DRAW);

    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(model_coefficients), model_coefficients);

    GLuint location = 0;
    GLint  number_of_dimensions = 4;
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(location);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLfloat color_coefficients[] = {
        1.0f, 0.5f, 0.0f, 1.0f,  // Orange color for cubes
        1.0f, 0.5f, 0.0f, 1.0f,
        0.0f, 0.5f, 1.0f, 1.0f,
        0.0f, 0.5f, 1.0f, 1.0f,
        1.0f, 0.5f, 0.0f, 1.0f,
        1.0f, 0.5f, 0.0f, 1.0f,
        0.0f, 0.5f, 1.0f, 1.0f,
        0.0f, 0.5f, 1.0f, 1.0f,
        1.0f, 0.0f, 0.0f, 1.0f,  // Red X axis
        1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,  // Green Y axis
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f,  // Blue Z axis
        0.0f, 0.0f, 1.0f, 1.0f,
    };
    GLuint VBO_color_coefficients_id;
    glGenBuffers(1, &VBO_color_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_color_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(color_coefficients), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(color_coefficients), color_coefficients);
    location = 1;
    number_of_dimensions = 4;
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint indices[] = {
        0, 1, 2,
        7, 6, 5,
        3, 2, 6,
        4, 0, 3,
        4, 5, 1,
        1, 5, 6,
        0, 2, 3,
        7, 5, 4,
        3, 6, 7,
        4, 3, 7,
        4, 1, 0,
        1, 6, 2,
        0, 1,
        1, 2,
        2, 3,
        3, 0,
        0, 4,
        4, 7,
        7, 6,
        6, 2,
        6, 5,
        5, 4,
        5, 1,
        7, 3,
        8 , 9,
        10, 11,
        12, 13
    };

    GLuint indices_id;
    glGenBuffers(1, &indices_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);

    SceneObject cube_faces;
    cube_faces.name = "Cube (colored faces)";
    cube_faces.first_index = (void*)0;
    cube_faces.num_indices = 36;
    cube_faces.rendering_mode = GL_TRIANGLES;
    g_VirtualScene["cube_faces"] = cube_faces;

    SceneObject cube_edges;
    cube_edges.name = "Cube (black edges)";
    cube_edges.first_index = (void*)(36*sizeof(GLuint));
    cube_edges.num_indices = 24;
    cube_edges.rendering_mode = GL_LINES;
    g_VirtualScene["cube_edges"] = cube_edges;

    SceneObject axes;
    axes.name = "XYZ Axes";
    axes.first_index = (void*)(60*sizeof(GLuint));
    axes.num_indices = 6;
    axes.rendering_mode = GL_LINES;
    g_VirtualScene["axes"] = axes;

    glBindVertexArray(0);

    return vertex_array_object_id;
}

GLuint LoadShader_Vertex(const char* filename)
{
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
    LoadShader(filename, vertex_shader_id);
    return vertex_shader_id;
}

GLuint LoadShader_Fragment(const char* filename)
{
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);
    LoadShader(filename, fragment_shader_id);
    return fragment_shader_id;
}

void LoadShader(const char* filename, GLuint shader_id)
{
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint shader_string_length = static_cast<GLint>( str.length() );

    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);
    glCompileShader(shader_id);

    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    if ( log_length != 0 )
    {
        std::string output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    delete [] log;
}

void LoadShadersFromFiles()
{
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);
}

GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    GLuint program_id = glCreateProgram();

    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    glLinkProgram(program_id);

    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    return program_id;
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    g_ScreenRatio = (float)width / height;
}

double g_LastCursorPosX, g_LastCursorPosY;

void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        g_MiddleMouseButtonPressed = false;
    }
}

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (g_LeftMouseButtonPressed)
    {
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        g_CameraTheta -= 0.01f*dx;
        g_CameraPhi   += 0.01f*dy;

        float phimax = 3.141592f/2;
        float phimin = -phimax;

        if (g_CameraPhi > phimax)
            g_CameraPhi = phimax;

        if (g_CameraPhi < phimin)
            g_CameraPhi = phimin;

        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_RightMouseButtonPressed)
    {
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        g_CameraTheta -= 0.01f*dx;
        g_CameraPhi   += 0.01f*dy;

        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_MiddleMouseButtonPressed)
    {
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        g_CameraTheta -= 0.01f*dx;
        g_CameraPhi   += 0.01f*dy;

        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }
}

void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    g_CameraDistance -= 0.1f*yoffset;

    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    float delta = 3.141592 / 16;

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Head rotation controls (H and J keys)
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        if (mod & GLFW_MOD_SHIFT)
            g_HeadAngleX -= delta;
        else
            g_HeadAngleX += delta;
    }

    if (key == GLFW_KEY_J && action == GLFW_PRESS)
    {
        if (mod & GLFW_MOD_SHIFT)
            g_HeadAngleY -= delta;
        else
            g_HeadAngleY += delta;
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }
}

void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;
    glm::vec4 p_clip = projection*p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", 10, 40, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, 10, 20, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", 10, 10, 1.0f);
    TextRendering_PrintString(window, "                                        v  ", 10, 0, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", 210, 40, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, 210, 20, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", 210, 10, 1.0f);
    TextRendering_PrintString(window, "                                        v  ", 210, 0, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", 410, 40, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, 410, 20, 1.0f);
}

void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, "Euler Angles:", 10, -lineheight, 1.0f);
}

void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, "Projection:", 10, -lineheight*2, 1.0f);
}

void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    static float old_seconds = (float)glfwGetTime();
    static int ellapsed_frames = 0;
    ellapsed_frames += 1;

    float seconds = (float)glfwGetTime();

    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        float fps = ellapsed_frames / ellapsed_seconds;
        old_seconds = seconds;
        ellapsed_frames = 0;

        char buffer[20];
        snprintf(buffer, 20, "%.2f fps", fps);

        float lineheight = TextRendering_LineHeight(window);
        float charwidth = TextRendering_CharWidth(window);
        TextRendering_PrintString(window, buffer, 1.0f-(strlen(buffer)+1)*charwidth, 1.0f-lineheight, 1.0f);
    }
}

