#ifndef MAZE_HPP
#define MAZE_HPP

#include <vector>
#include <string>
#include <random>
#include <iostream>
#include <memory>
#include <list>
#include <tiny_obj_loader.h>


// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel {
  tinyobj::attrib_t                attrib;
  std::vector<tinyobj::shape_t>    shapes;
  std::vector<tinyobj::material_t> materials;

  // Construtor vazio para criação programática
  ObjModel() = default;

  // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
  // Veja: https://github.com/syoyo/tinyobjloader
  ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true) {
    printf("Carregando objetos do arquivo \"%s\"...\n", filename);

    // Se basepath == NULL, então setamos basepath como o dirname do
    // filename, para que os arquivos MTL sejam corretamente carregados caso
    // estejam no mesmo diretório dos arquivos OBJ.
    std::string fullpath(filename);
    std::string dirname;
    if (basepath == NULL) {
      auto i = fullpath.find_last_of("/");
      if (i != std::string::npos) {
        dirname  = fullpath.substr(0, i + 1);
        basepath = dirname.c_str();
      }
    }

    std::string warn;
    std::string err;
    bool        ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

    if (!err.empty())
      fprintf(stderr, "\n%s\n", err.c_str());

    if (!ret)
      throw std::runtime_error("Erro ao carregar modelo.");

    for (size_t shape = 0; shape < shapes.size(); ++shape) {
      if (shapes[shape].name.empty()) {
        fprintf(stderr,
                "*********************************************\n"
                "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                "*********************************************\n",
                filename);
        throw std::runtime_error("Objeto sem nome.");
      }
      printf("- Objeto '%s'\n", shapes[shape].name.c_str());
    }

    printf("OK.\n");
  }
};


class MazeGenerator {
  private:
  struct Cell {
    int  x, y;
    bool visited  = false;
    bool walls[4] = {true, true, true, true}; // Norte, Sul, Leste, Oeste
  };

  struct Wall {
    float x, y, z;
    float width, height, depth;
    int   id;
  };

  int                            width, height;
  std::vector<std::vector<Cell>> grid;
  std::list<Wall>                walls;
  std::mt19937                   rng;
  int                            wallCounter = 0;

  // Direções: Norte, Sul, Leste, Oeste
  const int dx[4] = {0, 0, 1, -1};
  const int dy[4] = {-1, 1, 0, 0};

  public:
  MazeGenerator(int w, int h, unsigned int seed = std::random_device{}())
      : width(w), height(h), rng(seed) {
    grid.resize(height, std::vector<Cell>(width));
    initializeGrid();
  }

