// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxProxy.h"
#include "RoxMath/RoxMatrix.h"
#include "RoxMath/RoxVector.h"
#include "RoxMath/RoxQuaternion.h"
#include "RoxMath/RoxFrustum.h"
#include <cstring>

namespace RoxScene
{

    class RoxCamera
    {
    public:
        // Projection setup
        void setProj(RoxMath::AngleDeg fov, float aspect, float near, float far);
        void setProj(float left, float right, float bottom, float top, float near, float far);
        void setProj(const RoxMath::Matrix4& mat);

        // Position and rotation setup
        void setPos(float x, float y, float z) { setPos(RoxMath::Vector3(x, y, z)); }
        void setPos(const RoxMath::Vector3& pos);
        void setRot(RoxMath::AngleDeg yaw, RoxMath::AngleDeg pitch, RoxMath::AngleDeg roll);
        void setRot(const RoxMath::Quaternion& rot);
        void setRot(const RoxMath::Vector3& direction);

    public:
        // Getters for matrices and RoxFrustum
        const RoxMath::Matrix4& getProjMatrix() const { return mProj; }
        const RoxMath::Matrix4& getViewMatrix() const;

        const RoxMath::RoxFrustum& getFrustum() const;

    public:
        // Getters for position, rotation, and direction
        const RoxMath::Vector3& getPos() const { return mPos; }
        const RoxMath::Quaternion& getRot() const { return mRot; }
        const RoxMath::Vector3 getDir() const;

    public:
        // Constructors
        RoxCamera() : mRecalcView(true), mRecalcFrustum(true) {}
        explicit RoxCamera(const RoxCamera& other) = default;

    private:
        // Projection and view matrices
        RoxMath::Matrix4 mProj;
        mutable RoxMath::Matrix4 mView;

        // Position and rotation
        RoxMath::Vector3 mPos;
        RoxMath::Quaternion mRot;

        // Frustum for culling
        mutable RoxMath::RoxFrustum mFrustum;

        // Flags to indicate if recalculations are needed
        mutable bool mRecalcView;
        mutable bool mRecalcFrustum;
    };

    // Renamed typedef to use the updated RoxProxy and RoxCamera classes
    using RoxCameraProxy = RoxProxy<RoxCamera>;

    // Function declarations for managing the active camera
    void setCamera(const RoxCameraProxy& cam);
    RoxCamera& getCamera();
    RoxCameraProxy& getCameraProxy();

} // namespace RoxScene
