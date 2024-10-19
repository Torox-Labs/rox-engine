// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// 
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#pragma once

#include "RoxFrustum.h"
#include <map>
#include <vector>
#include <float.h>

namespace RoxMath
{
	class Quadtree
	{
	public:
		void addObject(const Aabb& box, int idx);
		void removeObject(int idx);

	public:
		bool getObjects(int x, int z, std::vector<int>& result) const;
		bool getObjects(int x, int z, int size_x, int size_z, std::vector<int>& result) const;
		bool getObjects(const Vector3& v, std::vector<int>& result) const;
		bool getObjects(const Aabb& box, std::vector<int>& result) const;
		//bool get_objects(const frustum &f, std::vector<int> &result) const; //ToDo

	public:
		const Aabb& getObjectAabb(int idx) const;

	public:
		Quadtree()
		{
		}

		Quadtree(int x, int z, int size_x, int size_z, int max_level);

	private:
		struct quad;
		int addObject(const quad& obj, int obj_idx, float min_y, float max_y, const quad& leaf, int leaf_idx,
		               int level);
		template <typename s>
		bool getObjects(s search, const quad& leaf, int leaf_idx, std::vector<int>& result) const;

	private:
		struct leaf
		{
			int leaves[2][2];
			std::vector<int> objects;
			float min_y, max_y;

			leaf()
			{
				leaves[0][0] = leaves[0][1] = leaves[1][0] = leaves[1][1] = -1;
				min_y = FLT_MAX;
				max_y = -FLT_MAX;
			}
		};

		struct quad
		{
			int x, z, size_x, size_z;

			quad()
			{
			}

			quad(const Aabb& box);
		};

		quad m_root;
		int m_max_level;
		std::vector<leaf> m_leaves;
		typedef std::map<int, Aabb> objects_map;
		objects_map m_objects;
	};
    
}
