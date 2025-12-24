#pragma once

#include <glm/glm.hpp>
#include "framework/boot.h"
#include "framework/gl_util.h"

// #include "swipedetection.h"
#include <iostream>

struct JoystickSettings
{
    bool movableMode = false;
    bool superMacMode = false;

    float bigRadius = 80.0f;
    float smallRadius = 30.0f;

    float catchupSpeed = 3.0f;
    float minY = 0.77f; // do not react to touch bellow this value (or above in screen )

};

struct Joystick
{
    static const char *JOYSTICK_VERTEX_SHADER;
    static const char *JOYSTICK_FRAGMENT_SHADER;

    GLuint id;
    GLuint VAO;
    glm::vec2 ndc; // x and y in [-1.0 .. 1.0]

    void set_coords(float x, float y) {
        // x, and y should be in range -1 .. 1
        this->bigCentre.x = this->screenWidth * 0.5f;
        this->bigCentre.y = this->screenHeight *  0.25f;
        // std::cerr << "big centre x = " << bigCentre.x << " y=" << bigCentre.y << std::endl;

        this->smallCentre.x = this->bigCentre.x - this->settings.bigRadius * x;
        this->smallCentre.y = this->bigCentre.y +
         this->settings.bigRadius * y;
    }
    void moveJoystick(glm::vec2 input, float deltaTime)
    {
        float ampX = 2.1f;
        float ampY = 3.0f;
        // input *= 3.0f;
        input.x = input.x * ampX;
        input.y = input.y * ampY;

        // input.x = input.x/deltaTime;
        // input.y = input.y/deltaTime;
        if (input.x != 0.0f || input.y != 0.0f) {
            // std::cerr << "spped input.x = " << input.x << " input.y = " << input.y << std::endl;
        }

        input.x = glm::min(1.0f, input.x);
        input.y = glm::min(1.0f, input.y);

        this->ndc.x -= input.x;
        this->ndc.y -= input.y;

        float mag = glm::length(this->ndc);

        // Step 2: If input is already 0, no change
        if (mag <= 1e-6f) {
            this->ndc.x = 0.0f;
            this->ndc.y = 0.0f;
        } else if(mag > 1.0f) {
            // Normalize the input vector
            float normX = this->ndc.x / mag;
            float normY = this->ndc.y / mag;

            this->ndc.x = normX;
            this->ndc.y = normY;
        }


        // ==== should 
    }

    void resetJoystick() {
        this->ndc = glm::vec2(0.0f);
    }

    void initDefaultFlatShaderProgram()
    {
        this->initFlatShaderProgram(
            JOYSTICK_VERTEX_SHADER, JOYSTICK_FRAGMENT_SHADER);
    }

    void initFlatShaderProgram(
        const char *vertexShaderText,
        const char *fragmentShaderText);



    void renderJoystick(int screenWidth, int screenHeight);

    JoystickSettings settings;

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

    bool kbdUpPressed = false;
    bool kbdDownPressed = false;
    bool kbdLeftPressed = false;
    bool kbdRightPressed = false;
    bool shouldUseKbdThrust = false;

    void processSdlEvent(SDL_Event &event, int screenWidth, int screenHeight)
    {

        // bool wasAnyKeyPressedBefore = upPressed || downPressed || leftPressed || rightPressed;
        if (event.type == SDL_WINDOWEVENT)
        {
            if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                screenHeight = screenHeight;
                screenWidth = screenWidth;

                mouseDown = false;
            }
        }

        if (event.type == SDL_KEYDOWN)
        {
            switch (event.key.keysym.sym)
            {
            case SDLK_UP:
            case SDLK_w:
                this->kbdUpPressed = true;
                break;
            case SDLK_DOWN:
            case SDLK_s:
                this->kbdDownPressed = true;
                break;
            case SDLK_LEFT:
            case SDLK_a:
                this->kbdLeftPressed = true;
                break;
            case SDLK_RIGHT:
            case SDLK_d:
                this->kbdRightPressed = true;
                break;
            }
        }
        if (event.type == SDL_KEYUP)
        {
            switch (event.key.keysym.sym)
            {
            case SDLK_UP:
            case SDLK_w:
                this->kbdUpPressed = false;
                break;
            case SDLK_DOWN:
            case SDLK_s:
                this->kbdDownPressed = false;
                break;
            case SDLK_LEFT:
            case SDLK_a:
                this->kbdLeftPressed = false;
                break;
            case SDLK_RIGHT:
            case SDLK_d:
                this->kbdRightPressed = false;
                break;
            }
        }

        glm::vec2 newKbdThrust = glm::vec2(0.0f);
        shouldUseKbdThrust = false;
        if (this->kbdUpPressed)
        {
            newKbdThrust.y += 1.0f;
            shouldUseKbdThrust = true;
        }
        if (this->kbdDownPressed)
        {
            newKbdThrust.y -= 1.0f;
            shouldUseKbdThrust = true;
        }
        if (this->kbdLeftPressed)
        {
            newKbdThrust.x -= 1.0f;
            shouldUseKbdThrust = true;
        }
        if (this->kbdRightPressed)
        {
            newKbdThrust.x += 1.0f;
            shouldUseKbdThrust = true;
        }

