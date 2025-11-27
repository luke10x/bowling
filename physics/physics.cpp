#include <cmath>

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp> // for glm::angleAxis, etc.o


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

// === Helpers ===
inline JPH::Vec3 ToJolt(const glm::vec3 &v)
{
    return JPH::Vec3(v.x, v.y, v.z);
}

inline JPH::Quat ToJolt(const glm::quat &q)
{
    // glm: (w, x, y, z)
    // Jolt: (x, y, z, w)
    return JPH::Quat(q.x, q.y, q.z, q.w);
}

glm::mat4 ToGlm(const JPH::RMat44 &m)
{
    glm::mat4 out;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            out[j][i] = m(i, j);
    return out;
}

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
    // glm::vec3 manualVelocity;
    float mPosDtLoan = 0.0f;
    float mAccumulator = 0.0f;
    // for toss momentum
    glm::vec3 filteredVelocity = glm::vec3(0.0f);
    bool hasFilteredVelocity = false;
    // For hook
    float lastDeltaTime;
    glm::quat lastDeltaQuat;
    glm::quat lastManualRot;
    Physics pub;
    float spinSpeed;
};

static JoltPhysicsInternal g_JoltPhysicsInternal;

struct PendingSpinKick
{
    JPH::BodyID pin;
    JPH::Vec3 impulse;
    JPH::Vec3 angularImpulse;
};

std::vector<PendingSpinKick> gPendingKicks;

class SpinContactListener : public JPH::ContactListener
{
public:
    virtual void OnContactAdded(const JPH::Body &body1,
                                const JPH::Body &body2,
                                const JPH::ContactManifold &,
                                JPH::ContactSettings &) override
    {
        JPH::BodyID ball = g_JoltPhysicsInternal.mBallID;

        JPH::BodyID a = body1.GetID();
        JPH::BodyID b = body2.GetID();

        JPH::BodyID pin;
        const JPH::Body* ballBody;
        const JPH::Body* pinBody;

        float spin = 2.0f * g_JoltPhysicsInternal.spinSpeed;
        if (fabs(spin) < 0.01f)
            return;

        if (a == ball) { pin = b; ballBody = &body1; pinBody = &body2; }
        else if (b == ball) { pin = a; ballBody = &body2; pinBody = &body1; }
        else { return; }

        // --- Impact normal (approximate) ---
        JPH::Vec3 ballPos = ballBody->GetCenterOfMassPosition(); 
        JPH::Vec3 pinPos  = pinBody->GetCenterOfMassPosition();
        JPH::Vec3 approxNormal = (pinPos - ballPos).NormalizedOr(JPH::Vec3::sAxisY());

        // --- wobble based on pin index (deterministic randomness) ---
        float hash   = float((pin.GetIndex() * 16807) % 997) * 0.001f;
        float wobble = (hash - 0.5f) * 1.3f;

        JPH::Vec3 lateralKick = spin * approxNormal.Cross(JPH::Vec3::sAxisY());
        JPH::Vec3 angularKick = 1.5f * (1.0f + wobble) * spin * approxNormal.Cross(JPH::Vec3::sAxisY());

        // Store for later safe application
        gPendingKicks.push_back({ pin, lateralKick, angularKick });
    }
};

static SpinContactListener gContactListener;

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
    ballBody.mRestitution = 0.02f;
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
    // https://www.dimensions.com/element/ten-pin-bowling-piI
    g_JoltPhysicsInternal.mTempAllocator = new JPH::TempAllocatorImpl(1024 * 1024);
    g_JoltPhysicsInternal.mJobSystem = new JPH::JobSystemSingleThreaded(JPH::cMaxPhysicsJobs);
    for (int i = 0; i < 10; i++)
    {
        JPH::CylinderShapeSettings pinShape(0.19f, 0.050f); // half-height, radius - radius reduced because it is cylinder not actual pin
        JPH::ShapeRefC pin = pinShape.Create().Get();
        JPH::BodyCreationSettings pinBody(pin, ToJolt(pinStart[i]), JPH::Quat::sIdentity(),
                                          JPH::EMotionType::Dynamic, Layers::DYNAMIC);
        /*
        Pins
            •	mRestitution = 0.1–0.2f
            •	mFriction = 0.3–0.5f (your value is fine)
        */
        pinBody.mRestitution = 0.3f;
        pinBody.mFriction = 0.3f;
        pinBody.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateMassAndInertia;
        pinBody.mMassPropertiesOverride.mMass = 1.53f; // Standard pin mass
        pinBody.mInertiaMultiplier = 1.0f;
        g_JoltPhysicsInternal.mPinID[i] = bodyIface.CreateAndAddBody(pinBody, JPH::EActivation::Activate);
    }

    g_JoltPhysicsInternal.lastManualPos = glm::vec3(0.0f);
    g_JoltPhysicsInternal.lastManualRot = glm::quat(1.0f,0,0,0);
    g_JoltPhysicsInternal.lastDeltaQuat = glm::quat(1.0f,0,0,0);
    g_JoltPhysicsInternal.lastDeltaTime = 0.0f;

    g_JoltPhysicsInternal.filteredVelocity = glm::vec3(0.0f);
    g_JoltPhysicsInternal.hasFilteredVelocity = false;
    g_JoltPhysicsInternal.mPosDtLoan = 0.0f;
    
    g_JoltPhysicsInternal.mPhysicsSystem->SetContactListener(&gContactListener);
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
            -6.0f, // Operational peak
            8.0f, // width of operatiion 
            15.0f // max strength in Newtons
        );

        g_JoltPhysicsInternal.mAccumulator -= g_JoltPhysicsInternal.FIXED_STEP;

        apply_spin_curve();
        
        apply_pending_spin_kicks();
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

