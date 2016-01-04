//
//  SixenseManager.cpp
//  input-plugins/src/input-plugins
//
//  Created by Andrzej Kapolka on 11/15/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SixenseManager.h"

#include <sixense.h>

#include <QCoreApplication>
#include <QtCore/QSysInfo>
#include <QtGlobal>

#include <controllers/UserInputMapper.h>
#include <GLMHelpers.h>
#include <NumericalConstants.h>
#include <PathUtils.h>
#include <PerfStat.h>
#include <plugins/PluginApplication.h>
#include <SettingHandle.h>

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(inputplugins)
Q_LOGGING_CATEGORY(inputplugins, "hifi.inputplugins")

static const unsigned int BUTTON_0 = 1U << 0; // the skinny button between 1 and 2
static const unsigned int BUTTON_1 = 1U << 5;
static const unsigned int BUTTON_2 = 1U << 6;
static const unsigned int BUTTON_3 = 1U << 3;
static const unsigned int BUTTON_4 = 1U << 4;
static const unsigned int BUTTON_FWD = 1U << 7;
static const unsigned int BUTTON_TRIGGER = 1U << 8;

const glm::vec3 SixenseManager::DEFAULT_AVATAR_POSITION { -0.25f, -0.35f, -0.3f }; // in hydra frame
const float SixenseManager::CONTROLLER_THRESHOLD { 0.35f };
bool SixenseManager::_sixenseLoaded = false;

#define BAIL_IF_NOT_LOADED \
    if (!_sixenseLoaded) { \
        return; \
    }



const QString SixenseManager::NAME = "Sixense";
const QString SixenseManager::HYDRA_ID_STRING = "Razer Hydra";

const QString MENU_PARENT = "Avatar";
const QString MENU_NAME = "Sixense";
const QString MENU_PATH = MENU_PARENT + ">" + MENU_NAME;
const QString TOGGLE_SMOOTH = "Smooth Sixense Movement";
const QString SHOW_DEBUG_RAW = "Debug Draw Raw Data";
const QString SHOW_DEBUG_CALIBRATED = "Debug Draw Calibrated Data";

bool SixenseManager::isSupported() const {
#if defined(Q_OS_OSX)
    return QSysInfo::macVersion() <= QSysInfo::MV_MAVERICKS;
#else
    return true;
#endif
}

void SixenseManager::activate() {
    InputPlugin::activate();
    
    //_container->addMenu(MENU_PATH);
    //_container->addMenuItem(PluginType::INPUT_PLUGIN, MENU_PATH, TOGGLE_SMOOTH,
    //                       [this] (bool clicked) { setSixenseFilter(clicked); },
    //                       true, true);

    auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
    userInputMapper->registerDevice(_inputDevice);

    loadSettings();
    _sixenseLoaded = (sixenseInit() == SIXENSE_SUCCESS);
}

void SixenseManager::deactivate() {
    BAIL_IF_NOT_LOADED
    InputPlugin::deactivate();

    //_container->removeMenuItem(MENU_NAME, TOGGLE_SMOOTH);
    //_container->removeMenu(MENU_PATH);

    _inputDevice->_poseStateMap.clear();
    _inputDevice->_collectedSamples.clear();

    if (_inputDevice->_deviceID != controller::Input::INVALID_DEVICE) {
        auto userInputMapper = DependencyManager::get<controller::UserInputMapper>();
        userInputMapper->removeDevice(_inputDevice->_deviceID);
    }

    sixenseExit();
    saveSettings();
}

void SixenseManager::setSixenseFilter(bool filter) {
    BAIL_IF_NOT_LOADED
    sixenseSetFilterEnabled(filter ? 1 : 0);
}

void SixenseManager::pluginUpdate(float deltaTime, bool jointsCaptured) {
    BAIL_IF_NOT_LOADED
    _inputDevice->update(deltaTime, jointsCaptured);
    if (_inputDevice->_requestReset) {
        qApp->resetSensors();
        _inputDevice->_requestReset = false;
    }
}

