#ifndef COLLISIONS_HPP
#define COLLISIONS_HPP

#include <glm/vec3.hpp>

namespace collision {

// Estruturas básicas para colisões
struct AABB {
  glm::vec3 min;
  glm::vec3 max;
};

struct Plane {
  glm::vec3 normal;
  float     distance;
};

struct Sphere {
  glm::vec3 center;
  float     radius;
};

struct Line {
  glm::vec3 start;
  glm::vec3 end;
  glm::vec3 direction() const {
    return end - start;
  }
};

// Função para testar colisão entre AABB e plano
bool testAABBPlane(const AABB& aabb, const Plane& plane) {
  // Encontrar o ponto da AABB mais distante na direção da normal do plano
  glm::vec3 positiveVertex;
  glm::vec3 negativeVertex;

  // Seleciona os vértices mais distantes na direção da normal
  if (plane.normal.x >= 0) {
    positiveVertex.x = aabb.max.x;
    negativeVertex.x = aabb.min.x;
  } else {
    positiveVertex.x = aabb.min.x;
    negativeVertex.x = aabb.max.x;
  }

  if (plane.normal.y >= 0) {
    positiveVertex.y = aabb.max.y;
    negativeVertex.y = aabb.min.y;
  } else {
    positiveVertex.y = aabb.min.y;
    negativeVertex.y = aabb.max.y;
  }

  if (plane.normal.z >= 0) {
    positiveVertex.z = aabb.max.z;
    negativeVertex.z = aabb.min.z;
  } else {
    positiveVertex.z = aabb.min.z;
    negativeVertex.z = aabb.max.z;
  }

  // Se o ponto mais distante na direção da normal está atrás do plano, não há colisão
  float positiveDistance = glm::dot(plane.normal, positiveVertex) - plane.distance;
  float negativeDistance = glm::dot(plane.normal, negativeVertex) - plane.distance;

  // Há colisão se um ponto estiver de cada lado do plano
  return (positiveDistance * negativeDistance <= 0);
}

// Função para testar colisão entre AABB e esfera
bool testAABBSphere(const AABB& aabb, const Sphere& sphere) {
  // Encontrar o ponto mais próximo da AABB à esfera
  glm::vec3 closestPoint;

  // Para cada eixo, encontre o ponto mais próximo
  closestPoint.x = std::max(aabb.min.x, std::min(sphere.center.x, aabb.max.x));
  closestPoint.y = std::max(aabb.min.y, std::min(sphere.center.y, aabb.max.y));
  closestPoint.z = std::max(aabb.min.z, std::min(sphere.center.z, aabb.max.z));

  // Calcular a distância ao quadrado entre o centro da esfera e o ponto mais próximo
  float distanceSquared = 0.0f;
  distanceSquared += (closestPoint.x - sphere.center.x) * (closestPoint.x - sphere.center.x);
  distanceSquared += (closestPoint.y - sphere.center.y) * (closestPoint.y - sphere.center.y);
  distanceSquared += (closestPoint.z - sphere.center.z) * (closestPoint.z - sphere.center.z);

  // Há colisão se a distância for menor ou igual ao raio da esfera
  return distanceSquared <= (sphere.radius * sphere.radius);
}

// Função para testar colisão entre AABB e AABB
bool testAABBAABB(const AABB& aabb1, const AABB& aabb2) {
  // Verificar se há sobreposição em todos os eixos
  if (aabb1.max.x < aabb2.min.x || aabb1.min.x > aabb2.max.x)
    return false;
  if (aabb1.max.y < aabb2.min.y || aabb1.min.y > aabb2.max.y)
    return false;
  if (aabb1.max.z < aabb2.min.z || aabb1.min.z > aabb2.max.z)
    return false;

  // Se não houver separação em nenhum eixo, há colisão
  return true;
}

// Função para testar colisão entre AABB e linha
bool testAABBLine(const AABB& aabb, const Line& line) {
  glm::vec3 dir = line.direction();
  glm::vec3 dirInv;

  // Avoid division by zero
  dirInv.x = dir.x != 0 ? 1.0f / dir.x : std::numeric_limits<float>::infinity();
  dirInv.y = dir.y != 0 ? 1.0f / dir.y : std::numeric_limits<float>::infinity();
  dirInv.z = dir.z != 0 ? 1.0f / dir.z : std::numeric_limits<float>::infinity();

  float t1 = (aabb.min.x - line.start.x) * dirInv.x;
  float t2 = (aabb.max.x - line.start.x) * dirInv.x;
  float t3 = (aabb.min.y - line.start.y) * dirInv.y;
  float t4 = (aabb.max.y - line.start.y) * dirInv.y;
  float t5 = (aabb.min.z - line.start.z) * dirInv.z;
  float t6 = (aabb.max.z - line.start.z) * dirInv.z;

  float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
  float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

  // If no overlap or segment entirely before/after the AABB
  if (tmax < 0 || tmin > tmax || tmin > 1.0f || tmax < 0.0f || tmin < 0.0f && tmax < 0.0f)
    return false;

  // Clamp to the segment (0 <= t <= 1)
  if (tmax < 0.0f || tmin > 1.0f)
    return false;

  return true;
}

// Função para testar colisão entre duas esferas
bool testSphereSphere(const Sphere& sphere1, const Sphere& sphere2) {
  float distance = glm::length(sphere1.center - sphere2.center);
  return distance <= (sphere1.radius + sphere2.radius);
}

} // namespace collision

#endif // COLLISIONS_HPP
