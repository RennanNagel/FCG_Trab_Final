//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Fundamentos de Computação Gráfica
//               Prof. Eduardo Gastal
//
//                   LABORATÓRIO 3
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
#include <limits>
#include <fstream>
#include <sstream>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>  // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h> // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers locais, definidos na pasta "include/"
#include "glm/detail/qualifier.hpp"
#include "utils.h"
#include "matrices.h"

#include "camera.hpp"

#define WIDTH 800
#define HEIGHT 800

// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void   DrawCube(GLint render_as_black_uniform);                              // Desenha um cubo
GLuint BuildTriangles();                                                     // Constrói triângulos para renderização
void   LoadShadersFromFiles();                                               // Carrega os shaders de vértice e fragmento, criando um programa de GPU
GLuint LoadShader_Vertex(const char* filename);                              // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename);                            // Carrega um fragment shader
void   LoadShader(const char* filename, GLuint shader_id);                   // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU

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
void ScrollCallback(GLFWwindow* window, SphericCamera camera, double xoffset, double yoffset);

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject {
  const char* name;           // Nome do objeto
  void*       first_index;    // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTriangles()
  int         num_indices;    // Número de índices do objeto dentro do vetor indices[] definido em BuildTriangles()
  GLenum      rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTriangles() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<const char*, SceneObject> g_VirtualScene;

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

// Camera
SphericCamera sphericCamera(0.5f,
                            g_CameraTheta,
                            g_CameraPhi,
                            g_CameraDistance,
                            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                            glm::vec4(0.0f, 1.0f, 0.0f, 0.0f),
                            -0.01f,
                            -10.0f,
                            3.141592 / 3.0f,
                            (float) WIDTH / HEIGHT,
                            true);

FreeCamera freeCamera(0.5f,
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

// Variáveis que controlam rotação do antebraço
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

// Variáveis que controlam translação do torso
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

// Variáveis para controle das partes do corpo do boneco
float g_HeadAngleX = 0.0f; // Rotação da cabeça em X
float g_HeadAngleY = 0.0f; // Rotação da cabeça em Y

// Braços
float g_LeftUpperArmAngleX  = 0.0f; // Rotação do braço superior esquerdo em X
float g_RightUpperArmAngleX = 0.0f; // Rotação do braço superior direito em X

float g_LeftForearmAngleX  = 0.0f; // Rotação do antebraço esquerdo em X
float g_RightForearmAngleX = 0.0f; // Rotação do antebraço direito em X

float g_LeftHandAngleX  = 0.0f; // Rotação da mão esquerda em X
float g_RightHandAngleX = 0.0f; // Rotação da mão direita em X

// Pernas
float g_LeftUpperLegAngleX  = 0.0f; // Rotação da coxa esquerda em X
float g_RightUpperLegAngleX = 0.0f; // Rotação da coxa direita em X

float g_LeftLowerLegAngleX  = 0.0f; // Rotação da canela esquerda em X
float g_RightLowerLegAngleX = 0.0f; // Rotação da canela direita em X

float g_LeftFootAngleX  = 0.0f; // Rotação do pé esquerdo em X
float g_RightFootAngleX = 0.0f; // Rotação do pé direito em X

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;

int main() {
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

  // Criamos uma janela do sistema operacional, com 800 colunas e 800 linhas
  // de pixels, e com título "INF01047 ...".
  GLFWwindow* window;
  window = glfwCreateWindow(WIDTH, HEIGHT, "INF01047 - 00315453 - Lucas Caique dos Santos Nogueira", NULL, NULL);
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
  // glfwSetScrollCallback(window, ScrollCallback);

  // Definimos a função de callback que será chamada sempre que a janela for
  // redimensionada, por consequência alterando o tamanho do "framebuffer"
  // (região de memória onde são armazenados os pixels da imagem).
  glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
  glfwSetWindowSize(window, WIDTH, HEIGHT); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

  // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
  glfwMakeContextCurrent(window);

  // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
  // biblioteca GLAD.
  gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

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

  // Construímos a representação de um triângulo
  GLuint vertex_array_object_id = BuildTriangles();

  // Inicializamos o código para renderização de texto.
  TextRendering_Init();

  // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
  // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
  // (GPU)! Veja arquivo "shader_vertex.glsl".
  GLint model_uniform           = glGetUniformLocation(g_GpuProgramID, "model");           // Variável da matriz "model"
  GLint view_uniform            = glGetUniformLocation(g_GpuProgramID, "view");            // Variável da matriz "view" em shader_vertex.glsl
  GLint projection_uniform      = glGetUniformLocation(g_GpuProgramID, "projection");      // Variável da matriz "projection" em shader_vertex.glsl
  GLint render_as_black_uniform = glGetUniformLocation(g_GpuProgramID, "render_as_black"); // Variável booleana em shader_vertex.glsl

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
    glBindVertexArray(vertex_array_object_id);

    glm::mat4 view       = camera->getMatrixView();
    glm::mat4 projection = camera->getMatrixProjection();

    glUniformMatrix4fv(view_uniform, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

    glm::mat4 model = Matrix_Identity(); // Transformação inicial do modelo

    // Translação global do modelo
    model = model * Matrix_Translate(0, 0.5, 0);

    // Matriz de modelo do torso
    glm::mat4 torso_model = model * Matrix_Scale(0.8f, 1.0f, 0.4f);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(torso_model));
    glUniform1i(render_as_black_uniform, false);
    DrawCube(render_as_black_uniform);

    // Cabeça
    PushMatrix(model);
    model                = model * Matrix_Translate(0.0f, 0.45f, 0.0f);
    model                = model * Matrix_Rotate_X(g_HeadAngleX);
    model                = model * Matrix_Rotate_Y(g_HeadAngleY);
    glm::mat4 head_model = model * Matrix_Scale(0.4f, 0.4f, 0.4f);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(head_model));
    DrawCube(render_as_black_uniform);
    PopMatrix(model);

    // Braço superior esquerdo
    PushMatrix(model);
    model                          = model * Matrix_Translate(-0.5f, 0.0f, 0.0f);
    model                          = model * Matrix_Rotate_X(g_LeftUpperArmAngleX);
    glm::mat4 left_upper_arm_model = model * Matrix_Scale(0.2f, 0.4f, 0.2f);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(left_upper_arm_model));
    DrawCube(render_as_black_uniform);

    // Antebraço esquerdo
    model                        = model * Matrix_Translate(0.0f, -0.4f, 0.0f);
    model                        = model * Matrix_Rotate_X(g_LeftForearmAngleX);
    glm::mat4 left_forearm_model = model * Matrix_Scale(0.2f, 0.4f, 0.2f);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(left_forearm_model));
    DrawCube(render_as_black_uniform);

    // Mão esquerda
    model                     = model * Matrix_Translate(0.0f, -0.4f, 0.0f);
    model                     = model * Matrix_Rotate_X(g_LeftHandAngleX);
    glm::mat4 left_hand_model = model * Matrix_Scale(0.15f, 0.2f, 0.15f);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(left_hand_model));
    DrawCube(render_as_black_uniform);
    PopMatrix(model);

    // Braço superior direito
    PushMatrix(model);
    model                           = model * Matrix_Translate(0.5f, 0.0f, 0.0f);
    model                           = model * Matrix_Rotate_X(g_RightUpperArmAngleX);
    glm::mat4 right_upper_arm_model = model * Matrix_Scale(0.2f, 0.4f, 0.2f);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(right_upper_arm_model));
    DrawCube(render_as_black_uniform);

    // Antebraço direito
    model                         = model * Matrix_Translate(0.0f, -0.4f, 0.0f);
    model                         = model * Matrix_Rotate_X(g_RightForearmAngleX);
    glm::mat4 right_forearm_model = model * Matrix_Scale(0.2f, 0.4f, 0.2f);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(right_forearm_model));
    DrawCube(render_as_black_uniform);

    // Mão direita
    model                      = model * Matrix_Translate(0.0f, -0.4f, 0.0f);
    model                      = model * Matrix_Rotate_X(g_RightHandAngleX);
    glm::mat4 right_hand_model = model * Matrix_Scale(0.15f, 0.2f, 0.15f);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(right_hand_model));
    DrawCube(render_as_black_uniform);
    PopMatrix(model);

    // Perna superior esquerda
    PushMatrix(model);
    model                          = model * Matrix_Translate(-0.2f, -1.05f, 0.0f);
    model                          = model * Matrix_Rotate_X(g_LeftUpperLegAngleX);
    glm::mat4 left_upper_leg_model = model * Matrix_Scale(0.2f, 0.4f, 0.2f);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(left_upper_leg_model));
    DrawCube(render_as_black_uniform);

    // Perna inferior esquerda
    model                          = model * Matrix_Translate(0.0f, -0.4f, 0.0f);
    model                          = model * Matrix_Rotate_X(g_LeftLowerLegAngleX);
    glm::mat4 left_lower_leg_model = model * Matrix_Scale(0.2f, 0.4f, 0.2f);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(left_lower_leg_model));
    DrawCube(render_as_black_uniform);

    // Pé esquerdo
    model                     = model * Matrix_Translate(0.0f, -0.4f, 0.1f);
    model                     = model * Matrix_Rotate_X(g_LeftFootAngleX);
    glm::mat4 left_foot_model = model * Matrix_Scale(0.2f, 0.1f, 0.3f);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(left_foot_model));
    DrawCube(render_as_black_uniform);
    PopMatrix(model);

    // Perna superior direita
    PushMatrix(model);
    model                           = model * Matrix_Translate(0.2f, -1.05f, 0.0f);
    model                           = model * Matrix_Rotate_X(g_RightUpperLegAngleX);
    glm::mat4 right_upper_leg_model = model * Matrix_Scale(0.2f, 0.4f, 0.2f);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(right_upper_leg_model));
    DrawCube(render_as_black_uniform);

    // Perna inferior direita
    model                           = model * Matrix_Translate(0.0f, -0.4f, 0.0f);
    model                           = model * Matrix_Rotate_X(g_RightLowerLegAngleX);
    glm::mat4 right_lower_leg_model = model * Matrix_Scale(0.2f, 0.4f, 0.2f);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(right_lower_leg_model));
    DrawCube(render_as_black_uniform);

    // Pé direito
    model                      = model * Matrix_Translate(0.0f, -0.4f, 0.1f);
    model                      = model * Matrix_Rotate_X(g_RightFootAngleX);
    glm::mat4 right_foot_model = model * Matrix_Scale(0.2f, 0.1f, 0.3f);
    glUniformMatrix4fv(model_uniform, 1, GL_FALSE, glm::value_ptr(right_foot_model));
    DrawCube(render_as_black_uniform);
    PopMatrix(model);

    // Imprimimos na tela os ângulos de Euler atuais
    TextRendering_ShowEulerAngles(window);

    // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
    TextRendering_ShowProjection(window);

    // Imprimimos na tela informação sobre o número de quadros renderizados
    // por segundo (frames per second).
    TextRendering_ShowFramesPerSecond(window);

    // O framebuffer onde OpenGL executa as operações de renderização não
    // é o mesmo que está sendo mostrado para o usuário, caso contrário
    // seria possível ver artefatos conhecidos como "screen tearing". A
    // chamada abaixo faz a troca dos buffers, mostrando para o usuário
    // tudo que foi renderizado pelas funções acima.
    // Veja o link: https://en.wikipedia.org/wiki/Multiple_buffering#Double_buffering_in_computer_graphics
    glfwSwapBuffers(window);

    // Verificamos com o sistema operacional se houve alguma interação do
    // usuário (teclado, mouse, ...). Caso positivo, as funções de
    // callback definidas anteriormente usando glfwSet*Callback() serão
    // chamadas pela biblioteca GLFW.
    glfwPollEvents();
  }

  // Finalizamos o uso dos recursos do sistema operacional
  glfwTerminate();

  // Fim do programa
  return 0;
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

