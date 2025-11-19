#include "glm/glm.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

// STL includes
#include <iostream>
#include <cstdarg>
#include <thread>

#include "physics.h"

namespace Layers
{
    static constexpr JPH::ObjectLayer STATIC = 0;
    static constexpr JPH::ObjectLayer DYNAMIC = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}

namespace BroadPhaseLayers
{
    static constexpr JPH::BroadPhaseLayer STATIC(0);
    static constexpr JPH::BroadPhaseLayer DYNAMIC(1);
    static constexpr uint32_t NUM_LAYERS = 2;
}

// BroadPhaseLayerInterface
class BPLayerInterfaceImpl : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        mMapping[Layers::STATIC] = BroadPhaseLayers::STATIC;
        mMapping[Layers::DYNAMIC] = BroadPhaseLayers::DYNAMIC;
    }

    virtual JPH::uint
    GetNumBroadPhaseLayers() const override
    {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    virtual JPH::BroadPhaseLayer
    GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        return mMapping[inLayer];
    }

    virtual const char *
    GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        return "GetBoradPhaseLayerName_NOT_IMPLEMENTED";
    }

private:
    JPH::BroadPhaseLayer mMapping[Layers::NUM_LAYERS];
};

// Object vs BroadPhase filter
class ObjectVsBPLayerFilter : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer, JPH::BroadPhaseLayer inBPLayer) const override
    {
        return true; // All layers collide (simplification)
    }
};

// Object layer pair filter
class ObjectLayerPairFilter : public JPH::ObjectLayerPairFilter
{
public:
    virtual bool ShouldCollide(JPH::ObjectLayer, JPH::ObjectLayer) const override
    {
        return true; // Everything collides
    }
};

struct JoltPhysicsInternal
{
    BPLayerInterfaceImpl bpLayerInterface;
    ObjectVsBPLayerFilter objVsBpFilter;
    ObjectLayerPairFilter objPairFilter;
    JPH::TempAllocatorImpl *mTempAllocator;
    JPH::JobSystemSingleThreaded *mJobSystem;
    JPH::PhysicsSystem *mPhysicsSystem;
    JPH::BodyID mBallID;
    JPH::BodyID mPinID[10];

    Physics pub;
};

static JoltPhysicsInternal g_JoltPhysicsInternal;

// Jolt includes (minimal set)
#ifdef JPH_ENABLE_ASSERTS
// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, uint inLine)
{
    // Print to the TTY
    std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << std::endl;

    // Breakpoint
    return true;
};
#endif // JPH_ENABLE_ASSERTS
// Callback for traces, connect this to your own trace function if you have one
static void TraceImpl(const char *inFMT, ...)
{
    // Format the message
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);

    // Print to the TTY
    std::cout << buffer << std::endl;
}

extern "C" void JPH_AssertFailure(const char *expr, const char *file, uint32_t line, const char *msg)
{
    // Do nothing (safe for Emscripten/release)
    (void)expr;
    (void)file;
    (void)line;
    (void)msg;
}

// === Global state ===

// === Helpers ===
inline JPH::Vec3 ToJolt(const glm::vec3 &v)
{
    return JPH::Vec3(v.x, v.y, v.z);
}

glm::mat4 ToGlm(const JPH::RMat44 &m)
{
    glm::mat4 out;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            out[j][i] = m(i, j);
    return out;
}

