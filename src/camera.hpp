#include <cmath>
#include <cstdio>
#include <iostream>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/string_cast.hpp>
#include "glm/ext/vector_float4.hpp"
#include "glm/geometric.hpp"
#include "matrices.h"
#include "collisions.hpp"

class Camera {
  public:
  bool              UsePerspectiveProjection;
  virtual glm::mat4 getMatrixView()                     = 0;
  virtual glm::mat4 getMatrixProjection()               = 0;
  virtual float     getTheta()                          = 0;
  virtual void      setTheta(float theta)               = 0;
  virtual float     getPhi()                            = 0;
  virtual void      setPhi(float phi)                   = 0;
  virtual glm::vec4 getPosition()                       = 0;
  virtual void      setPosition(glm::vec4 position)     = 0;
  virtual void      setUsePerspectiveProjection(bool b) = 0;
  virtual float     getScreenRatio()                    = 0;
  virtual void      setScreenRatio(float screenRatio)   = 0;
  virtual void      setDistance(float Distance)         = 0;
  virtual float     getDistance()                       = 0;
  virtual void      MoveForward(float deltaTime)        = 0;
  virtual void      MoveBackward(float deltaTime)       = 0;
  virtual void      MoveLeft(float deltaTime)           = 0;
  virtual void      MoveRight(float deltaTime)          = 0;
  virtual void      MoveUpwards(float deltaTime)        = 0;
  virtual void      MoveDownwards(float deltaTime)      = 0;
  virtual void      setLookAt(glm::vec4 lookAt)         = 0;
  virtual glm::vec4 getViewVector()                     = 0;
  virtual glm::vec4 getUpVector()                       = 0;


  private:
  float Radius;
};

class SphericCamera : public Camera {
  private:
  float Radius;
  float Speed;
  float Theta;    // Ângulo no plano ZX em relação ao eixo Z
  float Phi;      // Ângulo em relação ao eixo Y
  float Distance; // Distância da câmera para a origem

  glm::vec4 Position;
  glm::vec4 LookAt;
  glm::vec4 ViewVector;
  glm::vec4 UpVector;

  float NearPlane;
  float FarPlane;
  float FieldOfView;
  float ScreenRatio;

  void updatePosition() {
    float x  = Distance * cos(Phi) * sin(Theta);
    float y  = Distance * sin(Phi);
    float z  = Distance * cos(Phi) * cos(Theta);
    Position = LookAt + glm::vec4(x, y, z, 0.0f);
    updateViewVector();
  }

  void updateViewVector() {
    ViewVector = LookAt - Position;
  }

  public:
  bool UsePerspectiveProjection;

  SphericCamera(float     speed,
                float     theta,
                float     phi,
                float     distance,
                glm::vec4 lookAt,
                glm::vec4 upVector,
                float     nearPlane,
                float     farPlane,
                float     fieldOfView,
                float     screenRatio,
                bool      usePerspectiveProjection) {
    Radius   = 0.1f;
    Speed    = speed;
    Theta    = theta;
    Phi      = phi;
    Distance = distance;

    NearPlane                = nearPlane;
    FarPlane                 = farPlane;
    FieldOfView              = fieldOfView;
    ScreenRatio              = screenRatio;
    UsePerspectiveProjection = usePerspectiveProjection;

    LookAt   = lookAt;
    UpVector = upVector;


    updatePosition();
  }


  glm::vec4 getPosition() {
    return Position;
  }

  glm::mat4 getMatrixView() {
    return Matrix_Camera_View(Position, ViewVector, UpVector);
  }

  glm::mat4 getMatrixProjection() {
    if (UsePerspectiveProjection)
      return Matrix_Perspective(FieldOfView, ScreenRatio, NearPlane, FarPlane);
    else {
      float t = 1.5f * Distance / 2.5f;
      float b = -t;
      float r = t * ScreenRatio;
      float l = -r;
      return Matrix_Orthographic(l, r, b, t, NearPlane, FarPlane);
    }
  }

  float getTheta() {
    return Theta;
  }

  void setTheta(float theta) {
    Theta = theta;
    updatePosition();
    // printf("New Theta: %f\n", Theta);
  }

  float getPhi() {
    return Phi;
  }

  void setPhi(float phi) {
    Phi = phi;

    // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
    float phimax = 3.141592f / 2;
    float phimin = -phimax;

    if (Phi > phimax)
      Phi = phimax;

    if (Phi < phimin)
      Phi = phimin;

    updatePosition();
    // printf("New Phi: %f\n", Phi);
  }

  float getDistance() {
    return Distance;
  }

  void setDistance(float distance) {
    Distance = distance;

    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    const float maxDistance     = 5.0f;

    if (Distance < verysmallnumber)
      Distance = verysmallnumber;

    if (Distance > maxDistance)
      Distance = maxDistance;

    updatePosition();
  }

  void setUsePerspectiveProjection(bool b) {
    UsePerspectiveProjection = b;
  }

  float getScreenRatio() {
    return ScreenRatio;
  }

  void setScreenRatio(float screenRatio) {
    ScreenRatio = screenRatio;
  }

  void MoveForward(float deltaTime) {
    // Para câmera esférica, não alteramos a distância durante movimento
    (void) deltaTime;
  }

  void MoveBackward(float deltaTime) {
    // Para câmera esférica, não alteramos a distância durante movimento
    (void) deltaTime;
  }

