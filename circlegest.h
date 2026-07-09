#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "framework/boot.h"
#include "framework/gl_util.h"

#include <cmath>
#include <iostream>

struct CircleSettings
{
    bool movableMode = false;
    bool superMacMode = false;

    float bigRadius = 80.0f;
    float smallRadius = 30.0f;

    float catchupSpeed = 3.0f;
    float minY = 0.77f; // do not react to touch bellow this value (or above in screen )
    float spinWhirlpoolSpeedScale = 0.015625f; // Raise/lower to tune the spin overlay phase speed.
    float spinWhirlpoolEyeRadiusScale = 0.80f;
    float spinWhirlpoolOuterRadiusScale = 2.85f;

};

struct Circle
{
    static const char *CIRCLE_VERTEX_SHADER;
    static const char *CIRCLE_FRAGMENT_SHADER;

    GLuint id;
    GLuint VAO;
    glm::vec2 ndc; // x and y in [-1.0 .. 1.0]
    float whirlpoolTime = 0.0f;
    float whirlpoolOuterScale = 1.0f;
    float whirlpoolOpacityScale = 1.0f;
    float whirlpoolVisualSpinAngle = 0.0f;
    float whirlpoolVisualSpinVelocity = 0.0f;
    float lastScreenSpinAngle = 0.0f;
    bool hasLastScreenSpinAngle = false;



    static constexpr int SECTOR_COUNT = 8;

    int lastSector = -1;
    int currentSector = -1;
    // +1 = clockwise, -1 = anticlockwise, 0 = undecided
    int direction = 0;
    // linear progress (can exceed 8 if enabled)
    int progress = 0;
    // configuration
    bool allowMultipleCircles = false;

    void set_coords(float x, float y) {
        // x, and y should be in range -1 .. 1
        this->bigCentre.x = this->screenWidth * 0.5f;
        this->bigCentre.y = this->screenHeight *  0.25f;
        // std::cerr << "big centre x = " << bigCentre.x << " y=" << bigCentre.y << std::endl;

        this->smallCentre.x = this->bigCentre.x - this->settings.bigRadius * x;
        this->smallCentre.y = this->bigCentre.y +
         this->settings.bigRadius * y;
    }
    int moveCircle(glm::vec2 input, float deltaTime)
    {
        input *= 5.0f; // Amp a bit

        input.x = glm::min(1.0f, input.x);
        input.y = glm::min(1.0f, input.y);

        this->ndc.x -= input.x;
        this->ndc.y -= input.y;

        float mag = glm::length(this->ndc);

        // Step 2: If input is already 0, no change
        if (mag <= 1e-6f) {
            this->ndc.x = 0.0f;
            this->ndc.y = 0.0f;
        } else {
            // Normalize the input vector
            float normX = this->ndc.x / mag;
            float normY = this->ndc.y / mag;

            this->ndc.x = normX;
            this->ndc.y = normY;
        }

        float angle = atan2(ndc.y, ndc.x); // returns radians, [-π, π]
        if (angle < 0) angle += 2.0f * glm::pi<float>();
        int sector = int(angle / (2.0f * glm::pi<float>() * 0.125f)); // or angle / (π/4)
        // this->visited[sector] = 1;


        this->updateSector(sector);

        return this->progress;
    }

    void resetCircle() {
        this->ndc = glm::vec2(0.0f);
        // for (int i = 0; i < SECTOR_COUNT; i++) {
        //     this->visited[i] = 0;
        // }
        this->currentSector = -1;
        this->lastSector = -1;
        // +1 = clockwise, -1 = anticlockwise, 0 = not decided yet
        this->direction = 0;
        this->progress = 0;
        this->whirlpoolOuterScale = 1.0f;
        this->whirlpoolOpacityScale = 1.0f;
        this->whirlpoolVisualSpinAngle = 0.0f;
        this->whirlpoolVisualSpinVelocity = 0.0f;
        this->hasLastScreenSpinAngle = false;

        // this->visitedCount = 0;
    }

    void loadCircleShaderProgram()
    {
        this->id = vtx::createShaderProgram(
            CIRCLE_VERTEX_SHADER, CIRCLE_FRAGMENT_SHADER);
    }

