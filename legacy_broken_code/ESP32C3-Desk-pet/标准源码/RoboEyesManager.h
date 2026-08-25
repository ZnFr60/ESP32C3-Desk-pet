#ifndef ROBO_EYES_MANAGER_H
#define ROBO_EYES_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <FluxGarage_RoboEyes.h>
#include <Config.h>

class RoboEyesManager {
private:
    Adafruit_SSD1306 &display;
    RoboEyes<Adafruit_SSD1306> roboEyes;

    enum EyeState {
        Default,
        Happy,
        Comfortable,
        Love,
        Excited,
        Bored,
        Surprised,
        Confused,
        Content,
        Curiosity,
        Stretch,
        LookAround,
        Sleepy,
        Asleep,
        Angry,
        Pain,
        Dizzy,
        Scare,
        Scared,
        TiltedLeft,
        TiltedRight,
        Thinking,
        Sad,
        Cool,
        Wink,
        // ========== Activities: what is he doing ==========
        Drinking,
        Reading,
        Eating,
        Listening,
        Yawning,
        Dancing,
        Drawing,
        Gaming,
        EatingIceCream,
        TakingPhoto,
        Writing,
        // ========== Activities ==========
        Exercising,
        BrushingTeeth,
        Cooking,
        Bathing,
        Calling,
        EatingNoodles,
        LookingMirror,
        StretchingBody,
    } currentState;

    // Timing
    unsigned long lastInteractionTime = 0;
    unsigned long lastMotionInteractionTime = 0;  // Separate cooldown for motion events
    unsigned long stateStartTime = 0;
    bool isSpecialAnimation = false;
    bool isSleeping = false;
    bool frameWasDrawn = false;
    int currentBrightness = DIM_LEVEL_NORMAL;

    // Configurable settings (loaded from settings)
    unsigned long idleTimeoutMs = DEFAULT_IDLE_TIMEOUT;
    bool autoDimEnabled = DEFAULT_AUTO_DIM;
    bool ecoModeEnabled = DEFAULT_ECO_MODE;
    unsigned long motionCooldownMs = DEFAULT_MOTION_COOLDOWN;  // User-configurable motion interaction cooldown

    // Time display
    String currentTimeStr = "";
    bool showTime = false;

    // Step counter display
    int stepCount = 0;
    bool showSteps = false;

    // Activity label (what is he doing)
    String activityLabel = "";

    // Mood level (0-100)
    int mood = MOOD_INITIAL;
    unsigned long lastMoodDecay = 0;

    // Eyes offset
    int eyesYOffset = 0;

    // Boot phase tracking
    unsigned long bootTime = 0;

    void setDefaultState() {
        roboEyes.setMood(ROBO_DEFAULT);
        roboEyes.setWidth(28, 28);
        roboEyes.setHeight(30, 30);
        roboEyes.setBorderradius(8, 8);
        roboEyes.setPosition(ROBO_DEFAULT);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_ON, 3, 2);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        roboEyes.setSpacebetween(10);
    }

    void adjustEyesY() {
        roboEyes.eyeLyNext += eyesYOffset;
        roboEyes.eyeRyNext += eyesYOffset;
    }

    void setBrightness(int level) {
        if (level != currentBrightness) {
            currentBrightness = level;
            // Send SSD1306 set contrast command via I2C directly
            // (avoids protected ssd1306_command() access issue)
            Wire.beginTransmission(SCREEN_ADDRESS);
            Wire.write(0x00);  // Command mode
            Wire.write(0x81);  // Set Contrast
            Wire.write(level); // Contrast value (0-255)
            Wire.endTransmission();
        }
    }

    void clearActivityLabel() { activityLabel = ""; }