  void initializeGrid() {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        grid[y][x].x = x;
        grid[y][x].y = y;
      }
    }
  }

  void generateMaze() {
    // Algoritmo de geração usando DFS recursivo com backtracking
    std::vector<std::pair<int, int>> stack;

    // Começar do centro
    int startX = width / 2;
    int startY = height / 2;

    grid[startY][startX].visited = true;
    stack.push_back({startX, startY});

    while (!stack.empty()) {
      auto [currentX, currentY] = stack.back();

      std::vector<int> neighbors;

      // Encontrar vizinhos não visitados
      for (int dir = 0; dir < 4; dir++) {
        int newX = currentX + dx[dir];
        int newY = currentY + dy[dir];

        if (isValidCell(newX, newY) && !grid[newY][newX].visited) {
          neighbors.push_back(dir);
        }
      }

      if (!neighbors.empty()) {
        // Escolher direção aleatória
        int randomDir = neighbors[rng() % neighbors.size()];
        int newX      = currentX + dx[randomDir];
        int newY      = currentY + dy[randomDir];

        // Remover parede entre células
        removeWall(currentX, currentY, newX, newY);

        grid[newY][newX].visited = true;
        stack.push_back({newX, newY});
      } else {
        stack.pop_back();
      }
    }

    // Criar múltiplas entradas e saídas
    createMultipleEntrances();

    // Converter grid em paredes 3D
    generateWalls();
  }

  void createMultipleEntrances() {
    // Criar 4-8 entradas/saídas aleatórias nas bordas
    int numEntrances = 4 + (rng() % 5); // 4 a 8 entradas

    for (int i = 0; i < numEntrances; i++) {
      int side = rng() % 4; // 0=norte, 1=sul, 2=leste, 3=oeste

      switch (side) {
      case 0: // Norte
      {
        int x = rng() % width;
        if (x > 0)
          grid[0][x].walls[0] = false;
      } break;
      case 1: // Sul
      {
        int x = rng() % width;
        if (x > 0)
          grid[height - 1][x].walls[1] = false;
      } break;
      case 2: // Leste
      {
        int y = rng() % height;
        if (y > 0)
          grid[y][width - 1].walls[2] = false;
      } break;
      case 3: // Oeste
      {
        int y = rng() % height;
        if (y > 0)
          grid[y][0].walls[3] = false;
      } break;
      }
    }
  }

  void generateWalls() {
    walls.clear();
    const float cellSize      = 2.0f;
    const float wallThickness = 0.2f;
    const float wallHeight    = 3.0f;

    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        float centerX = x * cellSize;
        float centerZ = y * cellSize;

        // Parede Norte
        if (grid[y][x].walls[0]) {
          Wall wall;
          wall.x      = centerX;
          wall.y      = wallHeight / 2.0f;
          wall.z      = centerZ - cellSize / 2.0f;
          wall.width  = cellSize;
          wall.height = wallHeight;
          wall.depth  = wallThickness;
          wall.id     = ++wallCounter;
          walls.push_back(wall);
        }

        // Parede Sul
        if (grid[y][x].walls[1]) {
          Wall wall;
          wall.x      = centerX;
          wall.y      = wallHeight / 2.0f;
          wall.z      = centerZ + cellSize / 2.0f;
          wall.width  = cellSize;
          wall.height = wallHeight;
          wall.depth  = wallThickness;
          wall.id     = ++wallCounter;
          walls.push_back(wall);
        }

        // Parede Leste
        if (grid[y][x].walls[2]) {
          Wall wall;
          wall.x      = centerX + cellSize / 2.0f;
          wall.y      = wallHeight / 2.0f;
          wall.z      = centerZ;
          wall.width  = wallThickness;
          wall.height = wallHeight;
          wall.depth  = cellSize;
          wall.id     = ++wallCounter;
          walls.push_back(wall);
        }

        // Parede Oeste
        if (grid[y][x].walls[3]) {
          Wall wall;
          wall.x      = centerX - cellSize / 2.0f;
          wall.y      = wallHeight / 2.0f;
          wall.z      = centerZ;
          wall.width  = wallThickness;
          wall.height = wallHeight;
          wall.depth  = cellSize;
          wall.id     = ++wallCounter;
          walls.push_back(wall);
        }
      }
    }
  }

  // Função principal: exporta diretamente para ObjModel
  std::unique_ptr<ObjModel> exportToObjModel() {
    auto objModel = std::unique_ptr<ObjModel>(new ObjModel());

    // Limpar dados
    objModel->attrib.vertices.clear();
    objModel->attrib.normals.clear();
    objModel->attrib.texcoords.clear();
    objModel->shapes.clear();
    objModel->materials.clear();

    // Criar material padrão para as paredes
    tinyobj::material_t wallMaterial;
    wallMaterial.name        = "wall_material";
    wallMaterial.ambient[0]  = 0.3f;
    wallMaterial.ambient[1]  = 0.3f;
    wallMaterial.ambient[2]  = 0.3f;
    wallMaterial.diffuse[0]  = 0.8f;
    wallMaterial.diffuse[1]  = 0.8f;
    wallMaterial.diffuse[2]  = 0.8f;
    wallMaterial.specular[0] = 0.1f;
    wallMaterial.specular[1] = 0.1f;
    wallMaterial.specular[2] = 0.1f;
    wallMaterial.shininess   = 10.0f;
    wallMaterial.dissolve    = 1.0f;
    objModel->materials.push_back(wallMaterial);

    for (const auto& wall : walls) {
      // Criar shape para cada parede
      tinyobj::shape_t shape;
      shape.name = "wall_" + std::to_string(wall.id);

      // Calcular dimensões da parede
      float halfWidth  = wall.width / 2.0f;
      float halfHeight = wall.height / 2.0f;
      float halfDepth  = wall.depth / 2.0f;

      // Índice base para os vértices desta parede
      size_t baseVertexIndex = objModel->attrib.vertices.size() / 3;

      // Adicionar os 8 vértices do cubo
      std::vector<float> cubeVertices = {
          // Vértice 0: (-x, -y, -z)
          wall.x - halfWidth, wall.y - halfHeight, wall.z - halfDepth,
          // Vértice 1: (+x, -y, -z)
          wall.x + halfWidth, wall.y - halfHeight, wall.z - halfDepth,
          // Vértice 2: (+x, +y, -z)
          wall.x + halfWidth, wall.y + halfHeight, wall.z - halfDepth,
          // Vértice 3: (-x, +y, -z)
          wall.x - halfWidth, wall.y + halfHeight, wall.z - halfDepth,
          // Vértice 4: (-x, -y, +z)
          wall.x - halfWidth, wall.y - halfHeight, wall.z + halfDepth,
          // Vértice 5: (+x, -y, +z)
          wall.x + halfWidth, wall.y - halfHeight, wall.z + halfDepth,
          // Vértice 6: (+x, +y, +z)
          wall.x + halfWidth, wall.y + halfHeight, wall.z + halfDepth,
          // Vértice 7: (-x, +y, +z)
          wall.x - halfWidth, wall.y + halfHeight, wall.z + halfDepth};

      // Adicionar vértices ao attrib
      objModel->attrib.vertices.insert(objModel->attrib.vertices.end(), cubeVertices.begin(), cubeVertices.end());

      // Adicionar normais para as faces
      std::vector<float> cubeNormals = {
          // 8 vértices, cada um com normal (0,0,-1) para face frontal
          0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f,
          0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f};
      objModel->attrib.normals.insert(objModel->attrib.normals.end(), cubeNormals.begin(), cubeNormals.end());

      // Adicionar coordenadas de textura básicas
      std::vector<float> cubeTexCoords = {
          0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
          0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};
      objModel->attrib.texcoords.insert(objModel->attrib.texcoords.end(), cubeTexCoords.begin(), cubeTexCoords.end());

      // Definir índices das faces do cubo (12 triângulos)
      std::vector<tinyobj::index_t> cubeIndices;

      // Face frontal (2 triângulos)
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 0), static_cast<int>(baseVertexIndex + 0), static_cast<int>(baseVertexIndex + 0)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 1), static_cast<int>(baseVertexIndex + 1), static_cast<int>(baseVertexIndex + 1)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 2), static_cast<int>(baseVertexIndex + 2), static_cast<int>(baseVertexIndex + 2)});

      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 0), static_cast<int>(baseVertexIndex + 0), static_cast<int>(baseVertexIndex + 0)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 2), static_cast<int>(baseVertexIndex + 2), static_cast<int>(baseVertexIndex + 2)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 3), static_cast<int>(baseVertexIndex + 3), static_cast<int>(baseVertexIndex + 3)});

      // Face traseira (2 triângulos)
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 4), static_cast<int>(baseVertexIndex + 4), static_cast<int>(baseVertexIndex + 4)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 7), static_cast<int>(baseVertexIndex + 7), static_cast<int>(baseVertexIndex + 7)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 6), static_cast<int>(baseVertexIndex + 6), static_cast<int>(baseVertexIndex + 6)});

      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 4), static_cast<int>(baseVertexIndex + 4), static_cast<int>(baseVertexIndex + 4)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 6), static_cast<int>(baseVertexIndex + 6), static_cast<int>(baseVertexIndex + 6)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 5), static_cast<int>(baseVertexIndex + 5), static_cast<int>(baseVertexIndex + 5)});

      // Face esquerda (2 triângulos)
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 0), static_cast<int>(baseVertexIndex + 0), static_cast<int>(baseVertexIndex + 0)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 4), static_cast<int>(baseVertexIndex + 4), static_cast<int>(baseVertexIndex + 4)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 5), static_cast<int>(baseVertexIndex + 5), static_cast<int>(baseVertexIndex + 5)});

      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 0), static_cast<int>(baseVertexIndex + 0), static_cast<int>(baseVertexIndex + 0)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 5), static_cast<int>(baseVertexIndex + 5), static_cast<int>(baseVertexIndex + 5)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 1), static_cast<int>(baseVertexIndex + 1), static_cast<int>(baseVertexIndex + 1)});

      // Face direita (2 triângulos)
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 2), static_cast<int>(baseVertexIndex + 2), static_cast<int>(baseVertexIndex + 2)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 6), static_cast<int>(baseVertexIndex + 6), static_cast<int>(baseVertexIndex + 6)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 7), static_cast<int>(baseVertexIndex + 7), static_cast<int>(baseVertexIndex + 7)});

      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 2), static_cast<int>(baseVertexIndex + 2), static_cast<int>(baseVertexIndex + 2)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 7), static_cast<int>(baseVertexIndex + 7), static_cast<int>(baseVertexIndex + 7)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 3), static_cast<int>(baseVertexIndex + 3), static_cast<int>(baseVertexIndex + 3)});

      // Face superior (2 triângulos)
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 3), static_cast<int>(baseVertexIndex + 3), static_cast<int>(baseVertexIndex + 3)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 7), static_cast<int>(baseVertexIndex + 7), static_cast<int>(baseVertexIndex + 7)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 6), static_cast<int>(baseVertexIndex + 6), static_cast<int>(baseVertexIndex + 6)});

      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 3), static_cast<int>(baseVertexIndex + 3), static_cast<int>(baseVertexIndex + 3)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 6), static_cast<int>(baseVertexIndex + 6), static_cast<int>(baseVertexIndex + 6)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 2), static_cast<int>(baseVertexIndex + 2), static_cast<int>(baseVertexIndex + 2)});

      // Face inferior (2 triângulos)
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 0), static_cast<int>(baseVertexIndex + 0), static_cast<int>(baseVertexIndex + 0)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 1), static_cast<int>(baseVertexIndex + 1), static_cast<int>(baseVertexIndex + 1)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 5), static_cast<int>(baseVertexIndex + 5), static_cast<int>(baseVertexIndex + 5)});

      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 0), static_cast<int>(baseVertexIndex + 0), static_cast<int>(baseVertexIndex + 0)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 5), static_cast<int>(baseVertexIndex + 5), static_cast<int>(baseVertexIndex + 5)});
      cubeIndices.push_back({static_cast<int>(baseVertexIndex + 4), static_cast<int>(baseVertexIndex + 4), static_cast<int>(baseVertexIndex + 4)});

      // Configurar mesh da parede
      shape.mesh.indices = cubeIndices;
      shape.mesh.num_face_vertices.resize(12, 3); // 12 triângulos, 3 vértices cada
      shape.mesh.material_ids.resize(12, 0);      // Usar material 0 (wall_material)

      objModel->shapes.push_back(shape);
    }

    return objModel;
  }

  int getWallCount() const {
    return static_cast<int>(walls.size());
  }

  // Função para obter lista de nomes das paredes
  std::list<std::string> getWallNames() const {
    std::list<std::string> wallNames;
    for (const auto& wall : walls) {
      wallNames.push_back("wall_" + std::to_string(wall.id));
    }
    return wallNames;
  }

  void printMazeInfo() const {
    std::cout << "Labirinto gerado:\n";
    std::cout << "Dimensões: " << width << "x" << height << "\n";
    std::cout << "Número de paredes: " << walls.size() << "\n";
    std::cout << "Múltiplas entradas e saídas criadas\n";
  }

  private:
  bool isValidCell(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height;
  }

  void removeWall(int x1, int y1, int x2, int y2) {
    if (x1 == x2) {                    // Movimento vertical
      if (y1 < y2) {                   // Movendo para sul
        grid[y1][x1].walls[1] = false; // Remove parede sul da célula atual
        grid[y2][x2].walls[0] = false; // Remove parede norte da próxima célula
      } else {                         // Movendo para norte
        grid[y1][x1].walls[0] = false; // Remove parede norte da célula atual
        grid[y2][x2].walls[1] = false; // Remove parede sul da próxima célula
      }
    } else {                           // Movimento horizontal
      if (x1 < x2) {                   // Movendo para leste
        grid[y1][x1].walls[2] = false; // Remove parede leste da célula atual
        grid[y2][x2].walls[3] = false; // Remove parede oeste da próxima célula
      } else {                         // Movendo para oeste
        grid[y1][x1].walls[3] = false; // Remove parede oeste da célula atual
        grid[y2][x2].walls[2] = false; // Remove parede leste da próxima célula
      }
    }
  }
};

#endif // MAZE_HPP
