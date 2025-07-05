//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//                   LABORATÓRIO 5
//

// Arquivos "headers" padrões de C podem ser incluídos em um
// programa C++, sendo necessário somente adicionar o caractere
// "c" antes de seu nome, e remover o sufixo ".h". Exemplo:
//    #include <stdio.h> // Em C
//  vira
//    #include <cstdio> // Em C++
//
#include <cmath>
#include <cstdio>
#include <cstdlib>

// Headers abaixo são específicos de C++
#include <map>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>  // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h> // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "glm/ext/vector_float3.hpp"
#include "utils.h"
#include "matrices.h"

#include "camera.hpp"
#include "collisions.hpp"
#include "maze.hpp"

#define WIDTH 800
#define HEIGHT 800

struct FaceGroup {
  int                 material_id;
  std::vector<size_t> face_indices;
};

struct SceneObject {
  std::string            name;
  std::vector<FaceGroup> groups;

  GLenum rendering_mode;
  GLuint vertex_array_object_id;

  glm::vec3 bbox_min;
  glm::vec3 bbox_max;

  glm::mat4 transform;

  std::vector<tinyobj::material_t> materials;
  tinyobj::material_t              default_material;
};


// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;
std::map<std::string, SceneObject> g_MazeWall;

// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void   BuildTrianglesAndAddToVirtualScene(ObjModel*);                        // Constrói representação de um ObjModel como malha de triângulos para renderização
void   ComputeNormals(ObjModel* model);                                      // Computa normais de um ObjModel, caso não existam.
void   LoadShadersFromFiles();                                               // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void   LoadTextureImage(const char* filename);                               // Função que carrega imagens de textura
void   DrawVirtualObject(const char* object_name);                           // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);                              // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename);                            // Carrega um fragment shader
void   LoadShader(const char* filename, GLuint shader_id);                   // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void   PrintObjModelInfo(ObjModel*);                                         // Função para debugging

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void  TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void  TextRendering_PrintString(GLFWwindow* window, const std::string& str, float x, float y, float scale = 1.0f);
void  TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void  TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void  TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void  TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void  TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);


// Key Stuff definitions
void processKeys(double currentTime);

struct KeyState {
  bool   isPressed = false;
  double lastTime  = 0.0;
};

float repeatDelay = 0.1;

std::unordered_map<int, KeyState> keys;

// End key stuff definitions


// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;
double g_CursorDeltaX, g_CursorDeltaY;

void processCursor(double xpos, double ypos);

// Abaixo definimos variáveis globais utilizadas em várias funções do código.


// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4> g_MatrixStack;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed   = false;
bool g_RightMouseButtonPressed  = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta    = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi      = 0.0f; // Ângulo em relação ao eixo Y
float g_CameraDistance = 3.5f; // Distância da câmera para a origem

// Variáveis que controlam rotação do antebraço
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

// Variáveis que controlam translação do torso
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint  g_model_uniform;
GLint  g_view_uniform;
GLint  g_projection_uniform;
GLint  g_object_id_uniform;
GLint  g_bbox_min_uniform;
GLint  g_bbox_max_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

tinyobj::material_t g_DefaultMaterial = [] {
  tinyobj::material_t mat;
  mat.name = "DefaultMaterial";

  // Ambient (Ka)
  mat.ambient[0] = 0.0f;
  mat.ambient[1] = 0.0f;
  mat.ambient[2] = 0.0f;

  // Diffuse (Kd)
  mat.diffuse[0] = 0.0f;
  mat.diffuse[1] = 0.0f;
  mat.diffuse[2] = 0.0f;

  // Specular (Ks)
  mat.specular[0] = 0.0f;
  mat.specular[1] = 0.0f;
  mat.specular[2] = 0.0f;

  // Emission (Ke)
  mat.emission[0] = 0.0f;
  mat.emission[1] = 0.0f;
  mat.emission[2] = 0.0f;

  // Shininess (Ns)
  mat.shininess = 1.0f;

  // Opacity (d)
  mat.dissolve = 1.0f;

  // No textures
  mat.ambient_texname  = "";
  mat.diffuse_texname  = "";
  mat.specular_texname = "";
  mat.normal_texname   = "";

  return mat;
}();

GLint g_kd_uniform;
GLint g_ka_uniform;
GLint g_ks_uniform;
GLint g_q_uniform;
GLint g_displacement_uniform;

// Camera
SphericCamera sphericCamera(5.0f,
                            g_CameraTheta,
                            g_CameraPhi,
                            g_CameraDistance,
                            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                            glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
                            -0.01f,
                            -1000.0f,
                            3.141592 / 3.0f,
                            (float) WIDTH / HEIGHT,
                            true);

