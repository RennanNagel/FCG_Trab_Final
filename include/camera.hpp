#include <cmath>
#include <cstdio>
#include <iostream>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/string_cast.hpp>
#include "matrices.h"

class SphericCamera {
  private:
  float Theta;    // Ângulo no plano ZX em relação ao eixo Z
  float Phi;      // Ângulo em relação ao eixo Y
  float Distance; // Distância da câmera para a origem

  float PositionX;
  float PositionY;
  float PositionZ;

  glm::vec4 Position;
  glm::vec4 LookAt;
  glm::vec4 ViewVector;
  glm::vec4 UpVector;

  float NearPlane;
  float FarPlane;
  float FieldOfView;
  float ScreenRatio;

  void updatePosition() {
    PositionY = Distance * sin(Phi);
    PositionZ = Distance * cos(Phi) * cos(Theta);
    PositionX = Distance * cos(Phi) * sin(Theta);
    Position  = glm::vec4(PositionX, PositionY, PositionZ, 1.0f);
    updateViewVector();
  }

  void updateViewVector() {
    ViewVector = LookAt - Position;
  }

  public:
  bool UsePerspectiveProjection;

  SphericCamera(float     theta,
                float     phi,
                float     distance,
                glm::vec4 lookAt,
                glm::vec4 upVector,
                float     nearPlane,
                float     farPlane,
                float     fieldOfView,
                float     screenRatio,
                bool      usePerspectiveProjection) {
    Theta    = theta;
    Phi      = phi;
    Distance = distance;

    LookAt   = lookAt;
    UpVector = upVector;

    NearPlane                = nearPlane;
    FarPlane                 = farPlane;
    FieldOfView              = fieldOfView;
    ScreenRatio              = screenRatio;
    UsePerspectiveProjection = usePerspectiveProjection;

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

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (Distance < verysmallnumber)
      Distance = verysmallnumber;

    updatePosition();
  }

  void setUsePerspectiveProjection(bool b) {
    UsePerspectiveProjection = b;
  }

  void setScreenRatio(float screenRatio) {
    ScreenRatio = screenRatio;
  }
};


class FreeCamera {
  private:
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

  void MoveForward() {
    Position -= w * Speed;
  }

  void MoveBackward() {
    Position += w * Speed;
  }

  void MoveLeft() {
    Position -= u * Speed;
  }

  void MoveRight() {
    Position += u * Speed;
  }

  void MoveUpwards() {
    Position += UpVector * Speed;
  }

  void MoveDownwards() {
    Position -= UpVector * Speed;
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

  void setDistance() {}

  void setScreenRatio(float screenRatio) {
    ScreenRatio = screenRatio;
  }
};