void Physics::set_manual_ball_position(const glm::vec3 &pos,
                                       const glm::quat &rot,
                                       float dt)
{
    using glm::epsilon;
    const float EPS = glm::epsilon<float>();

    g_JoltPhysicsInternal.ballPhysicsActive = false;

    // If position unchanged, accumulate loaned dt and bail out early.
    if (glm::length(pos - g_JoltPhysicsInternal.lastManualPos) <= EPS) {
        g_JoltPhysicsInternal.mPosDtLoan += dt;
        // still update rotation delta if rotation changed and dt available
        if (dt > EPS && glm::length(rot - g_JoltPhysicsInternal.lastManualRot) > EPS) {
            g_JoltPhysicsInternal.lastDeltaQuat = rot * glm::inverse(g_JoltPhysicsInternal.lastManualRot);
            g_JoltPhysicsInternal.lastDeltaTime = dt;
            g_JoltPhysicsInternal.lastManualRot = rot;
        }
        return;
    }

    // Accumulate loaned dt and use total dt
    float dt_total = dt + g_JoltPhysicsInternal.mPosDtLoan;
    g_JoltPhysicsInternal.mPosDtLoan = 0.0f;

    // Protect against very small dt_total
    if (dt_total <= EPS) {
        // treat velocity as zero (can't compute reliable velocity)
        g_JoltPhysicsInternal.filteredVelocity = glm::vec3(0.0f);
    } else {
        // instantaneous velocity
        glm::vec3 v = (pos - g_JoltPhysicsInternal.lastManualPos) / dt_total;

        // exponential smoothing (newer input dominates)
        const float weight = 0.15f;
        if (!g_JoltPhysicsInternal.hasFilteredVelocity) {
            g_JoltPhysicsInternal.filteredVelocity = v;
            g_JoltPhysicsInternal.hasFilteredVelocity = true;
        } else {
            g_JoltPhysicsInternal.filteredVelocity =
                glm::mix(g_JoltPhysicsInternal.filteredVelocity, v, weight);
        }
    }

    // Save delta rotation (if dt is sane)
    if (dt > EPS) {
        g_JoltPhysicsInternal.lastDeltaQuat = rot * glm::inverse(g_JoltPhysicsInternal.lastManualRot);
        g_JoltPhysicsInternal.lastDeltaTime = dt;
    } else {
        // zero rotation delta
        g_JoltPhysicsInternal.lastDeltaQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        g_JoltPhysicsInternal.lastDeltaTime = 0.0f;
    }

    // update stored manual pos/rot for next frame
    g_JoltPhysicsInternal.lastManualPos = pos;
    g_JoltPhysicsInternal.lastManualRot = rot;

    // Update Jolt body safely
    JPH::BodyInterface &bodyIface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();

    bodyIface.SetMotionType(g_JoltPhysicsInternal.mBallID,
                            JPH::EMotionType::Kinematic,
                            JPH::EActivation::DontActivate);

    bodyIface.SetLinearVelocity(g_JoltPhysicsInternal.mBallID, JPH::Vec3::sZero());
    bodyIface.SetAngularVelocity(g_JoltPhysicsInternal.mBallID, JPH::Vec3::sZero());

    bodyIface.SetPositionAndRotation(g_JoltPhysicsInternal.mBallID,
                                     ToJolt(pos),
                                     ToJolt(rot),
                                     JPH::EActivation::DontActivate);

    mBallMatrix = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot);
}

