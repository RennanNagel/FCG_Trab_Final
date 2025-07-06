#ifndef MAZE_HPP
#define MAZE_HPP

#include <vector>
#include <string>
#include <random>
#include <iostream>
#include <memory>
#include <list>
#include <fstream>
#include <tiny_obj_loader.h>


using namespace std;

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel {
  tinyobj::attrib_t           attrib;
  vector<tinyobj::shape_t>    shapes;
  vector<tinyobj::material_t> materials;

  // Construtor vazio para criação programática
  ObjModel() = default;

  // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
  // Veja: https://github.com/syoyo/tinyobjloader
  ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true) {
    printf("Carregando objetos do arquivo \"%s\"...\n", filename);

    // Se basepath == NULL, então setamos basepath como o dirname do
    // filename, para que os arquivos MTL sejam corretamente carregados caso
    // estejam no mesmo diretório dos arquivos OBJ.
    string fullpath(filename);
    string dirname;
    if (basepath == NULL) {
      auto i = fullpath.find_last_of("/");
      if (i != string::npos) {
        dirname  = fullpath.substr(0, i + 1);
        basepath = dirname.c_str();
      }
    }

    string warn;
    string err;
    bool   ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

    if (!err.empty())
      fprintf(stderr, "\n%s\n", err.c_str());

    if (!ret)
      throw runtime_error("Erro ao carregar modelo.");

    for (size_t shape = 0; shape < shapes.size(); ++shape) {
      if (shapes[shape].name.empty()) {
        fprintf(stderr,
                "*********************************************\n"
                "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                "*********************************************\n",
                filename);
        throw runtime_error("Objeto sem nome.");
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

  int                  width, height;
  vector<vector<Cell>> grid;
  list<Wall>           walls;
  mt19937              rng;
  int                  wallCounter = 0;

  // Direções: Norte, Sul, Leste, Oeste
  const int dx[4] = {0, 0, 1, -1};
  const int dy[4] = {-1, 1, 0, 0};

  public:
  MazeGenerator(int w, int h, unsigned int seed = random_device{}())
      : width(w), height(h), rng(seed) {
    grid.resize(height, vector<Cell>(width));
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
    vector<pair<int, int>> stack;

    // Começar do centro
    int startX = width / 2;
    int startY = height / 2;

    grid[startY][startX].visited = true;
    stack.push_back({startX, startY});

    while (!stack.empty()) {
      auto current  = stack.back();
      int  currentX = current.first;
      int  currentY = current.second;

      vector<int> neighbors;

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
    int numEntrances = 8 + (rng() % 8); // 8 a 16 entradas

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
  map<string, unique_ptr<ObjModel>> exportToObjModels() {
    map<string, unique_ptr<ObjModel>> wallModels;

    for (const auto& wall : walls) {
      auto objModel = unique_ptr<ObjModel>(new ObjModel());

      // Material para as paredes
      tinyobj::material_t material;
      material.name = "wall_material";

      // Propriedades do material
      material.ambient[0] = 0.2f;
      material.ambient[1] = 0.2f;
      material.ambient[2] = 0.2f;

      material.diffuse[0] = 0.8f;
      material.diffuse[1] = 0.8f;
      material.diffuse[2] = 0.8f;

      material.specular[0] = 0.1f;
      material.specular[1] = 0.1f;
      material.specular[2] = 0.1f;

      material.shininess = 32.0f;
      material.dissolve  = 1.0f;

      objModel->materials.push_back(material);

      tinyobj::shape_t shape;
      shape.name = "wall_" + to_string(wall.id);

      float hw = wall.width * 0.5f;
      float hh = wall.height * 0.5f;
      float hd = wall.depth * 0.5f;

      // Vertices do cubo
      vector<float> vertices = {
          wall.x - hw, wall.y - hh, wall.z - hd, // 0
          wall.x + hw, wall.y - hh, wall.z - hd, // 1
          wall.x + hw, wall.y + hh, wall.z - hd, // 2
          wall.x - hw, wall.y + hh, wall.z - hd, // 3
          wall.x - hw, wall.y - hh, wall.z + hd, // 4
          wall.x + hw, wall.y - hh, wall.z + hd, // 5
          wall.x + hw, wall.y + hh, wall.z + hd, // 6
          wall.x - hw, wall.y + hh, wall.z + hd  // 7
      };
      objModel->attrib.vertices.insert(objModel->attrib.vertices.end(), vertices.begin(), vertices.end());

      // Normais para cada face do cubo
      vector<float> normals = {
          // Face frontal (z-)
          0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f,
          // Face traseira (z+)
          0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
          // Face esquerda (x-)
          -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
          // Face direita (x+)
          1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
          // Face inferior (y-)
          0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f,
          // Face superior (y+)
          0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f};
      objModel->attrib.normals.insert(objModel->attrib.normals.end(), normals.begin(), normals.end());

      // Coordenadas de textura corrigidas para orientação consistente
      vector<float> texcoords = {
          // Face frontal (z-) - vértices 0,3,2,1
          0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
          // Face traseira (z+) - vértices 4,5,6,7
          1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
          // Face esquerda (x-) - vértices 0,4,7,3
          1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
          // Face direita (x+) - vértices 2,6,5,1
          0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
          // Face inferior (y-) - vértices 0,1,5,4
          0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,
          // Face superior (y+) - vértices 7,6,2,3
          0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f};
      objModel->attrib.texcoords.insert(objModel->attrib.texcoords.end(), texcoords.begin(), texcoords.end());

      // Faces do cubo com ordem correta para normais apontando para fora
      const int faces[6][4] = {
          {0, 3, 2, 1}, // Face frontal (z-) - ordem anti-horária
          {4, 5, 6, 7}, // Face traseira (z+) - ordem anti-horária
          {0, 4, 7, 3}, // Face esquerda (x-) - ordem anti-horária
          {2, 6, 5, 1}, // Face direita (x+) - ordem anti-horária
          {0, 1, 5, 4}, // Face inferior (y-) - ordem anti-horária
          {7, 6, 2, 3}  // Face superior (y+) - ordem anti-horária
      };

      for (int f = 0; f < 6; ++f) {
        int v0 = faces[f][0];
        int v1 = faces[f][1];
        int v2 = faces[f][2];
        int v3 = faces[f][3];

        // Índices das normais e texturas para cada face
        int n_base = f * 4; // 4 normais por face
        int t_base = f * 4; // 4 coordenadas de textura por face

        // Primeiro triângulo da face (ordem anti-horária)
        shape.mesh.indices.push_back({v0, t_base + 0, n_base + 0});
        shape.mesh.indices.push_back({v1, t_base + 1, n_base + 1});
        shape.mesh.indices.push_back({v2, t_base + 2, n_base + 2});

        // Segundo triângulo da face (ordem anti-horária)
        shape.mesh.indices.push_back({v0, t_base + 0, n_base + 0});
        shape.mesh.indices.push_back({v2, t_base + 2, n_base + 2});
        shape.mesh.indices.push_back({v3, t_base + 3, n_base + 3});

        shape.mesh.num_face_vertices.push_back(3);
        shape.mesh.num_face_vertices.push_back(3);
        shape.mesh.material_ids.push_back(0);
        shape.mesh.material_ids.push_back(0);
      }

      objModel->shapes.push_back(shape);

      string wallName      = "wall_" + to_string(wall.id);
      wallModels[wallName] = std::move(objModel);
    }

    return wallModels;
  }

  int getWallCount() const {
    return static_cast<int>(walls.size());
  }

  // Função para obter lista de nomes das paredes
  list<string> getWallNames() const {
    list<string> wallNames;
    for (const auto& wall : walls) {
      wallNames.push_back("wall_" + to_string(wall.id));
    }
    return wallNames;
  }

  // Função para obter posições válidas para movimento (células acessíveis)
  vector<pair<int, int>> getValidPositions() const {
    vector<pair<int, int>> validPositions;
    
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        // Verificar se a célula tem pelo menos uma parede removida (é acessível)
        bool hasOpenPath = false;
        
        // Verificar se alguma parede foi removida
        for (int dir = 0; dir < 4; dir++) {
          if (!grid[y][x].walls[dir]) {
            hasOpenPath = true;
            break;
          }
        }
        
        // Se tem caminho aberto, é uma posição válida
        if (hasOpenPath) {
          validPositions.push_back({x, y});
        }
      }
    }
    
    return validPositions;
  }

  // Função para converter coordenadas de célula para coordenadas do mundo
  pair<float, float> cellToWorldCoords(int cellX, int cellY) const {
    const float cellSize = 2.0f;
    float worldX = cellX * cellSize;
    float worldZ = cellY * cellSize;
    return {worldX, worldZ};
  }

  // Função para verificar se uma posição de célula é válida e acessível
  bool isValidPosition(int x, int y) const {
    if (x < 0 || x >= width || y < 0 || y >= height) {
      return false;
    }
    
    // Verificar se a célula tem pelo menos uma parede removida (é acessível)
    for (int dir = 0; dir < 4; dir++) {
      if (!grid[y][x].walls[dir]) {
        return true;
      }
    }
    
    return false;
  }

  // Função para obter vizinhos válidos de uma posição
  vector<pair<int, int>> getValidNeighbors(int x, int y) const {
    vector<pair<int, int>> neighbors;
    
    // Direções: Norte, Sul, Leste, Oeste
    const int dx[4] = {0, 0, 1, -1};
    const int dy[4] = {-1, 1, 0, 0};
    
    for (int dir = 0; dir < 4; dir++) {
      int newX = x + dx[dir];
      int newY = y + dy[dir];
      
      // Verificar se a nova posição é válida
      if (isValidPosition(newX, newY)) {
        // Verificar se não há parede bloqueando o caminho
        if (!grid[y][x].walls[dir]) {
          neighbors.push_back({newX, newY});
        }
      }
    }
    
    return neighbors;
  }


  void printMazeInfo() const {
    cout << "Labirinto gerado:\n";
    cout << "Dimensões: " << width << "x" << height << "\n";
    cout << "Número de paredes: " << walls.size() << "\n";
    cout << "Múltiplas entradas e saídas criadas\n";
  }

  // Função para salvar o labirinto como imagem PPM
  void saveToPPM(const string& filename) const {
    ofstream file(filename);
    if (!file.is_open()) {
      cerr << "Erro: Não foi possível abrir o arquivo " << filename << " para escrita.\n";
      return;
    }

    // Calcular dimensões da imagem (cada célula = 10x10 pixels)
    const int cellPixels  = 10;
    const int imageWidth  = width * cellPixels;
    const int imageHeight = height * cellPixels;

    // Escrever cabeçalho PPM
    file << "P3\n";
    file << imageWidth << " " << imageHeight << "\n";
    file << "255\n";

    // Cores
    const int wallColor[3] = {0, 0, 0};       // Preto para paredes
    const int pathColor[3] = {255, 255, 255}; // Branco para caminhos

    // Gerar imagem pixel por pixel
    for (int y = 0; y < imageHeight; y++) {
      for (int x = 0; x < imageWidth; x++) {
        // Determinar qual célula do grid corresponde a este pixel
        int cellX = x / cellPixels;
        int cellY = y / cellPixels;

        // Posição dentro da célula (0-9)
        int pixelX = x % cellPixels;
        int pixelY = y % cellPixels;

        bool isWall = false;

        // Verificar se estamos numa borda de parede
        if (cellX < width && cellY < height) {
          // Parede norte (topo da célula)
          if (pixelY == 0 && grid[cellY][cellX].walls[0]) {
            isWall = true;
          }
          // Parede sul (fundo da célula)
          else if (pixelY == cellPixels - 1 && grid[cellY][cellX].walls[1]) {
            isWall = true;
          }
          // Parede oeste (esquerda da célula)
          else if (pixelX == 0 && grid[cellY][cellX].walls[3]) {
            isWall = true;
          }
          // Parede leste (direita da célula)
          else if (pixelX == cellPixels - 1 && grid[cellY][cellX].walls[2]) {
            isWall = true;
          }
        }

        // Escrever cor do pixel
        if (isWall) {
          file << wallColor[0] << " " << wallColor[1] << " " << wallColor[2] << " ";
        } else {
          file << pathColor[0] << " " << pathColor[1] << " " << pathColor[2] << " ";
        }
      }
      file << "\n";
    }

    file.close();
    cout << "Labirinto salvo como " << filename << "\n";
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