void SixenseManager::InputDevice::update(float deltaTime, bool jointsCaptured) {
    BAIL_IF_NOT_LOADED
    _buttonPressedMap.clear();

    static const float MAX_DISCONNECTED_TIME = 2.0f;
    static bool disconnected { false };
    static float disconnectedInterval { 0.0f };
    if (sixenseGetNumActiveControllers() == 0) {
        if (!disconnected) {
            disconnectedInterval += deltaTime;
        }
        if (disconnectedInterval > MAX_DISCONNECTED_TIME) {
            disconnected = true;
            _axisStateMap.clear();
            _buttonPressedMap.clear();
            _poseStateMap.clear();
            _collectedSamples.clear();
        }
        return;
    }

    if (disconnected) {
        disconnected = 0;
        disconnectedInterval = 0.0f;
    }

    PerformanceTimer perfTimer("sixense");
    // FIXME send this message once when we've positively identified hydra hardware
    //UserActivityLogger::getInstance().connectedDevice("spatial_controller", "hydra");

    int maxControllers = sixenseGetMaxControllers();

    // we only support two controllers
    SixenseControllerData controllers[2];

    // store the raw controller data for debug rendering
    controller::Pose rawPoses[2];

    int numActiveControllers = 0;
    for (int i = 0; i < maxControllers && numActiveControllers < 2; i++) {
        if (!sixenseIsControllerEnabled(i)) {
            continue;
        }
        SixenseControllerData* data = controllers + numActiveControllers;
        ++numActiveControllers;
        sixenseGetNewestData(i, data);

        // NOTE: Sixense API returns pos data in millimeters but we IMMEDIATELY convert to meters.
        glm::vec3 position(data->pos[0], data->pos[1], data->pos[2]);
        position *= METERS_PER_MILLIMETER;
        bool left = i == 0;
        using namespace controller;
        // Check to see if this hand/controller is on the base
        const float CONTROLLER_AT_BASE_DISTANCE = 0.075f;
        if (glm::length(position) >= CONTROLLER_AT_BASE_DISTANCE) {
            handleButtonEvent(data->buttons, left);
            _axisStateMap[left ? LX : RX] = data->joystick_x;
            _axisStateMap[left ? LY : RY] = data->joystick_y;
            _axisStateMap[left ? LT : RT] = data->trigger;

            if (!jointsCaptured) {
                //  Rotation of Palm
                glm::quat rotation(data->rot_quat[3], data->rot_quat[0], data->rot_quat[1], data->rot_quat[2]);
                handlePoseEvent(deltaTime, position, rotation, left);
                rawPoses[i] = controller::Pose(position, rotation, Vectors::ZERO, Vectors::ZERO);
            } else {
                _poseStateMap.clear();
                _collectedSamples.clear();
            }
        } else {
            auto hand = left ? controller::StandardPoseChannel::LEFT_HAND : controller::StandardPoseChannel::RIGHT_HAND;
            _poseStateMap[hand] = controller::Pose();
            _collectedSamples[hand].first.clear();
            _collectedSamples[hand].second.clear();
        }
    }

    if (numActiveControllers == 2) {
        updateCalibration(controllers);
    }

    for (auto axisState : _axisStateMap) {
        if (fabsf(axisState.second) < CONTROLLER_THRESHOLD) {
            _axisStateMap[axisState.first] = 0.0f;
        }
    }
}

// the calibration sequence is:
// (1) reach arm straight out to the sides (xAxis is to the left)
// (2) press BUTTON_FWD on both hands and hold for one second
// (3) release both BUTTON_FWDs
//
// The code will:
// (4) assume that the orb is on a flat surface (yAxis is UP)
// (5) compute the forward direction (zAxis = xAxis cross yAxis)

