#pragma once

// dialogbox.h
// Minimal story dialog window with fast "typing" text and optional choice buttons.
//
// Rendering: Clay UI.
// Events: emits integer story events that game.cpp can react to.

#include <stdint.h>
#include <string.h>
#include <algorithm>

#include "clayton/clayton.h"
#include "clayton/clayton_click.h"
#include "clayton/claytheme.h"

#include "storyline.h"

static inline bool Story_SpeakerUsesAngelAvatar(int32_t speaker)
{
    return speaker == SPEAKER_ANGEL;
}

struct DialogBox
{
    bool active = false;
    TxlLanguage language = TXL_LANG_EN_US;
    bool openedThisFrame = false;
    float dialogAppearDelayLeft = 0.0f; // overlay shows immediately; panel appears after delay

    // Dialog owns its Clay click targets (encapsulation; no dependency on WindowStack).
    Clayton_Click optionClicks[4];

    struct Line
    {
        int32_t storyId = 0;
        int32_t speaker = 0;
        const char *text = nullptr;
        int32_t typedChars = 0;
        bool typing = false;
        float preDelayLeft = 0.0f;
    };

    static constexpr int32_t MAX_LINES = 16;
    Line lines[MAX_LINES];
    int32_t lineCount = 0;

    // Active choice panel (separate from message panels).
    int32_t activeChoiceGroup = CHOICE_NONE;

    // Auto-advance chain (next_storyline) after current line finishes typing.
    int32_t pendingAutoNextStoryId = 0;
    float pendingAutoNextTimer = 0.0f;

    // Typing state
    float typeTimer = 0.0f;
    int32_t typedNonWhitespaceThisFrame = 0;

    // Interaction state
    bool waitingChoice = false;
    bool closeRequested = false;

    // One-shot event output for the game
    int32_t emittedEvent = EVENT_NONE;
    // Defer events until the dialog finishes (so actions like opening windows happen after story ends).
    int32_t deferredEventOnClose = EVENT_NONE;

    void open(int32_t storyId)
    {
        const StorylineNode *n = Story_FindNode(storyId);
        if (!n)
            return;
        active = true;
        closeRequested = false;
        emittedEvent = EVENT_NONE;
        deferredEventOnClose = EVENT_NONE;
        dialogAppearDelayLeft = 0.5f;
        lineCount = 0;
        activeChoiceGroup = CHOICE_NONE;
        pendingAutoNextStoryId = 0;
        pendingAutoNextTimer = 0.0f;
        loadNode(*n);
    }

    void close()
    {
        active = false;
        closeRequested = false;
        waitingChoice = false;
        emittedEvent = EVENT_NONE;
        deferredEventOnClose = EVENT_NONE;
        dialogAppearDelayLeft = 0.0f;
        lineCount = 0;
        activeChoiceGroup = CHOICE_NONE;
        pendingAutoNextStoryId = 0;
        pendingAutoNextTimer = 0.0f;
        typeTimer = 0.0f;
    }

    // Close but keep a deferred event for the game to consume next tick.
    void finalizeClose()
    {
        active = false;
        closeRequested = false;
        waitingChoice = false;
        emittedEvent = deferredEventOnClose;
        deferredEventOnClose = EVENT_NONE;
        dialogAppearDelayLeft = 0.0f;
        lineCount = 0;
        activeChoiceGroup = CHOICE_NONE;
        pendingAutoNextStoryId = 0;
        pendingAutoNextTimer = 0.0f;
        typeTimer = 0.0f;
    }

    bool isTypingComplete() const
    {
        if (lineCount <= 0)
            return true;
        const Line &l = lines[lineCount - 1];
        if (!l.text)
            return true;
        return l.typedChars >= (int32_t)strlen(l.text);
    }

