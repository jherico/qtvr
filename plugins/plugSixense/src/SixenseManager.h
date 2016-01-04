//
//  SixenseManager.h
//  input-plugins/src/input-plugins
//
//  Created by Andrzej Kapolka on 11/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SixenseManager_h
#define hifi_SixenseManager_h

#include <SimpleMovingAverage.h>

#include <controllers/InputDevice.h>
#include <controllers/StandardControls.h>

#include <plugins/InputPlugin.h>

struct _sixenseControllerData;
using SixenseControllerData = _sixenseControllerData;

// Handles interaction with the Sixense SDK (e.g., Razer Hydra).
class SixenseManager : public InputPlugin {
    Q_OBJECT
public:
    // Plugin functions
    virtual bool isSupported() const override;
    virtual bool isJointController() const override { return true; }
    const QString& getName() const override { return NAME; }
    const QString& getID() const override { return HYDRA_ID_STRING; }

    virtual void activate() override;
    virtual void deactivate() override;

    virtual void pluginFocusOutEvent() override { _inputDevice->focusOutEvent(); }
    virtual void pluginUpdate(float deltaTime, bool jointsCaptured) override;

    virtual void saveSettings() const override;
    virtual void loadSettings() override;

public slots:
    void setSixenseFilter(bool filter);

private:
    static const int MAX_NUM_AVERAGING_SAMPLES = 50; // At ~100 updates per seconds this means averaging over ~.5s
    static const int CALIBRATION_STATE_IDLE = 0;
    static const int CALIBRATION_STATE_IN_PROGRESS = 1;
    static const int CALIBRATION_STATE_COMPLETE = 2;
    static const glm::vec3 DEFAULT_AVATAR_POSITION;
    static const float CONTROLLER_THRESHOLD;

    template<typename T>
    using SampleAverage = MovingAverage<T, MAX_NUM_AVERAGING_SAMPLES>;
    using Samples = std::pair<SampleAverage<glm::vec3>, SampleAverage<glm::vec3>>;
    using MovingAverageMap = std::map<int, Samples>;

    class InputDevice : public controller::InputDevice {
    public:
        InputDevice() : controller::InputDevice("Hydra") {}
    private:
        // Device functions
        virtual controller::Input::NamedVector getAvailableInputs() const override;
        virtual QString getDefaultMappingConfig() const override;
        virtual void update(float deltaTime, bool jointsCaptured) override;
        virtual void focusOutEvent() override;

        void handleButtonEvent(unsigned int buttons, bool left);
        void handlePoseEvent(float deltaTime, glm::vec3 position, glm::quat rotation, bool left);
        void updateCalibration(SixenseControllerData* controllers);

        friend class SixenseManager;

        MovingAverageMap _collectedSamples;

        int _calibrationState { CALIBRATION_STATE_IDLE };
        // these are calibration results
        glm::vec3 _avatarPosition { DEFAULT_AVATAR_POSITION }; // in hydra-frame
        glm::quat _avatarRotation; // in hydra-frame

        float _lastDistance;
        bool _requestReset { false };
        bool _debugDrawRaw { false };
        bool _debugDrawCalibrated { false };
        // these are measured values used to compute the calibration results
        quint64 _lockExpiry;
        glm::vec3 _averageLeft;
        glm::vec3 _averageRight;
        glm::vec3 _reachLeft;
        glm::vec3 _reachRight;
    };

    std::shared_ptr<InputDevice> _inputDevice { std::make_shared<InputDevice>() };

    static const QString NAME;
    static const QString HYDRA_ID_STRING;
    
    static bool _sixenseLoaded;
};

#endif // hifi_SixenseManager_h