public:
    RoboEyesManager(Adafruit_SSD1306 &disp)
        : display(disp), roboEyes(disp), currentState(Default),
          lastInteractionTime(0), stateStartTime(0), isSpecialAnimation(false),
          isSleeping(false), showTime(false), eyesYOffset(EYES_ZONE_Y - (SCREEN_HEIGHT - 36) / 2) {}

    void begin(int screenWidth, int screenHeight, int fps) {
        roboEyes.begin(screenWidth, screenHeight, fps);
        roboEyes.skipAutoDisplay = true;
        setDefaultState();
        // Set initial brightness immediately so first frame uses correct level
        setBrightness(DIM_LEVEL_NORMAL);
        bootTime = millis();
        lastInteractionTime = millis();
        stateStartTime = millis();
        lastMoodDecay = millis();
    }

    // Call this after begin() if waking from deep sleep to skip the 15s boot phase
    void skipBootPhase() {
        bootTime = millis() - BOOT_PHASE_DURATION_MS - 1;  // Force boot phase to be "over"
    }

    void setIdleTimeout(unsigned long timeout) {
        idleTimeoutMs = timeout;
    }

    void setAutoDim(bool enabled) {
        autoDimEnabled = enabled;
    }

    void setEcoMode(bool enabled) {
        ecoModeEnabled = enabled;
        if (enabled) {
            setBrightness(DIM_LEVEL_DIM);
        } else {
            setBrightness(DIM_LEVEL_NORMAL);
        }
    }

    void setMotionCooldown(unsigned long ms) {
        motionCooldownMs = ms;
    }

    void onWakeup() {
        recordInteraction();
        // recordInteraction() handles isSleeping flag and calls enterSurprisedState() if needed
        // Extra enterSurprisedState() call for deep sleep wake (where isSleeping is already false)
        enterSurprisedState();
    }

    void setStepCount(int steps) {
        stepCount = steps;
        showSteps = (steps > 0);
    }

    int getMood() { return mood; }

    // ============================================================
    // Main update - dynamic expressions & state transitions
    // ============================================================
    void update() {
        unsigned long currentTime = millis();
        unsigned long idleTime = currentTime - lastInteractionTime;
        unsigned long stateDuration = currentTime - stateStartTime;

        // Mood decay
        if (currentTime - lastMoodDecay > MOOD_DECAY_INTERVAL) {
            mood -= MOOD_DECAY_AMOUNT;
            if (mood < MOOD_MIN) mood = MOOD_MIN;
            lastMoodDecay = currentTime;
        }

        // Auto-dim
        if (ecoModeEnabled) {
            setBrightness(DIM_LEVEL_DIM);
        } else if (!autoDimEnabled) {
            setBrightness(DIM_LEVEL_NORMAL);
        } else {
            if (isSleeping) {
                // Sleep: keep screen dim but visible (eyes are still drawn)
                setBrightness(DIM_LEVEL_DIM);
            } else if (idleTime > DIM_AFTER_MS * 2) {
                setBrightness(DIM_LEVEL_VERY_DIM);
            } else if (idleTime > DIM_AFTER_MS) {
                setBrightness(DIM_LEVEL_DIM);
            } else {
                setBrightness(DIM_LEVEL_NORMAL);
            }
        }

        // Auto-sleep
        if (!isSpecialAnimation && idleTime >= idleTimeoutMs && currentState != Asleep && currentState != Sleepy) {
            enterSleepyState();
        }

        // ============================================================
        // Dynamic state behaviors + transitions
        // ============================================================
        if (!isSpecialAnimation) {
            switch (currentState) {

                // ========== Default: random expressions & activities ==========
                case Default: {
                    // Check if in boot phase (first 15 seconds) - apply IMMEDIATELY
                    // (don't wait 6s of idle for boot phase activities)
                    bool inBootPhase = (millis() - bootTime < BOOT_PHASE_DURATION_MS);
                    bool shouldSwitch = inBootPhase
                        ? (stateDuration >= 2000)   // boot phase: switch every 2s
                        : (idleTime >= 6000 && idleTime < idleTimeoutMs);

                    if (shouldSwitch) {
                        if (inBootPhase) {
                            // Boot phase: only surprised (50%) and stretch (50%)
                            if (random(2) == 0) {
                                enterSurprisedState();
                            } else {
                                enterStretchState();
                            }
                        } else {
                            // Normal phase: 30% chance for activity, 70% for expression
                            if (idleTime >= ACTIVITY_MIN_IDLE_TIME && random(100) < ACTIVITY_CHANCE_PERCENT) {
                                enterRandomActivity();
                            } else {
                                // Expression weights: LookAround 60%, others 40% total
                                unsigned int r = random(100);
                                if (r < 60) {
                                    // 60% - Look around (东张西望)
                                    enterLookAroundState();
                                } else if (r < 64) {
                                    // 4% - Stretch
                                    enterStretchState();
                                } else if (r < 68) {
                                    // 4% - Bored
                                    enterBoredState();
                                } else if (r < 72) {
                                    // 4% - Curiosity
                                    enterCuriosityState();
                                } else if (r < 76) {
                                    // 4% - Thinking
                                    enterThinkingState();
                                } else if (r < 80) {
                                    // 4% - Wink
                                    enterWinkState();
                                } else if (r < 84) {
                                    // 4% - Happy
                                    enterHappyState();
                                } else if (r < 88) {
                                    // 4% - Cool
                                    enterCoolState();
                                } else if (r < 92) {
                                    // 4% - Love
                                    enterLoveState();
                                } else if (r < 96) {
                                    // 4% - Content (or Sad if mood is low)
                                    if (mood < 30) {
                                        enterSadState();
                                    } else {
                                        enterContentState();
                                    }
                                } else {
                                    // 4% - Stay in default
                                    enterDefaultState();
                                }
                            }
                        }
                    }
                    break;
                }
                // ========== Expressions with dynamic behavior ==========
                case Happy:
                    // Dynamic: gentle height bounce (slow, small amplitude)
                    if (stateDuration > 500 && stateDuration < EXPR_HAPPY_DURATION - 500) {
                        int bounce = (stateDuration / 400) % 2;  // Slow: 400ms cycle
                        roboEyes.setHeight(30 - bounce, 30 - bounce);  // Small: 1px change
                    }
                    if (stateDuration >= EXPR_HAPPY_DURATION) enterDefaultState();
                    break;

                case Comfortable:
                    // Dynamic: gentle breathing
                    if (stateDuration > 1000 && stateDuration < EXPR_COMFORTABLE_DURATION - 500) {
                        int breathe = (stateDuration / 800) % 2;
                        roboEyes.setHeight(20 - breathe * 2, 20 - breathe * 2);
                    }
                    if (stateDuration >= EXPR_COMFORTABLE_DURATION) enterContentState();
                    break;

                case Love:
                    // Dynamic: pulsing eyes
                    if (stateDuration < EXPR_LOVE_DURATION - 500) {
                        int pulse = (stateDuration / 300) % 2;
                        roboEyes.setWidth(26 + pulse * 4, 26 + pulse * 4);
                        roboEyes.setHeight(28 + pulse * 4, 28 + pulse * 4);
                    }
                    if (stateDuration >= EXPR_LOVE_DURATION) enterContentState();
                    break;

                case Excited:
                    // Dynamic: gentle bounce (slower, smaller amplitude)
                    if (stateDuration < EXPR_EXCITED_DURATION - 500) {
                        int bounce = (stateDuration / 300) % 2;  // Slower: 300ms cycle (was 150ms)
                        roboEyes.setHeight(34 - bounce * 2, 34 - bounce * 2);  // Small: 2px change
                    }
                    if (stateDuration >= EXPR_EXCITED_DURATION) enterHappyState();
                    break;

                case Bored:
                    // Dynamic: slow movement, occasional half-closed
                    if (stateDuration > 1000 && stateDuration < EXPR_BORED_DURATION - 1000) {
                        if (random(100) < 2) {
                            roboEyes.setPosition(random(ROBO_N, ROBO_S + 1));
                            adjustEyesY();
                        }
                    }
                    if (stateDuration >= EXPR_BORED_DURATION) {
                        if (random(1, 4) > 2) enterStretchState();
                        else enterSleepyState();
                    }
                    break;

                case Surprised:
                    // Dynamic: 0-800ms big eyes fixed; 800ms+ looking around (东看西看)
                    if (stateDuration > 800 && stateDuration < EXPR_SURPRISED_DURATION - 200) {
                        roboEyes.setCuriosity(ROBO_ON);
                        if (random(100) < 5) {
                            roboEyes.eyeLxNext = random(roboEyes.getScreenConstraint_X());
                            roboEyes.eyeLyNext = random(EYES_ZONE_Y, EYES_ZONE_Y + EYES_ZONE_HEIGHT - 30);
                            roboEyes.eyeRxNext = roboEyes.eyeLxNext + roboEyes.eyeLwidthCurrent + roboEyes.spaceBetweenCurrent;
                            roboEyes.eyeRyNext = roboEyes.eyeLyNext;
                        }
                    }
                    if (stateDuration >= EXPR_SURPRISED_DURATION) enterDefaultState();
                    break;

                case Confused:
                    // Dynamic: head tilt left-right
                    if (stateDuration > 500 && stateDuration < EXPR_CONFUSED_DURATION - 500) {
                        roboEyes.setPosition(((stateDuration / 1000) % 2 == 0) ? ROBO_E : ROBO_W);
                        adjustEyesY();
                    }
                    if (stateDuration >= EXPR_CONFUSED_DURATION) enterCuriosityState();
                    break;

                case Content:
                    // Dynamic: gentle sway
                    if (stateDuration > 500 && stateDuration < EXPR_CONTENT_DURATION - 500) {
                        int sway = (stateDuration / 600) % 2;
                        roboEyes.setPosition(sway ? ROBO_SE : ROBO_SW);
                        adjustEyesY();
                    }
                    if (stateDuration >= EXPR_CONTENT_DURATION) enterDefaultState();
                    break;

                case Stretch:
                    if (stateDuration >= EXPR_STRETCH_DURATION) enterDefaultState();
                    break;

                case LookAround:
                    // Dynamic: random eye movement (东张西望)
                    if (stateDuration > 300 && random(100) < 4) {
                        roboEyes.eyeLxNext = random(roboEyes.getScreenConstraint_X());
                        int maxY = roboEyes.getScreenConstraint_Y() + eyesYOffset;
                        roboEyes.eyeLyNext = random(EYES_ZONE_Y, maxY + 1);
                        roboEyes.eyeRxNext = roboEyes.eyeLxNext + roboEyes.eyeLwidthCurrent + roboEyes.spaceBetweenCurrent;
                        roboEyes.eyeRyNext = roboEyes.eyeLyNext;
                    }
                    if (stateDuration >= EXPR_CURIOUS_DURATION) {
                        if (random(1, 4) > 2) enterStretchState();
                        else enterDefaultState();
                    }
                    break;

                case Curiosity:
                    // Dynamic: rapid blinking + curious movement
                    if (stateDuration > 300 && stateDuration < EXPR_CURIOUS_DURATION - 500) {
                        roboEyes.setAutoblinker(ROBO_ON, 2, 1);
                        if (random(100) < 3) {
                            roboEyes.eyeLxNext = random(roboEyes.getScreenConstraint_X());
                            int maxY = roboEyes.getScreenConstraint_Y() + eyesYOffset;
                            roboEyes.eyeLyNext = random(EYES_ZONE_Y, maxY + 1);
                            roboEyes.eyeRxNext = roboEyes.eyeLxNext + roboEyes.eyeLwidthCurrent + roboEyes.spaceBetweenCurrent;
                            roboEyes.eyeRyNext = roboEyes.eyeLyNext;
                        }
                    }
                    if (stateDuration >= EXPR_CURIOUS_DURATION) {
                        if (random(1, 4) > 2) enterLookAroundState();
                        else enterDefaultState();
                    }
                    break;

                case Thinking:
                    // Dynamic: looking NE, then furrow brow (皱眉)
                    if (stateDuration > 1500 && stateDuration < EXPR_THINKING_DURATION - 500) {
                        roboEyes.setHFlicker(ROBO_ON, 2);
                    } else {
                        roboEyes.setHFlicker(ROBO_OFF);
                    }
                    if (stateDuration >= EXPR_THINKING_DURATION) enterDefaultState();
                    break;

                case Sad:
                    // Dynamic: occasional tear flicker
                    if (stateDuration > 500 && stateDuration < EXPR_SAD_DURATION - 500) {
                        if (random(100) < 3) roboEyes.setVFlicker(ROBO_ON, 2);
                        else roboEyes.setVFlicker(ROBO_OFF);
                    }
                    if (stateDuration >= EXPR_SAD_DURATION) enterDefaultState();
                    break;

                case Cool:
                    // Dynamic: occasional eyebrow raise (挑眉)
                    if (stateDuration > 500 && stateDuration < EXPR_COOL_DURATION - 500) {
                        roboEyes.setHFlicker(((stateDuration / 800) % 2 == 0) ? ROBO_ON : ROBO_OFF, 1);
                    }
                    if (stateDuration >= EXPR_COOL_DURATION) enterDefaultState();
                    break;

                case Wink:
                    if (stateDuration >= EXPR_WINK_DURATION) enterDefaultState();
                    break;

                case Sleepy:
                    // Dynamic: eyes gradually closing in stages over EXPR_SLEEPY_DURATION
                    // NOTE: do NOT check idleTime here — idleTime >= idleTimeoutMs is ALWAYS true
                    // when we enter Sleepy, which would cause an instant jump to Asleep.
                    if (stateDuration > 1000 && stateDuration < EXPR_SLEEPY_DURATION) {
                        int closeProgress = constrain((int)(stateDuration - 1000) / 1000, 0, 4);
                        // 4 stages: 18 -> 12 -> 8 -> 5 -> 3
                        int h = 18 - closeProgress * 4;
                        if (closeProgress >= 4) h = 3;
                        roboEyes.setHeight(h, h);
                    }
                    if (stateDuration >= EXPR_SLEEPY_DURATION) {
                        enterAsleepState();
                    }
                    break;

                case Angry:
                    // Dynamic: shaking with anger
                    if (stateDuration > 300 && stateDuration < EXPR_ANGRY_DURATION - 300) {
                        int shake = (stateDuration / 100) % 2;
                        roboEyes.setHFlicker(ROBO_ON, 3 + shake);
                    }
                    if (stateDuration >= EXPR_ANGRY_DURATION) enterDefaultState();
                    break;

                case Pain:
                    if (stateDuration >= EXPR_PAIN_DURATION) enterScaredState();
                    break;

                case Dizzy:
                    // Dynamic: rapid left-right movement
                    if (stateDuration > 200 && stateDuration < EXPR_DIZZY_DURATION - 500) {
                        roboEyes.setPosition(((stateDuration / 200) % 2 == 0) ? ROBO_W : ROBO_E);
                        adjustEyesY();
                    }
                    if (stateDuration >= EXPR_DIZZY_DURATION) enterConfusedState();
                    break;

                case Scare:
                    if (stateDuration >= EXPR_SCARE_DURATION) enterDefaultState();
                    break;

                case Scared:
                    if (stateDuration >= EXPR_SCARED_DURATION) enterDefaultState();
                    break;

                case TiltedLeft:
                case TiltedRight:
                    if (stateDuration >= 1000) enterDefaultState();
                    break;

                case Asleep:
                    break;

                // ========== Activities: what is he doing ==========

                case Drinking:
                    // Dynamic: looking down, then drinking motion, then swallow
                    if (stateDuration < 1500) {
                        roboEyes.setPosition(ROBO_S);
                        adjustEyesY();
                    } else if (stateDuration < 3000) {
                        int pulse = (stateDuration / 400) % 2;
                        roboEyes.setHeight(24 + pulse * 2, 24 + pulse * 2);
                    } else if (stateDuration < 3500) {
                        roboEyes.setHeight(4, 4);  // Swallow - blink
                    } else {
                        roboEyes.setHeight(26, 26);
                        roboEyes.setPosition(ROBO_S);
                        adjustEyesY();
                    }
                    if (stateDuration >= ACTIVITY_DRINKING_DURATION) {
                        clearActivityLabel();
                        enterContentState();
                    }
                    break;

                case Reading:
                    // Dynamic: small eyes, scanning left-right (扫读)
                    if (stateDuration < ACTIVITY_READING_DURATION - 500) {
                        roboEyes.setWidth(20, 20);
                        roboEyes.setHeight(22, 22);
                        int scanPos = (stateDuration / 800) % 3;
                        if (scanPos == 0) roboEyes.setPosition(ROBO_W);
                        else if (scanPos == 1) roboEyes.setPosition(ROBO_DEFAULT);
                        else roboEyes.setPosition(ROBO_E);
                        adjustEyesY();
                    }
                    if (stateDuration >= ACTIVITY_READING_DURATION) {
                        clearActivityLabel();
                        enterContentState();
                    }
                    break;

                case Eating:
                    // Dynamic: chewing motion (咀嚼)
                    if (stateDuration < ACTIVITY_EATING_DURATION - 500) {
                        int chew = (stateDuration / 300) % 2;
                        roboEyes.setHeight(24 + chew * 3, 24 + chew * 3);
                        roboEyes.setPosition(ROBO_S);
                        adjustEyesY();
                    }
                    if (stateDuration >= ACTIVITY_EATING_DURATION) {
                        clearActivityLabel();
                        enterHappyState();
                    }
                    break;

                case Listening:
                    // Dynamic: half-closed, gentle sway, enjoying
                    if (stateDuration < ACTIVITY_LISTENING_DURATION - 500) {
                        roboEyes.setHeight(20, 20);
                        int sway = (stateDuration / 500) % 3;
                        if (sway == 0) roboEyes.setPosition(ROBO_NW);
                        else if (sway == 1) roboEyes.setPosition(ROBO_N);
                        else roboEyes.setPosition(ROBO_NE);
                        adjustEyesY();
                    }
                    if (stateDuration >= ACTIVITY_LISTENING_DURATION) {
                        clearActivityLabel();
                        enterContentState();
                    }
                    break;

                case Yawning:
                    // Dynamic: big -> close -> big -> normal (打哈欠)
                    if (stateDuration < 500) {
                        roboEyes.setHeight(38, 38);
                    } else if (stateDuration < 1000) {
                        roboEyes.setHeight(36, 36);
                    } else if (stateDuration < 1500) {
                        roboEyes.setHeight(8, 8);
                    } else if (stateDuration < 2000) {
                        roboEyes.setHeight(34, 34);
                    } else {
                        roboEyes.setHeight(30, 30);
                    }
                    if (stateDuration >= ACTIVITY_YAWNING_DURATION) {
                        clearActivityLabel();
                        enterSleepyState();
                    }
                    break;

                case Dancing:
                    // Dynamic: happy bouncing + swaying (combined)
                    if (stateDuration < ACTIVITY_DANCING_DURATION - 500) {
                        int bounce = (stateDuration / 200) % 2;  // 0=default/down, 1=up
                        int sway = (stateDuration / 400) % 3;     // 0=left, 1=center, 2=right
                        
                        // Combine bounce and sway into single position
                        if (sway == 0) {
                            roboEyes.setPosition(bounce ? ROBO_NW : ROBO_W);
                        } else if (sway == 1) {
                            roboEyes.setPosition(bounce ? ROBO_N : ROBO_DEFAULT);
                        } else {
                            roboEyes.setPosition(bounce ? ROBO_NE : ROBO_E);
                        }
                        adjustEyesY();
                    }
                    if (stateDuration >= ACTIVITY_DANCING_DURATION) {
                        clearActivityLabel();
                        enterHappyState();
                    }
                    break;

                case Drawing:
                    // Dynamic: focused, following hand movement
                    if (stateDuration < ACTIVITY_DRAWING_DURATION - 500) {
                        roboEyes.setWidth(22, 22);
                        roboEyes.setHeight(24, 24);
                        int follow = (stateDuration / 1000) % 4;
                        if (follow == 0) roboEyes.setPosition(ROBO_NW);
                        else if (follow == 1) roboEyes.setPosition(ROBO_N);
                        else if (follow == 2) roboEyes.setPosition(ROBO_NE);
                        else roboEyes.setPosition(ROBO_NW);
                        adjustEyesY();
                        if (random(100) < 2) roboEyes.setHeight(18, 18);
                    }
                    if (stateDuration >= ACTIVITY_DRAWING_DURATION) {
                        clearActivityLabel();
                        enterContentState();
                    }
                    break;

                case Gaming:
                    // Dynamic: intense, rapid blinking, focused
                    if (stateDuration < ACTIVITY_GAMING_DURATION - 500) {
                        roboEyes.setWidth(30, 30);
                        roboEyes.setHeight(32, 32);
                        roboEyes.setAutoblinker(ROBO_ON, 1, 1);
                        int focus = (stateDuration / 600) % 3;
                        if (focus == 0) roboEyes.setPosition(ROBO_W);
                        else if (focus == 1) roboEyes.setPosition(ROBO_DEFAULT);
                        else roboEyes.setPosition(ROBO_E);
                        adjustEyesY();
                    }
                    if (stateDuration >= ACTIVITY_GAMING_DURATION) {
                        clearActivityLabel();
                        enterBoredState();  // Tired after gaming
                    }
                    break;

                case EatingIceCream:
                    // Dynamic: enjoying, slow licking motion
                    if (stateDuration < ACTIVITY_EATING_ICE_CREAM_DURATION - 500) {
                        roboEyes.setMood(ROBO_HAPPY);
                        int lick = (stateDuration / 500) % 3;
                        if (lick == 0) {
                            roboEyes.setHeight(28, 28);
                            roboEyes.setPosition(ROBO_N);
                        } else if (lick == 1) {
                            roboEyes.setHeight(26, 26);
                            roboEyes.setPosition(ROBO_NE);
                        } else {
                            roboEyes.setHeight(28, 28);
                            roboEyes.setPosition(ROBO_N);
                        }
                        adjustEyesY();
                    }
                    if (stateDuration >= ACTIVITY_EATING_ICE_CREAM_DURATION) {
                        clearActivityLabel();
                        enterHappyState();
                    }
                    break;

                case TakingPhoto:
                    // Dynamic: squint -> flash (blink) -> look at photo
                    if (stateDuration < 1000) {
                        roboEyes.setHeight(16, 16);
                    } else if (stateDuration < 1500) {
                        roboEyes.setHeight(4, 4);  // Flash
                    } else if (stateDuration < 2000) {
                        roboEyes.setHeight(30, 30);
                    } else {
                        roboEyes.setHeight(28, 28);
                        roboEyes.setMood(ROBO_HAPPY);
                    }
                    if (stateDuration >= ACTIVITY_TAKING_PHOTO_DURATION) {
                        clearActivityLabel();
                        enterHappyState();
                    }
                    break;

                case Writing:
                    // Dynamic: looking down, scanning left-right
                    if (stateDuration < ACTIVITY_WRITING_DURATION - 500) {
                        roboEyes.setWidth(22, 22);
                        roboEyes.setHeight(24, 24);
                        roboEyes.setPosition(ROBO_S);
                        adjustEyesY();
                        int scan = (stateDuration / 700) % 2;
                        if (scan == 0) {
                            roboEyes.eyeLxNext = 10;
                            roboEyes.eyeRxNext = roboEyes.eyeLxNext + roboEyes.eyeLwidthCurrent + roboEyes.spaceBetweenCurrent;
                        } else {
                            roboEyes.eyeLxNext = roboEyes.getScreenConstraint_X() - 10;
                            roboEyes.eyeRxNext = roboEyes.eyeLxNext + roboEyes.eyeLwidthCurrent + roboEyes.spaceBetweenCurrent;
                        }
                    }
                    if (stateDuration >= ACTIVITY_WRITING_DURATION) {
                        clearActivityLabel();
                        enterContentState();
                    }
                    break;

                // ========== Activities ==========

                case Exercising:
                    // Dynamic: intense bouncing, eyes wide open, energetic
                    if (stateDuration < ACTIVITY_EXERCISING_DURATION - 500) {
                        int bounce = (stateDuration / 120) % 2;
                        roboEyes.setPosition(bounce ? ROBO_N : ROBO_DEFAULT);
                        adjustEyesY();
                        int widen = (stateDuration / 300) % 2;
                        roboEyes.setHeight(32 + widen * 4, 32 + widen * 4);
                        roboEyes.setWidth(30 + widen * 2, 30 + widen * 2);
                    }
                    if (stateDuration >= ACTIVITY_EXERCISING_DURATION) {
                        clearActivityLabel();
                        enterHappyState();
                    }
                    break;

                case BrushingTeeth:
                    // Dynamic: rapid left-right scrubbing motion
                    if (stateDuration < ACTIVITY_BRUSHING_TEETH_DURATION - 500) {
                        int scrub = (stateDuration / 150) % 4;
                        if (scrub == 0) roboEyes.setPosition(ROBO_W);
                        else if (scrub == 1) roboEyes.setPosition(ROBO_DEFAULT);
                        else if (scrub == 2) roboEyes.setPosition(ROBO_E);
                        else roboEyes.setPosition(ROBO_DEFAULT);
                        adjustEyesY();
                        // Mouth open/close feel via height
                        int mouth = (stateDuration / 250) % 2;
                        roboEyes.setHeight(22 + mouth * 6, 22 + mouth * 6);
                    }
                    if (stateDuration >= ACTIVITY_BRUSHING_TEETH_DURATION) {
                        clearActivityLabel();
                        enterContentState();
                    }
                    break;

                case Cooking:
                    // Dynamic: looking down at pot, occasionally looking up
                    if (stateDuration < ACTIVITY_COOKING_DURATION - 500) {
                        int phase = (stateDuration / 1200) % 3;
                        if (phase == 0) {
                            roboEyes.setPosition(ROBO_S);
                            roboEyes.setHeight(24, 24);
                        } else if (phase == 1) {
                            roboEyes.setPosition(ROBO_DEFAULT);
                            roboEyes.setHeight(28, 28);
                        } else {
                            roboEyes.setPosition(ROBO_S);
                            roboEyes.setHeight(22, 22);
                        }
                        adjustEyesY();
                        if (random(100) < 2) roboEyes.setSweat(ROBO_ON);
                        else roboEyes.setSweat(ROBO_OFF);
                    }
                    if (stateDuration >= ACTIVITY_COOKING_DURATION) {
                        clearActivityLabel();
                        roboEyes.setSweat(ROBO_OFF);
                        enterHappyState();
                    }
                    break;

                case Bathing:
                    // Dynamic: half-closed relaxed eyes, gentle sway
                    if (stateDuration < ACTIVITY_BATHING_DURATION - 500) {
                        roboEyes.setHeight(16, 16);
                        roboEyes.setWidth(26, 26);
                        int sway = (stateDuration / 600) % 3;
                        if (sway == 0) roboEyes.setPosition(ROBO_NW);
                        else if (sway == 1) roboEyes.setPosition(ROBO_N);
                        else roboEyes.setPosition(ROBO_NE);
                        adjustEyesY();
                        // Occasional content blink
                        if (random(100) < 2) roboEyes.setHeight(4, 4);
                    }
                    if (stateDuration >= ACTIVITY_BATHING_DURATION) {
                        clearActivityLabel();
                        enterComfortableState();
                    }
                    break;

                case Calling:
                    // Dynamic: head tilted, one eye slightly squinted
                    if (stateDuration < ACTIVITY_CALLING_DURATION - 500) {
                        roboEyes.setPosition(ROBO_E);
                        adjustEyesY();
                        // Tilt effect: right eye slightly smaller
                        int squint = (stateDuration / 400) % 2;
                        roboEyes.setWidth(26, 22 + squint * 2);
                        roboEyes.setHeight(24, 24);
                        // Occasional nod
                        if (random(100) < 3) {
                            roboEyes.setPosition(ROBO_SE);
                            adjustEyesY();
                        }
                    }
                    if (stateDuration >= ACTIVITY_CALLING_DURATION) {
                        clearActivityLabel();
                        enterContentState();
                    }
                    break;

                case EatingNoodles:
                    // Dynamic: slurping motion - eyes narrow, head goes up-down
                    if (stateDuration < ACTIVITY_EATING_NOODLES_DURATION - 500) {
                        int slurp = (stateDuration / 400) % 3;
                        if (slurp == 0) {
                            roboEyes.setHeight(28, 28);
                            roboEyes.setPosition(ROBO_N);
                        } else if (slurp == 1) {
                            roboEyes.setHeight(12, 12);
                            roboEyes.setPosition(ROBO_S);
                        } else {
                            roboEyes.setHeight(26, 26);
                            roboEyes.setPosition(ROBO_DEFAULT);
                        }
                        adjustEyesY();
                        roboEyes.setWidth(24, 24);
                    }
                    if (stateDuration >= ACTIVITY_EATING_NOODLES_DURATION) {
                        clearActivityLabel();
                        enterHappyState();
                    }
                    break;

                case LookingMirror:
                    // Dynamic: narcissistic - eyes widen, check left-right, smile
                    if (stateDuration < ACTIVITY_LOOKING_MIRROR_DURATION - 500) {
                        int vanity = (stateDuration / 800) % 3;
                        if (vanity == 0) {
                            roboEyes.setWidth(30, 30);
                            roboEyes.setHeight(32, 32);
                            roboEyes.setPosition(ROBO_DEFAULT);
                        } else if (vanity == 1) {
                            roboEyes.setWidth(28, 28);
                            roboEyes.setHeight(30, 30);
                            roboEyes.setPosition(ROBO_W);
                        } else {
                            roboEyes.setWidth(28, 28);
                            roboEyes.setHeight(30, 30);
                            roboEyes.setPosition(ROBO_E);
                        }
                        adjustEyesY();
                        roboEyes.setMood(ROBO_HAPPY);
                    }
                    if (stateDuration >= ACTIVITY_LOOKING_MIRROR_DURATION) {
                        clearActivityLabel();
                        enterCoolState();
                    }
                    break;

                case StretchingBody:
                    // Dynamic: eyes slowly widen then back to normal (big stretch)
                    if (stateDuration < ACTIVITY_STRETCHING_BODY_DURATION - 500) {
                        int progress = constrain((int)stateDuration / 800, 0, 3);
                        int h = 22 + progress * 4;
                        roboEyes.setHeight(h, h);
                        roboEyes.setWidth(26 + progress * 2, 26 + progress * 2);
                        roboEyes.setPosition(ROBO_N);
                        adjustEyesY();
                    }
                    if (stateDuration >= ACTIVITY_STRETCHING_BODY_DURATION) {
                        clearActivityLabel();
                        enterComfortableState();
                    }
                    break;
            }
        }

        // Track frame draw
        unsigned long prevFpsTimer = roboEyes.fpsTimer;
        roboEyes.update();
        frameWasDrawn = (roboEyes.fpsTimer != prevFpsTimer);
    }

    bool wasFrameDrawn() { return frameWasDrawn; }

    void recordInteraction() {
        lastInteractionTime = millis();
        mood += MOOD_INTERACTION_GAIN;
        if (mood > MOOD_MAX) mood = MOOD_MAX;
        lastMoodDecay = millis();
        clearActivityLabel();  // Cancel any ongoing activity
        if (isSleeping) {
            isSleeping = false;
            enterSurprisedState();
        }
    }
    
    // Check if motion-triggered interaction is allowed (configurable cooldown)
    bool canTriggerMotionInteraction() {
        unsigned long currentTime = millis();
        if (currentTime - lastMotionInteractionTime < motionCooldownMs) {
            return false;  // Still in cooldown
        }
        lastMotionInteractionTime = currentTime;
        return true;
    }

    void onButtonPress() {
        // Check sleep state BEFORE recordInteraction() changes it
        bool wasSleeping = (currentState == Asleep || currentState == Sleepy);
        recordInteraction();
        // recordInteraction() already calls enterSurprisedState() when isSleeping is true
        if (wasSleeping) {
            return;
        }
        if (!isSpecialAnimation) {
            // Single press = petting (抚摸) → mood-based expression
            if (mood > 70) {
                enterComfortableState();
            } else if (mood > 40) {
                enterHappyState();
            } else {
                enterSadState();
            }
        }
    }

    // Double press = custom action (user-defined)
    // Shows a random special expression or activity
    void onDoublePress() {
        recordInteraction();
        if (!isSpecialAnimation) {
            int r = random(5);
            switch (r) {
                case 0: enterLoveState(); break;
                case 1: enterCoolState(); break;
                case 2: enterExcitedState(); break;
                case 3: enterWinkState(); break;
                case 4: {
                    // Random activity (all 19 activities)
                    int a = random(19);
                    switch (a) {
                        case 0: enterDancingState(); break;
                        case 1: enterGamingState(); break;
                        case 2: enterEatingIceCreamState(); break;
                        case 3: enterTakingPhotoState(); break;
                        case 4: enterListeningState(); break;
                        case 5: enterDrawingState(); break;
                        case 6: enterEatingNoodlesState(); break;
                        case 7: enterLookingMirrorState(); break;
                        case 8: enterDrinkingState(); break;
                        case 9: enterReadingState(); break;
                        case 10: enterEatingState(); break;
                        case 11: enterYawningState(); break;
                        case 12: enterWritingState(); break;
                        case 13: enterExercisingState(); break;
                        case 14: enterBrushingTeethState(); break;
                        case 15: enterCookingState(); break;
                        case 16: enterBathingState(); break;
                        case 17: enterCallingState(); break;
                        case 18: enterStretchingBodyState(); break;
                    }
                    break;
                }
            }
        }
    }

    void onDrop() {
        if (!canTriggerMotionInteraction()) return;
        recordInteraction();
        if (!isSpecialAnimation) enterPainState();
    }

    void onShake() {
        if (!canTriggerMotionInteraction()) return;
        recordInteraction();
        if (!isSpecialAnimation) enterDizzyState();
    }

    void onFlick() {
        if (!canTriggerMotionInteraction()) return;
        recordInteraction();
        mood -= 10;
        if (mood < MOOD_MIN) mood = MOOD_MIN;
        if (!isSpecialAnimation) enterAngryState();
    }

    void onTiltLeft() {
        if (!canTriggerMotionInteraction()) return;
        recordInteraction();
        if (!isSpecialAnimation) enterTiltedLeftState();
    }

    void onTiltRight() {
        if (!canTriggerMotionInteraction()) return;
        recordInteraction();
        if (!isSpecialAnimation) enterTiltedRightState();
    }

    void onTap() {
        if (!canTriggerMotionInteraction()) return;
        recordInteraction();
        if (!isSpecialAnimation) {
            if (random(2) == 0) enterSurprisedState();
            else enterWinkState();
        }
    }

    void onDoubleTap() {
        if (!canTriggerMotionInteraction()) return;
        recordInteraction();
        mood += 15;
        if (mood > MOOD_MAX) mood = MOOD_MAX;
        if (!isSpecialAnimation) enterExcitedState();
    }

    void setTimeDisplay(String timeStr, bool show) {
        currentTimeStr = timeStr;
        showTime = show;
    }

    // Draw overlays: time + step count + activity label
    void drawOverlays(Adafruit_SSD1306 &disp, bool showTimeOverlay) {
        disp.fillRect(0, 0, SCREEN_WIDTH, EYES_ZONE_Y, SSD1306_BLACK);

        if (currentState == Asleep) {
            if (showTimeOverlay && currentTimeStr.length() > 0) {
                disp.setTextSize(2);
                disp.setTextColor(SSD1306_WHITE);
                int16_t x1, y1;
                uint16_t w, h;
                disp.getTextBounds(currentTimeStr.c_str(), 0, 0, &x1, &y1, &w, &h);
                int x = (SCREEN_WIDTH - w) / 2;
                disp.setCursor(x, 2);
                disp.print(currentTimeStr);
            }
        } else {
            // Activity label (top-left, highest priority)
            if (activityLabel.length() > 0) {
                disp.setTextSize(1);
                disp.setTextColor(SSD1306_WHITE);
                disp.setCursor(INFO_ZONE_X, INFO_ZONE_Y);
                disp.print(activityLabel);
            }
            // Time (top-right)
            else if (showTimeOverlay && currentTimeStr.length() > 0) {
                disp.setTextSize(1);
                disp.setTextColor(SSD1306_WHITE);
                int16_t x1, y1;
                uint16_t w, h;
                disp.getTextBounds(currentTimeStr.c_str(), 0, 0, &x1, &y1, &w, &h);
                int x = SCREEN_WIDTH - w - 2;
                disp.setCursor(x, TIME_ZONE_Y + 2);
                disp.print(currentTimeStr);
            }
            // Step count (top-left if no activity)
            if (showSteps && stepCount > 0 && activityLabel.length() == 0) {
                disp.setTextSize(1);
                disp.setTextColor(SSD1306_WHITE);
                disp.setCursor(INFO_ZONE_X, INFO_ZONE_Y + 2);
                disp.printf("%d", stepCount);
            }
        }
        disp.display();
    }

    bool getIsSleeping() { return isSleeping; }
    EyeState getCurrentState() { return currentState; }

    void setSpecialAnimation(bool active) { isSpecialAnimation = active; }

