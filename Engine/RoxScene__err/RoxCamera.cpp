//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxCamera.h"
#include "RoxRender/RoxRender.h"
#include "RoxMath/RoxQuaternion.h"
#include "RoxMath/RoxConstants.h"
#include "RoxRender/RoxTransform.h"
#include "RoxMemory/RoxInvalidObject.h"
#include <cmath>

namespace RoxScene
{

    namespace { RoxCameraProxy activeCamera = RoxCameraProxy(RoxCamera()); }

    void RoxCamera::setProj(RoxMath::AngleDeg fov, float aspect, float near, float far)
    {
        mProj.identity();
        mProj.perspective(fov, aspect, near, far);
        if (this == activeCamera.operator->())
            RoxRender::setProjectionMatrix(mProj);

        mRecalcFrustum = true;
    }

    void RoxCamera::setProj(float left, float right, float bottom, float top, float near, float far)
    {
        mProj.identity();
        mProj.ortho(left, right, bottom, top, near, far);
        if (this == activeCamera.operator->())
            RoxRender::setProjectionMatrix(mProj);

        mRecalcFrustum = true;
    }

    void RoxCamera::setProj(const RoxMath::Matrix4& mat)
    {
        mProj = mat;
        if (this == activeCamera.operator->())
            RoxRender::setProjectionMatrix(mProj);

        mRecalcFrustum = true;
    }

    void RoxCamera::setPos(const RoxMath::Vector3& pos)
    {
        mPos = pos;
        mRecalcView = true;
        mRecalcFrustum = true;
    }

    void RoxCamera::setRot(const RoxMath::Quaternion& rot)
    {
        mRot = rot;
        mRecalcView = true;
        mRecalcFrustum = true;
    }

    void RoxCamera::setRot(RoxMath::AngleDeg yaw, RoxMath::AngleDeg pitch, RoxMath::AngleDeg roll)
    {
        setRot(RoxMath::Quaternion(pitch, yaw, roll));
    }

    void RoxCamera::setRot(const RoxMath::Vector3& dir)
    {
        const float eps = 1.0e-6f;
        const RoxMath::Vector3 v = RoxMath::Vector3::normalize(dir);
        const float xzSqDist = v.x * v.x + v.z * v.z;

        const float newYaw = (xzSqDist > eps * eps) ? (std::atan2(v.x, v.z) + RoxMath::Constants::pi) : mRot.getEuler().y;
        const float newPitch = (std::fabs(v.y) > eps) ? (std::atan2(v.y, std::sqrt(xzSqDist))) : 0.0f;
        setRot(RoxMath::Quaternion(newPitch, newYaw, 0.0f));
    }

    const RoxMath::Matrix4& RoxCamera::getViewMatrix() const
    {
        if (mRecalcView)
        {
            mRecalcView = false;
            mView = RoxMath::Matrix4(mRot);
            mView.transpose();
            mView.translate(-mPos);
        }

        return mView;
    }

    const RoxMath::RoxFrustum& RoxCamera::getFrustum() const
    {
        if (mRecalcFrustum)
        {
            mRecalcFrustum = false;

            if (RoxRender::RoxTransform::get().hasOrientationMatrix())
                mFrustum = RoxMath::RoxFrustum(getViewMatrix() * getProjMatrix() * RoxRender::RoxTransform::get().getOrientationMatrix());
            else
                mFrustum = RoxMath::RoxFrustum(getViewMatrix() * getProjMatrix());
        }

        return mFrustum;
    }

    const RoxMath::Vector3 RoxCamera::getDir() const
    {
        return mRot.rotate(RoxMath::Vector3::forward());
    }

    void setCamera(const RoxCameraProxy& cam)
    {
        activeCamera = cam;
        if (cam.isValid())
            RoxRender::setProjectionMatrix(cam->getProjMatrix());
    }

    RoxCameraProxy& getCameraProxy() { return activeCamera; }

    RoxCamera& getCamera()
    {
        if (!activeCamera.isValid())
            return RoxMemory::invalidObject<RoxCamera>();

        return activeCamera.get();
    }

} // namespace RoxScene