FreeCamera freeCamera(5.0f,
                      g_CameraTheta,
                      g_CameraPhi,
                      glm::vec4(-10.0f, 0.0f, 0.0f, 1.0f),
                      glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
                      -0.01f,
                      -1000.0f,
                      3.141592 / 3.0f,
                      (float) WIDTH / HEIGHT,
                      true);

Camera* camera = &freeCamera;

float deltaTime     = 0.0f;
float lastFrameTime = 0.0f;

int main(int argc, char* argv[]) {
  // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
  // sistema operacional, onde poderemos renderizar com OpenGL.
  int success = glfwInit();
  if (!success) {
    fprintf(stderr, "ERROR: glfwInit() failed.\n");
    std::exit(EXIT_FAILURE);
  }

  // Definimos o callback para impressão de erros da GLFW no terminal
  glfwSetErrorCallback(ErrorCallback);

  // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

#ifdef __APPLE__
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

  // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
  // funções modernas de OpenGL.
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
  // de pixels, e com título "INF01047 ...".
  GLFWwindow* window;
  window = glfwCreateWindow(WIDTH, HEIGHT, "INF01047 - Lucas Nogueira - 00315453", NULL, NULL);
  if (!window) {
    glfwTerminate();
    fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
    std::exit(EXIT_FAILURE);
  }

  // Definimos a função de callback que será chamada sempre que o usuário
  // pressionar alguma tecla do teclado ...
  glfwSetKeyCallback(window, KeyCallback);
  // ... ou clicar os botões do mouse ...
  glfwSetMouseButtonCallback(window, MouseButtonCallback);
  // ... ou movimentar o cursor do mouse em cima da janela ...
  glfwSetCursorPosCallback(window, CursorPosCallback);
  // ... ou rolar a "rodinha" do mouse.
  glfwSetScrollCallback(window, ScrollCallback);

  // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
  glfwMakeContextCurrent(window);

  // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
  // biblioteca GLAD.
  gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

  // Definimos a função de callback que será chamada sempre que a janela for
  // redimensionada, por consequência alterando o tamanho do "framebuffer"
  // (região de memória onde são armazenados os pixels da imagem).
  glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
  FramebufferSizeCallback(window, 800, 600); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

  // Imprimimos no terminal informações sobre a GPU do sistema
  const GLubyte* vendor      = glGetString(GL_VENDOR);
  const GLubyte* renderer    = glGetString(GL_RENDERER);
  const GLubyte* glversion   = glGetString(GL_VERSION);
  const GLubyte* glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

  printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

  // Carregamos os shaders de vértices e de fragmentos que serão utilizados
  // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
  //
  LoadShadersFromFiles();

  // Carregamos duas imagens para serem utilizadas como textura
  LoadTextureImage("../../data/plane.png");             // TextureImage0
  LoadTextureImage("../../data/floor_normals.png");     // TextureImage1
  LoadTextureImage("../../data/maze.jpg");              // TextureImage2
  LoadTextureImage("../../data/pacman_ghost_blue.png"); // TextureImage3

  // Construímos a representação de objetos geométricos através de malhas de triângulos
  // ObjModel spheremodel("../../data/sphere.obj");
  // ComputeNormals(&spheremodel);
  // BuildTrianglesAndAddToVirtualScene(&spheremodel);
  //
  // ObjModel bunnymodel("../../data/bunny.obj");
  // ComputeNormals(&bunnymodel);
  // BuildTrianglesAndAddToVirtualScene(&bunnymodel);

  ObjModel planemodel("../../data/plane.obj");
  ComputeNormals(&planemodel);
  BuildTrianglesAndAddToVirtualScene(&planemodel);
  SceneObject* planeobj = &g_VirtualScene["the_plane"];
  planeobj->transform   = Matrix_Identity() * Matrix_Translate(0.0f, -1.1f, 0.0f) * Matrix_Scale(50, 1, 50);
  planeobj->bbox_min    = glm::vec3(planeobj->transform * glm::vec4(planeobj->bbox_min, 1.0f));
  planeobj->bbox_max    = glm::vec3(planeobj->transform * glm::vec4(planeobj->bbox_max, 1.0f));

  // ObjModel maze("../../data/maze.obj");
  // ComputeNormals(&maze);
  // BuildTrianglesAndAddToVirtualScene(&maze);

  // ObjModel wall("../../data/cube.obj");
  // ComputeNormals(&wall);
  // BuildTrianglesAndAddToVirtualScene(&wall);
  // SceneObject* wallobj = &g_VirtualScene["cube"];
  // wallobj->transform   = Matrix_Identity() * Matrix_Translate(10.0f, 0.0f, 0.0f) * Matrix_Scale(1.0f, 1.0f, 1.0f);
  // wallobj->bbox_min    = glm::vec3(wallobj->transform * glm::vec4(wallobj->bbox_min, 1.0f));
  // wallobj->bbox_max    = glm::vec3(wallobj->transform * glm::vec4(wallobj->bbox_max, 1.0f));

  ObjModel ghostmodel("../../data/pacman_ghost.obj");
  ComputeNormals(&ghostmodel);
  BuildTrianglesAndAddToVirtualScene(&ghostmodel);
  SceneObject* ghost = &g_VirtualScene["ghost"];
  ghost->transform   = Matrix_Identity() * Matrix_Scale(0.01, 0.01, 0.01);

  // Generate the maze
  // MazeGenerator maze(20, 20);
  // maze.generateMaze();
  //
  // // Export each wall into a separate ObjModel
  // auto mazeWalls = maze.exportToObjModels();
  //
  // for (auto& pair : mazeWalls) {
  //   // const std::string&         wallName  = pair.first;
  //   std::unique_ptr<ObjModel>& wallModel = pair.second;
  //
  //   // Compute normals (you can skip if you add normals during export)
  //   ComputeNormals(wallModel.get());
  //
  //   // Add to the scene
  //   BuildTrianglesAndAddToVirtualScene(wallModel.get());
  // }

  // Guardar nomes das paredes para desenhar depois
  // std::list<std::string> wallNames = maze.getWallNames();

  if (argc > 1) {
    ObjModel model(argv[1]);
    BuildTrianglesAndAddToVirtualScene(&model);
  }

  // Inicializamos o código para renderização de texto.
  TextRendering_Init();

  // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
  glEnable(GL_DEPTH_TEST);

  // Habilitamos o Backface Culling. Veja slides 8-13 do documento Aula_02_Fundamentos_Matematicos.pdf, slides 23-34 do documento
  // Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);


  // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela
  while (!glfwWindowShouldClose(window)) {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(g_GpuProgramID);

    glm::mat4 view       = camera->getMatrixView();
    glm::mat4 projection = camera->getMatrixProjection();
    glUniformMatrix4fv(g_view_uniform, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(g_projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

#define SPHERE 0
#define BUNNY 1
#define PLANE 2
#define GHOST 3
#define MAZE 4

    float currentFrameTime = glfwGetTime(); // Time in seconds
    deltaTime              = currentFrameTime - lastFrameTime;
    lastFrameTime          = currentFrameTime;

    glm::mat4 model = Matrix_Identity();

    // Desenhamos o modelo do coelho
    // model = Matrix_Translate(1.1f, 0.0f, 0.0f) * Matrix_Rotate_X(g_AngleX + currentFrameTime * 0.1f);
    // glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    // glUniform1i(g_object_id_uniform, BUNNY);
    // DrawVirtualObject("the_bunny");


    // Desenhamos o plano do chão
    model = g_VirtualScene["the_plane"].transform;
    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, PLANE);
    DrawVirtualObject("the_plane");

    model = g_VirtualScene["ghost"].transform;
    glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(g_object_id_uniform, GHOST);
    DrawVirtualObject("ghost");

    // model = Matrix_Translate(0.0f, -1.1f, 0.0f);
    // glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    // glUniform1i(g_object_id_uniform, MAZE);
    // glUniform1i(g_displacement_uniform, 20.0f);
    // DrawVirtualObject("maze");

    // model = g_VirtualScene["cube"].transform;
    // glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    // glUniform1i(g_object_id_uniform, PLANE);
    // DrawVirtualObject("cube");

    // Desenhar todas as paredes do labirinto
    // for (const std::string& wallName : wallNames) {
    //   model = Matrix_Identity() * Matrix_Translate(0.0f, -1.1f, 0.0f);
    //   glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    //   glUniform1i(g_object_id_uniform, MAZE);
    //   DrawVirtualObject(wallName.c_str());
    // }

    glfwSwapBuffers(window);

    processCursor(g_LastCursorPosX, g_LastCursorPosY);
    processKeys(currentFrameTime);

    glfwPollEvents();
  }

  // Finalizamos o uso dos recursos do sistema operacional
  glfwTerminate();

  // Fim do programa
  return 0;
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename) {
  printf("Carregando imagem \"%s\"... ", filename);

  // Primeiro fazemos a leitura da imagem do disco
  stbi_set_flip_vertically_on_load(true);
  int            width;
  int            height;
  int            channels;
  unsigned char* data = stbi_load(filename, &width, &height, &channels, 3);

  if (data == NULL) {
    fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
    std::exit(EXIT_FAILURE);
  }

  printf("OK (%dx%d).\n", width, height);

  // Agora criamos objetos na GPU com OpenGL para armazenar a textura
  GLuint texture_id;
  GLuint sampler_id;
  glGenTextures(1, &texture_id);
  glGenSamplers(1, &sampler_id);

  // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
  glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_REPEAT);

  // Parâmetros de amostragem da textura.
  glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Agora enviamos a imagem lida do disco para a GPU
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

  GLuint textureunit = g_NumLoadedTextures;
  glActiveTexture(GL_TEXTURE0 + textureunit);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);
  glBindSampler(textureunit, sampler_id);

  stbi_image_free(data);

  g_NumLoadedTextures += 1;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name) {
  const SceneObject& obj = g_VirtualScene[object_name];

  glBindVertexArray(obj.vertex_array_object_id);

  // Pass bounding box uniforms
  glm::vec3 bbox_min = obj.bbox_min;
  glm::vec3 bbox_max = obj.bbox_max;
  glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
  glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

  // Draw each material group
  for (const auto& group : obj.groups) {
    const tinyobj::material_t& material =
        (group.material_id >= 0 && group.material_id < obj.materials.size())
            ? obj.materials[group.material_id]
            : obj.default_material;

    // Set material diffuse color (you can expand this to textures later)
    glUniform3fv(g_kd_uniform, 1, material.diffuse);
    glUniform3fv(g_ka_uniform, 1, material.ambient);
    glUniform3fv(g_ks_uniform, 1, material.specular);
    glUniform1f(g_q_uniform, material.shininess);

    // Draw all faces with this material
    for (size_t face : group.face_indices) {
      size_t offset = face * 3 * sizeof(GLuint);
      glDrawElements(obj.rendering_mode, 3, GL_UNSIGNED_INT, (void*) (offset));
    }
  }

  glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles() {
  // Note que o caminho para os arquivos "shader_vertex.glsl" e
  // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
  // da seguinte estrutura no sistema de arquivos:
  //
  //    + FCG_Lab_01/
  //    |
  //    +--+ bin/
  //    |  |
  //    |  +--+ Release/  (ou Debug/ ou Linux/)
  //    |     |
  //    |     o-- main.exe
  //    |
  //    +--+ src/
  //       |
  //       o-- shader_vertex.glsl
  //       |
  //       o-- shader_fragment.glsl
  //
  GLuint vertex_shader_id   = LoadShader_Vertex("../../src/shader_vertex.glsl");
  GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

  // Deletamos o programa de GPU anterior, caso ele exista.
  if (g_GpuProgramID != 0)
    glDeleteProgram(g_GpuProgramID);

  // Criamos um programa de GPU utilizando os shaders carregados acima.
  g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

  // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
  // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
  // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
  g_model_uniform        = glGetUniformLocation(g_GpuProgramID, "model");      // Variável da matriz "model"
  g_view_uniform         = glGetUniformLocation(g_GpuProgramID, "view");       // Variável da matriz "view" em shader_vertex.glsl
  g_projection_uniform   = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
  g_object_id_uniform    = glGetUniformLocation(g_GpuProgramID, "object_id");  // Variável "object_id" em shader_fragment.glsl
  g_bbox_min_uniform     = glGetUniformLocation(g_GpuProgramID, "bbox_min");
  g_bbox_max_uniform     = glGetUniformLocation(g_GpuProgramID, "bbox_max");
  g_kd_uniform           = glGetUniformLocation(g_GpuProgramID, "kd");
  g_ka_uniform           = glGetUniformLocation(g_GpuProgramID, "ka");
  g_ks_uniform           = glGetUniformLocation(g_GpuProgramID, "ks");
  g_q_uniform            = glGetUniformLocation(g_GpuProgramID, "q");
  g_displacement_uniform = glGetUniformLocation(g_GpuProgramID, "displacementScale");


  // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
  glUseProgram(g_GpuProgramID);
  glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage0"), 0);
  glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage1"), 1);
  glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage2"), 2);
  glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureImage3"), 3);
  glUseProgram(0);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M) {
  g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M) {
  if (g_MatrixStack.empty()) {
    M = Matrix_Identity();
  } else {
    M = g_MatrixStack.top();
    g_MatrixStack.pop();
  }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model) {
  if (!model->attrib.normals.empty())
    return;

  // Primeiro computamos as normais para todos os TRIÂNGULOS.
  // Segundo, computamos as normais dos VÉRTICES através do método proposto
  // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
  // todas as faces que compartilham este vértice.

  size_t num_vertices = model->attrib.vertices.size() / 3;

  std::vector<int>       num_triangles_per_vertex(num_vertices, 0);
  std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));

  for (size_t shape = 0; shape < model->shapes.size(); ++shape) {
    size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

    for (size_t triangle = 0; triangle < num_triangles; ++triangle) {
      assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

      glm::vec4 vertices[3];
      for (size_t vertex = 0; vertex < 3; ++vertex) {
        tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];
        const float      vx  = model->attrib.vertices[3 * idx.vertex_index + 0];
        const float      vy  = model->attrib.vertices[3 * idx.vertex_index + 1];
        const float      vz  = model->attrib.vertices[3 * idx.vertex_index + 2];
        vertices[vertex]     = glm::vec4(vx, vy, vz, 1.0);
      }

      const glm::vec4 a = vertices[0];
      const glm::vec4 b = vertices[1];
      const glm::vec4 c = vertices[2];

      const glm::vec4 n = crossproduct(b - a, c - a);

      for (size_t vertex = 0; vertex < 3; ++vertex) {
        tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];
        num_triangles_per_vertex[idx.vertex_index] += 1;
        vertex_normals[idx.vertex_index] += n;
        model->shapes[shape].mesh.indices[3 * triangle + vertex].normal_index = idx.vertex_index;
      }
    }
  }

  model->attrib.normals.resize(3 * num_vertices);

  for (size_t i = 0; i < vertex_normals.size(); ++i) {
    glm::vec4 n = vertex_normals[i] / (float) num_triangles_per_vertex[i];
    n /= norm(n);
    model->attrib.normals[3 * i + 0] = n.x;
    model->attrib.normals[3 * i + 1] = n.y;
    model->attrib.normals[3 * i + 2] = n.z;
  }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model) {
  GLuint vertex_array_object_id;
  glGenVertexArrays(1, &vertex_array_object_id);
  glBindVertexArray(vertex_array_object_id);

  std::vector<GLuint> indices;
  std::vector<float>  model_coefficients;
  std::vector<float>  normal_coefficients;
  std::vector<float>  texture_coefficients;

  for (size_t shape = 0; shape < model->shapes.size(); ++shape) {
    auto&  mesh      = model->shapes[shape].mesh;
    size_t num_faces = mesh.num_face_vertices.size();

    glm::vec3 bbox_min(std::numeric_limits<float>::max());
    glm::vec3 bbox_max(std::numeric_limits<float>::lowest());

    SceneObject theobject;
    theobject.name                   = model->shapes[shape].name;
    theobject.rendering_mode         = GL_TRIANGLES;
    theobject.vertex_array_object_id = vertex_array_object_id;
    theobject.transform              = Matrix_Identity();

    if (model->materials.empty()) {
      // OBJ has no .mtl — just empty
      theobject.default_material = g_DefaultMaterial;
    } else {
      theobject.materials        = model->materials;
      theobject.default_material = g_DefaultMaterial; // Always safe fallback
    }

    // Grouping faces by material
    std::map<int, FaceGroup> group_map;

    for (size_t face = 0; face < num_faces; ++face) {
      assert(mesh.num_face_vertices[face] == 3);

      int material_id                    = mesh.material_ids[face];
      group_map[material_id].material_id = material_id;
      group_map[material_id].face_indices.push_back(face);

      for (size_t vertex = 0; vertex < 3; ++vertex) {
        tinyobj::index_t idx = mesh.indices[3 * face + vertex];

        indices.push_back(indices.size());

        const float vx = model->attrib.vertices[3 * idx.vertex_index + 0];
        const float vy = model->attrib.vertices[3 * idx.vertex_index + 1];
        const float vz = model->attrib.vertices[3 * idx.vertex_index + 2];

        model_coefficients.push_back(vx);
        model_coefficients.push_back(vy);
        model_coefficients.push_back(vz);
        model_coefficients.push_back(1.0f);

        bbox_min.x = std::min(bbox_min.x, vx);
        bbox_min.y = std::min(bbox_min.y, vy);
        bbox_min.z = std::min(bbox_min.z, vz);
        bbox_max.x = std::max(bbox_max.x, vx);
        bbox_max.y = std::max(bbox_max.y, vy);
        bbox_max.z = std::max(bbox_max.z, vz);

        if (idx.normal_index != -1) {
          const float nx = model->attrib.normals[3 * idx.normal_index + 0];
          const float ny = model->attrib.normals[3 * idx.normal_index + 1];
          const float nz = model->attrib.normals[3 * idx.normal_index + 2];
          normal_coefficients.push_back(nx);
          normal_coefficients.push_back(ny);
          normal_coefficients.push_back(nz);
          normal_coefficients.push_back(0.0f);
        }

        if (idx.texcoord_index != -1) {
          const float u = model->attrib.texcoords[2 * idx.texcoord_index + 0];
          const float v = model->attrib.texcoords[2 * idx.texcoord_index + 1];
          texture_coefficients.push_back(u);
          texture_coefficients.push_back(v);
        }
      }
    }

    theobject.bbox_min = bbox_min;
    theobject.bbox_max = bbox_max;

    for (auto& pair : group_map) {
      theobject.groups.push_back(pair.second);
    }


    g_VirtualScene[theobject.name] = theobject;
  }

  // Upload vertex data

  GLuint VBO_model_coefficients_id;
  glGenBuffers(1, &VBO_model_coefficients_id);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
  glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  if (!normal_coefficients.empty()) {
    GLuint VBO_normal_coefficients_id;
    glGenBuffers(1, &VBO_normal_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  if (!texture_coefficients.empty()) {
    GLuint VBO_texture_coefficients_id;
    glGenBuffers(1, &VBO_texture_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
  }

  GLuint indices_id;
  glGenBuffers(1, &indices_id);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());

  glBindVertexArray(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename) {
  // Criamos um identificador (ID) para este shader, informando que o mesmo
  // será aplicado nos vértices.
  GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

  // Carregamos e compilamos o shader
  LoadShader(filename, vertex_shader_id);

  // Retorna o ID gerado acima
  return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename) {
  // Criamos um identificador (ID) para este shader, informando que o mesmo
  // será aplicado nos fragmentos.
  GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

  // Carregamos e compilamos o shader
  LoadShader(filename, fragment_shader_id);

  // Retorna o ID gerado acima
  return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id) {
  // Lemos o arquivo de texto indicado pela variável "filename"
  // e colocamos seu conteúdo em memória, apontado pela variável
  // "shader_string".
  std::ifstream file;
  try {
    file.exceptions(std::ifstream::failbit);
    file.open(filename);
  } catch (std::exception& e) {
    fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
    std::exit(EXIT_FAILURE);
  }
  std::stringstream shader;
  shader << file.rdbuf();
  std::string   str                  = shader.str();
  const GLchar* shader_string        = str.c_str();
  const GLint   shader_string_length = static_cast<GLint>(str.length());

  // Define o código do shader GLSL, contido na string "shader_string"
  glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

  // Compila o código do shader GLSL (em tempo de execução)
  glCompileShader(shader_id);

  // Verificamos se ocorreu algum erro ou "warning" durante a compilação
  GLint compiled_ok;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

  GLint log_length = 0;
  glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

  // Alocamos memória para guardar o log de compilação.
  // A chamada "new" em C++ é equivalente ao "malloc()" do C.
  GLchar* log = new GLchar[log_length];
  glGetShaderInfoLog(shader_id, log_length, &log_length, log);

  // Imprime no terminal qualquer erro ou "warning" de compilação
  if (log_length != 0) {
    std::string output;

    if (!compiled_ok) {
      output += "ERROR: OpenGL compilation of \"";
      output += filename;
      output += "\" failed.\n";
      output += "== Start of compilation log\n";
      output += log;
      output += "== End of compilation log\n";
    } else {
      output += "WARNING: OpenGL compilation of \"";
      output += filename;
      output += "\".\n";
      output += "== Start of compilation log\n";
      output += log;
      output += "== End of compilation log\n";
    }

    fprintf(stderr, "%s", output.c_str());
  }

  // A chamada "delete" em C++ é equivalente ao "free()" do C
  delete[] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id) {
  // Criamos um identificador (ID) para este programa de GPU
  GLuint program_id = glCreateProgram();

  // Definição dos dois shaders GLSL que devem ser executados pelo programa
  glAttachShader(program_id, vertex_shader_id);
  glAttachShader(program_id, fragment_shader_id);

  // Linkagem dos shaders acima ao programa
  glLinkProgram(program_id);

  // Verificamos se ocorreu algum erro durante a linkagem
  GLint linked_ok = GL_FALSE;
  glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

  // Imprime no terminal qualquer erro de linkagem
  if (linked_ok == GL_FALSE) {
    GLint log_length = 0;
    glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];

    glGetProgramInfoLog(program_id, log_length, &log_length, log);

    std::string output;

    output += "ERROR: OpenGL linking of program failed.\n";
    output += "== Start of link log\n";
    output += log;
    output += "\n== End of link log\n";

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete[] log;

    fprintf(stderr, "%s", output.c_str());
  }

  // Os "Shader Objects" podem ser marcados para deleção após serem linkados
  glDeleteShader(vertex_shader_id);
  glDeleteShader(fragment_shader_id);

  // Retornamos o ID gerado acima
  return program_id;
}

void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
  glViewport(0, 0, width, height);
  camera->setScreenRatio((float) width / height);
}

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
    // posição atual do cursor nas variáveis g_LastCursorPosX e
    // g_LastCursorPosY.  Também, setamos a variável
    // g_LeftMouseButtonPressed como true, para saber que o usuário está
    // com o botão esquerdo pressionado.
    glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
    g_LeftMouseButtonPressed = true;
  }
  if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
    // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
    // variável abaixo para false.
    g_LeftMouseButtonPressed = false;
  }
  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
    // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
    // posição atual do cursor nas variáveis g_LastCursorPosX e
    // g_LastCursorPosY.  Também, setamos a variável
    // g_RightMouseButtonPressed como true, para saber que o usuário está
    // com o botão esquerdo pressionado.
    glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
    g_RightMouseButtonPressed = true;
  }
  if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
    // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
    // variável abaixo para false.
    g_RightMouseButtonPressed = false;
  }
  if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
    // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
    // posição atual do cursor nas variáveis g_LastCursorPosX e
    // g_LastCursorPosY.  Também, setamos a variável
    // g_MiddleMouseButtonPressed como true, para saber que o usuário está
    // com o botão esquerdo pressionado.
    glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
    g_MiddleMouseButtonPressed = true;
  }
  if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE) {
    // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
    // variável abaixo para false.
    g_MiddleMouseButtonPressed = false;
  }
}

