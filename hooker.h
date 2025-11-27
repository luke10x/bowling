#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

struct SpinTracker {
    glm::vec2 lastPos = glm::vec2(0.0f);
    glm::vec2 lastVel = glm::vec2(0.0f);
    float spinSpeed = 0.0f;        // radians per sec
    float curveAccum = 0.0f;      // new member in SpinTracker

};

float computeSpinFromAim(
    SpinTracker &st,
    const glm::vec2 &aimFlatPos,
    float dt,
    float spinGain,
    float damping,    // How fast it decay, lower value spins longer
    float curveDeadZone,   // dull curves ignored
    float consistencyTau, // how long curve must persist seconds
    float sharpnessExp    // >1 = emphasise sharp curves
)
{
    if (dt <= 0.0f)
        return st.spinSpeed;

    // Compute velocity
    glm::vec2 vel = (aimFlatPos - st.lastPos) / dt;

    // Kill spin when player pulls backwards
    if (vel.y > +0.0001f) {
        st.spinSpeed = 0.0f;
        st.curveAccum = 0.0f;      // new member in SpinTracker
        st.lastPos = aimFlatPos;
        st.lastVel = vel;
        return 0.0f;
    }

    // 2D signed curvature
    float curve = st.lastVel.x * vel.y - st.lastVel.y * vel.x;

    float absCurve = fabsf(curve);

    // --- 1. CURVATURE DEAD-ZONE ---
    if (absCurve < curveDeadZone) {
        // decay accumulator gently
        st.curveAccum *= expf(-dt / consistencyTau);
    } else {
        // --- 2. TRACK CONSISTENCY ---
        // If sign flipped, reset accumulator entirely
        if (curve * st.curveAccum < 0.0f)
            st.curveAccum = 0.0f;

        // integrate curvature over time
        st.curveAccum += curve * dt;
    }

    // Update last state
    st.lastPos = aimFlatPos;
    st.lastVel = vel;

    // --- 3. ONLY PRODUCE SPIN IF CONSISTENCY IS HIGH ---
    float consistency = fabsf(st.curveAccum);

    if (consistency < curveDeadZone) {
        // not enough consistent curve → spin only damps
        st.spinSpeed *= expf(-damping * dt);
        return st.spinSpeed;
    }

    // --- 4. CONVERT CONSISTENT CURVATURE INTO SPIN ---
    float strength = powf(consistency, sharpnessExp); // emphasise sharp radius
    float sign = (st.curveAccum >= 0.0f) ? +1.f : -1.f;

    // Lateral speed matters
    float speed = glm::length(vel);

    float addedSpin = strength * speed * spinGain * sign;
    st.spinSpeed += addedSpin;

    // --- 5. DAMP SPIN ---
    st.spinSpeed *= expf(-damping * dt);

    return st.spinSpeed;
}


float computeSpinSimple(
    SpinTracker &st,
    const glm::vec2 &aimFlatPos,
    float dt,
    float spinGain,
    float damping,
    float sensitivity
)
{
    if (dt <= 0.0f)
        return st.spinSpeed;

    // Velocity (screen-plane movement)
    glm::vec2 vel = (aimFlatPos - st.lastPos) / dt;

    // Kill spin if user drags backwards
    if (vel.y > 0.0001f) {
        st.spinSpeed = 0.0f;
        st.curveAccum = 0.0f;
        st.lastPos = aimFlatPos;
        st.lastVel = vel;
        return 0.0f;
    }

    // 2D signed curvature (simple cross-product)
    float curve = st.lastVel.x * vel.y - st.lastVel.y * vel.x;

    // Absolute curve magnitude
    float absCurve = fabsf(curve);

    // Integrate consistent curve (forget quickly when curve is tiny)
    float forget = expf(-dt * sensitivity);
    if (absCurve < sensitivity)
        st.curveAccum *= forget;
    else
        st.curveAccum += curve * dt;

    // Save state
    st.lastPos = aimFlatPos;
    st.lastVel = vel;

    // If not enough curve history, only apply damping
    float acc = fabsf(st.curveAccum);
    if (acc < sensitivity) {
        st.spinSpeed *= expf(-damping * dt);
        return st.spinSpeed;
    }

    // Convert curvature history into spin
    float speed = glm::length(vel);
    float sign = (st.curveAccum >= 0.f ? 1.f : -1.f);

    // Smooth response
    float addedSpin = spinGain * speed * (acc / (acc + sensitivity)) * sign;

    st.spinSpeed += addedSpin;

    // Apply damping
    st.spinSpeed *= expf(-damping * dt);

    return st.spinSpeed;
}