    void initCircleThing()
    {
        this->loadCircleShaderProgram();

        float quadVertices[] = {
            -1.0f, -1.0f, // one corner
            1.0f, -1.0f,  // another corner
            -1.0f, 1.0f,  // another
            1.0f, 1.0f    // fourth
        };

        GLuint VBO;
        glGenVertexArrays(1, &this->VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(this->VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(0);
    }

    void renderCircle(
        int screenWidth,
        int screenHeight,
        float spinIntensity = 0.0f,
        float spinDir = 1.0f,
        float deltaTime = 0.0f,
        float screenSpinAngle = 0.0f
    );

    CircleSettings settings;

    float baseX;
    float baseY;
    float targetX;
    float targetY;
    float rawTargetY;

    bool mouseDown;
    int screenHeight;
    int screenWidth;
    // These are supposed to be used for displaying
    // and are pixel coords
    glm::vec2 thrust = glm::vec2(0.0f);
    glm::vec2 bigCentre;
    glm::vec2 smallCentre;

    int forwardDistance(int from, int to)
    {
        return (to - from + SECTOR_COUNT) % SECTOR_COUNT;
    }

    int backwardDistance(int from, int to)
    {
        return (from - to + SECTOR_COUNT) % SECTOR_COUNT;
    }
    
    int sectorDelta(int from, int to)
    {
        int d = to - from;

        if (d >  SECTOR_COUNT / 2) d -= SECTOR_COUNT;
        if (d < -SECTOR_COUNT / 2) d += SECTOR_COUNT;

        return d; // ∈ { -1, 0, +1 } in practice
    }

    void updateSector(int newSector)
    {
        if (lastSector == -1)
        {
            lastSector = newSector;
            currentSector = newSector;
            progress = 0;
            return;
        }

        int step = sectorDelta(currentSector, newSector);

        // No movement
        if (step == 0)
            return;

        // Lock direction on first real movement
        if (direction == 0)
            direction = (step > 0) ? +1 : -1;

        // ---- Moving forward ----
        if (step == direction)
        {
            if (allowMultipleCircles || progress < SECTOR_COUNT)
            {
                progress++;
            }
        }
        // ---- Moving backward ----
        else if (step == -direction)
        {
            // Only allow undoing existing progress
            if (progress > 0)
            {
                progress--;
            }
        }

        if (progress == 0) {
            // Reset if come back to 0, so we can start again
            this->currentSector = -1;
            this->lastSector = -1;
            // +1 = clockwise, -1 = anticlockwise, 0 = not decided yet
            this->direction = 0;
            return;
        }

        // Any other case is ignored

        lastSector = currentSector;
        currentSector = newSector;
    }
    int wrapSector(int s)
    {
        return (s % SECTOR_COUNT + SECTOR_COUNT) % SECTOR_COUNT;
    }

    int getFirstSector()
    {
        // No movement or no direction yet
        if (direction == 0 || progress == 0)
            return currentSector;

        // Full circle (or more)
        if (progress >= SECTOR_COUNT)
        {
            return wrapSector(currentSector + direction);
        }

        // Partial progress: walk back exactly `progress` steps
        return wrapSector(currentSector - progress * direction);
    }

    float sectorToRadians(int sector)
    {
        constexpr float TWO_PI = glm::two_pi<float>();
        float sectorAngle = TWO_PI / SECTOR_COUNT;

        return (sector + 0.5f - 0.5f) * sectorAngle;
    } 
};

const char *Circle::CIRCLE_VERTEX_SHADER =
    GLSL_VERSION
    R"(
    layout (location = 0) in vec2 a_position; // Fullscreen quad positions
    out vec2 v_texCoord; // Pass to fragment shader

    void main() {
        v_texCoord = (a_position + 1.0) * 0.5; // Map [-1,1] to [0,1]
        gl_Position = vec4(a_position, -1.0, 1.0);
    }
    )";
const char *Circle::CIRCLE_FRAGMENT_SHADER =
    GLSL_VERSION
    R"(
    precision mediump float;