  void MoveLeft(float deltaTime) {
    (void) deltaTime;
  }
  void MoveRight(float deltaTime) {
    (void) deltaTime;
  }
  void MoveUpwards(float deltaTime) {
    (void) deltaTime;
  }
  void MoveDownwards(float deltaTime) {
    (void) deltaTime;
  }

  void setPosition(glm::vec4 position) {
    Position = position;
    Distance = glm::length(Position - LookAt);

    // Recalcular Theta e Phi com base na nova posição
    glm::vec3 dir = glm::vec3(Position.x - LookAt.x, Position.y - LookAt.y, Position.z - LookAt.z);
    Phi           = asin(dir.y / Distance);
    Theta         = atan2(dir.x, dir.z);

    updateViewVector();
  }

  void setLookAt(glm::vec4 lookAt) {
    LookAt = lookAt;
    updatePosition();
  }

  glm::vec4 getViewVector() {
    return ViewVector;
  }

  glm::vec4 getUpVector() {
    return UpVector;
  }
};


class FreeCamera : public Camera {
  private:
  float Radius;
  float Speed;
  float Theta; // Ângulo no plano ZX em relação ao eixo Z
  float Phi;   // Ângulo em relação ao eixo Y

  glm::vec4 Position;
  glm::vec4 ViewVector;
  glm::vec4 UpVector;

  glm::vec4 u;
  glm::vec4 v;
  glm::vec4 w;

  float NearPlane;
  float FarPlane;
  float FieldOfView;
  float ScreenRatio;

  void updateViewVector() {
    ViewVector.x = cos(Phi) * cos(Theta);
    ViewVector.y = sin(Phi);
    ViewVector.z = cos(Phi) * sin(Theta);
    ViewVector.w = 0.0f;
    ViewVector   = glm::normalize(ViewVector);
    updateUVW();
  }

  void updateUVW() {
    w = -glm::normalize(ViewVector);
    u = glm::normalize(crossproduct(UpVector, w));
    v = crossproduct(w, u);
  }

  public:
  bool UsePerspectiveProjection;

  FreeCamera(float     speed,
             float     theta,
             float     phi,
             glm::vec4 position,
             glm::vec4 upVector,
             float     nearPlane,
             float     farPlane,
             float     fieldOfView,
             float     screenRatio,
             bool      usePerspectiveProjection) {
    Radius                   = 0.1f;
    Speed                    = speed;
    Theta                    = theta;
    Phi                      = phi;
    Position                 = position;
    UpVector                 = upVector;
    NearPlane                = nearPlane;
    FarPlane                 = farPlane;
    FieldOfView              = fieldOfView;
    ScreenRatio              = screenRatio;
    UsePerspectiveProjection = usePerspectiveProjection;

    updateViewVector();
  }

  glm::vec4 getPosition() {
    return Position;
  }

  glm::mat4 getMatrixView() {
    return Matrix_Camera_View(Position, ViewVector, UpVector);
  }

  glm::mat4 getMatrixProjection() {
    if (UsePerspectiveProjection)
      return Matrix_Perspective(FieldOfView, ScreenRatio, NearPlane, FarPlane);
    else {
      float t = 1.5f * glm::length(Position) / 2.5f;
      float b = -t;
      float r = t * ScreenRatio;
      float l = -r;
      return Matrix_Orthographic(l, r, b, t, NearPlane, FarPlane);
    }
  }

  void MoveForward(float deltaTime) {
    glm::vec4 forward = glm::normalize(glm::vec4(ViewVector.x, 0.0, ViewVector.z, 0.0));
    Position += forward * Speed * deltaTime;
  }

  void MoveBackward(float deltaTime) {
    glm::vec4 forward = glm::normalize(glm::vec4(ViewVector.x, 0.0, ViewVector.z, 0.0));
    Position -= forward * Speed * deltaTime;
  }

  void MoveLeft(float deltaTime) {
    Position -= u * Speed * deltaTime;
  }

  void MoveRight(float deltaTime) {
    Position += glm::normalize(crossproduct(ViewVector, UpVector)) * Speed * deltaTime;
  }

  void MoveUpwards(float deltaTime) {
    Position -= glm::normalize(crossproduct(ViewVector, UpVector)) * Speed * deltaTime;
  }

  void MoveDownwards(float deltaTime) {
    Position -= UpVector * Speed * deltaTime;
  }

  void setPosition(glm::vec4 position) {
    Position = position;
  }


  void setTheta(float theta) {
    Theta = theta;
    updateViewVector();
  }

  float getTheta() {
    return Theta;
  }

  void setPhi(float phi) {
    Phi = phi;

    float phimax = 3.141592f / 2;
    float phimin = -phimax;

    if (Phi > phimax)
      Phi = phimax;

    if (Phi < phimin)
      Phi = phimin;

    updateViewVector();
  }

  float getPhi() {
    return Phi;
  }

  void setSpeed(float speed) {
    Speed = speed;
  }

  float getSpeed() {
    return Speed;
  }

  void setDistance(float distance) {
    (void) distance;
  }

  float getDistance() {
    return glm::length(Position);
  }

  float getScreenRatio() {
    return ScreenRatio;
  }

  void setScreenRatio(float screenRatio) {
    ScreenRatio = screenRatio;
  }

  void setUsePerspectiveProjection(bool b) {
    UsePerspectiveProjection = b;
  }

  void setLookAt(glm::vec4 lookAt) {
    (void) lookAt;
  }

  glm::vec4 getViewVector() {
    return ViewVector;
  }

  glm::vec4 getUpVector() {
    return UpVector;
  }
};
