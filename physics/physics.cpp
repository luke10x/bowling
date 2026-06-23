#include <cmath>

#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp> // for glm::angleAxis, etc.o

// clang-format off
// Jolt.h has to be fist before any other Jolt headers
#include <Jolt/Jolt.h>
// clang-format on

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

// STL includes
#include <cstdarg>
#include <iostream>
#include <thread>

#include "physics.h"

namespace Layers
{
static constexpr JPH::ObjectLayer STATIC = 0;
static constexpr JPH::ObjectLayer DYNAMIC = 1;
static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
} // namespace Layers

namespace BroadPhaseLayers
{
static constexpr JPH::BroadPhaseLayer STATIC(0);
static constexpr JPH::BroadPhaseLayer DYNAMIC(1);
static constexpr uint32_t NUM_LAYERS = 2;
} // namespace BroadPhaseLayers

// BroadPhaseLayerInterface
#if defined(__APPLE__)
    #include <TargetConditionals.h>
#endif

class BPLayerInterfaceImpl : public JPH::BroadPhaseLayerInterface
{
public:
    BPLayerInterfaceImpl()
    {
        mMapping[Layers::STATIC]  = BroadPhaseLayers::STATIC;
        mMapping[Layers::DYNAMIC] = BroadPhaseLayers::DYNAMIC;
    }

    // Override base class functions — no 'virtual' needed with 'override'
    JPH::uint GetNumBroadPhaseLayers() const override
    {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
    {
        return mMapping[inLayer];
    }

#ifdef JPH_DEBUG_RENDERER
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
    {
        // Fix typo in the string and keep it simple
        return "GetBroadPhaseLayerName_NOT_IMPLEMENTED";
    }
#endif

private:
    JPH::BroadPhaseLayer mMapping[Layers::NUM_LAYERS];
};


// Object vs BroadPhase filter
class ObjectVsBPLayerFilter : public JPH::ObjectVsBroadPhaseLayerFilter
{
  public:
    virtual bool
    ShouldCollide(JPH::ObjectLayer inLayer, JPH::BroadPhaseLayer inBPLayer) const override
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
inline glm::vec3 ToGlm(const JPH::Vec3 &v)
{
    return glm::vec3(v.GetX(), v.GetY(), v.GetZ());
}

struct FracturedBlockManager
{
    std::vector<JPH::BodyID> fragmentBodies;
    std::vector<JPH::Constraint *> constraints;
    JPH::BodyID anchorBody;
    float breakSpeed = 8.5f;
    float spawnZ = 0.0f;
    int variantIndex = 0;
    bool broken = false;
    bool breakPending = false;
    bool hasAnchor = false;
    int ballContactCount = 0;
    int ballFirstContactCount = 0;
    float lastBallContactTimeSeconds = -1000.0f;
    bool hadBallContact = false;
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
    JPH::BodyID mLaneId;
    JPH::BodyID mPinID[10];
    bool ballPhysicsActive;
    glm::vec3 lastManualPos;

    JPH::DistanceConstraintSettings rope;
    JPH::BodyID pivotID;

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

    int numberOfImpacts;
    bool pinWasHit[10];
    bool settlingStarted;
    bool mBallIsAlreadyHung;
    JPH::Constraint *mRopeConstraint;
    JPH::Body *pivotBodyRef;

    bool lanePushbackEnabled = true;
    float lanePushbackPeakZ = -6.0f;
    float lanePushbackHalfWidth = 8.0f;
    float lanePushbackMaxStrength = 15.0f;
    float lanePushbackOilStartZ = -18.3f;
    float lanePushbackOilEndZ = -5.0f;
    float lanePushbackOilEaseExp = 2.0f;

    glm::vec3 pendingReleaseAngularVel = glm::vec3(0.0f);

    // Simulation time tracking (for cooldown timers)
    float simTimeSeconds = 0.0f;