static const float MINIMUM_ARM_REACH = 0.3f; // meters
static const float MAXIMUM_NOISE_LEVEL = 0.05f; // meters
static const quint64 LOCK_DURATION = USECS_PER_SECOND / 4; // time for lock to be acquired

static bool calibrationRequested(SixenseControllerData* controllers) {
    return (controllers[0].buttons == BUTTON_FWD && controllers[1].buttons == BUTTON_FWD);
}

void SixenseManager::InputDevice::updateCalibration(SixenseControllerData* controllers) {
    BAIL_IF_NOT_LOADED
    const SixenseControllerData* dataLeft = controllers;
    const SixenseControllerData* dataRight = controllers + 1;

    // Calibration buttons aren't set, so check the state, and request a reset if necessary.
    if (!calibrationRequested(controllers)) {
        switch (_calibrationState) {
            case CALIBRATION_STATE_IDLE:
                return;
                
            case CALIBRATION_STATE_COMPLETE: {
                    // compute calibration results
                    _avatarPosition = -0.5f * (_reachLeft + _reachRight); // neck is midway between right and left hands
                    glm::vec3 xAxis = glm::normalize(_reachRight - _reachLeft);
                    glm::vec3 zAxis = glm::normalize(glm::cross(xAxis, Vectors::UNIT_Y));
                    xAxis = glm::normalize(glm::cross(Vectors::UNIT_Y, zAxis));
                    _avatarRotation = glm::inverse(glm::quat_cast(glm::mat3(xAxis, Vectors::UNIT_Y, zAxis)));
                    const float Y_OFFSET_CALIBRATED_HANDS_TO_AVATAR = -0.3f;
                    _avatarPosition.y += Y_OFFSET_CALIBRATED_HANDS_TO_AVATAR;
                    qCDebug(inputplugins, "succeess: sixense calibration");
                    _requestReset = true;
                }
                break;
            default:
                qCDebug(inputplugins, "failed: sixense calibration");
                break;
        }

        _calibrationState = CALIBRATION_STATE_IDLE;
        return;
    }

    // Calibration buttons are set, continue calibration work
    // NOTE: Sixense API returns pos data in millimeters but we IMMEDIATELY convert to meters.
    const float* pos = dataLeft->pos;
    glm::vec3 positionLeft(pos[0], pos[1], pos[2]);
    positionLeft *= METERS_PER_MILLIMETER;
    pos = dataRight->pos;
    glm::vec3 positionRight(pos[0], pos[1], pos[2]);
    positionRight *= METERS_PER_MILLIMETER;

    // Gather initial calibration data
    if (_calibrationState == CALIBRATION_STATE_IDLE) {
        float reach = glm::distance(positionLeft, positionRight);
        if (reach > 2.0f * MINIMUM_ARM_REACH) {
            qCDebug(inputplugins, "started: sixense calibration");
            _averageLeft = positionLeft;
            _averageRight = positionRight;
            _reachLeft = _averageLeft;
            _reachRight = _averageRight;
            _lastDistance = reach;
            _lockExpiry = usecTimestampNow() + LOCK_DURATION;
            // move to next state
            _calibrationState = CALIBRATION_STATE_IN_PROGRESS;
        }
        return;
    }

    quint64 now = usecTimestampNow() + LOCK_DURATION;
    // these are weighted running averages
    _averageLeft = 0.9f * _averageLeft + 0.1f * positionLeft;
    _averageRight = 0.9f * _averageRight + 0.1f * positionRight;

    if (_calibrationState == CALIBRATION_STATE_IN_PROGRESS) {
        // compute new sliding average
        float distance = glm::distance(_averageLeft, _averageRight);
        if (fabsf(distance - _lastDistance) > MAXIMUM_NOISE_LEVEL) {
            // distance is increasing so acquire the data and push the expiry out
            _reachLeft = _averageLeft;
            _reachRight = _averageRight;
            _lastDistance = distance;
            _lockExpiry = now + LOCK_DURATION;
        } else if (now > _lockExpiry) {
            // lock has expired so clamp the data and move on
            _lockExpiry = now + LOCK_DURATION;
            _lastDistance = 0.0f;
            _calibrationState = CALIBRATION_STATE_COMPLETE;
            qCDebug(inputplugins, "success: sixense calibration: left");
        }
    }
}