    void update(float dt)
    {
        if (!active)
            return;

        if (dialogAppearDelayLeft > 0.0f)
        {
            dialogAppearDelayLeft = std::max(0.0f, dialogAppearDelayLeft - dt);
            // Do not progress typing until panel is visible.
            typedNonWhitespaceThisFrame = 0;
            return;
        }

        // Important: WindowStack currently calls update() before render() each frame.
        // If we start typing immediately, the first frame after opening can already show
        // some characters (especially when deltaTime is large). Force the first rendered
        // frame of each node to show 0 chars.
        if (openedThisFrame)
        {
            openedThisFrame = false;
            return;
        }

        if (lineCount <= 0)
            return;

        Line &cur = lines[lineCount - 1];
        if (!cur.text)
            return;

        typedNonWhitespaceThisFrame = 0;

        // Small pause right before each story starts typing.
        if (cur.preDelayLeft > 0.0f)
        {
            cur.preDelayLeft = std::max(0.0f, cur.preDelayLeft - dt);
            return;
        }

        // Typing for the last line only.
        const int textLen = (int)strlen(cur.text);
        if (cur.typedChars < textLen)
        {
            // Fast "console typing"
            const float cps = 90.0f; // chars per second
            typeTimer += dt;
            int add = (int)(typeTimer * cps);
            if (add > 0)
            {
                // Avoid huge hitches (tab switch / first frame after loading) instantly dumping the whole line.
                // We still catch up over subsequent frames.
                const int maxAddPerFrame = 12;
                add = std::min(add, maxAddPerFrame);
                const int oldChars = cur.typedChars;
                cur.typedChars = (int32_t)std::min<int>(cur.typedChars + add, textLen);
                // Count non-whitespace typed chars for typewriter SFX.
                for (int i = oldChars; i < cur.typedChars; i++)
                {
                    const unsigned char c = (unsigned char)cur.text[i];
                    if (c != ' ' && c != '\n' && c != '\r' && c != '\t')
                        typedNonWhitespaceThisFrame++;
                }
                typeTimer = 0.0f;
            }
        }

        if (cur.typedChars >= textLen)
        {
            // When a line finishes typing, either:
            // - show a choice panel (separate panel below)
            // - or auto-append next_storyline after a short delay
            waitingChoice = (activeChoiceGroup != CHOICE_NONE);
            if (!waitingChoice && pendingAutoNextStoryId != 0)
            {
                pendingAutoNextTimer += dt;
                if (pendingAutoNextTimer >= 0.15f)
                {
                    const int32_t nextId = pendingAutoNextStoryId;
                    pendingAutoNextStoryId = 0;
                    pendingAutoNextTimer = 0.0f;
                    const StorylineNode *n = Story_FindNode(nextId);
                    if (n)
                        loadNode(*n);
                }
            }
        }
    }

    // Consume event in game.cpp by reading and clearing.
    int32_t consumeEvent()
    {
        int32_t e = emittedEvent;
        emittedEvent = EVENT_NONE;
        return e;
    }

    int32_t consumeTypedNonWhitespaceCount()
    {
        const int32_t n = typedNonWhitespaceThisFrame;
        typedNonWhitespaceThisFrame = 0;
        return n;
    }

    int32_t peekTypedNonWhitespaceCount() const { return typedNonWhitespaceThisFrame; }

    // Called by WindowStack when an option is clicked.
    void onSelectOption(const StoryChoiceOption &opt)
    {
        if (!active)
            return;

        if (opt.trigger_event != EVENT_NONE)
            deferredEventOnClose = opt.trigger_event;

        // Choice panel disappears after selection; if goto_storyline exists it replaces it by a new node.
        activeChoiceGroup = CHOICE_NONE;
        waitingChoice = false;

        if (opt.goto_storyline != 0)
        {
            const StorylineNode *n = Story_FindNode(opt.goto_storyline);
            if (n)
                loadNode(*n);
        }
        else
        {
            // No next node: selecting this option closes the dialog.
            closeRequested = true;
        }
    }

