#pragma once
#include <cstdint>
#include <cmath>

struct FVector {
    double x = 0, y = 0, z = 0;

    FVector() = default;
    FVector(double x, double y, double z) : x(x), y(y), z(z) {}
};

struct FRotator {
    double pitch = 0, yaw = 0, roll = 0;
};

struct FVector2D {
    float x = 0, y = 0;
    FVector2D() = default;
    FVector2D(float x, float y) : x(x), y(y) {}
};

struct FMatrix {
    double m[4][4] = {};

    FMatrix operator*(const FMatrix& rhs) const {
        FMatrix result{};
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                for (int k = 0; k < 4; ++k)
                    result.m[i][j] += m[i][k] * rhs.m[k][j];
        return result;
    }
};

// Build a view-projection matrix from camera data
FMatrix BuildViewProjectionMatrix(const FVector& location, const FRotator& rotation, float fov,
    float screenWidth, float screenHeight);

// Project a 3D world position to 2D screen coordinates
// Returns true if the point is in front of the camera
bool WorldToScreen(const FVector& worldPos, const FMatrix& viewMatrix, const FMatrix& projMatrix,
    const FVector2D& screenSize, FVector2D& screenOut);

// Convenience: build both matrices and project in one call
bool WorldToScreen(const FVector& worldPos, const FVector& camLoc, const FRotator& camRot,
    float fov, float screenWidth, float screenHeight, FVector2D& screenOut);