void SixenseManager::InputDevice::focusOutEvent() {
    BAIL_IF_NOT_LOADED
    _axisStateMap.clear();
    _buttonPressedMap.clear();
};

void SixenseManager::InputDevice::handleButtonEvent(unsigned int buttons, bool left) {
    BAIL_IF_NOT_LOADED
    using namespace controller;
    if (buttons & BUTTON_0) {
        _buttonPressedMap.insert(left ? BACK : START);
    }
    if (buttons & BUTTON_1) {
        _buttonPressedMap.insert(left ? DL : X);
    }
    if (buttons & BUTTON_2) {
        _buttonPressedMap.insert(left ? DD : A);
    }
    if (buttons & BUTTON_3) {
        _buttonPressedMap.insert(left ? DR : B);
    }
    if (buttons & BUTTON_4) {
        _buttonPressedMap.insert(left ? DU : Y);
    }
    if (buttons & BUTTON_FWD) {
        _buttonPressedMap.insert(left ? LB : RB);
    }
    if (buttons & BUTTON_TRIGGER) {
        _buttonPressedMap.insert(left ? LS : RS);
    }
}

void SixenseManager::InputDevice::handlePoseEvent(float deltaTime, glm::vec3 position, glm::quat rotation, bool left) {
    BAIL_IF_NOT_LOADED
    auto hand = left ? controller::StandardPoseChannel::LEFT_HAND : controller::StandardPoseChannel::RIGHT_HAND;

    // From ABOVE the sixense coordinate frame looks like this:
    //
    //       |
    //   USB cables
    //       |
    //      .-.                             user
    //     (Orb) --neckX----               forward
    //      '-'                               |
    //       |                                |     user
    //     neckZ          y                   +---- right
    //       |             (o)-----x
    //                      |
    //                      |
    //                      z
    auto prevPose = _poseStateMap[hand];

    // Transform the measured position into body frame.
    position = _avatarRotation * (position + _avatarPosition);

    // From ABOVE the hand canonical axes look like this:
    //
    //      | | | |          y        | | | |
    //      | | | |          |        | | | |
    //      |     |          |        |     |
    //      |left | /  x----(+)     \ |right|
    //      |     _/           z     \_     |
    //       |   |                     |   |
    //       |   |                     |   |
    //

    // To convert sixense's delta-rotation into the hand's frame we will have to transform it like so:
    //
    //     deltaHand = Qsh^ * deltaSixense * Qsh
    //
    // where  Qsh = transform from sixense axes to hand axes.  By inspection we can determine Qsh:
    //
    //     Qsh = angleAxis(PI, zAxis) * angleAxis(-PI/2, xAxis)
    //
    const glm::quat sixenseToHand = glm::angleAxis(PI, Vectors::UNIT_Z) * glm::angleAxis(-PI/2.0f, Vectors::UNIT_X);

    // In addition to Qsh each hand has pre-offset introduced by the shape of the sixense controllers
    // and how they fit into the hand in their relaxed state.  This offset is a quarter turn about
    // the sixense's z-axis, with its direction different for the two hands:
    float sign = left ? 1.0f : -1.0f;
    const glm::quat preOffset = glm::angleAxis(sign * PI / 2.0f, Vectors::UNIT_Z);

    // Finally, there is a post-offset (same for both hands) to get the hand's rest orientation
    // (fingers forward, palm down) aligned properly in the avatar's model-frame,
    // and then a flip about the yAxis to get into model-frame.
    const glm::quat postOffset = glm::angleAxis(PI, Vectors::UNIT_Y) * glm::angleAxis(PI / 2.0f, Vectors::UNIT_X);

    // The total rotation of the hand uses the formula:
    //
    //     rotation = postOffset * Qsh^ * (measuredRotation * preOffset) * Qsh
    //
    // TODO: find a shortcut with fewer rotations.
    rotation = _avatarRotation * postOffset * glm::inverse(sixenseToHand) * rotation * preOffset * sixenseToHand;

    glm::vec3 velocity(0.0f);
    glm::vec3 angularVelocity(0.0f);

    if (prevPose.isValid() && deltaTime > std::numeric_limits<float>::epsilon()) {
        auto& samples = _collectedSamples[hand];

        velocity = (position - prevPose.getTranslation()) / deltaTime;
        samples.first.addSample(velocity);
        velocity = samples.first.average;

        auto deltaRot = glm::normalize(rotation * glm::conjugate(prevPose.getRotation()));
        auto axis = glm::axis(deltaRot);
        auto speed = glm::angle(deltaRot) / deltaTime;
        assert(!glm::isnan(speed));
        angularVelocity = speed * axis;
        samples.second.addSample(angularVelocity);
        angularVelocity = samples.second.average;
    } else if (!prevPose.isValid()) {
        _collectedSamples[hand].first.clear();
        _collectedSamples[hand].second.clear();
    }

    _poseStateMap[hand] = controller::Pose(position, rotation, velocity, angularVelocity);
}