private:
    void enterDefaultState() {
        currentState = Default;
        isSleeping = false;
        clearActivityLabel();
        setDefaultState();
        stateStartTime = millis();
    }

    void enterHappyState() {
        currentState = Happy;
        roboEyes.setMood(ROBO_HAPPY);
        roboEyes.setWidth(28, 28);
        roboEyes.setHeight(30, 30);
        roboEyes.setBorderradius(10, 10);
        // No anim_laugh() - use gentle manual bounce in update() instead
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setAutoblinker(ROBO_ON, 2, 1);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterComfortableState() {
        currentState = Comfortable;
        roboEyes.setMood(ROBO_HAPPY);
        roboEyes.setWidth(30, 30);
        roboEyes.setHeight(20, 20);
        roboEyes.setBorderradius(10, 10);
        roboEyes.setSpacebetween(8);
        roboEyes.setPosition(ROBO_S);
        roboEyes.setAutoblinker(ROBO_ON, 4, 2);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterLoveState() {
        currentState = Love;
        roboEyes.setMood(ROBO_HAPPY);
        roboEyes.setWidth(26, 26);
        roboEyes.setHeight(28, 28);
        roboEyes.setBorderradius(12, 12);
        roboEyes.setSpacebetween(6);
        roboEyes.setPosition(ROBO_N);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_ON, 4, 2);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterExcitedState() {
        currentState = Excited;
        roboEyes.setMood(ROBO_DEFAULT);
        roboEyes.setWidth(32, 32);
        roboEyes.setHeight(34, 34);
        roboEyes.setBorderradius(12, 12);
        roboEyes.setPosition(ROBO_N);
        adjustEyesY();
        // No anim_laugh() - use gentle manual bounce in update() instead
        roboEyes.setAutoblinker(ROBO_OFF);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterBoredState() {
        currentState = Bored;
        roboEyes.setMood(ROBO_TIRED);
        roboEyes.setWidth(26, 26);
        roboEyes.setHeight(22, 22);
        roboEyes.setBorderradius(8, 8);
        roboEyes.setPosition(ROBO_S);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_ON, 5, 3);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterSurprisedState() {
        currentState = Surprised;
        roboEyes.setMood(ROBO_DEFAULT);
        roboEyes.setWidth(34, 34);
        roboEyes.setHeight(36, 36);
        roboEyes.setBorderradius(14, 14);
        roboEyes.setPosition(ROBO_DEFAULT);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_OFF);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterConfusedState() {
        currentState = Confused;
        roboEyes.setMood(ROBO_DEFAULT);
        roboEyes.setWidth(28, 28);
        roboEyes.setHeight(30, 30);
        roboEyes.setBorderradius(8, 8);
        roboEyes.anim_confused();
        roboEyes.setAutoblinker(ROBO_ON, 2, 1);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterContentState() {
        currentState = Content;
        roboEyes.setMood(ROBO_HAPPY);
        roboEyes.setWidth(24, 24);
        roboEyes.setHeight(26, 26);
        roboEyes.setBorderradius(10, 10);
        roboEyes.setAutoblinker(ROBO_ON, 4, 2);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterStretchState() {
        currentState = Stretch;
        roboEyes.setMood(ROBO_TIRED);
        roboEyes.setWidth(30, 30);
        roboEyes.setHeight(34, 34);
        roboEyes.setBorderradius(10, 10);
        roboEyes.setPosition(ROBO_N);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_OFF);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterLookAroundState() {
        currentState = LookAround;
        roboEyes.setMood(ROBO_DEFAULT);
        roboEyes.setWidth(28, 28);
        roboEyes.setHeight(30, 30);
        roboEyes.setBorderradius(8, 8);
        roboEyes.setAutoblinker(ROBO_ON, 2, 2);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_ON);
        roboEyes.eyeLxNext = random(roboEyes.getScreenConstraint_X());
        roboEyes.eyeLyNext = random(roboEyes.getScreenConstraint_Y()) + eyesYOffset;
        roboEyes.eyeRxNext = roboEyes.eyeLxNext + roboEyes.eyeLwidthCurrent + roboEyes.spaceBetweenCurrent;
        roboEyes.eyeRyNext = roboEyes.eyeLyNext;
        stateStartTime = millis();
    }

    void enterCuriosityState() {
        currentState = Curiosity;
        roboEyes.setMood(ROBO_DEFAULT);
        roboEyes.setWidth(28, 28);
        roboEyes.setHeight(30, 30);
        roboEyes.setBorderradius(8, 8);
        roboEyes.setAutoblinker(ROBO_ON, 3, 2);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_ON);
        stateStartTime = millis();
    }

    void enterThinkingState() {
        currentState = Thinking;
        roboEyes.setMood(ROBO_DEFAULT);
        roboEyes.setWidth(24, 24);
        roboEyes.setHeight(26, 26);
        roboEyes.setBorderradius(8, 8);
        roboEyes.setPosition(ROBO_NE);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_ON, 3, 2);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterSadState() {
        currentState = Sad;
        roboEyes.setMood(ROBO_TIRED);
        roboEyes.setWidth(24, 24);
        roboEyes.setHeight(18, 18);
        roboEyes.setBorderradius(8, 8);
        roboEyes.setPosition(ROBO_S);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_ON, 5, 3);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterCoolState() {
        currentState = Cool;
        roboEyes.setMood(ROBO_HAPPY);
        roboEyes.setWidth(32, 32);
        roboEyes.setHeight(16, 16);
        roboEyes.setBorderradius(4, 4);
        roboEyes.setPosition(ROBO_DEFAULT);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_OFF);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterWinkState() {
        currentState = Wink;
        roboEyes.setMood(ROBO_HAPPY);
        roboEyes.setWidth(28, 28);
        roboEyes.setHeight(30, 30);
        roboEyes.setBorderradius(8, 8);
        roboEyes.blink(true, false);
        roboEyes.setAutoblinker(ROBO_OFF);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterPainState() {
        currentState = Pain;
        roboEyes.setMood(ROBO_ANGRY);
        roboEyes.setWidth(26, 26);
        roboEyes.setHeight(24, 24);
        roboEyes.setBorderradius(4, 4);
        roboEyes.setSweat(ROBO_ON);
        roboEyes.setVFlicker(ROBO_ON, 8);
        roboEyes.setHFlicker(ROBO_ON, 3);
        roboEyes.setAutoblinker(ROBO_OFF);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterDizzyState() {
        currentState = Dizzy;
        roboEyes.setMood(ROBO_DEFAULT);
        roboEyes.setWidth(28, 28);
        roboEyes.setHeight(30, 30);
        roboEyes.setBorderradius(8, 8);
        roboEyes.anim_confused();
        roboEyes.setHFlicker(ROBO_ON, 15);
        roboEyes.setVFlicker(ROBO_ON, 5);
        roboEyes.setSweat(ROBO_ON);
        roboEyes.setAutoblinker(ROBO_OFF);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterSleepyState() {
        currentState = Sleepy;
        isSleeping = true;
        clearActivityLabel();
        roboEyes.setMood(ROBO_TIRED);
        roboEyes.setHeight(18, 18);
        roboEyes.setPosition(ROBO_DEFAULT);
        adjustEyesY();
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setAutoblinker(ROBO_ON, 2, 2);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterAsleepState() {
        currentState = Asleep;
        isSleeping = true;
        clearActivityLabel();
        roboEyes.setMood(ROBO_DEFAULT);
        roboEyes.setPosition(ROBO_S);
        adjustEyesY();
        roboEyes.setHeight(3, 3);
        roboEyes.setAutoblinker(ROBO_OFF);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setBorderradius(0, 0);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterAngryState() {
        currentState = Angry;
        roboEyes.setMood(ROBO_ANGRY);
        roboEyes.setWidth(30, 30);
        roboEyes.setHeight(28, 28);
        roboEyes.setBorderradius(6, 6);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_ON, 3);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setAutoblinker(ROBO_OFF);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterScaredState() {
        currentState = Scared;
        roboEyes.setMood(ROBO_TIRED);
        roboEyes.setWidth(28, 28);
        roboEyes.setHeight(30, 30);
        roboEyes.setBorderradius(8, 8);
        roboEyes.setSweat(ROBO_ON);
        roboEyes.setVFlicker(ROBO_ON, 1);   // Low frequency, mild shake (was 3)
        roboEyes.setHFlicker(ROBO_ON, 1);   // Low frequency, mild shake (was 3)
        roboEyes.setAutoblinker(ROBO_OFF);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterScareState() {
        currentState = Scare;
        roboEyes.setMood(ROBO_TIRED);
        roboEyes.setWidth(28, 28);
        roboEyes.setHeight(30, 30);
        roboEyes.setBorderradius(8, 8);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_ON, 3);
        roboEyes.setHFlicker(ROBO_ON, 3);
        roboEyes.setAutoblinker(ROBO_OFF);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterTiltedLeftState() {
        currentState = TiltedLeft;
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setPosition(ROBO_W);
        adjustEyesY();
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_ON);
        stateStartTime = millis();
    }

    void enterTiltedRightState() {
        currentState = TiltedRight;
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setPosition(ROBO_E);
        adjustEyesY();
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_ON);
        stateStartTime = millis();
    }

    // ========== Activity enter methods ==========

    void enterDrinkingState() {
        currentState = Drinking;
        activityLabel = "Drinking...";
        roboEyes.setMood(ROBO_DEFAULT);
        roboEyes.setWidth(26, 26);
        roboEyes.setHeight(26, 26);
        roboEyes.setBorderradius(8, 8);
        roboEyes.setPosition(ROBO_S);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_ON, 4, 2);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterReadingState() {
        currentState = Reading;
        activityLabel = "Reading...";
        roboEyes.setMood(ROBO_DEFAULT);
        roboEyes.setWidth(20, 20);
        roboEyes.setHeight(22, 22);
        roboEyes.setBorderradius(6, 6);
        roboEyes.setPosition(ROBO_DEFAULT);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_ON, 5, 3);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterEatingState() {
        currentState = Eating;
        activityLabel = "Eating...";
        roboEyes.setMood(ROBO_HAPPY);
        roboEyes.setWidth(26, 26);
        roboEyes.setHeight(24, 24);
        roboEyes.setBorderradius(8, 8);
        roboEyes.setPosition(ROBO_S);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_ON, 4, 2);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterListeningState() {
        currentState = Listening;
        activityLabel = "Listening...";
        roboEyes.setMood(ROBO_HAPPY);
        roboEyes.setWidth(24, 24);
        roboEyes.setHeight(20, 20);
        roboEyes.setBorderradius(8, 8);
        roboEyes.setPosition(ROBO_N);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_ON, 5, 3);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterYawningState() {
        currentState = Yawning;
        activityLabel = "Yawning...";
        roboEyes.setMood(ROBO_TIRED);
        roboEyes.setWidth(28, 28);
        roboEyes.setHeight(30, 30);
        roboEyes.setBorderradius(10, 10);
        roboEyes.setPosition(ROBO_DEFAULT);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_OFF);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterDancingState() {
        currentState = Dancing;
        activityLabel = "Dancing~~";
        roboEyes.setMood(ROBO_HAPPY);
        roboEyes.setWidth(30, 30);
        roboEyes.setHeight(32, 32);
        roboEyes.setBorderradius(10, 10);
        roboEyes.setPosition(ROBO_DEFAULT);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_OFF);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterDrawingState() {
        currentState = Drawing;
        activityLabel = "Drawing...";
        roboEyes.setMood(ROBO_DEFAULT);
        roboEyes.setWidth(22, 22);
        roboEyes.setHeight(24, 24);
        roboEyes.setBorderradius(6, 6);
        roboEyes.setPosition(ROBO_NW);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_ON, 4, 3);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterGamingState() {
        currentState = Gaming;
        activityLabel = "Gaming...";
        roboEyes.setMood(ROBO_DEFAULT);
        roboEyes.setWidth(30, 30);
        roboEyes.setHeight(32, 32);
        roboEyes.setBorderradius(8, 8);
        roboEyes.setPosition(ROBO_DEFAULT);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_ON, 1, 1);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterEatingIceCreamState() {
        currentState = EatingIceCream;
        activityLabel = "Ice cream!";
        roboEyes.setMood(ROBO_HAPPY);
        roboEyes.setWidth(26, 26);
        roboEyes.setHeight(28, 28);
        roboEyes.setBorderradius(10, 10);
        roboEyes.setPosition(ROBO_N);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_ON, 4, 2);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterTakingPhotoState() {
        currentState = TakingPhoto;
        activityLabel = "Photo! :D";
        roboEyes.setMood(ROBO_DEFAULT);
        roboEyes.setWidth(28, 28);
        roboEyes.setHeight(16, 16);
        roboEyes.setBorderradius(6, 6);
        roboEyes.setPosition(ROBO_DEFAULT);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_OFF);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterWritingState() {
        currentState = Writing;
        activityLabel = "Writing...";
        roboEyes.setMood(ROBO_DEFAULT);
        roboEyes.setWidth(22, 22);
        roboEyes.setHeight(24, 24);
        roboEyes.setBorderradius(6, 6);
        roboEyes.setPosition(ROBO_S);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_ON, 5, 3);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    // ========== Activity enter methods ==========

    void enterExercisingState() {
        currentState = Exercising;
        activityLabel = "Workout...";
        roboEyes.setMood(ROBO_HAPPY);
        roboEyes.setWidth(30, 30);
        roboEyes.setHeight(32, 32);
        roboEyes.setBorderradius(10, 10);
        roboEyes.setPosition(ROBO_DEFAULT);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_OFF);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterBrushingTeethState() {
        currentState = BrushingTeeth;
        activityLabel = "Brushing...";
        roboEyes.setMood(ROBO_DEFAULT);
        roboEyes.setWidth(26, 26);
        roboEyes.setHeight(24, 24);
        roboEyes.setBorderradius(8, 8);
        roboEyes.setPosition(ROBO_DEFAULT);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_OFF);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterCookingState() {
        currentState = Cooking;
        activityLabel = "Cooking...";
        roboEyes.setMood(ROBO_DEFAULT);
        roboEyes.setWidth(26, 26);
        roboEyes.setHeight(24, 24);
        roboEyes.setBorderradius(8, 8);
        roboEyes.setPosition(ROBO_S);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_ON, 4, 2);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterBathingState() {
        currentState = Bathing;
        activityLabel = "Bathing...";
        roboEyes.setMood(ROBO_HAPPY);
        roboEyes.setWidth(26, 26);
        roboEyes.setHeight(16, 16);
        roboEyes.setBorderradius(10, 10);
        roboEyes.setPosition(ROBO_N);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_ON, 5, 3);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterCallingState() {
        currentState = Calling;
        activityLabel = "Calling...";
        roboEyes.setMood(ROBO_DEFAULT);
        roboEyes.setWidth(26, 24);
        roboEyes.setHeight(24, 24);
        roboEyes.setBorderradius(8, 8);
        roboEyes.setPosition(ROBO_E);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_ON, 4, 2);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterEatingNoodlesState() {
        currentState = EatingNoodles;
        activityLabel = "Noodles~~";
        roboEyes.setMood(ROBO_HAPPY);
        roboEyes.setWidth(24, 24);
        roboEyes.setHeight(26, 26);
        roboEyes.setBorderradius(8, 8);
        roboEyes.setPosition(ROBO_DEFAULT);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_OFF);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterLookingMirrorState() {
        currentState = LookingMirror;
        activityLabel = "Mirror...";
        roboEyes.setMood(ROBO_HAPPY);
        roboEyes.setWidth(30, 30);
        roboEyes.setHeight(32, 32);
        roboEyes.setBorderradius(12, 12);
        roboEyes.setPosition(ROBO_DEFAULT);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_ON, 3, 2);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    void enterStretchingBodyState() {
        currentState = StretchingBody;
        activityLabel = "Stretching...";
        roboEyes.setMood(ROBO_TIRED);
        roboEyes.setWidth(26, 26);
        roboEyes.setHeight(22, 22);
        roboEyes.setBorderradius(10, 10);
        roboEyes.setPosition(ROBO_N);
        adjustEyesY();
        roboEyes.setAutoblinker(ROBO_OFF);
        roboEyes.setIdleMode(ROBO_OFF);
        roboEyes.setVFlicker(ROBO_OFF);
        roboEyes.setHFlicker(ROBO_OFF);
        roboEyes.setSweat(ROBO_OFF);
        roboEyes.setCuriosity(ROBO_OFF);
        stateStartTime = millis();
    }

    // Random activity picker
    void enterRandomActivity() {
        int r = random(0, 19);
        switch (r) {
            case 0: enterDrinkingState(); break;
            case 1: enterReadingState(); break;
            case 2: enterEatingState(); break;
            case 3: enterListeningState(); break;
            case 4: enterYawningState(); break;
            case 5: enterDancingState(); break;
            case 6: enterDrawingState(); break;
            case 7: enterGamingState(); break;
            case 8: enterEatingIceCreamState(); break;
            case 9: enterTakingPhotoState(); break;
            case 10: enterWritingState(); break;
            case 11: enterExercisingState(); break;
            case 12: enterBrushingTeethState(); break;
            case 13: enterCookingState(); break;
            case 14: enterBathingState(); break;
            case 15: enterCallingState(); break;
            case 16: enterEatingNoodlesState(); break;
            case 17: enterLookingMirrorState(); break;
            case 18: enterStretchingBodyState(); break;
        }
    }
};

#endif