// === Public API ===
void Physics::physics_init(
    const float *laneVerts,
    unsigned int laneVertCount,
    const unsigned int *laneIndices,
    unsigned int laneIndexCount,
    glm::vec3 *pinStart,
    glm::vec3 ballStart)
{
    JPH::RegisterDefaultAllocator();
    JPH::Trace = TraceImpl;
    JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    // Allocators
    JPH::TempAllocatorImpl tempAllocator(1024 * 1024); // 1 MB (stack-like, reused per step)
    JPH::JobSystemSingleThreaded jobSystem(JPH::cMaxPhysicsJobs);

    // Physics system
    g_JoltPhysicsInternal.mPhysicsSystem = new JPH::PhysicsSystem();
    g_JoltPhysicsInternal.mPhysicsSystem->Init(
        1024, // max bodies
        0,    // body mutexes (0 = single-threaded)
        1024, // max body pairs
        1024, // max contact constraints
        g_JoltPhysicsInternal.bpLayerInterface,
        g_JoltPhysicsInternal.objVsBpFilter,
        g_JoltPhysicsInternal.objPairFilter);

    JPH::BodyInterface &bodyIface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();

    // === Static lane mesh ===
    JPH::Array<JPH::Float3> verts;
    JPH::Array<JPH::IndexedTriangle> tris;

    verts.resize(laneVertCount / 3);
    for (JPH::uint i = 0; i < laneVertCount / 3; ++i)
    {
        verts[i] = JPH::Float3(laneVerts[i * 3], laneVerts[i * 3 + 1], laneVerts[i * 3 + 2]);
    }

    tris.resize(laneIndexCount / 3);
    for (JPH::uint i = 0; i < laneIndexCount / 3; ++i)
    {
        tris[i] = JPH::IndexedTriangle(
            laneIndices[i * 3], laneIndices[i * 3 + 1], laneIndices[i * 3 + 2], 0);
    }

    JPH::MeshShapeSettings meshSettings(verts, tris);
    JPH::ShapeRefC meshShape = meshSettings.Create().Get();
    JPH::BodyCreationSettings lane(meshShape, JPH::RVec3::sZero(), JPH::Quat::sIdentity(),
                                   JPH::EMotionType::Static, Layers::STATIC);
    bodyIface.CreateAndAddBody(lane, JPH::EActivation::DontActivate);

    // === Ball (sphere) ===
    JPH::SphereShapeSettings ballShape(0.11f);
    JPH::ShapeRefC ball = ballShape.Create().Get();
    JPH::BodyCreationSettings ballBody(ball, ToJolt(ballStart), JPH::Quat::sIdentity(),
                                       JPH::EMotionType::Dynamic, Layers::DYNAMIC);
    ballBody.mRestitution = 0.05f;
    ballBody.mFriction = 0.15f;

    /*
    Ball
    •	mRestitution = 0.05f (bowling balls barely bounce)
    •	mFriction = 0.15f (syn-thetic lane → slippery)
    */
    ballBody.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateMassAndInertia;
    ballBody.mMassPropertiesOverride.mMass = 6.5f; // Middle of legal range
    ballBody.mInertiaMultiplier = 1.0f;            // Realistic rolling

    g_JoltPhysicsInternal.mBallID = bodyIface.CreateAndAddBody(ballBody, JPH::EActivation::Activate);

    // === Pin (cylinder) ===

    g_JoltPhysicsInternal.mTempAllocator = new JPH::TempAllocatorImpl(1024 * 1024);
    g_JoltPhysicsInternal.mJobSystem = new JPH::JobSystemSingleThreaded(JPH::cMaxPhysicsJobs);
    for (int i = 0; i < 10; i++)
    {
        JPH::CylinderShapeSettings pinShape(0.19f, 0.06f); // half-height, radius
        JPH::ShapeRefC pin = pinShape.Create().Get();
        JPH::BodyCreationSettings pinBody(pin, ToJolt(pinStart[i]), JPH::Quat::sIdentity(),
                                          JPH::EMotionType::Dynamic, Layers::DYNAMIC);
        /*
        Pins
            •	mRestitution = 0.1–0.2f
            •	mFriction = 0.3–0.5f (your value is fine)
        */
        pinBody.mRestitution = 0.2f;
        pinBody.mFriction = 0.5f;
        pinBody.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateMassAndInertia;
        pinBody.mMassPropertiesOverride.mMass = 1.53f; // Standard pin mass
        pinBody.mInertiaMultiplier = 1.0f;
        g_JoltPhysicsInternal.mPinID[i] = bodyIface.CreateAndAddBody(pinBody, JPH::EActivation::Activate);
    }
}

void Physics::physics_step(float deltaSeconds)
{
    g_JoltPhysicsInternal.mPhysicsSystem->Update(
        deltaSeconds,
        1,
        g_JoltPhysicsInternal.mTempAllocator,
        g_JoltPhysicsInternal.mJobSystem);

    JPH::BodyInterface &bodyIface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();
    this->mBallMatrix = ToGlm(bodyIface.GetWorldTransform(g_JoltPhysicsInternal.mBallID));

    for (int i = 0; i < 10; i++)
    {
        this->mPinMatrix[i] = ToGlm(bodyIface.GetWorldTransform(g_JoltPhysicsInternal.mPinID[i]));
    }
}

const glm::mat4 &Physics::physics_get_ball_matrix()
{
    return this->mBallMatrix;
}

const glm::mat4 &Physics::physics_get_pin_matrix(int i)
{
    return this->mPinMatrix[i];
}

void Physics::physics_reset(glm::vec3 *newPinPos, glm::vec3 newBallPos)
{
    JPH::BodyInterface &bodyIface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();

    bodyIface.SetPositionAndRotation(g_JoltPhysicsInternal.mBallID, ToJolt(newBallPos), JPH::Quat::sIdentity(), JPH::EActivation::Activate);

    bodyIface.SetLinearVelocity(g_JoltPhysicsInternal.mBallID, JPH::Vec3::sZero());
    bodyIface.SetAngularVelocity(g_JoltPhysicsInternal.mBallID, JPH::Vec3::sZero());
    this->mBallMatrix = ToGlm(bodyIface.GetWorldTransform(g_JoltPhysicsInternal.mBallID));

    for (int i = 0; i < 10; i++)
    {
        bodyIface.SetPositionAndRotation(g_JoltPhysicsInternal.mPinID[i], ToJolt(newPinPos[i]), JPH::Quat::sIdentity(), JPH::EActivation::Activate);
        bodyIface.SetLinearVelocity(g_JoltPhysicsInternal.mPinID[i], JPH::Vec3::sZero());
        bodyIface.SetAngularVelocity(g_JoltPhysicsInternal.mPinID[i], JPH::Vec3::sZero());
        this->mPinMatrix[i] = ToGlm(bodyIface.GetWorldTransform(g_JoltPhysicsInternal.mPinID[i]));
    }
}