// Função que desenha um cubo com arestas em preto, definido dentro da função BuildTriangles().
void DrawCube(GLint render_as_black_uniform) {
  // Informamos para a placa de vídeo (GPU) que a variável booleana
  // "render_as_black" deve ser colocada como "false". Veja o arquivo
  // "shader_vertex.glsl".
  glUniform1i(render_as_black_uniform, false);

  // Pedimos para a GPU rasterizar os vértices do cubo apontados pelo
  // VAO como triângulos, formando as faces do cubo. Esta
  // renderização irá executar o Vertex Shader definido no arquivo
  // "shader_vertex.glsl", e o mesmo irá utilizar as matrizes
  // "model", "view" e "projection" definidas acima e já enviadas
  // para a placa de vídeo (GPU).
  //
  // Veja a definição de g_VirtualScene["cube_faces"] dentro da
  // função BuildTriangles(), e veja a documentação da função
  // glDrawElements() em http://docs.gl/gl3/glDrawElements.
  glDrawElements(g_VirtualScene["cube_faces"].rendering_mode, // Veja slides 182-188 do documento Aula_04_Modelagem_Geometrica_3D.pdf
                 g_VirtualScene["cube_faces"].num_indices,    //
                 GL_UNSIGNED_INT, (void*) g_VirtualScene["cube_faces"].first_index);

  // Pedimos para OpenGL desenhar linhas com largura de 4 pixels.
  glLineWidth(4.0f);

  // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
  // apontados pelo VAO como linhas. Veja a definição de
  // g_VirtualScene["axes"] dentro da função BuildTriangles(), e veja
  // a documentação da função glDrawElements() em
  // http://docs.gl/gl3/glDrawElements.
  //
  // Importante: estes eixos serão desenhamos com a matriz "model"
  // definida acima, e portanto sofrerão as mesmas transformações
  // geométricas que o cubo. Isto é, estes eixos estarão
  // representando o sistema de coordenadas do modelo (e não o global)!
  glDrawElements(g_VirtualScene["axes"].rendering_mode, g_VirtualScene["axes"].num_indices, GL_UNSIGNED_INT,
                 (void*) g_VirtualScene["axes"].first_index);

  // Informamos para a placa de vídeo (GPU) que a variável booleana
  // "render_as_black" deve ser colocada como "true". Veja o arquivo
  // "shader_vertex.glsl".
  glUniform1i(render_as_black_uniform, true);

  // Pedimos para a GPU rasterizar os vértices do cubo apontados pelo
  // VAO como linhas, formando as arestas pretas do cubo. Veja a
  // definição de g_VirtualScene["cube_edges"] dentro da função
  // BuildTriangles(), e veja a documentação da função
  // glDrawElements() em http://docs.gl/gl3/glDrawElements.
  glDrawElements(g_VirtualScene["cube_edges"].rendering_mode, g_VirtualScene["cube_edges"].num_indices, GL_UNSIGNED_INT,
                 (void*) g_VirtualScene["cube_edges"].first_index);
}

