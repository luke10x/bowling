#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>

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
    inline static constexpr float FIXED_STEP = 0.005f; // 5 ms

    BPLayerInterfaceImpl bpLayerInterface;
    ObjectVsBPLayerFilter objVsBpFilter;
    ObjectLayerPairFilter objPairFilter;
    JPH::TempAllocatorImpl *mTempAllocator;
    JPH::JobSystemSingleThreaded *mJobSystem;
    JPH::PhysicsSystem *mPhysicsSystem;
    JPH::BodyID mBallID;
    JPH::BodyID mPinID[10];
    bool ballPhysicsActive;
    glm::vec3 lastManualPos;
    glm::vec3 manualVelocity;
    float mPosDtLoan = 0.0f;
    float mAccumulator = 0.0f;
    Physics pub;
};

static JoltPhysicsInternal g_JoltPhysicsInternal;

// Jolt includes (minimal set)
#ifdef JPH_ENABLE_ASSERTS
// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, JPH::uint inLine)
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

    g_JoltPhysicsInternal.ballPhysicsActive = true; // start with physics enabled

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

    lane.mFriction = 0.35f;    // good start for bowling lane
    lane.mRestitution = 0.01f; // very low bounce
    /*
     *
     */

    bodyIface.CreateAndAddBody(lane, JPH::EActivation::DontActivate);

    // === Ball (sphere) ===
    JPH::SphereShapeSettings ballShape(0.11f);
    JPH::ShapeRefC ball = ballShape.Create().Get();
    JPH::BodyCreationSettings ballBody(ball, ToJolt(ballStart), JPH::Quat::sIdentity(),
                                       JPH::EMotionType::Dynamic, Layers::DYNAMIC);
    ballBody.mRestitution = 0.05f;
    ballBody.mFriction = 0.08f;

    /*
    Ball
    •	mRestitution = 0.05f (bowling balls barely bounce)
    •	mFriction = 0.15f (syn-thetic lane → slippery)
    */
    ballBody.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateMassAndInertia;
    ballBody.mMassPropertiesOverride.mMass = 7.25f; // Middle of legal range 6 - 7.26
    ballBody.mInertiaMultiplier = 1.0f;             // Realistic rolling

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
    g_JoltPhysicsInternal.mAccumulator += deltaSeconds;

    // Run as many fixed 10ms physics steps as needed
    while (g_JoltPhysicsInternal.mAccumulator >= g_JoltPhysicsInternal.FIXED_STEP)
    {
        g_JoltPhysicsInternal.mPhysicsSystem->Update(
            g_JoltPhysicsInternal.FIXED_STEP,
            1, // still *1*; this is not number of steps!
            g_JoltPhysicsInternal.mTempAllocator,
            g_JoltPhysicsInternal.mJobSystem);

        this->apply_lane_pushback(
            -8.0f, // Operational pead
            6.0f, // width of operatiion 
            8.0f // max strength in Newtons
        );

        g_JoltPhysicsInternal.mAccumulator -= g_JoltPhysicsInternal.FIXED_STEP;
    }

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

void Physics::set_manual_ball_position(const glm::vec3 &pos, float dt)
{
    g_JoltPhysicsInternal.ballPhysicsActive = false;
    if (glm::length(pos - g_JoltPhysicsInternal.lastManualPos) == 0.0f) {
        g_JoltPhysicsInternal.mPosDtLoan += dt; // Emscripten on Desktop chrome sends same position in every other move event
                                                // Loaning till next frame normally it moves with double speed on the next frame
    } else {
        dt += g_JoltPhysicsInternal.mPosDtLoan;
        g_JoltPhysicsInternal.mPosDtLoan = 0.0f;
        g_JoltPhysicsInternal.manualVelocity = (pos - g_JoltPhysicsInternal.lastManualPos) / dt;
        g_JoltPhysicsInternal.lastManualPos = pos;
    }

    JPH::BodyInterface &bodyIface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();

    // Disable gravity and make it kinematic
    bodyIface.SetMotionType(g_JoltPhysicsInternal.mBallID,
                            JPH::EMotionType::Kinematic,
                            JPH::EActivation::DontActivate);

    // Reset velocities
    bodyIface.SetLinearVelocity(g_JoltPhysicsInternal.mBallID, JPH::Vec3::sZero());
    bodyIface.SetAngularVelocity(g_JoltPhysicsInternal.mBallID, JPH::Vec3::sZero());

    // Update position
    bodyIface.SetPositionAndRotation(g_JoltPhysicsInternal.mBallID,
                                     ToJolt(pos),
                                     JPH::Quat::sIdentity(),
                                     JPH::EActivation::DontActivate);

    mBallMatrix = glm::translate(glm::mat4(1.0f), pos);
}

void Physics::enable_physics_on_ball()
{
    g_JoltPhysicsInternal.ballPhysicsActive = true;

    JPH::BodyInterface &bodyIface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();

    // Re-enable normal physics
    bodyIface.SetMotionType(g_JoltPhysicsInternal.mBallID,
                            JPH::EMotionType::Dynamic,
                            JPH::EActivation::Activate);

    // Apply the carried velocity
    bodyIface.SetLinearVelocity(g_JoltPhysicsInternal.mBallID, ToJolt(g_JoltPhysicsInternal.manualVelocity));
    bodyIface.SetAngularVelocity(g_JoltPhysicsInternal.mBallID, JPH::Vec3::sZero()); // no spin initially

    // Wake it up
    // Activate body so it starts simulation
    bodyIface.ActivateBody(g_JoltPhysicsInternal.mBallID);
}

bool Physics::is_ball_physics_active() const
{
    return g_JoltPhysicsInternal.ballPhysicsActive;
}

void Physics::apply_lane_pushback(float peakZ, float halfWidth, float maxStrength)
{
    auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();

    JPH::RVec3 pos = iface.GetPosition(g_JoltPhysicsInternal.mBallID);
    JPH::Vec3 vel = iface.GetLinearVelocity(g_JoltPhysicsInternal.mBallID);

    float x = pos.GetX();
    float z = pos.GetZ();

    // ------------------------------------------------
    // 0. CONDITIONS: APPLY ONLY WHEN THE BALL IS ROLLING
    // ------------------------------------------------

    // Ball height check (your ball radius is 0.108m normally)
    bool onLane = pos.GetY() < 0.25f;           // small tolerance

    // Not flying or bouncing
    bool verticalStable = fabs(vel.GetY()) < 0.5f;

    // Moving forward in lane direction
    bool movingForward = fabs(vel.GetZ()) > 0.2f;

    if (!(onLane && verticalStable && movingForward))
        return; // do NOT apply force


    // ------------------------------------------------
    // 1. CURVED PROFILE ALONG Z (Gaussian-style)
    // ------------------------------------------------
    float dz = (z - peakZ) / halfWidth;
    float laneFactor = glm::clamp(1.0f - dz * dz, 0.0f, 1.0f);

    // ------------------------------------------------
    // 2. STRONGER NEAR THE EDGES
    // ------------------------------------------------
    // Your enhanced cubic edge factor
    float edgeFactor = glm::clamp(glm::abs(x * x * x), 0.0f, 1.0f);

    // ------------------------------------------------
    // 3. FINAL FORCE
    // ------------------------------------------------
    float strength = maxStrength * laneFactor * edgeFactor;
    float forceX = -glm::sign(x) * strength;

    iface.AddForce(g_JoltPhysicsInternal.mBallID, JPH::Vec3(forceX, 0.0f, 0.0f));
}