        if (shouldUseKbdThrust)
        {
            this->thrust = newKbdThrust;
            baseX = 0.5f;
            baseY = 0.75f;
            targetX = 0.5f;
            targetY = 0.75f;
            rawTargetY = 0.57;
            return;
        }

        float x;
        float y;
        if (event.type == SDL_KEYDOWN)
        {
            if (event.key.repeat == 0)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_LCTRL:
                case SDLK_RCTRL:
                case SDLK_SPACE:
                    mouseDown = true;
                    int mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);
                    std::cout << "Mouse Position: (" << mouseX << ", " << mouseY << ")\n";
                    x = static_cast<float>(mouseX) / screenWidth;
                    y = static_cast<float>(mouseY) / screenHeight;
                    goto GESTURE_DOWN;
                    break;
                }
            }
        }

        if (event.type == SDL_KEYUP)
        {
            if (event.key.repeat == 0)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_LCTRL:
                case SDLK_RCTRL:
                case SDLK_SPACE:
                    mouseDown = false;

                    // this->swipe.reset();

                    break;
                }
            }
        }

        // Start gesture
        if (event.type == SDL_FINGERDOWN)
        {
            x = event.tfinger.x;
            y = event.tfinger.y;
            goto GESTURE_DOWN;
        }

        if (event.type == SDL_MOUSEBUTTONDOWN &&
            event.button.button == SDL_BUTTON_LEFT)
        {
            x = static_cast<float>(event.button.x) / screenWidth;
            y = static_cast<float>(event.button.y) / screenHeight;
        GESTURE_DOWN:
            // clamp not to go too much to sides
            if (settings.movableMode)
            {
                x = glm::clamp(x,                                                               // small circle always on screen
                               settings.smallRadius / (float)this->screenWidth,                 // left limit
                               (1.0f - (this->settings.smallRadius / (float)this->screenWidth)) // right limit
                );

                y = glm::clamp(y, this->settings.minY, (1.0f - (this->settings.smallRadius / (float)this->screenHeight)));
            }
            float dx = targetX - baseX;
            float dy = targetY - baseY;

            if (settings.movableMode)
            {
                targetX = x;
                baseX = targetX - dx;

                targetY = y;
                rawTargetY = y;
                baseY = targetY - dy;
            }
            else
            {
            }
        }

        // Continue gesture
        if (event.type == SDL_FINGERMOTION)
        {
            float x = event.tfinger.x;
            float y = event.tfinger.y;
            float originalY = y;

            // difference of X in relative units 0..1
            float dx = (settings.smallRadius) / (float)screenWidth;
            float d = (settings.bigRadius + settings.smallRadius) / screenHeight;
            if (settings.movableMode)
            {
                x = glm::clamp(x, dx, 1.0f - dx);
                y = glm::clamp(y, this->settings.minY - d, 1.0f - (settings.smallRadius / screenHeight));
            }
            targetX = x; // value in range 0 .. 1
            targetY = y;
            rawTargetY = originalY;
        }
        if (event.type == SDL_MOUSEMOTION && (mouseDown || !settings.movableMode))
        {
            float x = static_cast<float>(event.motion.x) / screenWidth;
            float y = static_cast<float>(event.motion.y) / screenHeight;
            float dx = (settings.smallRadius) / screenWidth;
            float d = (settings.bigRadius + settings.smallRadius) / screenHeight;
            float originalY = y;
            if (settings.movableMode)
            {
                x = glm::clamp(x, dx, 1.0f - (dx));
                y = glm::clamp(y, this->settings.minY - d, 1.0f - (settings.smallRadius / screenHeight));
            }
            else
            {
                x *= 5.0f;
                y *= 5.0f;
            }
            // std::cerr << "Clamping rationyyp mouse " << (y / originalY) * (screenHeight / screenWidth ) << std::endl;
            targetX = x;
            targetY = y;
            rawTargetY = originalY;
        }

        // End gesture
        if (event.type == SDL_FINGERUP)
        {

            mouseDown = false;
            return;
        }
        if (event.type == SDL_MOUSEBUTTONUP &&
            event.button.button == SDL_BUTTON_LEFT)
        {
            // this->swipe.reset();

            mouseDown = false;
            return;
        }


        // Set circle sizes depending on touchpad mode
        int smallerEdge = glm::min(this->screenWidth, this->screenHeight);
        if (this->settings.superMacMode)
        {
            // circle is slightly bigger for superMacMode
            this->settings.bigRadius = glm::min(smallerEdge / 5, 100) * 1.25f;
        }
        else
        {
            this->settings.bigRadius = glm::min(smallerEdge / 5, 100);
        }
        this->settings.smallRadius = this->settings.bigRadius / 3;
    }
};

struct FlatShaderProgram
{
};

const char *Joystick::JOYSTICK_VERTEX_SHADER =
    GLSL_VERSION
    R"(
    layout (location = 0) in vec2 a_position; // Fullscreen quad positions
    out vec2 v_texCoord; // Pass to fragment shader

    void main() {
        v_texCoord = (a_position + 1.0) * 0.5; // Map [-1,1] to [0,1]
        gl_Position = vec4(a_position, -1.0, 1.0);
    }
    )";
const char *Joystick::JOYSTICK_FRAGMENT_SHADER =
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

void Joystick::initFlatShaderProgram(
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


void Joystick::renderJoystick(int screenWidth, int screenHeight)
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