// Constrói triângulos para futura renderização
GLuint BuildTriangles() {
  // Primeiro, definimos os atributos de cada vértice.

  // A posição de cada vértice é definida por coeficientes em um sistema de
  // coordenadas local de cada modelo geométrico. Note o uso de coordenadas
  // homogêneas.  Veja as seguintes referências:
  //
  //  - slides 35-48 do documento Aula_08_Sistemas_de_Coordenadas.pdf;
  //  - slides 184-190 do documento Aula_08_Sistemas_de_Coordenadas.pdf;
  //
  // Este vetor "model_coefficients" define a GEOMETRIA (veja slides 103-110 do documento Aula_04_Modelagem_Geometrica_3D.pdf).
  //
  GLfloat model_coefficients[] = {
      // Vértices de um cubo
      //    X      Y     Z     W
      -0.5f, 0.0f, 0.5f, 1.0f,   // posição do vértice 0
      -0.5f, -1.0f, 0.5f, 1.0f,  // posição do vértice 1
      0.5f, -1.0f, 0.5f, 1.0f,   // posição do vértice 2
      0.5f, 0.0f, 0.5f, 1.0f,    // posição do vértice 3
      -0.5f, 0.0f, -0.5f, 1.0f,  // posição do vértice 4
      -0.5f, -1.0f, -0.5f, 1.0f, // posição do vértice 5
      0.5f, -1.0f, -0.5f, 1.0f,  // posição do vértice 6
      0.5f, 0.0f, -0.5f, 1.0f,   // posição do vértice 7
                                 // Vértices para desenhar o eixo X
                                 //    X      Y     Z     W
      0.0f, 0.0f, 0.0f, 1.0f,    // posição do vértice 8
      1.0f, 0.0f, 0.0f, 1.0f,    // posição do vértice 9
                                 // Vértices para desenhar o eixo Y
                                 //    X      Y     Z     W
      0.0f, 0.0f, 0.0f, 1.0f,    // posição do vértice 10
      0.0f, 1.0f, 0.0f, 1.0f,    // posição do vértice 11
                                 // Vértices para desenhar o eixo Z
                                 //    X      Y     Z     W
      0.0f, 0.0f, 0.0f, 1.0f,    // posição do vértice 12
      0.0f, 0.0f, 1.0f, 1.0f,    // posição do vértice 13
  };

  // Criamos o identificador (ID) de um Vertex Buffer Object (VBO).  Um VBO é
  // um buffer de memória que irá conter os valores de um certo atributo de
  // um conjunto de vértices; por exemplo: posição, cor, normais, coordenadas
  // de textura.  Neste exemplo utilizaremos vários VBOs, um para cada tipo de atributo.
  // Agora criamos um VBO para armazenarmos um atributo: posição.
  GLuint VBO_model_coefficients_id;
  glGenBuffers(1, &VBO_model_coefficients_id);

  // Criamos o identificador (ID) de um Vertex Array Object (VAO).  Um VAO
  // contém a definição de vários atributos de um certo conjunto de vértices;
  // isto é, um VAO irá conter ponteiros para vários VBOs.
  GLuint vertex_array_object_id;
  glGenVertexArrays(1, &vertex_array_object_id);

  // "Ligamos" o VAO ("bind"). Informamos que iremos atualizar o VAO cujo ID
  // está contido na variável "vertex_array_object_id".
  glBindVertexArray(vertex_array_object_id);

  // "Ligamos" o VBO ("bind"). Informamos que o VBO cujo ID está contido na
  // variável VBO_model_coefficients_id será modificado a seguir. A
  // constante "GL_ARRAY_BUFFER" informa que esse buffer é de fato um VBO, e
  // irá conter atributos de vértices.
  glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);

  // Alocamos memória para o VBO "ligado" acima. Como queremos armazenar
  // nesse VBO todos os valores contidos no array "model_coefficients", pedimos
  // para alocar um número de bytes exatamente igual ao tamanho ("size")
  // desse array. A constante "GL_STATIC_DRAW" dá uma dica para o driver da
  // GPU sobre como utilizaremos os dados do VBO. Neste caso, estamos dizendo
  // que não pretendemos alterar tais dados (são estáticos: "STATIC"), e
  // também dizemos que tais dados serão utilizados para renderizar ou
  // desenhar ("DRAW").  Pense que:
  //
  //            glBufferData()  ==  malloc() do C  ==  new do C++.
  //
  glBufferData(GL_ARRAY_BUFFER, sizeof(model_coefficients), NULL, GL_STATIC_DRAW);

  // Finalmente, copiamos os valores do array model_coefficients para dentro do
  // VBO "ligado" acima.  Pense que:
  //
  //            glBufferSubData()  ==  memcpy() do C.
  //
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(model_coefficients), model_coefficients);

  // Precisamos então informar um índice de "local" ("location"), o qual será
  // utilizado no shader "shader_vertex.glsl" para acessar os valores
  // armazenados no VBO "ligado" acima. Também, informamos a dimensão (número de
  // coeficientes) destes atributos. Como em nosso caso são pontos em coordenadas
  // homogêneas, temos quatro coeficientes por vértice (X,Y,Z,W). Isso define
  // um tipo de dado chamado de "vec4" em "shader_vertex.glsl": um vetor com
  // quatro coeficientes. Finalmente, informamos que os dados estão em ponto
  // flutuante com 32 bits (GL_FLOAT).
  // Esta função também informa que o VBO "ligado" acima em glBindBuffer()
  // está dentro do VAO "ligado" acima por glBindVertexArray().
  // Veja https://www.khronos.org/opengl/wiki/Vertex_Specification#Vertex_Buffer_Object
  GLuint location             = 0; // "(location = 0)" em "shader_vertex.glsl"
  GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
  glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);

  // "Ativamos" os atributos. Informamos que os atributos com índice de local
  // definido acima, na variável "location", deve ser utilizado durante o
  // rendering.
  glEnableVertexAttribArray(location);

  // "Desligamos" o VBO, evitando assim que operações posteriores venham a
  // alterar o mesmo. Isso evita bugs.
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // Agora repetimos todos os passos acima para atribuir um novo atributo a
  // cada vértice: uma cor (veja slides 109-112 do documento Aula_03_Rendering_Pipeline_Grafico.pdf e slide 111 do documento
  // Aula_04_Modelagem_Geometrica_3D.pdf). Tal cor é definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é: Vermelho, Verde, Azul, Alpha
  // (valor de transparência). Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
  GLfloat color_coefficients[] = {
      // Cores dos vértices do cubo
      //  R     G     B     A
      1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 0
      1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 1
      0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 2
      0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 3
      1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 4
      1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 5
      0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 6
      0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 7
                              // Cores para desenhar o eixo X
      1.0f, 0.0f, 0.0f, 1.0f, // cor do vértice 8
      1.0f, 0.0f, 0.0f, 1.0f, // cor do vértice 9
                              // Cores para desenhar o eixo Y
      0.0f, 1.0f, 0.0f, 1.0f, // cor do vértice 10
      0.0f, 1.0f, 0.0f, 1.0f, // cor do vértice 11
                              // Cores para desenhar o eixo Z
      0.0f, 0.0f, 1.0f, 1.0f, // cor do vértice 12
      0.0f, 0.0f, 1.0f, 1.0f, // cor do vértice 13
  };
  GLuint VBO_color_coefficients_id;
  glGenBuffers(1, &VBO_color_coefficients_id);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_color_coefficients_id);
  glBufferData(GL_ARRAY_BUFFER, sizeof(color_coefficients), NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(color_coefficients), color_coefficients);
  location             = 1; // "(location = 1)" em "shader_vertex.glsl"
  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
  glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(location);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  // Vamos então definir polígonos utilizando os vértices do array
  // model_coefficients.
  //
  // Para referência sobre os modos de renderização, veja slides 182-188 do documento Aula_04_Modelagem_Geometrica_3D.pdf.
  //
  // Este vetor "indices" define a TOPOLOGIA (veja slides 103-110 do documento Aula_04_Modelagem_Geometrica_3D.pdf).
  //
  GLuint indices[] = {
      // Definimos os índices dos vértices que definem as FACES de um cubo
      // através de 12 triângulos que serão desenhados com o modo de renderização
      // GL_TRIANGLES.
      0, 1, 2, // triângulo 1
      7, 6, 5, // triângulo 2
      3, 2, 6, // triângulo 3
      4, 0, 3, // triângulo 4
      4, 5, 1, // triângulo 5
      1, 5, 6, // triângulo 6
      0, 2, 3, // triângulo 7
      7, 5, 4, // triângulo 8
      3, 6, 7, // triângulo 9
      4, 3, 7, // triângulo 10
      4, 1, 0, // triângulo 11
      1, 6, 2, // triângulo 12
               // Definimos os índices dos vértices que definem as ARESTAS de um cubo
               // através de 12 linhas que serão desenhadas com o modo de renderização
               // GL_LINES.
      0, 1,    // linha 1
      1, 2,    // linha 2
      2, 3,    // linha 3
      3, 0,    // linha 4
      0, 4,    // linha 5
      4, 7,    // linha 6
      7, 6,    // linha 7
      6, 2,    // linha 8
      6, 5,    // linha 9
      5, 4,    // linha 10
      5, 1,    // linha 11
      7, 3,    // linha 12
               // Definimos os índices dos vértices que definem as linhas dos eixos X, Y,
               // Z, que serão desenhados com o modo GL_LINES.
      8, 9,    // linha 1
      10, 11,  // linha 2
      12, 13   // linha 3
  };

  // Criamos um primeiro objeto virtual (SceneObject) que se refere às faces
  // coloridas do cubo.
  SceneObject cube_faces;
  cube_faces.name           = "Cubo (faces coloridas)";
  cube_faces.first_index    = (void*) 0;    // Primeiro índice está em indices[0]
  cube_faces.num_indices    = 36;           // Último índice está em indices[35]; total de 36 índices.
  cube_faces.rendering_mode = GL_TRIANGLES; // Índices correspondem ao tipo de rasterização GL_TRIANGLES.

  // Adicionamos o objeto criado acima na nossa cena virtual (g_VirtualScene).
  g_VirtualScene["cube_faces"] = cube_faces;

  // Criamos um segundo objeto virtual (SceneObject) que se refere às arestas
  // pretas do cubo.
  SceneObject cube_edges;
  cube_edges.name           = "Cubo (arestas pretas)";
  cube_edges.first_index    = (void*) (36 * sizeof(GLuint)); // Primeiro índice está em indices[36]
  cube_edges.num_indices    = 24;                            // Último índice está em indices[59]; total de 24 índices.
  cube_edges.rendering_mode = GL_LINES;                      // Índices correspondem ao tipo de rasterização GL_LINES.

  // Adicionamos o objeto criado acima na nossa cena virtual (g_VirtualScene).
  g_VirtualScene["cube_edges"] = cube_edges;

  // Criamos um terceiro objeto virtual (SceneObject) que se refere aos eixos XYZ.
  SceneObject axes;
  axes.name              = "Eixos XYZ";
  axes.first_index       = (void*) (60 * sizeof(GLuint)); // Primeiro índice está em indices[60]
  axes.num_indices       = 6;                             // Último índice está em indices[65]; total de 6 índices.
  axes.rendering_mode    = GL_LINES;                      // Índices correspondem ao tipo de rasterização GL_LINES.
  g_VirtualScene["axes"] = axes;

  // Criamos um buffer OpenGL para armazenar os índices acima
  GLuint indices_id;
  glGenBuffers(1, &indices_id);

  // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);

  // Alocamos memória para o buffer.
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), NULL, GL_STATIC_DRAW);

  // Copiamos os valores do array indices[] para dentro do buffer.
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(indices), indices);

  // NÃO faça a chamada abaixo! Diferente de um VBO (GL_ARRAY_BUFFER), um
  // array de índices (GL_ELEMENT_ARRAY_BUFFER) não pode ser "desligado",
  // caso contrário o VAO irá perder a informação sobre os índices.
  //
  // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
  //

  // "Desligamos" o VAO, evitando assim que operações posteriores venham a
  // alterar o mesmo. Isso evita bugs.
  glBindVertexArray(0);

  // Retornamos o ID do VAO. Isso é tudo que será necessário para renderizar
  // os triângulos definidos acima. Veja a chamada glDrawElements() em main().
  return vertex_array_object_id;
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

  // Retornamos o ID gerado acima
  return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
  // Indicamos que queremos renderizar em toda região do framebuffer. A
  // função "glViewport" define o mapeamento das "normalized device
  // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
  // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
  glViewport(0, 0, width, height);

  // Atualizamos também a razão que define a proporção da janela (largura /
  // altura), a qual será utilizada na definição das matrizes de projeção,
  // tal que não ocorra distorções durante o processo de "Screen Mapping"
  // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
  //
  // O cast para float é necessário pois números inteiros são arredondados ao
  // serem divididos!
  // g_ScreenRatio = (float) width / height;
  camera->setScreenRatio((float) width / height);
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

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

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
  // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
  // pressionado, computamos quanto que o mouse se movimento desde o último
  // instante de tempo, e usamos esta movimentação para atualizar os
  // parâmetros que definem a posição da câmera dentro da cena virtual.
  // Assim, temos que o usuário consegue controlar a câmera.

  if (g_LeftMouseButtonPressed) {
    // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;

    // Atualizamos parâmetros da câmera com os deslocamentos
    // g_CameraTheta -= 0.01f * dx;
    // g_CameraPhi += 0.01f * dy;

    float newTheta = camera->getTheta();
    newTheta -= 0.01f * dx;
    camera->setTheta(newTheta);

    float newPhi = camera->getPhi();
    newPhi -= 0.01f * dy;
    camera->setPhi(newPhi);

    // Atualizamos as variáveis globais para armazenar a posição atual do
    // cursor como sendo a última posição conhecida do cursor.
    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;
  }

  if (g_RightMouseButtonPressed) {
    // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;

    // Atualizamos parâmetros da antebraço com os deslocamentos
    g_ForearmAngleZ -= 0.01f * dx;
    g_ForearmAngleX += 0.01f * dy;

    // Atualizamos as variáveis globais para armazenar a posição atual do
    // cursor como sendo a última posição conhecida do cursor.
    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;
  }

  if (g_MiddleMouseButtonPressed) {
    // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
    float dx = xpos - g_LastCursorPosX;
    float dy = ypos - g_LastCursorPosY;

    // Atualizamos parâmetros da antebraço com os deslocamentos
    g_TorsoPositionX += 0.01f * dx;
    g_TorsoPositionY -= 0.01f * dy;

    // Atualizamos as variáveis globais para armazenar a posição atual do
    // cursor como sendo a última posição conhecida do cursor.
    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;
  }
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
  // Atualizamos a distância da câmera para a origem utilizando a
  // movimentação da "rodinha", simulando um ZOOM.
  // g_CameraDistance -= 0.1f * yoffset;
  float newDistance = camera->getDistance();
  newDistance -= 0.1f * yoffset;
  camera->setDistance(newDistance);
}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod) {
  if (key == GLFW_KEY_C && action == GLFW_PRESS) {
    float screenRatio = camera->getScreenRatio();
    camera            = (camera == &freeCamera) ? (Camera*) &sphericCamera : (Camera*) &freeCamera;
    camera->setScreenRatio(screenRatio);
  }

  if (key == GLFW_KEY_Q && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GL_TRUE);

  if (key == GLFW_KEY_W && action == GLFW_PRESS) {
    camera->MoveForward();
  }

  if (key == GLFW_KEY_A && action == GLFW_PRESS) {
    camera->MoveLeft();
  }

  if (key == GLFW_KEY_S && action == GLFW_PRESS) {
    camera->MoveBackward();
  }

  if (key == GLFW_KEY_D && action == GLFW_PRESS) {
    camera->MoveRight();
  }

  // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
  if (key == GLFW_KEY_P && action == GLFW_PRESS) {
    camera->UsePerspectiveProjection = true;
  }

  // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
  if (key == GLFW_KEY_O && action == GLFW_PRESS) {
    camera->UsePerspectiveProjection = false;
  }


  if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && mod != GLFW_MOD_SHIFT) {
    camera->MoveUpwards();
  }

  if (key == GLFW_KEY_SPACE && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT) {
    camera->MoveDownwards();
  }

  // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
  if (key == GLFW_KEY_H && action == GLFW_PRESS) {
    g_ShowInfoText = !g_ShowInfoText;
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