    in vec2 v_texCoord; // Interpolated from vertex shader
    uniform vec2 u_bigCentre;
    uniform vec2 u_smallCentre;
    uniform float u_bigRadius;
    uniform float u_smallRadius;
    uniform vec4 u_bigColour;
    uniform vec4 u_smallColour;

    uniform float u_screenWidth;
    uniform float u_screenHeight;
    uniform float u_fromAngle;
    uniform float u_toAngle;
    uniform float u_dir;
    uniform float u_time;
    uniform float u_spinIntensity;
    uniform float u_spinDir;
    uniform float u_screenSpinAngle;
    uniform float u_whirlpoolEyeRadiusScale;
    uniform float u_whirlpoolOuterRadiusScale;
    uniform float u_whirlpoolOuterScale;
    uniform float u_whirlpoolOpacityScale;
    out vec4 fragColor;

    const float PI = 3.14159265358979323846;
    const float TWO_PI = 6.28318530718;

    float normaliseAngle(float a)
    {
        a = mod(a, TWO_PI);
        return (a < 0.0) ? a + TWO_PI : a;
    }
    bool angleInArc(float angle, float fromA, float toA)
    {
        angle = normaliseAngle(angle);
        fromA = normaliseAngle(fromA);
        toA   = normaliseAngle(toA);

        // If from <= to → simple interval
        if (fromA <= toA)
            return angle >= fromA && angle <= toA;

        // If from > to → wrap-around interval
        return angle >= fromA || angle <= toA;
    }

    float ringBand(float value, float center, float halfWidth)
    {
        return 1.0 - smoothstep(halfWidth * 0.45, halfWidth, abs(value - center));
    }

    float signedAngleDelta(float a, float b)
    {
        return atan(sin(a - b), cos(a - b));
    }

    vec3 spectralColor(float x)
    {
        return 0.58 + 0.42 * cos(TWO_PI * (x + vec3(0.00, 0.33, 0.67)));
    }