void Physics::enable_physics_on_ball()
{
    g_JoltPhysicsInternal.ballPhysicsActive = true;

    JPH::BodyInterface &bodyIface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();

    // Re-enable normal physics
    bodyIface.SetMotionType(g_JoltPhysicsInternal.mBallID,
                            JPH::EMotionType::Dynamic,
                            JPH::EActivation::Activate);

    // Apply linear velocity
    bodyIface.SetLinearVelocity(g_JoltPhysicsInternal.mBallID, ToJolt(g_JoltPhysicsInternal.filteredVelocity));

    // --- Compute angular velocity safely ---
    glm::quat deltaRot = g_JoltPhysicsInternal.lastDeltaQuat;
    float dt = g_JoltPhysicsInternal.lastDeltaTime;

    JPH::Vec3 angularVel = JPH::Vec3::sZero();

    if (dt > 0.0f && glm::length2(glm::vec3(deltaRot.x, deltaRot.y, deltaRot.z)) > 1e-8f)
    {
        // Clamp w to [-1, 1] to avoid NaN in acos
        float w = glm::clamp(deltaRot.w, -1.0f, 1.0f);
        float angle = 2.0f * acosf(w);

        // Normalize axis safely
        glm::vec3 axis(deltaRot.x, deltaRot.y, deltaRot.z);
        float axisLength = glm::length(axis);
        if (axisLength > 1e-8f)
        {
            axis /= axisLength;
            angularVel = ToJolt(axis * (angle / dt));
        }
    }
    // Else: angularVel remains zero (no rotation or invalid dt)

    bodyIface.SetAngularVelocity(g_JoltPhysicsInternal.mBallID, angularVel);

    // Wake it up
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
    float edgeFactor = glm::clamp(glm::abs(x * x), 0.0f, 1.0f);

    // ------------------------------------------------
    // 3. FINAL FORCE
    // ------------------------------------------------
    float strength = maxStrength * laneFactor * edgeFactor;
    float forceX = -glm::sign(x) * strength;

    iface.AddForce(g_JoltPhysicsInternal.mBallID, JPH::Vec3(forceX, 0.0f, 0.0f));
}

void Physics::apply_spin_curve()
{
    auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();
    
    JPH::BodyID ballID = g_JoltPhysicsInternal.mBallID;

    // Get current position and velocity
    JPH::RVec3 pos = iface.GetPosition(ballID);
    JPH::Vec3 vel = iface.GetLinearVelocity(ballID);

    // Only apply if ball is near lane surface
    if (pos.GetY() > 0.15f) // assuming lane height ~0
        return;
   
    float minY = -10.0f;
    float maxY = -1.0f;
    // Map pos.GetY() to 0..1 gradually
    // I want it only be effective towards the end 
    float effectiveness = glm::clamp((pos.GetY() - minY) / (maxY - minY), 0.0f, 1.0f);

    // Get angular velocity
    JPH::Vec3 angVel = iface.GetAngularVelocity(ballID);

    // Compute lateral velocity contribution (forward = -Z)
    JPH::Vec3 forward(0.0f, 0.0f, -1.0f);
    JPH::Vec3 lateral = angVel.Cross(forward) * 0.0001f; // small factor

    lateral *= effectiveness;

    // Apply lateral velocity increment
    iface.SetLinearVelocity(ballID, vel + lateral);
}

void Physics::set_spin_speed(float spinSpeed) {
    g_JoltPhysicsInternal.spinSpeed = spinSpeed;
}
void Physics::apply_pending_spin_kicks()
{
    auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();

    int i = 0;
    for (auto &kick : gPendingKicks)
    {
        i += 1;
        float sign  = i % 2 == 0 ? 1.0f : -1.0f;
        iface.AddImpulse(kick.pin, kick.impulse * JPH::Vec3(sign, 0.0f, 0.0f));
        iface.AddAngularImpulse(kick.pin, kick.angularImpulse);
    }

    gPendingKicks.clear();
}

int Physics::checkThrowComplete(float stillThreshold, float floorY)
{
    JPH::BodyInterface &iface =
        g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterfaceNoLock();

    bool anyMoving = false;
    int fallenCount = 0;

    // --- Check ball ---
    {
        JPH::BodyID ball = g_JoltPhysicsInternal.mBallID;

        JPH::Vec3 v  = iface.GetLinearVelocity(ball);
        JPH::Vec3 av = iface.GetAngularVelocity(ball);
        JPH::Vec3 p  = iface.GetPosition(ball);

        float speed = v.LengthSq() + av.LengthSq();

        // ball still moving?
        if (speed > stillThreshold * stillThreshold)
            anyMoving = true;

        // ball fell off?
        if (p.GetY() < floorY)
            // fallenCount++; // treat as "done"
            anyMoving = false;
    }

    // --- Check pins ---
    for (int i = 0; i < 10; i++)
    {
        JPH::BodyID pin = g_JoltPhysicsInternal.mPinID[i];

        JPH::Vec3 v  = iface.GetLinearVelocity(pin);
        JPH::Vec3 av = iface.GetAngularVelocity(pin);
        JPH::Vec3 p  = iface.GetPosition(pin);

        float speed = v.LengthSq() + av.LengthSq();

        if (p.GetY() < floorY) {
            fallenCount++;
            continue;
        }

        if (speed > stillThreshold * stillThreshold)
            anyMoving = true;
    }

    if (anyMoving)
        return -1;     // still simulating

    return fallenCount;   // ready to enter RESULT phase
}