    // Ball->lane impact tracking
    int laneHitCount = 0;
    float lastLaneHitTimeSeconds = -1000.0f;
    bool ballAirborneSinceLastLaneHit = true;
    float ballAirborneMinTime = 0.0f;
    FracturedBlockManager fracturedBlock;
};

static JoltPhysicsInternal g_JoltPhysicsInternal;

static inline float smoothstep01(float x)
{
    x = glm::clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

struct PendingSpinKick
{
    JPH::BodyID pin;
    JPH::Vec3 impulse;
    JPH::Vec3 angularImpulse;
};

std::vector<PendingSpinKick> gPendingKicks;

static bool IsFracturedBlockBody(JPH::BodyID id)
{
    const auto &bodies = g_JoltPhysicsInternal.fracturedBlock.fragmentBodies;
    return std::find(bodies.begin(), bodies.end(), id) != bodies.end();
}

static void BreakFracturedBlockInternal()
{
    FracturedBlockManager &block = g_JoltPhysicsInternal.fracturedBlock;
    if (block.broken)
        return;

    if (g_JoltPhysicsInternal.mPhysicsSystem != nullptr)
    {
        for (JPH::Constraint *constraint : block.constraints)
        {
            if (constraint != nullptr)
                g_JoltPhysicsInternal.mPhysicsSystem->RemoveConstraint(constraint);
        }
    }
    block.constraints.clear();
    block.broken = true;
    block.breakPending = false;
}

static void ClearFracturedBlockInternal()
{
    FracturedBlockManager &block = g_JoltPhysicsInternal.fracturedBlock;
    BreakFracturedBlockInternal();

    if (g_JoltPhysicsInternal.mPhysicsSystem != nullptr)
    {
        auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();
        for (JPH::BodyID id : block.fragmentBodies)
        {
            iface.RemoveBody(id);
            iface.DestroyBody(id);
        }
        if (block.hasAnchor)
        {
            iface.RemoveBody(block.anchorBody);
            iface.DestroyBody(block.anchorBody);
        }
    }

    block.fragmentBodies.clear();
    block.anchorBody = JPH::BodyID();
    block.breakSpeed = 8.5f;
    block.spawnZ = 0.0f;
    block.variantIndex = 0;
    block.broken = false;
    block.breakPending = false;
    block.hasAnchor = false;
    block.ballContactCount = 0;
    block.ballFirstContactCount = 0;
    block.lastBallContactTimeSeconds = -1000.0f;
    block.hadBallContact = false;
}

class SpinContactListener : public JPH::ContactListener
{
  public:
    virtual void OnContactAdded(
        const JPH::Body &body1, const JPH::Body &body2, const JPH::ContactManifold &,
        JPH::ContactSettings &
    ) override
    {
        JPH::BodyID ball = g_JoltPhysicsInternal.mBallID;
        JPH::BodyID lane = g_JoltPhysicsInternal.mLaneId;

        JPH::BodyID a = body1.GetID();
        JPH::BodyID b = body2.GetID();

        if (!g_JoltPhysicsInternal.fracturedBlock.broken)
        {
            const bool ballHitsBlock =
                ((a == ball) && IsFracturedBlockBody(b)) || ((b == ball) && IsFracturedBlockBody(a));
            if (ballHitsBlock)
            {
                constexpr float kBlockBallContactCooldownSeconds = 0.045f;
                const float now = g_JoltPhysicsInternal.simTimeSeconds;
                if ((now - g_JoltPhysicsInternal.fracturedBlock.lastBallContactTimeSeconds) >=
                    kBlockBallContactCooldownSeconds)
                {
                    g_JoltPhysicsInternal.fracturedBlock.ballContactCount += 1;
                    g_JoltPhysicsInternal.fracturedBlock.lastBallContactTimeSeconds = now;
                    if (!g_JoltPhysicsInternal.fracturedBlock.hadBallContact)
                    {
                        g_JoltPhysicsInternal.fracturedBlock.hadBallContact = true;
                        g_JoltPhysicsInternal.fracturedBlock.ballFirstContactCount += 1;
                    }
                }

                const JPH::Body &ballBodyForSpeed = (a == ball) ? body1 : body2;
                const float linearSpeed = ballBodyForSpeed.GetLinearVelocity().Length();
                const float angularSpeed = ballBodyForSpeed.GetAngularVelocity().Length();
                const float impactIntensity = linearSpeed + 0.20f * angularSpeed;
                if (impactIntensity >= g_JoltPhysicsInternal.fracturedBlock.breakSpeed)
                    g_JoltPhysicsInternal.fracturedBlock.breakPending = true;
            }
        }

        // Ball hitting the lane: play thud on each meaningful impact (including rebounds),
        // but avoid spamming on resting contact by using a cooldown + velocity threshold.
        if ((a == ball && b == lane) || (b == ball && a == lane))
        {
            const JPH::Body &ballBody = (a == ball) ? body1 : body2;
            JPH::Vec3 v = ballBody.GetLinearVelocity();
            float vy = v.GetY();
            float speed = v.Length();

            const float cooldown = 0.08f;
            float now = g_JoltPhysicsInternal.simTimeSeconds;
            bool offCooldown = (now - g_JoltPhysicsInternal.lastLaneHitTimeSeconds) >= cooldown;

            // Require either a decent vertical slap or a decent overall speed.
            bool meaningful = (std::abs(vy) > 0.35f) || (speed > 1.25f);
            // Also require that the ball had some airtime since the previous lane hit,
            // otherwise resting contact / tiny jitter can trigger false positives.
            bool hadAirtime = g_JoltPhysicsInternal.ballAirborneSinceLastLaneHit;
            if (offCooldown && meaningful && hadAirtime)
            {
                g_JoltPhysicsInternal.laneHitCount += 1;
                g_JoltPhysicsInternal.lastLaneHitTimeSeconds = now;
                g_JoltPhysicsInternal.ballAirborneSinceLastLaneHit = false;
                g_JoltPhysicsInternal.ballAirborneMinTime = 0.0f;
            }
            // Still allow pin-hit logic below (ball might clip lane & pin on same frame), so don't return.
        }

        /* register pins as hit */ {
            // Helper lambda
            auto markIfPin = [](JPH::BodyID id)
            {
                for (int i = 0; i < 10; i++)
                {
                    if (id == g_JoltPhysicsInternal.mPinID[i])
                    {
                        if (!g_JoltPhysicsInternal.pinWasHit[i])
                        {
                            // Here: it was hit!
                            g_JoltPhysicsInternal.pinWasHit[i] = true;
                            g_JoltPhysicsInternal.numberOfImpacts += 1;
                        }
                    }
                }
            };

            // Mark both sides if they are pins
            markIfPin(a);
            markIfPin(b);
        }

        JPH::BodyID pin;
        const JPH::Body *ballBody;
        const JPH::Body *pinBody;

        if (a == ball)
        {
            pin = b;
            ballBody = &body1;
            pinBody = &body2;
        }
        else if (b == ball)
        {
            pin = a;
            ballBody = &body2;
            pinBody = &body1;
        }
        else
        {
            // Collision does not the ball
            return;
        }

        // check if pin is really a pin (and not lane for example)
        bool isPinReallyAPin = false;
        for (int i = 0; i < 10; i++)
        {
            if (pin == g_JoltPhysicsInternal.mPinID[i])
            {
                isPinReallyAPin = true;
            }
        }
        if (!isPinReallyAPin)
        {
            return;
        }

        g_JoltPhysicsInternal.settlingStarted = true;

        float spin = 2.0f * g_JoltPhysicsInternal.spinSpeed;
        if (fabs(spin) < 0.01f)
            return;

        // --- Impact normal (approximate) ---
        JPH::Vec3 ballPos = ballBody->GetCenterOfMassPosition();
        JPH::Vec3 pinPos = pinBody->GetCenterOfMassPosition();
        JPH::Vec3 approxNormal = (pinPos - ballPos).NormalizedOr(JPH::Vec3::sAxisY());

        // --- wobble based on pin index (deterministic randomness) ---
        float hash = float((pin.GetIndex() * 16807) % 997) * 0.001f;
        float wobble = (hash - 0.5f) * 3.3f;

        // Lateral is not completelly lateral but goes half forward half to spin side
        // Normal points back to hook more
        JPH::Vec3 lateralKick =
            spin * (-approxNormal * 0.33f + 0.66f * approxNormal.Cross(JPH::Vec3::sAxisY()));
        JPH::Vec3 angularKick =
            2.5f * (1.0f + wobble) * spin * approxNormal.Cross(JPH::Vec3::sAxisY());

        // Store for later safe application
        gPendingKicks.push_back({pin, lateralKick, angularKick});
    }
};

static SpinContactListener gContactListener;

// Jolt includes (minimal set)
#ifdef JPH_ENABLE_ASSERTS
// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(
    const char *inExpression, const char *inMessage, const char *inFile, JPH::uint inLine
)
{
    // Print to the TTY
    std::cout << inFile << ":" << inLine << ": (" << inExpression << ") "
              << (inMessage != nullptr ? inMessage : "") << std::endl;

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

extern "C" void
JPH_AssertFailure(const char *expr, const char *file, uint32_t line, const char *msg)
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
    const float *laneVerts, unsigned int laneVertCount, const unsigned int *laneIndices,
    unsigned int laneIndexCount, glm::vec3 *pinStart, glm::vec3 ballStart
)
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
        g_JoltPhysicsInternal.bpLayerInterface, g_JoltPhysicsInternal.objVsBpFilter,
        g_JoltPhysicsInternal.objPairFilter
    );