    void main() {
        vec2 fragCoord = v_texCoord * vec2(u_screenWidth, u_screenHeight); // Example screen resolution
        vec4 color = vec4(0.0);

        // Compute the big circle
        float distBig = distance(fragCoord, u_bigCentre);
        if (distBig < u_bigRadius) {
            // float gradient = 1.0 - distBig / u_bigRadius;
            // gradient = smoothstep(0.4, 1.0, gradient);
            if ((distBig / u_bigRadius) > 0.8) {
                color = mix(color, u_bigColour, 0.75);
            }
        }

        if (distBig < u_bigRadius)
        {
            float ring = distBig / u_bigRadius;

            if (ring > 0.8)
            {
                float angle = atan(
                    u_bigCentre.y - fragCoord.y,  // Y flipped ✔
                    fragCoord.x - u_bigCentre.x
                );
                angle += PI;


                bool inArc = angleInArc(angle, u_fromAngle, u_toAngle);
                if (u_dir < 0.0) {
                    inArc = !inArc;
                }
                if (inArc)
                {
                    color = mix(color, u_smallColour, 0.75);
                }
            }
        }

        float spinPower = clamp(u_spinIntensity, 0.0, 1.0);
        float eyeRadius = max(0.25, u_whirlpoolEyeRadiusScale);
        float outerRadius = max(eyeRadius + 0.10, u_whirlpoolOuterRadiusScale);
        if (spinPower > 0.001 && distBig < u_bigRadius * outerRadius)
        {
            float r = distBig / max(u_bigRadius, 1.0);
            float angle = atan(
                u_bigCentre.y - fragCoord.y,
                fragCoord.x - u_bigCentre.x
            ) - u_screenSpinAngle;

            float dir = (u_spinDir < 0.0) ? -1.0 : 1.0;
            float cycloneR = clamp((r - eyeRadius) / (outerRadius - eyeRadius), 0.0, 1.0);
            float t = u_time * (1.5 + spinPower * 8.0);
            float outerFade = pow(1.0 - smoothstep(0.50, 1.00, cycloneR), 1.35);
            float envelope = smoothstep(0.00, 0.10, cycloneR) * outerFade;

            vec3 whirlColor = spectralColor(u_time * 0.08 + cycloneR * 0.72);
            float expanded01 = smoothstep(1.04, 1.50, u_whirlpoolOuterScale);
            whirlColor = mix(whirlColor, vec3(1.0), expanded01 * 0.28);

            float blades = 0.0;
            float brightHeads = 0.0;
            for (int i = 0; i < 6; ++i)
            {
                float fi = float(i);
                float armAngle = fi * TWO_PI / 6.0 + 0.12 * sin(fi * 7.13);
                float curve = pow(cycloneR, 0.72) * (5.8 + spinPower * 4.2);
                float bladeAngle = armAngle + dir * (curve - t);
                float d = signedAngleDelta(angle, bladeAngle);

                // Only the trailing side of each curved blade is broad. That makes the
                // whirl's silhouette mirror when the ball spin changes direction.
                float trailingSide = dir * d;
                float leadingSide = max(0.0, -trailingSide);
                float trailingWidth = mix(0.16, 0.62, cycloneR) * (0.80 + spinPower * 0.55);
                float leadingWidth = 0.075 + spinPower * 0.055;
                float trailingTail = smoothstep(-0.060, 0.060, trailingSide) *
                                     (1.0 - smoothstep(trailingWidth * 0.36, trailingWidth, trailingSide));
                float leadingCut = 1.0 - smoothstep(leadingWidth * 0.28, leadingWidth, leadingSide);
                float radialFade = envelope;
                float blade = max(trailingTail, leadingCut * 0.48) * radialFade;

                float headR = 0.70 + 0.10 * sin(fi * 3.91);
                float head = (1.0 - smoothstep(0.060, 0.180, abs(d))) *
                             (1.0 - smoothstep(0.075, 0.210, abs(cycloneR - headR)));
                blades = max(blades, blade);
                brightHeads = max(brightHeads, head);
            }

            float whirl = (blades * 0.95 + brightHeads * 1.25) * spinPower;
            float alpha = clamp(whirl * (0.10 + spinPower * 0.26), 0.0, 0.42) * u_whirlpoolOpacityScale;
            float oldAlpha = color.a;
            color.rgb = mix(whirlColor, color.rgb, oldAlpha);
            color.a = max(oldAlpha, alpha);
        }

        // Compute the small circle
        float distSmall = distance(fragCoord, u_smallCentre);
        if (distSmall < u_smallRadius) {
            // color = mix(color, u_smallColour, 0.75);
        }

        fragColor = color;
    }
    )";