    bool processEvent(Clayton *clayton, SDL_Event e)
    {
        if (!active || !clayton)
            return false;

        // Expedite typing: any key press or pointer click while typing instantly reveals the rest
        // of the current line (including skipping the pre-delay).
        // This is global (click anywhere), because the dialog is modal.
        const bool anyKeyDown = (e.type == SDL_KEYDOWN);
        const bool anyPointerDown =
            (e.type == SDL_MOUSEBUTTONDOWN) || (e.type == SDL_FINGERDOWN) || (e.type == SDL_MOUSEWHEEL);
        if (!waitingChoice && (anyKeyDown || anyPointerDown))
        {
            if (dialogAppearDelayLeft > 0.0f)
            {
                // If the panel hasn't appeared yet, treat input as "skip delay".
                dialogAppearDelayLeft = 0.0f;
                openedThisFrame = true; // ensure we still render 0 chars for the first visible frame
                return true;
            }

            if (lineCount > 0)
            {
                Line &cur = lines[lineCount - 1];
                if (cur.text)
                {
                    const int textLen = (int)strlen(cur.text);
                    if (cur.preDelayLeft > 0.0f || cur.typedChars < textLen)
                    {
                        cur.preDelayLeft = 0.0f;
                        cur.typedChars = textLen;
                        typeTimer = 0.0f;
                        typedNonWhitespaceThisFrame = 0;
                        return true;
                    }
                }
            }
        }

        if (waitingChoice)
        {
            const int choiceGroup = activeChoiceGroup;
            const StoryChoiceOption *slots[4] = {nullptr, nullptr, nullptr, nullptr};
                    int slotCount = 0;
                    for (int i = 0; i < STORY_OPTIONS_COUNT && slotCount < 4; i++)
                    {
                        const StoryChoiceOption &opt = STORY_OPTIONS[i];
                        if (opt.choice_id == choiceGroup)
                        {
                            slots[slotCount++] = &opt;
                        }
                    }

            for (int s = 0; s < slotCount; s++)
            {
                if (isClaytonClicked(&optionClicks[s], e))
                {
                    onSelectOption(*slots[s]);
                    return true;
                }
            }
        }

        if (Clay_PointerOver(CLAY_ID("StoryDialogContainer")))
        {
            const bool mouseDown = e.type == SDL_MOUSEBUTTONDOWN;
            const bool mouseUp = e.type == SDL_MOUSEBUTTONUP;
            const bool mouseMove = e.type == SDL_MOUSEMOTION;
            const bool mouseWheel = e.type == SDL_MOUSEWHEEL;
            const bool fingerDown = e.type == SDL_FINGERDOWN;
            const bool fingerUp = e.type == SDL_FINGERUP;
            const bool fingerMove = e.type == SDL_FINGERMOTION;
            if (mouseDown || mouseUp || mouseMove || mouseWheel || fingerDown || fingerUp || fingerMove)
            {
                return true;
            }
        }

        return false;
    }

