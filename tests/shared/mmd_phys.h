//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "btBulletDynamicsCommon.h"
#include "mmd_mesh.h"

class mmd_phys_controller
{
public:
    void init(mmd_mesh *mesh,btDiscreteDynamicsWorld *world);
    void release();

    void update_pre();
    void update_post();
    
    void reset();

    void skip_bone(const char* name,bool skip);
    void reset_bones_skip();

public:
    mmd_phys_controller(): m_mesh(0),m_world(0) {}

private:
    mmd_mesh *m_mesh;
    btDiscreteDynamicsWorld *m_world;

    struct phys_body
    {
        int bone_idx;
        nya_math::vec3 bone_offset;
        btMotionState *motion_state;
        btCollisionShape *col_shape;
		btRigidBody *rigid_body;
        btTransform bone_tr;
        btTransform bone_tr_inv;
        bool kinematic;
        bool skip;

        phys_body(): bone_idx(-1),motion_state(0),col_shape(0),rigid_body(0),kinematic(false),skip(false) {}
    };

    std::vector<phys_body> m_phys_bodies;

    struct phys_joint
    {
        btGeneric6DofSpringConstraint *constraint;

        phys_joint(): constraint(0) {}
    };

    std::vector<phys_joint> m_phys_joints;

private:
    class skeleton_helper
    {
    public:
        void init_bone(int idx,const nya_render::skeleton &sk);
        void set_transform(const nya_math::vec3 &pos,const nya_math::quat &rot,const nya_math::vec3 &scale);
        void set_bone(int idx,const nya_math::vec3 &pos,const nya_math::quat &rot);
        nya_math::vec3 get_bone_local_pos(int idx,const nya_render::skeleton &sk) const;
        nya_math::quat get_bone_local_rot(int idx,const nya_render::skeleton &sk) const;

    private:
        nya_math::vec3 get_bone_pos(int idx,const nya_render::skeleton &sk) const;
        nya_math::quat get_bone_rot(int idx,const nya_render::skeleton &sk) const;

    private:
        std::vector<int> m_parents;
        std::vector<int> m_map;

        struct bone { nya_math::vec3 offset,pos; nya_math::quat rot; };
        std::vector<bone> m_bones;
        nya_scene::transform m_tr;
    };

    skeleton_helper m_skeleton_helper;
};

class mmd_phys_world
{
public:
    void init();
    void update(int dt);
    void release();

    void set_gravity(float g);
    void set_floor(float y,bool enable);

public:
    btDiscreteDynamicsWorld *get_world();

public:
    mmd_phys_world(): m_col_conf(0),m_dispatcher(0),m_broadphase(0),m_solver(0),m_world(0),m_ground(0),m_dt_acc(0) {}

private:
	btDefaultCollisionConfiguration *m_col_conf;
	btCollisionDispatcher *m_dispatcher;
	btBroadphaseInterface *m_broadphase;
	btSequentialImpulseConstraintSolver *m_solver;
	btDiscreteDynamicsWorld *m_world;
    btRigidBody *m_ground;
    btMotionState *m_ground_state;
    btCollisionShape *m_ground_shape;
    int m_dt_acc;
};
