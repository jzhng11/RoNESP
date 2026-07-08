#include "w2s.h"
#include <numbers>

static inline double DegToRad(double deg) {
    return deg * std::numbers::pi / 180.0;
}

// UE FRotator to camera axes. UE: +X forward, +Y right, +Z up.
// pitch=+Y axis (pos=look down), yaw=+Z (clockwise from +X), roll=+X (pos=tilt right)
static void BuildCameraAxes(const FRotator& rotation, FVector& right, FVector& up, FVector& forward) {
    double sp = sin(DegToRad(rotation.pitch));
    double cp = cos(DegToRad(rotation.pitch));
    double sy = sin(DegToRad(rotation.yaw));
    double cy = cos(DegToRad(rotation.yaw));
    double sr = sin(DegToRad(rotation.roll));
    double cr = cos(DegToRad(rotation.roll));

    forward.x = cp * cy;
    forward.y = cp * sy;
    forward.z = sp;

    right.x = cy * sp * sr - cr * sy;
    right.y = sy * sp * sr + cr * cy;
    right.z = -cp * sr;

    up.x = -cr * cy * sp - sr * sy;
    up.y = -cr * sy * sp + sr * cy;
    up.z = cp * cr;
}

bool WorldToScreen(const FVector& worldPos, const FVector& camLoc, const FRotator& camRot,
    float fov, float screenWidth, float screenHeight, FVector2D& screenOut) {

    FVector right, up, forward;
    BuildCameraAxes(camRot, right, up, forward);

    // Delta from camera to world point
    double dx = worldPos.x - camLoc.x;
    double dy = worldPos.y - camLoc.y;
    double dz = worldPos.z - camLoc.z;

    // View-space coordinates
    double vx = dx * right.x + dy * right.y + dz * right.z;   // camera-relative right
    double vy = dx * up.x    + dy * up.y    + dz * up.z;      // camera-relative up
    double vz = dx * forward.x + dy * forward.y + dz * forward.z; // camera-relative depth

    if (vz < 0.01)
        return false;

    // Projection
    double fovRad = DegToRad(fov);
    double tanHalfFov = tan(fovRad * 0.5);
    double aspect = screenWidth / screenHeight;

    double ndcX = (vx / vz) / tanHalfFov / aspect;
    double ndcY = (vy / vz) / tanHalfFov;

    screenOut.x = static_cast<float>(( ndcX * 0.5 + 0.5) * screenWidth);
    screenOut.y = static_cast<float>((-ndcY * 0.5 + 0.5) * screenHeight);

    return true;
}

static FVector2D screenSizeDummy;
FMatrix BuildViewProjectionMatrix(const FVector& location, const FRotator& rotation, float fov,
    float screenWidth, float screenHeight) {
    return FMatrix{}; // deprecated - kept for compilation
}

bool WorldToScreen(const FVector& worldPos, const FMatrix& viewMatrix, const FMatrix& projMatrix,
    const FVector2D& screenSize, FVector2D& screenOut) {
    return false; // deprecated - kept for compilation
}