    g_JoltPhysicsInternal.ballPhysicsActive = true; // start with physics enabled

    JPH::BodyInterface &bodyIface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();

    // === Static lane collider (analytic) ===
    //
    // We intentionally avoid a triangle-mesh collider here because internal triangle edges / seams
    // can cause subtle contact-normal changes that look like "random" lateral kicks on a rolling ball.
    // Instead we approximate the lane as a simple box aligned with world axes.
    //
    // We derive the lane bounds from the render mesh vertices passed in by game.cpp.
    float minX = +1e9f, minY = +1e9f, minZ = +1e9f;
    float maxX = -1e9f, maxY = -1e9f, maxZ = -1e9f;
    for (unsigned int i = 0; i + 2 < laneVertCount; i += 3)
    {
        float x = laneVerts[i + 0];
        float y = laneVerts[i + 1];
        float z = laneVerts[i + 2];
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        minZ = std::min(minZ, z);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
        maxZ = std::max(maxZ, z);
    }

    // We only care about the *top* face of the lane. The lane surface in gameplay/render
    // is at y ~= 0, so we anchor the top of the collider at y=0 and extend downward.
    // This avoids accidentally using any raised mesh features (gutters/walls) as "topY".
    const float topY = 0.0f;
    const float halfY = 0.35f; // slab thickness below the lane surface
    const float centerY = topY - halfY;

    float halfX = 0.5f * (maxX - minX);
    float halfZ = 0.5f * (maxZ - minZ);
    if (!std::isfinite(halfX) || halfX < 0.01f)
        halfX = 0.60f;
    if (!std::isfinite(halfZ) || halfZ < 0.01f)
        halfZ = 10.0f;
    // Ensure we at least cover the mesh bounds.
    halfX = std::max(0.05f, halfX + 0.01f);

    // The lane render mesh can include gutters / side geometry, which makes the AABB
    // much wider than the playable lane surface. For stable gameplay, clamp the collider
    // width to the standard lane surface width (~41.857 inches).
    // (We still keep a small margin so the ball doesn't "fall off" due to numerical jitter.)
    constexpr float kLaneSurfaceWidthM = 41.857f * 0.0254f;
    constexpr float kLaneHalfWidthM = 0.5f * kLaneSurfaceWidthM;
    constexpr float kLaneHalfWidthMarginM = 0.02f;
    halfX = std::min(halfX, kLaneHalfWidthM + kLaneHalfWidthMarginM);

    JPH::Vec3 halfExtents(halfX, std::max(0.02f, halfY), halfZ);
    // Keep collider centered to the lane mesh bounds (X/Z). We clamp the width above, so even if the
    // mesh includes gutters, the collider won't become too wide, but it will still be positioned correctly
    // if the authored lane is offset in world space.
    JPH::RVec3 center(0.5 * (minX + maxX), centerY, 0.5 * (minZ + maxZ));
    JPH::BoxShapeSettings boxSettings(halfExtents);
    JPH::ShapeRefC laneShape = boxSettings.Create().Get();

    JPH::BodyCreationSettings lane(
        laneShape, center, JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::STATIC
    );

    // Keep lane friction low; gameplay friction is driven per-frame from game.cpp
    // via Physics::set_ball_friction (combined friction is sqrt(lane * ball)).
    lane.mFriction = 0.05f;
    lane.mRestitution = 0.01f; // very low bounce
    /*
     *
     */

    g_JoltPhysicsInternal.mLaneId =
        bodyIface.CreateAndAddBody(lane, JPH::EActivation::DontActivate);

    // === Ball (sphere) ===
    JPH::SphereShapeSettings ballShape(0.11f);
    JPH::ShapeRefC ball = ballShape.Create().Get();
    JPH::BodyCreationSettings ballBody(
        ball, ToJolt(ballStart), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::DYNAMIC
    );
    ballBody.mRestitution = 0.02f;
    // Ball friction is set dynamically during play (skid/bite curve).
    ballBody.mFriction = 0.01f;

    /*
    Ball
    •	mRestitution = 0.05f (bowling balls barely bounce)
    •	mFriction = 0.15f (syn-thetic lane → slippery)
    */
    ballBody.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateMassAndInertia;
    ballBody.mMassPropertiesOverride.mMass = 7.25f; // Middle of legal range 6 - 7.26
    ballBody.mInertiaMultiplier = 1.0f;             // Realistic rolling

    g_JoltPhysicsInternal.mBallID =
        bodyIface.CreateAndAddBody(ballBody, JPH::EActivation::Activate);

    // === Pin (cylinder) ===
    // https://www.dimensions.com/element/ten-pin-bowling-piI
    g_JoltPhysicsInternal.mTempAllocator = new JPH::TempAllocatorImpl(1024 * 1024);
    g_JoltPhysicsInternal.mJobSystem = new JPH::JobSystemSingleThreaded(JPH::cMaxPhysicsJobs);
    for (int i = 0; i < 10; i++)
    {
        this->mPinDead[i] = false;
        JPH::CylinderShapeSettings pinShape(
            0.19f, 0.050f
        ); // half-height, radius - radius reduced because it is cylinder not actual pin
        JPH::ShapeRefC pin = pinShape.Create().Get();
        JPH::BodyCreationSettings pinBody(
            pin, ToJolt(pinStart[i]), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic,
            Layers::DYNAMIC
        );
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
        g_JoltPhysicsInternal.mPinID[i] =
            bodyIface.CreateAndAddBody(pinBody, JPH::EActivation::Activate);
    }

    g_JoltPhysicsInternal.lastManualPos = glm::vec3(0.0f);
    g_JoltPhysicsInternal.lastManualRot = glm::quat(1.0f, 0, 0, 0);
    g_JoltPhysicsInternal.lastDeltaQuat = glm::quat(1.0f, 0, 0, 0);
    g_JoltPhysicsInternal.lastDeltaTime = 0.0f;

