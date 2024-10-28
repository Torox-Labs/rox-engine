//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "proxy.h"
#include "RoxMath/RoxMatrix.h"
#include "RoxMath/RoxVector.h"
#include "RoxMath/RoxQuaternion.h"
#include "RoxMath/RoxFrustum.h"

namespace RoxScene
{
	class camera
	{
	public:
		// Projection and Transformation Methods
		void set_proj(RoxMath::AngleDeg fov, float aspect, float near, float far);
		void set_proj(float left, float right, float bottom, float top, float near, float far);
		void set_proj(const RoxMath::Matrix4& mat);

		void set_pos(float x, float y, float z) { set_pos(RoxMath::Vector3(x, y, z)); }
		void set_pos(const RoxMath::Vector3& pos);
		void set_rot(RoxMath::AngleDeg yaw, RoxMath::AngleDeg pitch, RoxMath::AngleDeg roll);
		void set_rot(const RoxMath::Quaternion& rot);
		void set_rot(const RoxMath::Vector3& direction);

	public:
		// Getters for Matrices and Position
		const RoxMath::Matrix4& get_proj_matrix() const { return m_proj; }
		const RoxMath::Matrix4& get_view_matrix() const;

		const RoxMath::RoxFrustum& get_frustum() const;

	public:
		const RoxMath::Vector3& get_pos() const { return m_pos; }
		const RoxMath::Quaternion& get_rot() const { return m_rot; }
		const RoxMath::Vector3 get_dir() const;

	public:
		camera(): m_recalc_view(true), m_recalc_frustum(true)
		{
		}

	private:
		// Projection and view matrices
		RoxMath::Matrix4 m_proj;
		mutable RoxMath::Matrix4 m_view;

		// Position and rotation
		RoxMath::Vector3 m_pos;
		RoxMath::Quaternion m_rot;

		// Frustum
		mutable RoxMath::RoxFrustum m_frustum;

		// Flags for recalculation
		mutable bool m_recalc_view;
		mutable bool m_recalc_frustum;
	};

	typedef proxy<camera> camera_proxy;

	// Global Camera Manager
	void set_camera(const camera_proxy& cam);
	camera& get_camera();
	camera_proxy& get_camera_proxy();

}