void Circle::renderCircle(
    int screenWidth,
    int screenHeight,
    float spinIntensity,
    float spinDir,
    float deltaTime,
    float screenSpinAngle
)
{
    this->screenWidth = screenWidth;
    this->screenHeight = screenHeight;
    this->set_coords(this->ndc.x, this->ndc.y);
    const float safeDeltaTime = glm::clamp(deltaTime, 0.0f, 0.05f);
    this->whirlpoolTime += safeDeltaTime *
                            glm::max(0.0f, this->settings.spinWhirlpoolSpeedScale);
    float rawScreenSpinDelta = 0.0f;
    if (this->hasLastScreenSpinAngle)
    {
        rawScreenSpinDelta = screenSpinAngle - this->lastScreenSpinAngle;
        if (std::fabs(rawScreenSpinDelta) > glm::two_pi<float>())
        {
            rawScreenSpinDelta = 0.0f;
        }
    }
    else
    {
        this->whirlpoolVisualSpinAngle = screenSpinAngle;
    }
    const float screenSpinDelta = std::fabs(rawScreenSpinDelta);
    this->lastScreenSpinAngle = screenSpinAngle;
    this->hasLastScreenSpinAngle = true;

    if (screenSpinDelta > 0.0025f && safeDeltaTime > 1e-4f)
    {
        const float measuredVelocity = rawScreenSpinDelta / safeDeltaTime;
        const float velocityEase = 1.0f - std::exp(-safeDeltaTime * 18.0f);
        this->whirlpoolVisualSpinVelocity +=
            (measuredVelocity - this->whirlpoolVisualSpinVelocity) * velocityEase;
        this->whirlpoolVisualSpinAngle += rawScreenSpinDelta;
    }
    else
    {
        this->whirlpoolVisualSpinAngle += this->whirlpoolVisualSpinVelocity * safeDeltaTime;
        this->whirlpoolVisualSpinVelocity *= std::exp(-safeDeltaTime * 1.15f);
        if (std::fabs(this->whirlpoolVisualSpinVelocity) < 0.01f)
        {
            this->whirlpoolVisualSpinVelocity = 0.0f;
        }
    }

    const float targetOuterScale = (screenSpinDelta > 0.0025f) ? 1.5f : 1.0f;
    const float easeSpeed = (targetOuterScale > this->whirlpoolOuterScale) ? 10.0f : 3.0f;
    const float ease = 1.0f - std::exp(-safeDeltaTime * easeSpeed);
    this->whirlpoolOuterScale += (targetOuterScale - this->whirlpoolOuterScale) * ease;
    const float targetOpacityScale = (screenSpinDelta > 0.0025f) ? 1.0f : 0.5f;
    const float opacityEaseSpeed = (targetOpacityScale > this->whirlpoolOpacityScale) ? 10.0f : 2.5f;
    const float opacityEase = 1.0f - std::exp(-safeDeltaTime * opacityEaseSpeed);
    this->whirlpoolOpacityScale += (targetOpacityScale - this->whirlpoolOpacityScale) * opacityEase;

    GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);


    glUseProgram(this->id);

    // clang-format off
    glUniform1f(glGetUniformLocation(this->id, "u_fromAngle"), this->sectorToRadians(this->getFirstSector()));
    glUniform1f(glGetUniformLocation(this->id, "u_toAngle"), this->sectorToRadians(this->currentSector));
    glUniform1f(glGetUniformLocation(this->id, "u_dir"), this->direction);
    glUniform1f(glGetUniformLocation(this->id, "u_time"), this->whirlpoolTime);
    glUniform1f(glGetUniformLocation(this->id, "u_spinIntensity"), glm::clamp(spinIntensity, 0.0f, 1.0f));
    glUniform1f(glGetUniformLocation(this->id, "u_spinDir"), spinDir < 0.0f ? -1.0f : 1.0f);
    glUniform1f(glGetUniformLocation(this->id, "u_screenSpinAngle"), this->whirlpoolVisualSpinAngle);
    glUniform1f(glGetUniformLocation(this->id, "u_whirlpoolEyeRadiusScale"), this->settings.spinWhirlpoolEyeRadiusScale);
    glUniform1f(glGetUniformLocation(this->id, "u_whirlpoolOuterRadiusScale"), this->settings.spinWhirlpoolOuterRadiusScale * this->whirlpoolOuterScale / 1.7f);
    glUniform1f(glGetUniformLocation(this->id, "u_whirlpoolOuterScale"), this->whirlpoolOuterScale);
    glUniform1f(glGetUniformLocation(this->id, "u_whirlpoolOpacityScale"), this->whirlpoolOpacityScale);
    glUniform1f(glGetUniformLocation(this->id, "u_screenWidth"), screenWidth);
    glUniform1f(glGetUniformLocation(this->id, "u_screenHeight"), screenHeight);
    glUniform2f(glGetUniformLocation(this->id, "u_bigCentre"), bigCentre.x, bigCentre.y);
    glUniform2f(glGetUniformLocation(this->id, "u_smallCentre"), smallCentre.x, smallCentre.y);
    glUniform1f(glGetUniformLocation(this->id, "u_bigRadius"), this->settings.bigRadius);
    glUniform1f(glGetUniformLocation(this->id, "u_smallRadius"), this->settings.smallRadius);
    glUniform4f(glGetUniformLocation(this->id, "u_bigColour"), 1.0, 1.0, 1.0, 0.5);
    glUniform4f(glGetUniformLocation(this->id, "u_smallColour"), 1.0, 0.0, 0.0, 0.7);
    // clang-format on

    glBindVertexArray(this->VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    if (depthWasEnabled)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    if (!blendWasEnabled)
        glDisable(GL_BLEND);
}
