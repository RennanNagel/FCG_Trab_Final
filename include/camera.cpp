#include "matrices.h"
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <cmath>

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

    bool UsePerspectiveProjection;

public:
    SphericCamera(float theta, float phi, float distance, glm::vec4 lookAt, glm::vec4 upVector, float nearPlane, float farPlane, float fieldOfView,
                  float screenRatio, bool usePerspectiveProjection) {
        Theta    = theta;
        Phi      = phi;
        Distance = distance;

        PositionY = Distance * sin(Phi);
        PositionZ = Distance * sin(Phi) * cos(Theta);
        PositionY = Distance * cos(Phi) * sin(Theta);

        Position   = glm::vec4(PositionX, PositionY, PositionZ, 1.0f);
        LookAt     = lookAt;
        ViewVector = LookAt - Position;
        UpVector   = upVector;

        NearPlane                = nearPlane;
        FarPlane                 = farPlane;
        FieldOfView              = fieldOfView;
        ScreenRatio              = screenRatio;
        UsePerspectiveProjection = usePerspectiveProjection;
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
};