    g_JoltPhysicsInternal.filteredVelocity = glm::vec3(0.0f);
    g_JoltPhysicsInternal.hasFilteredVelocity = false;
    g_JoltPhysicsInternal.mPosDtLoan = 0.0f;

    g_JoltPhysicsInternal.mPhysicsSystem->SetContactListener(&gContactListener);

    JPH::BodyCreationSettings pivotSettings(
        new JPH::SphereShape(0.01f), // tiny, invisible
        // Initial pivot position is not gameplay-critical; game.cpp immediately drives it via
        // Physics::change_pivot_point. Keep a stable default here to avoid duplicating tunables.
        JPH::Vec3(0.0f, 1.2f, -18.3), JPH::Quat::sIdentity(),
        JPH::EMotionType::Static, // world anchor
        Layers::STATIC
    );

    g_JoltPhysicsInternal.pivotID =
        bodyIface.CreateAndAddBody(pivotSettings, JPH::EActivation::DontActivate);

    auto &lockInterface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyLockInterface();
    JPH::BodyLockWrite lockPivot(lockInterface, g_JoltPhysicsInternal.pivotID);
    if (!lockPivot.Succeeded())
    {
        std::cerr << "Failed to lock pivot body!" << std::endl;
        return;
    }
    g_JoltPhysicsInternal.pivotBodyRef = &lockPivot.GetBody();

    g_JoltPhysicsInternal.mBallIsAlreadyHung = false;
}