void processCursor(double xpos, double ypos) {
  if (g_LeftMouseButtonPressed) {
    float newTheta = camera->getTheta();
    newTheta += 0.01f * g_CursorDeltaX;
    camera->setTheta(newTheta);

    float newPhi = camera->getPhi();
    newPhi -= 0.01f * g_CursorDeltaY;
    camera->setPhi(newPhi);
  }

  if (g_RightMouseButtonPressed) {
    g_ForearmAngleZ -= 0.01f * g_CursorDeltaX;
    g_ForearmAngleX += 0.01f * g_CursorDeltaY;
  }

  if (g_MiddleMouseButtonPressed) {
    g_TorsoPositionX += 0.01f * g_CursorDeltaX;
    g_TorsoPositionY -= 0.01f * g_CursorDeltaY;
  }

  g_CursorDeltaX = 0.0f;
  g_CursorDeltaY = 0.0f;
}

void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
  g_CursorDeltaX = xpos - g_LastCursorPosX;
  g_CursorDeltaY = g_LastCursorPosY - ypos;

  g_LastCursorPosX = xpos;
  g_LastCursorPosY = ypos;
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
  float newDistance = camera->getDistance();
  newDistance -= 0.1f * yoffset;
  camera->setDistance(newDistance);
}

void updateKeyState(KeyState& key_state, bool isPressed, double time) {
  key_state.lastTime  = time;
  key_state.isPressed = isPressed;
}

