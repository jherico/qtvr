//
//  SDL2Manager.h
//  input-plugins/src/input-plugins
//
//  Created by Sam Gondelman on 6/5/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi__SDL2Manager_h
#define hifi__SDL2Manager_h

#ifdef HAVE_SDL2
#include <SDL.h>
#endif

#include <controllers/UserInputMapper.h>
#include <plugins/InputPlugin.h>
#include "Joystick.h"

class SDL2Manager : public InputPlugin {
    Q_OBJECT
    
public:
    SDL2Manager();
    
    // Plugin functions
    virtual bool isSupported() const override;
    virtual bool isJointController() const override { return false; }
    virtual const QString& getName() const override { return NAME; }

    virtual void init() override;
    virtual void deinit() override;

    /// Called when a plugin is being activated for use.  May be called multiple times.
    virtual bool activate() override;
    /// Called when a plugin is no longer being used.  May be called multiple times.
    virtual void deactivate() override;

    virtual void pluginFocusOutEvent() override;
    virtual void pluginUpdate(float deltaTime, const controller::InputCalibrationData& inputCalibrationData, bool jointsCaptured) override;
    
signals:
    void joystickAdded(Joystick* joystick);
    void joystickRemoved(Joystick* joystick);
    
private:
#ifdef HAVE_SDL2
    SDL_JoystickID getInstanceId(SDL_GameController* controller);
    
    int axisInvalid() const { return SDL_CONTROLLER_AXIS_INVALID; }
    int axisLeftX() const { return SDL_CONTROLLER_AXIS_LEFTX; }
    int axisLeftY() const { return SDL_CONTROLLER_AXIS_LEFTY; }
    int axisRightX() const { return SDL_CONTROLLER_AXIS_RIGHTX; }
    int axisRightY() const { return SDL_CONTROLLER_AXIS_RIGHTY; }
    int axisTriggerLeft() const { return SDL_CONTROLLER_AXIS_TRIGGERLEFT; }
    int axisTriggerRight() const { return SDL_CONTROLLER_AXIS_TRIGGERRIGHT; }
    int axisMax() const { return SDL_CONTROLLER_AXIS_MAX; }
    
    int buttonInvalid() const { return SDL_CONTROLLER_BUTTON_INVALID; }
    int buttonFaceBottom() const { return SDL_CONTROLLER_BUTTON_A; }
    int buttonFaceRight() const { return SDL_CONTROLLER_BUTTON_B; }
    int buttonFaceLeft() const { return SDL_CONTROLLER_BUTTON_X; }
    int buttonFaceTop() const { return SDL_CONTROLLER_BUTTON_Y; }
    int buttonBack() const { return SDL_CONTROLLER_BUTTON_BACK; }
    int buttonGuide() const { return SDL_CONTROLLER_BUTTON_GUIDE; }
    int buttonStart() const { return SDL_CONTROLLER_BUTTON_START; }
    int buttonLeftStick() const { return SDL_CONTROLLER_BUTTON_LEFTSTICK; }
    int buttonRightStick() const { return SDL_CONTROLLER_BUTTON_RIGHTSTICK; }
    int buttonLeftShoulder() const { return SDL_CONTROLLER_BUTTON_LEFTSHOULDER; }
    int buttonRightShoulder() const { return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER; }
    int buttonDpadUp() const { return SDL_CONTROLLER_BUTTON_DPAD_UP; }
    int buttonDpadDown() const { return SDL_CONTROLLER_BUTTON_DPAD_DOWN; }
    int buttonDpadLeft() const { return SDL_CONTROLLER_BUTTON_DPAD_LEFT; }
    int buttonDpadRight() const { return SDL_CONTROLLER_BUTTON_DPAD_RIGHT; }
    int buttonMax() const { return SDL_CONTROLLER_BUTTON_MAX; }
    
    int buttonPressed() const { return SDL_PRESSED; }
    int buttonRelease() const { return SDL_RELEASED; }

    QMap<SDL_JoystickID, Joystick::Pointer> _openJoysticks;
#endif
    bool _isInitialized;
    static const QString NAME;
};

#endif // hifi__SDL2Manager_h