static const auto L0 = controller::BACK;
static const auto L1 = controller::DL;
static const auto L2 = controller::DD;
static const auto L3 = controller::DR;
static const auto L4 = controller::DU;
static const auto R0 = controller::START;
static const auto R1 = controller::X;
static const auto R2 = controller::A;
static const auto R3 = controller::B;
static const auto R4 = controller::Y;

controller::Input::NamedVector SixenseManager::InputDevice::getAvailableInputs() const {
    using namespace controller;
    static const Input::NamedVector availableInputs {
        makePair(L0, "L0"),
        makePair(L1, "L1"),
        makePair(L2, "L2"),
        makePair(L3, "L3"),
        makePair(L4, "L4"),
        makePair(LB, "LB"),
        makePair(LS, "LS"),
        makePair(LX, "LX"),
        makePair(LY, "LY"),
        makePair(LT, "LT"),
        makePair(R0, "R0"),
        makePair(R1, "R1"),
        makePair(R2, "R2"),
        makePair(R3, "R3"),
        makePair(R4, "R4"),
        makePair(RB, "RB"),
        makePair(RS, "RS"),
        makePair(RX, "RX"),
        makePair(RY, "RY"),
        makePair(RT, "RT"),
        makePair(LEFT_HAND, "LeftHand"),
        makePair(RIGHT_HAND, "RightHand"),
    };
    return availableInputs;
};


QString SixenseManager::InputDevice::getDefaultMappingConfig() const {
    static const QString MAPPING_JSON = PathUtils::resourcesPath() + "/controllers/hydra.json";
    return MAPPING_JSON;
}

// virtual
void SixenseManager::saveSettings() const {
    Settings settings;
    QString idString = getID();
    settings.beginGroup(idString);
    {
        settings.setVec3Value(QString("avatarPosition"), _inputDevice->_avatarPosition);
        settings.setQuatValue(QString("avatarRotation"), _inputDevice->_avatarRotation);
    }
    settings.endGroup();
}

void SixenseManager::loadSettings() {
    Settings settings;
    QString idString = getID();
    settings.beginGroup(idString);
    {
        settings.getVec3ValueIfValid(QString("avatarPosition"), _inputDevice->_avatarPosition);
        settings.getQuatValueIfValid(QString("avatarRotation"), _inputDevice->_avatarRotation);
    }
    settings.endGroup();
}
