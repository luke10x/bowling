#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include "framework/boot.h"
#include "framework/gl_util.h"

#include <iostream>

struct CircleSettings
{
    bool movableMode = false;
    bool superMacMode = false;

    float bigRadius = 80.0f;
    float smallRadius = 30.0f;

    float catchupSpeed = 3.0f;
    float minY = 0.77f; // do not react to touch bellow this value (or above in screen )

};

struct Circle
{
    static const char *CIRCLE_VERTEX_SHADER;
    static const char *CIRCLE_FRAGMENT_SHADER;

    GLuint id;
    GLuint VAO;
    glm::vec2 ndc; // x and y in [-1.0 .. 1.0]



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

        // int sum = 0;
        // for (int i = 0; i < 8; i++) {
        //     sum += this->visited[i];
        // }
        // return sum;
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

    void renderCircle(int screenWidth, int screenHeight);

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
            progress = 1;
            return;
        }

        int step = sectorDelta(currentSector, newSector);

        // No movement
        if (step == 0)
            return;

        std::cerr << "sector Delta " << step << std::endl;

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
            if (progress > 1)
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
            this->progress = 0;
        }

        // Any other case is ignored

        lastSector = currentSector;
        currentSector = newSector;
    }
    int wrapSector(int s)
    {
        return (s % SECTOR_COUNT + SECTOR_COUNT) % SECTOR_COUNT;
    }

    int getFirstSector(
        // int currentSector,
        // int progress,
        // int direction,
        // bool allowMultipleCircles
    )
    {
        if (direction == 0 || progress <= 1)
            return currentSector;

        // Full circle reached?
        bool fullCircle = progress >= SECTOR_COUNT;

        if (fullCircle)
        {
            // First step is ahead of current
            return wrapSector(currentSector + direction);
        }

        // Partial progress: look back
        int offset = (progress - 1) * direction;
        return wrapSector(currentSector - offset);
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
    out vec4 fragColor;
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
                float angle = atan(fragCoord.y - u_bigCentre.y,
                                fragCoord.x - u_bigCentre.x);

                if (angleInArc(angle, u_fromAngle, u_toAngle))
                {
                    color = mix(color, u_bigColour, 0.75);
                }
            }
        }

        // Compute the small circle
        float distSmall = distance(fragCoord, u_smallCentre);
        if (distSmall < u_smallRadius) {
            color = mix(color, u_smallColour, 0.75);
        }

        fragColor = color;
    }
    )";

void Circle::renderCircle(int screenWidth, int screenHeight)
{
    this->screenWidth = screenWidth;
    this->screenHeight = screenHeight;
    this->set_coords(this->ndc.x, this->ndc.y);


    glUseProgram(this->id);

    // clang-format off
    glUniform1f(glGetUniformLocation(this->id, "u_fromAngle"), this->sectorToRadians(this->getFirstSector()));
    glUniform1f(glGetUniformLocation(this->id, "u_toAngle"), this->sectorToRadians(this->currentSector));
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
}