void tryMove(void (*callback)(float deltaTime)) {
  // Salvar posição atual da câmera antes de mover
  glm::vec4 oldPosition = camera->getPosition();

  // Tentar mover a câmera
  callback(deltaTime);

  // Criar uma esfera representando a câmera
  collision::Sphere cameraSphere;
  cameraSphere.center =
      glm::vec3(camera->getPosition().x,
                camera->getPosition().y,
                camera->getPosition().z);
  cameraSphere.radius = 0.1f; // Raio da câmera

  // Verificar colisão com todos os objetos da cena

  bool collision = false;
  for (const auto& pair : g_VirtualScene) {
    const SceneObject& obj = pair.second;

    // Criar AABB do objeto
    collision::AABB objAABB;
    objAABB.min = glm::vec3(obj.transform * glm::vec4(obj.bbox_min, 1.0f));
    objAABB.max = glm::vec3(obj.transform * glm::vec4(obj.bbox_max, 1.0f));

    // Testar colisão entre a câmera (esfera) e o objeto(AABB)
    if (collision::testAABBSphere(objAABB, cameraSphere)) {
      collision = true;
      break;
    }
  }

  // Se houve colisão, restaurar posição anterior
  if (collision) {
    camera->setPosition(oldPosition);
  }
}

void processKeys(double currentTime) {
  for (std::unordered_map<int, KeyState>::iterator it = keys.begin(); it != keys.end(); ++it) {
    int      key       = it->first;
    KeyState key_state = it->second;

    if (key_state.isPressed) {
      double& lastTime = key_state.lastTime;
      if (currentTime - lastTime >= repeatDelay) {
        if (key == GLFW_KEY_W) {
          tryMove([](float dt) { camera->MoveForward(dt); });
        } else if (key == GLFW_KEY_A) {
          tryMove([](float dt) { camera->MoveLeft(dt); });
        } else if (key == GLFW_KEY_S) {
          tryMove([](float dt) { camera->MoveBackward(dt); });
        } else if (key == GLFW_KEY_D) {
          tryMove([](float dt) { camera->MoveRight(dt); });
        }

        lastTime = currentTime;
      }
    }
  }
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (action == GLFW_PRESS) {
    keys[key].isPressed = true;

    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
      float screenRatio = camera->getScreenRatio();
      camera            = (camera == &freeCamera) ? (Camera*) &sphericCamera : (Camera*) &freeCamera;
      camera->setScreenRatio(screenRatio);
    }

    if (key == GLFW_KEY_Q && action == GLFW_PRESS)
      glfwSetWindowShouldClose(window, GL_TRUE);

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS) {
      camera->UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS) {
      camera->UsePerspectiveProjection = false;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS) {
      g_ShowInfoText = !g_ShowInfoText;
    }

  } else if (action == GLFW_RELEASE) {
    keys[key].isPressed = false;
  }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description) {
  fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model) {
  if (!g_ShowInfoText)
    return;

  glm::vec4 p_world  = model * p_model;
  glm::vec4 p_camera = view * p_world;
  glm::vec4 p_clip   = projection * p_camera;
  glm::vec4 p_ndc    = p_clip / p_clip.w;

  float pad = TextRendering_LineHeight(window);

  TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f - pad, 1.0f);
  TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f - 2 * pad, 1.0f);

  TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f - 6 * pad, 1.0f);
  TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f - 7 * pad, 1.0f);
  TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f - 8 * pad, 1.0f);

  TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f - 9 * pad, 1.0f);
  TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f - 10 * pad, 1.0f);

  TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f - 14 * pad, 1.0f);
  TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f - 15 * pad, 1.0f);
  TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f - 16 * pad, 1.0f);

  TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f - 17 * pad, 1.0f);
  TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f - 18 * pad, 1.0f);

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  glm::vec2 a = glm::vec2(-1, -1);
  glm::vec2 b = glm::vec2(+1, +1);
  glm::vec2 p = glm::vec2(0, 0);
  glm::vec2 q = glm::vec2(width, height);

  glm::mat4 viewport_mapping = Matrix((q.x - p.x) / (b.x - a.x), 0.0f, 0.0f, (b.x * p.x - a.x * q.x) / (b.x - a.x), 0.0f, (q.y - p.y) / (b.y - a.y),
                                      0.0f, (b.y * p.y - a.y * q.y) / (b.y - a.y), 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);

  TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f - 22 * pad, 1.0f);
  TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f - 23 * pad, 1.0f);
  TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f - 24 * pad, 1.0f);

  TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f - 25 * pad, 1.0f);
  TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f - 26 * pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow* window) {
  if (!g_ShowInfoText)
    return;

  float pad = TextRendering_LineHeight(window);

  char buffer[80];
  snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

  TextRendering_PrintString(window, buffer, -1.0f + pad / 10, -1.0f + 2 * pad / 10, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window) {
  if (!g_ShowInfoText)
    return;

  float lineheight = TextRendering_LineHeight(window);
  float charwidth  = TextRendering_CharWidth(window);

  if (g_UsePerspectiveProjection)
    TextRendering_PrintString(window, "Perspective", 1.0f - 13 * charwidth, -1.0f + 2 * lineheight / 10, 1.0f);
  else
    TextRendering_PrintString(window, "Orthographic", 1.0f - 13 * charwidth, -1.0f + 2 * lineheight / 10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window) {
  if (!g_ShowInfoText)
    return;

  // Variáveis estáticas (static) mantém seus valores entre chamadas
  // subsequentes da função!
  static float old_seconds     = (float) glfwGetTime();
  static int   ellapsed_frames = 0;
  static char  buffer[20]      = "?? fps";
  static int   numchars        = 7;

  ellapsed_frames += 1;

  // Recuperamos o número de segundos que passou desde a execução do programa
  float seconds = (float) glfwGetTime();

  // Número de segundos desde o último cálculo do fps
  float ellapsed_seconds = seconds - old_seconds;

  if (ellapsed_seconds > 1.0f) {
    numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

    old_seconds     = seconds;
    ellapsed_frames = 0;
  }

  float lineheight = TextRendering_LineHeight(window);
  float charwidth  = TextRendering_CharWidth(window);

  TextRendering_PrintString(window, buffer, 1.0f - (numchars + 1) * charwidth, 1.0f - lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model) {
  const tinyobj::attrib_t&                attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>&    shapes    = model->shapes;
  const std::vector<tinyobj::material_t>& materials = model->materials;

  printf("# of vertices  : %d\n", (int) (attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int) (attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int) (attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int) shapes.size());
  printf("# of materials : %d\n", (int) materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v), static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]), static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v), static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]), static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v), static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i), shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i), static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() == shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i), static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f), static_cast<unsigned long>(fnum));
      getchar();

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f), static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f), shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i), static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t), shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i), materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n", static_cast<const double>(materials[i].ambient[0]), static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n", static_cast<const double>(materials[i].diffuse[0]), static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n", static_cast<const double>(materials[i].specular[0]), static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n", static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]), static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n", static_cast<const double>(materials[i].emission[0]), static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n", static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n", static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n", materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}
