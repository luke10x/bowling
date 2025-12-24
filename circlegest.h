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
    int visited[8];

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
            input *= 5.0f;

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
    
        this->visited[sector] = 1;
        int sum = 0;
        for (int i = 0; i < 8; i++) {
            sum += this->visited[i];
        }
        return sum;
    }

    void resetCircle() {
        this->ndc = glm::vec2(0.0f);
        for (int i = 0; i < 8; i++) {
            this->visited[i] = 0;
        }
    }

    void initDefaultCircleShaderProgram()
    {
        this->initCircleShaderProgram(CIRCLE_VERTEX_SHADER, CIRCLE_FRAGMENT_SHADER);
    }

    void initCircleShaderProgram(
        const char *vertexShaderText,
        const char *fragmentShaderText);



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
out vec4 fragColor;

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

    // Compute the small circle
    float distSmall = distance(fragCoord, u_smallCentre);
    if (distSmall < u_smallRadius) {
        color = mix(color, u_smallColour, 0.75);
    }

    fragColor = color;
}
    )";

void Circle::initCircleShaderProgram(
    const char *vertexShaderText,
    const char *fragmentShaderText)
{
    this->id = vtx::createShaderProgram(
        vertexShaderText, fragmentShaderText);

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


void Circle::renderCircle(int screenWidth, int screenHeight)
{
    this->screenWidth = screenWidth;
    this->screenHeight = screenHeight;
    this->set_coords(this->ndc.x, this->ndc.y);


    glUseProgram(this->id);

    // clang-format off
    glUniform1f(glGetUniformLocation(this->id, "u_screenWidth"), screenWidth);
    glUniform1f(glGetUniformLocation(this->id, "u_screenHeight"), screenHeight);
    glUniform2f(glGetUniformLocation(this->id, "u_bigCentre"), bigCentre.x, bigCentre.y);
    glUniform2f(glGetUniformLocation(this->id, "u_smallCentre"), smallCentre.x, smallCentre.y);
    glUniform1f(glGetUniformLocation(this->id, "u_bigRadius"), this->settings.bigRadius);
    glUniform1f(glGetUniformLocation(this->id, "u_smallRadius"), this->settings.smallRadius);
    glUniform4f(glGetUniformLocation(this->id, "u_bigColour"), 1.0, 1.0, 1.0, 0.5);
    glUniform4f(glGetUniformLocation(this->id, "u_smallColour"), 1.0, 1.0, 1.0, 0.7);
    // clang-format on

    glBindVertexArray(this->VAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