void Physics::physics_step(float deltaSeconds, float physicsInterval)
{
    g_JoltPhysicsInternal.mAccumulator += deltaSeconds;

    // Run as many fixed 10ms physics steps as needed
    while (g_JoltPhysicsInternal.mAccumulator >= physicsInterval)
    {
        g_JoltPhysicsInternal.simTimeSeconds += physicsInterval;

        g_JoltPhysicsInternal.mPhysicsSystem->Update(
            physicsInterval,
            1, // still 1, this is not number of steps!
            g_JoltPhysicsInternal.mTempAllocator, g_JoltPhysicsInternal.mJobSystem
        );

        if (g_JoltPhysicsInternal.fracturedBlock.breakPending &&
            !g_JoltPhysicsInternal.fracturedBlock.broken)
        {
            BreakFracturedBlockInternal();
        }

        // Track whether the ball has had some airtime since the last lane hit.
        {
            auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();
            JPH::RVec3 pos = iface.GetPosition(g_JoltPhysicsInternal.mBallID);
            JPH::Vec3 vel = iface.GetLinearVelocity(g_JoltPhysicsInternal.mBallID);

            // Consider it "airborne" if above the lane by a noticeable amount and moving upward/downward.
            // Lane surface is around y=0; ball in contact tends to be below ~0.25 based on earlier logic.
            bool airborne = (pos.GetY() > 0.35f) || (pos.GetY() > 0.25f && std::abs(vel.GetY()) > 0.35f);
            if (airborne)
            {
                g_JoltPhysicsInternal.ballAirborneMinTime += physicsInterval;
                if (g_JoltPhysicsInternal.ballAirborneMinTime >= 0.03f)
                    g_JoltPhysicsInternal.ballAirborneSinceLastLaneHit = true;
            }
        }

        if (g_JoltPhysicsInternal.lanePushbackEnabled)
        {
            // Oil-profile pushback: strongest at startZ, fades to 0 at endZ.
            {
                auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();
                JPH::RVec3 pos = iface.GetPosition(g_JoltPhysicsInternal.mBallID);
                JPH::Vec3 vel = iface.GetLinearVelocity(g_JoltPhysicsInternal.mBallID);

                float x = pos.GetX();
                float z = pos.GetZ();

                bool onLane = pos.GetY() < 0.25f;
                bool verticalStable = fabs(vel.GetY()) < 0.5f;
                bool movingForward = fabs(vel.GetZ()) > 0.2f;
                if (onLane && verticalStable && movingForward)
                {
                    float startZ = g_JoltPhysicsInternal.lanePushbackOilStartZ;
                    float endZ = g_JoltPhysicsInternal.lanePushbackOilEndZ;
                    if (startZ > endZ)
                        std::swap(startZ, endZ);

                    float denom = (endZ - startZ);
                    float t = (denom > 1e-6f) ? ((z - startZ) / denom) : 1.0f;
                    t = glm::clamp(t, 0.0f, 1.0f);

                    float ramp = smoothstep01(t);
                    ramp = powf(ramp, glm::max(0.1f, g_JoltPhysicsInternal.lanePushbackOilEaseExp));
                    float oilFactor = 1.0f - ramp; // start strong, fade out with oil

                    float edgeFactor = glm::clamp(glm::abs(x * x), 0.0f, 1.0f);
                    float strength = g_JoltPhysicsInternal.lanePushbackMaxStrength * oilFactor * edgeFactor;
                    float forceX = -glm::sign(x) * strength;
                    iface.AddForce(g_JoltPhysicsInternal.mBallID, JPH::Vec3(forceX, 0.0f, 0.0f));
                }
            }
        }

        g_JoltPhysicsInternal.mAccumulator -= physicsInterval;
        if (g_JoltPhysicsInternal.mAccumulator > 2.0f)
        {
            std::cerr << "Warning physics left far behind " << g_JoltPhysicsInternal.mAccumulator
                      << std::endl;
            g_JoltPhysicsInternal.mAccumulator =
                2.0f; // Avoids hyper buffering, drain it until manageable 2s buffer
        }

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

glm::vec3 Physics::get_ball_angular_velocity() const
{
    auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();
    return ToGlm(iface.GetAngularVelocity(g_JoltPhysicsInternal.mBallID));
}

void Physics::physics_reset(glm::vec3 *newPinPos, glm::vec3 newBallPos, bool reviveAll)
{
    JPH::BodyInterface &bodyIface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();

    bodyIface.SetPositionAndRotation(
        g_JoltPhysicsInternal.mBallID, ToJolt(newBallPos), JPH::Quat::sIdentity(),
        JPH::EActivation::Activate
    );

    bodyIface.SetLinearVelocity(g_JoltPhysicsInternal.mBallID, JPH::Vec3::sZero());
    bodyIface.SetAngularVelocity(g_JoltPhysicsInternal.mBallID, JPH::Vec3::sZero());
    this->mBallMatrix = ToGlm(bodyIface.GetWorldTransform(g_JoltPhysicsInternal.mBallID));

    for (int i = 0; i < 10; i++)
    {
        if (reviveAll)
        {
            this->mPinDead[i] = false;
        }
        glm::vec3 pos = newPinPos[i];
        if (this->mPinDead[i])
        {
            pos.y += -1.0f;
            pos.z += 1.5f;
        }
        bodyIface.SetPositionAndRotation(
            g_JoltPhysicsInternal.mPinID[i], ToJolt(pos), JPH::Quat::sIdentity(),
            JPH::EActivation::Activate
        );
        bodyIface.SetLinearVelocity(g_JoltPhysicsInternal.mPinID[i], JPH::Vec3::sZero());
        bodyIface.SetAngularVelocity(g_JoltPhysicsInternal.mPinID[i], JPH::Vec3::sZero());
        this->mPinMatrix[i] = ToGlm(bodyIface.GetWorldTransform(g_JoltPhysicsInternal.mPinID[i]));
    }

    // Reset per-throw impact counters.
    g_JoltPhysicsInternal.laneHitCount = 0;
    g_JoltPhysicsInternal.lastLaneHitTimeSeconds = g_JoltPhysicsInternal.simTimeSeconds;
    g_JoltPhysicsInternal.ballAirborneSinceLastLaneHit = true;
    g_JoltPhysicsInternal.ballAirborneMinTime = 0.0f;
}

void Physics::set_manual_ball_position(const glm::vec3 &pos, const glm::quat &rot, float dt)
{
    using glm::epsilon;
    const float EPS = glm::epsilon<float>();

    g_JoltPhysicsInternal.ballPhysicsActive = false;

    // If position unchanged, accumulate loaned dt and bail out early.
    if (glm::length(pos - g_JoltPhysicsInternal.lastManualPos) <= EPS)
    {
        g_JoltPhysicsInternal.mPosDtLoan += dt;
        // still update rotation delta if rotation changed and dt available
        if (dt > EPS && glm::length(rot - g_JoltPhysicsInternal.lastManualRot) > EPS)
        {
            g_JoltPhysicsInternal.lastDeltaQuat =
                rot * glm::inverse(g_JoltPhysicsInternal.lastManualRot);
            g_JoltPhysicsInternal.lastDeltaTime = dt;
            g_JoltPhysicsInternal.lastManualRot = rot;
        }
        return;
    }

    // Accumulate loaned dt and use total dt
    float dt_total = dt + g_JoltPhysicsInternal.mPosDtLoan;
    g_JoltPhysicsInternal.mPosDtLoan = 0.0f;

    // Protect against very small dt_total
    if (dt_total <= EPS)
    {
        // treat velocity as zero (can't compute reliable velocity)
        g_JoltPhysicsInternal.filteredVelocity = glm::vec3(0.0f);
    }
    else
    {
        // instantaneous velocity
        glm::vec3 v = (pos - g_JoltPhysicsInternal.lastManualPos) / dt_total;

        // exponential smoothing (newer input dominates)
        const float weight = 0.15f;
        if (!g_JoltPhysicsInternal.hasFilteredVelocity)
        {
            g_JoltPhysicsInternal.filteredVelocity = v;
            g_JoltPhysicsInternal.hasFilteredVelocity = true;
        }
        else
        {
            g_JoltPhysicsInternal.filteredVelocity =
                glm::mix(g_JoltPhysicsInternal.filteredVelocity, v, weight);
        }
    }

    // Save delta rotation (if dt is sane)
    if (dt > EPS)
    {
        g_JoltPhysicsInternal.lastDeltaQuat =
            rot * glm::inverse(g_JoltPhysicsInternal.lastManualRot);
        g_JoltPhysicsInternal.lastDeltaTime = dt;
    }
    else
    {
        // zero rotation delta
        g_JoltPhysicsInternal.lastDeltaQuat = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        g_JoltPhysicsInternal.lastDeltaTime = 0.0f;
    }

    // update stored manual pos/rot for next frame
    g_JoltPhysicsInternal.lastManualPos = pos;
    g_JoltPhysicsInternal.lastManualRot = rot;

    // Update Jolt body safely
    JPH::BodyInterface &bodyIface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();

    bodyIface.SetMotionType(
        g_JoltPhysicsInternal.mBallID, JPH::EMotionType::Kinematic, JPH::EActivation::DontActivate
    );

    bodyIface.SetLinearVelocity(g_JoltPhysicsInternal.mBallID, JPH::Vec3::sZero());
    bodyIface.SetAngularVelocity(g_JoltPhysicsInternal.mBallID, JPH::Vec3::sZero());

    bodyIface.SetPositionAndRotation(
        g_JoltPhysicsInternal.mBallID, ToJolt(pos), ToJolt(rot), JPH::EActivation::DontActivate
    );

    mBallMatrix = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot);
}

void Physics::set_ball_hanging(const glm::vec3 pivotPoint, const glm::vec3 ballPos)
{

    if (g_JoltPhysicsInternal.mBallIsAlreadyHung)
    {
        return;
    }
    g_JoltPhysicsInternal.mBallIsAlreadyHung = true;

    // Get ball body by acquiring a lock
    const JPH::BodyLockInterface &lockInterface =
        g_JoltPhysicsInternal.mPhysicsSystem->GetBodyLockInterface();
    JPH::BodyLockWrite lockBall(lockInterface, g_JoltPhysicsInternal.mBallID);
    JPH::Body &ballBody = lockBall.GetBody();

    // pivot point body is stored in the struct
    JPH::Body &pivotBody = *g_JoltPhysicsInternal.pivotBodyRef;

    JPH::DistanceConstraintSettings rope;
    // Use the explicit pivot/ball positions provided by the caller (game.cpp) so AIM (manual)
    // and SWING (Jolt) agree on rope length. Then clamp slightly shorter to avoid the
    // lowest swing point dipping under lane lips/edges and getting stuck.
    rope.mPoint1 = ToJolt(pivotPoint);
    rope.mPoint2 = ToJolt(ballPos);

    float distance = (rope.mPoint1 - rope.mPoint2).Length();
    distance = glm::max(0.01f, distance); // prevent 0

    // Make the SWING rope slightly shorter than AIM to avoid scraping / getting stuck,
    // but don't hard-cap it (a hard cap can distort the swing arc).
    const float kSwingRopeShorten = 0.05f;
    distance = glm::max(0.01f, distance - kSwingRopeShorten);
    rope.mMinDistance = distance;
    rope.mMaxDistance = distance;

    // JPH::Constraint* ropeConstraint = rope.Create(pivotBody, ballBody);
    g_JoltPhysicsInternal.mRopeConstraint = rope.Create(pivotBody, ballBody);

    g_JoltPhysicsInternal.mPhysicsSystem->AddConstraint(g_JoltPhysicsInternal.mRopeConstraint);
}

void Physics::change_pivot_point(glm::vec3 newPivot)
{
    JPH::BodyInterface &bodyIface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();
    bodyIface.SetPosition(
        g_JoltPhysicsInternal.pivotID, ToJolt(newPivot), JPH::EActivation::DontActivate
    );
}

void Physics::set_ball_free()
{
    g_JoltPhysicsInternal.mBallIsAlreadyHung = false;
    if (g_JoltPhysicsInternal.mRopeConstraint)
    {
        g_JoltPhysicsInternal.mPhysicsSystem->RemoveConstraint(
            g_JoltPhysicsInternal.mRopeConstraint
        );

        // Optional: delete if you manage memory manually
        // delete g_JoltPhysicsInternal.mRopeConstraint;
        g_JoltPhysicsInternal.mRopeConstraint = nullptr;
    }
}

void Physics::enable_physics_on_ball()
{
    g_JoltPhysicsInternal.settlingStarted = false;
    
    // Reset hit flags
    for (int i = 0; i < 10; i++)
    {
        g_JoltPhysicsInternal.pinWasHit[i] = false;
    }
    g_JoltPhysicsInternal.numberOfImpacts = 0;

    g_JoltPhysicsInternal.ballPhysicsActive = true;

    JPH::BodyInterface &bodyIface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();

    // Re-enable normal physics
    bodyIface.SetMotionType(
        g_JoltPhysicsInternal.mBallID, JPH::EMotionType::Dynamic, JPH::EActivation::Activate
    );

    // Apply linear velocity
    bodyIface.SetLinearVelocity(
        g_JoltPhysicsInternal.mBallID, ToJolt(g_JoltPhysicsInternal.filteredVelocity)
    );

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

    angularVel += ToJolt(g_JoltPhysicsInternal.pendingReleaseAngularVel);
    g_JoltPhysicsInternal.pendingReleaseAngularVel = glm::vec3(0.0f);

    bodyIface.SetAngularVelocity(g_JoltPhysicsInternal.mBallID, angularVel);

    // Wake it up
    bodyIface.ActivateBody(g_JoltPhysicsInternal.mBallID);
}

bool Physics::is_settling_started() const
{
    return g_JoltPhysicsInternal.settlingStarted;
}

bool Physics::was_pin_hit(int i) const
{
    return g_JoltPhysicsInternal.pinWasHit[i];
}

int Physics::get_number_of_impacts() const {
    return g_JoltPhysicsInternal.numberOfImpacts;
}

bool Physics::is_ball_physics_active() const
{
    return g_JoltPhysicsInternal.ballPhysicsActive;
}

glm::vec3 Physics::get_ball_swing_movement() const
{
    auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();
    JPH::Vec3 vel = iface.GetLinearVelocity(g_JoltPhysicsInternal.mBallID);
    return ToGlm(vel);
}

void Physics::set_ball_swing_movement(glm::vec3 vel)
{
    auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();
    iface.SetLinearVelocity(g_JoltPhysicsInternal.mBallID, ToJolt(vel));
}

void Physics::set_ball_mass(float mass)
{

    auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();

    JPH::BodyLockWrite lock(
        g_JoltPhysicsInternal.mPhysicsSystem->GetBodyLockInterface(),
        g_JoltPhysicsInternal.mBallID
    );

    if (lock.Succeeded())
    {
        JPH::Body &body = lock.GetBody();

        JPH::MotionProperties *mp = body.GetMotionProperties();
        JPH_ASSERT(mp != nullptr);

        float newMass = mass;

        // Jolt stores inverse mass
        mp->SetInverseMass(1.0f / newMass);


        // body.adctiv(true);
    }
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
    bool onLane = pos.GetY() < 0.25f; // small tolerance

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

void Physics::set_lane_pushback_params(float peakZ, float halfWidth, float maxStrength, bool enabled)
{
    g_JoltPhysicsInternal.lanePushbackEnabled = enabled;
    g_JoltPhysicsInternal.lanePushbackPeakZ = peakZ;
    g_JoltPhysicsInternal.lanePushbackHalfWidth = halfWidth;
    g_JoltPhysicsInternal.lanePushbackMaxStrength = maxStrength;
}

void Physics::set_lane_pushback_oil_profile(float startZ, float endZ, float maxStrength, float easeExp, bool enabled)
{
    g_JoltPhysicsInternal.lanePushbackEnabled = enabled;
    g_JoltPhysicsInternal.lanePushbackOilStartZ = startZ;
    g_JoltPhysicsInternal.lanePushbackOilEndZ = endZ;
    g_JoltPhysicsInternal.lanePushbackOilEaseExp = easeExp;
    g_JoltPhysicsInternal.lanePushbackMaxStrength = maxStrength;
}

void Physics::set_pending_release_angular_velocity(const glm::vec3 &angVel)
{
    g_JoltPhysicsInternal.pendingReleaseAngularVel = angVel;
}

void Physics::add_ball_angular_velocity(const glm::vec3 &angVel)
{
    auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();
    JPH::Vec3 current = iface.GetAngularVelocity(g_JoltPhysicsInternal.mBallID);
    iface.SetAngularVelocity(g_JoltPhysicsInternal.mBallID, current + ToJolt(angVel));
    iface.ActivateBody(g_JoltPhysicsInternal.mBallID);
}

void Physics::set_ball_rotation(const glm::quat &rot)
{
    auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();
    iface.SetRotation(g_JoltPhysicsInternal.mBallID, ToJolt(rot), JPH::EActivation::Activate);
}

void Physics::apply_friction_to_lane(float friction)
{
    JPH::BodyID laneId = g_JoltPhysicsInternal.mLaneId;
    auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();
    iface.SetFriction(g_JoltPhysicsInternal.mLaneId, friction);
}

void Physics::set_ball_friction(float friction)
{
    auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();
    iface.SetFriction(g_JoltPhysicsInternal.mBallID, friction);
}

void Physics::apply_restitution_to_lane(float restitution)
{
    auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();
    iface.SetRestitution(g_JoltPhysicsInternal.mLaneId, restitution);
}

void Physics::set_ball_restitution(float restitution)
{
    auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();
    iface.SetRestitution(g_JoltPhysicsInternal.mBallID, restitution);
}

void Physics::set_pins_restitution(float restitution)
{
    auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();
    for (int i = 0; i < 10; i++)
    {
        iface.SetRestitution(g_JoltPhysicsInternal.mPinID[i], restitution);
    }
}

void Physics::set_pins_friction(float friction)
{
    auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();
    for (int i = 0; i < 10; i++)
    {
        iface.SetFriction(g_JoltPhysicsInternal.mPinID[i], friction);
    }
}

void Physics::set_pins_mass(float mass)
{
    auto &lockInterface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyLockInterface();
    for (int i = 0; i < 10; i++)
    {
        JPH::BodyLockWrite lock(lockInterface, g_JoltPhysicsInternal.mPinID[i]);
        if (!lock.Succeeded())
            continue;
        JPH::Body &body = lock.GetBody();
        JPH::MotionProperties *mp = body.GetMotionProperties();
        if (!mp)
            continue;
        float newMass = glm::max(0.001f, mass);
        mp->SetInverseMass(1.0f / newMass);
    }
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

    // Compute lateral velocity contribution (forward = +Z in this project)
    JPH::Vec3 forward(0.0f, 0.0f, 1.0f);
    JPH::Vec3 lateral = angVel.Cross(forward) * 0.0001f; // small factor

    lateral *= effectiveness;

    // Apply lateral velocity increment
    iface.SetLinearVelocity(ballID, vel + lateral);
}

// This method is used to add smashing power only
void Physics::set_spin_speed(float spinSpeed)
{
    g_JoltPhysicsInternal.spinSpeed = spinSpeed;
}

void Physics::apply_angular_velocity_on_ball(float spinSpeed)
{
    JPH::BodyInterface &bodyIface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();

    JPH::Vec3 currentAngular = bodyIface.GetAngularVelocity(g_JoltPhysicsInternal.mBallID);

    // Only makes the ball spin, there is another function that affect smashing power
    JPH::Vec3 addedSpin(0.0f, spinSpeed, 0.0f);

    bodyIface.SetAngularVelocity(g_JoltPhysicsInternal.mBallID, currentAngular + addedSpin);

    bodyIface.ActivateBody(g_JoltPhysicsInternal.mBallID);
}

void Physics::apply_pending_spin_kicks()
{
    auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();

    int i = 0;
    for (auto &kick : gPendingKicks)
    {
        i += 1;
        float sign = i % 2 == 0 ? 1.0f : -1.0f;
        iface.AddImpulse(kick.pin, kick.impulse * JPH::Vec3(sign, 0.0f, 0.0f));
        iface.AddAngularImpulse(kick.pin, kick.angularImpulse);
    }

    gPendingKicks.clear();
}

int Physics::checkThrowComplete(float stillThreshold, float floorY)
{
    JPH::BodyInterface &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterfaceNoLock();

    bool anyMoving = false;
    int fallenCount = 0;

    // --- Check ball ---
    {
        JPH::BodyID ball = g_JoltPhysicsInternal.mBallID;

        JPH::Vec3 v = iface.GetLinearVelocity(ball);
        JPH::Vec3 av = iface.GetAngularVelocity(ball);
        JPH::Vec3 p = iface.GetPosition(ball);

        float speed = v.LengthSq() + av.LengthSq();

        // ball still moving?
        if (speed > stillThreshold * stillThreshold)
            anyMoving = true;

        // ball fell off?
        if (p.GetY() < floorY)
        {
            anyMoving = false; // This will be overriden again if any pin is still moving
        }
    }

    // --- Check pins ---
    for (int i = 0; i < 10; i++)
    {
        if (this->mPinDead[i])
        {
            continue;
        }
        JPH::BodyID pin = g_JoltPhysicsInternal.mPinID[i];

        JPH::Vec3 v = iface.GetLinearVelocity(pin);
        JPH::Vec3 av = iface.GetAngularVelocity(pin);
        JPH::Vec3 p = iface.GetPosition(pin);

        float speed = v.LengthSq() + av.LengthSq();

        if (p.GetY() < floorY)
        {
            this->mPinDead[i] = true;
            continue;
        }

        if (speed > stillThreshold * stillThreshold)
        {
            anyMoving = true; // I told you here is overriden, even if the ball fell off
        }
    }

    if (anyMoving)
    {
        return -1; // still simulating
    }
    else
    {
        for (int i = 0; i < 10; i++)
        {
            // Orientation test
            JPH::BodyID pin = g_JoltPhysicsInternal.mPinID[i];
            if (this->mPinDead[i])
            {
                fallenCount++; // maybe dead because of the position
                               // Note that it could have been changed before frames
                continue;
                // if dead already, don't die again
            }

            JPH::Vec3 up = iface.GetRotation(pin) * JPH::Vec3::sAxisY();
            float dot = up.Dot(JPH::Vec3::sAxisY());
            bool isStanding = dot > 0.85f; // 30 deg
            if (!isStanding)
            {
                fallenCount++;
                this->mPinDead[i] = true;
            }
        }
    }

    return fallenCount;
}

int Physics::estimatePinsDown(float floorY, float standingDotThreshold) const
{
    JPH::BodyInterface &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterfaceNoLock();

    int downCount = 0;
    float thresh = standingDotThreshold;
    if (!std::isfinite(thresh))
        thresh = 0.85f;
    thresh = std::clamp(thresh, 0.0f, 1.0f);

    for (int i = 0; i < 10; i++)
    {
        JPH::BodyID pin = g_JoltPhysicsInternal.mPinID[i];
        JPH::Vec3 p = iface.GetPosition(pin);
        if (p.GetY() < floorY)
        {
            downCount++;
            continue;
        }

        JPH::Vec3 up = iface.GetRotation(pin) * JPH::Vec3::sAxisY();
        float dot = up.Dot(JPH::Vec3::sAxisY());
        bool isStanding = dot > thresh;
        if (!isStanding)
            downCount++;
    }

    return downCount;
}

int Physics::get_lane_hit_count() const
{
    return g_JoltPhysicsInternal.laneHitCount;
}

void Physics::GenerateFracturedBlock(
    const FracturedBlockSettings &settings,
    std::vector<FracturedBlockFragmentGeometry> *outFragments
)
{
    ClearFracturedBlockInternal();
    if (outFragments != nullptr)
        outFragments->clear();

    if (g_JoltPhysicsInternal.mPhysicsSystem == nullptr)
        return;

    const float width = std::max(0.05f, settings.width);
    const float height = std::max(0.05f, settings.height);
    const float thickness = std::max(0.02f, settings.thickness);
    const float breakSpeed = std::max(0.1f, settings.breakSpeed);
    const std::vector<FracturedBlockFragmentGeometry> fragments =
        Block_GenerateVoronoiFragments(settings);

    if (fragments.empty())
        return;

    auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterface();

    FracturedBlockManager &block = g_JoltPhysicsInternal.fracturedBlock;
    block.breakSpeed = breakSpeed;
    block.spawnZ = settings.center.z;
    block.variantIndex = settings.variantIndex;
    block.broken = false;
    block.breakPending = false;
    block.hasAnchor = false;
    block.ballContactCount = 0;
    block.ballFirstContactCount = 0;
    block.lastBallContactTimeSeconds = -1000.0f;
    block.hadBallContact = false;

    JPH::Body *anchorBody = nullptr;
    if (settings.anchorToWorldWhenIntact)
    {
        JPH::BodyCreationSettings anchorSettings(
            new JPH::SphereShape(0.01f),
            ToJolt(settings.center),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Static,
            Layers::STATIC
        );
        anchorBody = iface.CreateBody(anchorSettings);
        if (anchorBody != nullptr)
        {
            iface.AddBody(anchorBody->GetID(), JPH::EActivation::DontActivate);
            block.anchorBody = anchorBody->GetID();
            block.hasAnchor = true;
        }
    }

    const float totalArea = width * height;
    std::vector<JPH::Body *> createdBodies;
    createdBodies.reserve(fragments.size());

    for (const FracturedBlockFragmentGeometry &geom : fragments)
    {
        std::vector<JPH::Vec3> hullPoints;
        hullPoints.reserve(geom.frontFace.size() * 2);
        const float halfThickness = 0.5f * thickness;
        for (const glm::vec2 &p : geom.frontFace)
            hullPoints.push_back(JPH::Vec3(p.x, p.y, -halfThickness));
        for (const glm::vec2 &p : geom.frontFace)
            hullPoints.push_back(JPH::Vec3(p.x, p.y, halfThickness));

        JPH::ConvexHullShapeSettings hullSettings(hullPoints.data(), int(hullPoints.size()), 0.0f);
        JPH::ShapeSettings::ShapeResult hullResult = hullSettings.Create();
        if (hullResult.HasError())
        {
            std::cerr << "GenerateFracturedBlock hull error: " << hullResult.GetError().c_str() << "\n";
            continue;
        }

        const float area = std::abs(Block_PolygonSignedArea(geom.frontFace));
        const float fragmentMass = std::max(0.1f, settings.totalMass * (area / std::max(totalArea, 1.0e-4f)));
        JPH::BodyCreationSettings bodySettings(
            hullResult.Get(),
            ToJolt(settings.center + geom.localOffset),
            JPH::Quat::sIdentity(),
            JPH::EMotionType::Dynamic,
            Layers::DYNAMIC
        );
        bodySettings.mFriction = settings.friction;
        bodySettings.mRestitution = settings.restitution;
        bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateMassAndInertia;
        bodySettings.mMassPropertiesOverride.mMass = fragmentMass;
        bodySettings.mInertiaMultiplier = 1.0f;

        JPH::Body *body = iface.CreateBody(bodySettings);
        if (body == nullptr)
            continue;

        iface.AddBody(body->GetID(), JPH::EActivation::DontActivate);
        const JPH::BodyID bodyId = body->GetID();
        block.fragmentBodies.push_back(bodyId);
        createdBodies.push_back(body);

        if (block.hasAnchor && anchorBody != nullptr)
        {
            JPH::FixedConstraintSettings fixed;
            fixed.mSpace = JPH::EConstraintSpace::WorldSpace;
            fixed.mAutoDetectPoint = true;
            JPH::Constraint *constraint = fixed.Create(*anchorBody, *body);
            if (constraint != nullptr)
            {
                g_JoltPhysicsInternal.mPhysicsSystem->AddConstraint(constraint);
                block.constraints.push_back(constraint);
            }
        }
        else if (createdBodies.size() >= 2)
        {
            JPH::FixedConstraintSettings fixed;
            fixed.mSpace = JPH::EConstraintSpace::WorldSpace;
            fixed.mAutoDetectPoint = true;
            JPH::Constraint *constraint =
                fixed.Create(*createdBodies.front(), *createdBodies.back());
            if (constraint != nullptr)
            {
                g_JoltPhysicsInternal.mPhysicsSystem->AddConstraint(constraint);
                block.constraints.push_back(constraint);
            }
        }

        if (outFragments != nullptr)
        {
            outFragments->push_back(geom);
        }
    }
}

void Physics::ClearFracturedBlock()
{
    ClearFracturedBlockInternal();
}

bool Physics::HasFracturedBlock() const
{
    return !g_JoltPhysicsInternal.fracturedBlock.fragmentBodies.empty();
}

bool Physics::IsFracturedBlockBroken() const
{
    return g_JoltPhysicsInternal.fracturedBlock.broken;
}

int Physics::GetFracturedBlockFragmentCount() const
{
    return int(g_JoltPhysicsInternal.fracturedBlock.fragmentBodies.size());
}

int Physics::GetFracturedBlockVariantIndex() const
{
    return g_JoltPhysicsInternal.fracturedBlock.variantIndex;
}

int Physics::GetFracturedBlockBallContactCount() const
{
    return g_JoltPhysicsInternal.fracturedBlock.ballContactCount;
}

int Physics::GetFracturedBlockBallFirstContactCount() const
{
    return g_JoltPhysicsInternal.fracturedBlock.ballFirstContactCount;
}

bool Physics::GetFracturedBlockFragmentMatrix(int index, glm::mat4 &outMatrix) const
{
    const auto &bodies = g_JoltPhysicsInternal.fracturedBlock.fragmentBodies;
    if (index < 0 || index >= int(bodies.size()) || g_JoltPhysicsInternal.mPhysicsSystem == nullptr)
        return false;

    auto &iface = g_JoltPhysicsInternal.mPhysicsSystem->GetBodyInterfaceNoLock();
    outMatrix = ToGlm(iface.GetWorldTransform(bodies[size_t(index)]));
    return true;
}