    void render(Clayton *clayton)
    {
        if (!active)
            return;

        Clay_TextElementConfig titleCfg = CLAY_THEME_TEXT_TITLE;
        Clay_TextElementConfig bodyCfg = CLAY_THEME_TEXT_BODY;
        Clay_TextElementConfig buttonCfg = CLAY_THEME_TEXT_BUTTON;

        // Align the dialog to the portrait middle column, same as window stack windows.
        Clay_BoundingBox rootBox = Clay_GetElementData(CLAY_ID("Root")).boundingBox;
        Clay_BoundingBox portraitBox = Clay_GetElementData(CLAY_ID("Portrait area")).boundingBox;
        const float rootCx = rootBox.x + rootBox.width * 0.5f;
        const float portraitCx = portraitBox.x + portraitBox.width * 0.5f;
        const float topMarginPx = 24.0f;
        const Clay_Vector2 portraitTopOffset = {
            portraitCx - rootCx,
            (portraitBox.y + topMarginPx) - rootBox.y,
        };

        // Dim overlay like window-stack modals (covers full screen).
        CLAY(
            CLAY_ID("StoryDialogDimOverlay"),
            {
                .layout =
                    {
                        .sizing = {.width = CLAY_SIZING_GROW(), .height = CLAY_SIZING_GROW()},
                    },
                .backgroundColor = CLAY_COLOR_WINDOW_STACK_OVERLAY,
                .floating = {
                    .offset = {0},
                    .zIndex = 119,
                    .attachPoints =
                        {CLAY_ATTACH_POINT_CENTER_CENTER, CLAY_ATTACH_POINT_CENTER_CENTER},
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                },
            }
        )
        {
        }

        // Keep overlay up immediately, but delay the panel appearance slightly.
        if (dialogAppearDelayLeft > 0.0f)
            return;

        CLAY(
            CLAY_ID("StoryDialogViewport"),
            {
                .layout =
                    {
                        .sizing = {.width = CLAY_SIZING_FIXED(portraitBox.width),
                                   .height = CLAY_SIZING_FIXED(portraitBox.height)},
                        // Center horizontally within the portrait column; top-align vertically with margin.
                        .childAlignment = {.x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                    },
                .floating = {
                    .offset = portraitTopOffset,
                    .zIndex = 120,
                    .attachPoints =
                        {CLAY_ATTACH_POINT_CENTER_TOP, CLAY_ATTACH_POINT_CENTER_TOP},
                    .attachTo = CLAY_ATTACH_TO_PARENT,
                },
            }
        )
        {
            // Container for hit-testing / pointer capture.
            CLAY(
                CLAY_ID("StoryDialogContainer"),
                {
                    .layout = {
                        .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                        .padding = {0, 0, 0, 0},
                        .childAlignment = {CLAY_ALIGN_X_CENTER, CLAY_ALIGN_Y_TOP},
                        .layoutDirection = CLAY_TOP_TO_BOTTOM,
                    },
                }
            )
            {
                // The dialog window now stacks multiple message panels top->bottom, with an optional
                // choice panel below. A single close button stays available.
                CLAY(CLAY_ID("StoryDialogWindow"), CLAY_THEME_WINDOW_PANEL)
                {
                    ClayArena *arena = &clayton->clayArena;
                    // Messages stack
                    CLAY(
                        CLAY_ID("StoryMessages"),
                        {
                            .layout = {
                                .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_GROW()},
                                .childGap = 10,
                                .layoutDirection = CLAY_TOP_TO_BOTTOM,
                            }
                        }
                    )
                    {
                        for (int32_t i = 0; i < lineCount; i++)
                        {
                            const Line &l = lines[i];
                            if (!l.text)
                                continue;

                            const char *speakerName = Story_SpeakerName(language, l.speaker);

                            // Slightly different tint for player replies.
                            Clay_ElementDeclaration panel = CLAY_THEME_SECTION;
                            if (l.speaker == SPEAKER_MYSELF)
                            {
                                // Brighter than the window bg so "ME" messages have contrast.
                                panel.backgroundColor = (Clay_Color){0.18f, 0.28f, 0.55f, 0.92f};
                            }
                            if (Story_SpeakerUsesAngelAvatar(l.speaker))
                            {
                                panel.layout.childGap = 12;
                                panel.layout.childAlignment = {CLAY_ALIGN_X_LEFT, CLAY_ALIGN_Y_TOP};
                                panel.layout.layoutDirection = CLAY_LEFT_TO_RIGHT;
                            }
                            CLAY(CLAY_IDI("StoryMsgPanel", (int)i), panel)
                            {
                                if (Story_SpeakerUsesAngelAvatar(l.speaker))
                                {
                                    CLAY(
                                        CLAY_IDI("StoryMsgAvatar", (int)i),
                                        {
                                            .layout = {
                                                .sizing = {CLAY_SIZING_FIXED(58), CLAY_SIZING_FIXED(58)},
                                            },
                                            .cornerRadius = {29, 29, 29, 29},
                                            .image = {.imageData = &clayton->botPreviewImage},
                                        }
                                    )
                                    {
                                    }
                                }

                                CLAY(
                                    CLAY_IDI("StoryMsgBodyWrap", (int)i),
                                    {
                                        .layout = {
                                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                            .childGap = 6,
                                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                        },
                                    }
                                )
                                {
                                    Clay_String title = ClayArena_FormatString(arena, "%s", speakerName);
                                    CLAY_TEXT(title, CLAY_TEXT_CONFIG(titleCfg));

                                    const int textLen = (int)strlen(l.text);
                                    const int shown = std::max(0, std::min<int>(l.typedChars, textLen));
                                    char *mem = (char *)ClayArena_Alloc(arena, (size_t)shown + 1);
                                    if (!mem)
                                    {
                                        CLAY_TEXT(CLAY_STRING("[OVF]"), CLAY_TEXT_CONFIG(bodyCfg));
                                    }
                                    else
                                    {
                                        if (shown > 0)
                                            memcpy(mem, l.text, (size_t)shown);
                                        mem[shown] = '\0';
                                        Clay_String typed = (Clay_String){
                                            .isStaticallyAllocated = false,
                                            .length = (int32_t)shown,
                                            .chars = mem,
                                        };
                                        CLAY_TEXT(typed, CLAY_TEXT_CONFIG(bodyCfg));
                                    }
                                }
                            }
                        }
                    }

                    // Choices are rendered in a separate panel below messages.
                    if (waitingChoice)
                    {
                        CLAY(CLAY_ID("StoryChoicePanel"), CLAY_THEME_SECTION)
                        {
                            CLAY(
                                CLAY_ID("StoryChoices"),
                                {
                                    .layout =
                                        {
                                            .sizing = {CLAY_SIZING_GROW(), CLAY_SIZING_FIT()},
                                            .padding = {.top = 10, .bottom = 0},
                                            .childGap = 10,
                                            .layoutDirection = CLAY_TOP_TO_BOTTOM,
                                        }
                                }
                            )
                            {
                                int btnSlot = 0;
                                for (int i = 0; i < STORY_OPTIONS_COUNT; i++)
                                {
                                    const StoryChoiceOption &opt = STORY_OPTIONS[i];
                                    if (opt.choice_id != activeChoiceGroup)
                                        continue;

                                    Clay_String label = ClayArena_AllocString(arena, Story_OptionText(language, opt));
                                    if (btnSlot == 0)
                                        CLAY(optionClicks[0].clayId, CLAY_THEME_BTN_HUD)
                                        {
                                            CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonCfg));
                                        }
                                    else if (btnSlot == 1)
                                        CLAY(optionClicks[1].clayId, CLAY_THEME_BTN_HUD)
                                        {
                                            CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonCfg));
                                        }
                                    else if (btnSlot == 2)
                                        CLAY(optionClicks[2].clayId, CLAY_THEME_BTN_HUD)
                                        {
                                            CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonCfg));
                                        }
                                    else if (btnSlot == 3)
                                        CLAY(optionClicks[3].clayId, CLAY_THEME_BTN_HUD)
                                        {
                                            CLAY_TEXT(label, CLAY_TEXT_CONFIG(buttonCfg));
                                        }
                                    btnSlot++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

  private:
    void pushLine(int32_t storyId, int32_t speaker, const char *text)
    {
        if (lineCount >= MAX_LINES)
        {
            // Drop the oldest to make room (simple ring-less shift).
            for (int i = 1; i < MAX_LINES; i++)
                lines[i - 1] = lines[i];
            lineCount = MAX_LINES - 1;
        }

        // Mark previous last line as non-typing.
        if (lineCount > 0)
            lines[lineCount - 1].typing = false;

        Line &l = lines[lineCount++];
        l.storyId = storyId;
        l.speaker = speaker;
        l.text = Story_Text(language, storyId, text);
        l.typedChars = 0;
        l.typing = true;
        l.preDelayLeft = 1.0f;
        openedThisFrame = true;
        typeTimer = 0.0f;
        waitingChoice = false;
        closeRequested = false;
    }

    void loadNode(const StorylineNode &n)
    {
        pushLine(n.storyline_id, n.speaker, n.text);

        // Contract: a node either has choices OR it has an automatic next node.
        activeChoiceGroup = n.choice_group;
        pendingAutoNextStoryId = 0;
        pendingAutoNextTimer = 0.0f;
        if (activeChoiceGroup != CHOICE_NONE)
        {
            pendingAutoNextStoryId = 0;
        }
        else if (n.next_storyline != 0)
        {
            pendingAutoNextStoryId = n.next_storyline;
        }
    }
};
