//
//  Created by Benjamin Arnold on 5/27/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "CompositorHelper.h"

#include <memory>
#include <math.h>

#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtGui/QWindow>
#include <QtGui/QScreen>
#include <QtWidgets/QDesktopWidget>
#include <glm/gtc/type_ptr.hpp>

#include <NumericalConstants.h>
#include <DependencyManager.h>
#include <CursorManager.h>

#include "../../PluginApplication.h"
// Used to animate the magnification windows

//static const quint64 TOOLTIP_DELAY = 500 * MSECS_TO_USECS;

static const float reticleSize = TWO_PI / 100.0f;

//EntityItemID CompositorHelper::_noItemId;
static QString _tooltipId;

const uvec2 CompositorHelper::VIRTUAL_SCREEN_SIZE = uvec2(3960, 1188); // ~10% more pixel density than old version, 72dx240d FOV
const float CompositorHelper::VIRTUAL_UI_ASPECT_RATIO = (float)VIRTUAL_SCREEN_SIZE.x / (float)VIRTUAL_SCREEN_SIZE.y;
const vec2 CompositorHelper::VIRTUAL_UI_TARGET_FOV = vec2(PI * 3.0f / 2.0f, PI * 3.0f / 2.0f / VIRTUAL_UI_ASPECT_RATIO);
const vec2 CompositorHelper::MOUSE_EXTENTS_ANGULAR_SIZE = vec2(PI * 2.0f, PI * 0.95f); // horizontal: full sphere,  vertical: ~5deg from poles
const vec2 CompositorHelper::MOUSE_EXTENTS_PIXELS = vec2(VIRTUAL_SCREEN_SIZE) * (MOUSE_EXTENTS_ANGULAR_SIZE / VIRTUAL_UI_TARGET_FOV);

// Return a point's cartesian coordinates on a sphere from pitch and yaw
glm::vec3 getPoint(float yaw, float pitch) {
    return glm::vec3(glm::cos(-pitch) * (-glm::sin(yaw)),
                     glm::sin(-pitch),
                     glm::cos(-pitch) * (-glm::cos(yaw)));
}

// FIXME move to GLMHelpers
//Checks if the given ray intersects the sphere at the origin. result will store a multiplier that should
//be multiplied by dir and added to origin to get the location of the collision
bool raySphereIntersect(const glm::vec3 &dir, const glm::vec3 &origin, float r, float* result)
{
    //Source: http://wiki.cgsociety.org/index.php/Ray_Sphere_Intersection

    //Compute A, B and C coefficients
    float a = glm::dot(dir, dir);
    float b = 2 * glm::dot(dir, origin);
    float c = glm::dot(origin, origin) - (r * r);

    //Find discriminant
    float disc = b * b - 4 * a * c;

    // if discriminant is negative there are no real roots, so return
    // false as ray misses sphere
    if (disc < 0) {
        return false;
    }

    // compute q as described above
    float distSqrt = sqrtf(disc);
    float q;
    if (b < 0) {
        q = (-b - distSqrt) / 2.0f;
    } else {
        q = (-b + distSqrt) / 2.0f;
    }

    // compute t0 and t1
    float t0 = q / a;
    float t1 = c / q;

    // make sure t0 is smaller than t1
    if (t0 > t1) {
        // if t0 is bigger than t1 swap them around
        float temp = t0;
        t0 = t1;
        t1 = temp;
    }

    // if t1 is less than zero, the object is in the ray's negative direction
    // and consequently the ray misses the sphere
    if (t1 < 0) {
        return false;
    }

    // if t0 is less than zero, the intersection point is at t1
    if (t0 < 0) {
        *result = t1;
        return true;
    } else { // else the intersection point is at t0
        *result = t0;
        return true;
    }
}

CompositorHelper::CompositorHelper() :
    _alphaPropertyAnimation(new QPropertyAnimation(this, "alpha")),
    _reticleInterface(new ReticleInterface(this))
{
    // FIX in a separate PR addressing the current mouse over entity bug
    //auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
    //connect(entityScriptingInterface.data(), &EntityScriptingInterface::hoverEnterEntity, [=](const EntityItemID& entityItemID, const MouseEvent& event) {
    //    if (_hoverItemId != entityItemID) {
    //        _hoverItemId = entityItemID;
    //        _hoverItemEnterUsecs = usecTimestampNow();
    //        auto properties = entityScriptingInterface->getEntityProperties(_hoverItemId);

    //        // check the format of this href string before we parse it
    //        QString hrefString = properties.getHref();

    //        auto cursor = Cursor::Manager::instance().getCursor();
    //        if (!hrefString.isEmpty()) {
    //            if (!hrefString.startsWith("hifi:")) {
    //                hrefString.prepend("hifi://");
    //            }

    //            // parse out a QUrl from the hrefString
    //            QUrl href = QUrl(hrefString);

    //            _hoverItemTitle = href.host();
    //            _hoverItemDescription = properties.getDescription();
    //            cursor->setIcon(Cursor::Icon::LINK);
    //        } else {
    //            _hoverItemTitle.clear();
    //            _hoverItemDescription.clear();
    //            cursor->setIcon(Cursor::Icon::DEFAULT);
    //        }
    //    }
    //});

    //connect(entityScriptingInterface.data(), &EntityScriptingInterface::hoverLeaveEntity, [=](const EntityItemID& entityItemID, const MouseEvent& event) {
    //    if (_hoverItemId == entityItemID) {
    //        _hoverItemId = _noItemId;

    //        _hoverItemTitle.clear();
    //        _hoverItemDescription.clear();

    //        auto cursor = Cursor::Manager::instance().getCursor();
    //        cursor->setIcon(Cursor::Icon::DEFAULT);
    //        if (!_tooltipId.isEmpty()) {
    //            qDebug() << "Closing tooltip " << _tooltipId;
    //            Tooltip::closeTip(_tooltipId);
    //            _tooltipId.clear();
    //        }
    //    }
    //});
}


bool CompositorHelper::isHMD() const {
    return _currentDisplayPlugin && _currentDisplayPlugin->isHmd();
}

QPointF CompositorHelper::getMouseEventPosition(QMouseEvent* event) {
    if (isHMD()) {
        QMutexLocker locker(&_reticleLock);
        return QPointF(_reticlePositionInHMD.x, _reticlePositionInHMD.y);
    }
    return event->localPos();
}

bool CompositorHelper::shouldCaptureMouse() const {
    // if we're in HMD mode, and some window of ours is active, but we're not currently showing a popup menu
    return _allowMouseCapture && isHMD() && _renderingSurface->isActive();
}

void CompositorHelper::setAllowMouseCapture(bool capture) {
    if (capture != _allowMouseCapture) {
        _allowMouseCapture = capture;
        emit allowMouseCaptureChanged();
    }
    _allowMouseCapture = capture;
}

void CompositorHelper::handleLeaveEvent() {
    if (shouldCaptureMouse()) {
        
        //QWidget* mainWidget = (QWidget*)qApp->getWindow();
        static QWindow* mainWindow = nullptr;
        if (mainWindow == nullptr && _renderingSurface != nullptr) {
            mainWindow = _renderingSurface;
        }
        QRect mainWidgetFrame;
        {
            mainWidgetFrame = _renderingSurface->geometry();
            auto topLeft = mainWidgetFrame.topLeft();
            auto topLeftScreen = _renderingSurface->mapToGlobal(topLeft);
            mainWidgetFrame.moveTopLeft(topLeftScreen);
        }
        QRect uncoveredRect = mainWidgetFrame;
        for (auto window : qApp->topLevelWindows()) {
            if (window->isVisible() && window != _renderingSurface) {
                QRect widgetFrame = window->frameGeometry();
                if (widgetFrame.intersects(uncoveredRect)) {
                    QRect intersection = uncoveredRect & widgetFrame;
                    if (intersection.top() > uncoveredRect.top()) {
                        uncoveredRect.setBottom(intersection.top() - 1);
                    } else if (intersection.bottom() < uncoveredRect.bottom()) {
                        uncoveredRect.setTop(intersection.bottom() + 1);
                    }

                    if (intersection.left() > uncoveredRect.left()) {
                        uncoveredRect.setRight(intersection.left() - 1);
                    } else if (intersection.right() < uncoveredRect.right()) {
                        uncoveredRect.setLeft(intersection.right() + 1);
                    }
                }
            }
        }

        _ignoreMouseMove = true;
        auto sendToPos = uncoveredRect.center();
        QCursor::setPos(sendToPos);
        _lastKnownRealMouse = sendToPos;
    }
}


bool CompositorHelper::handleRealMouseMoveEvent(bool sendFakeEvent) {
    // If the mouse move came from a capture mouse related move, we completely ignore it.
    if (_ignoreMouseMove) {
        _ignoreMouseMove = false;
        return true; // swallow the event
    }

    // If we're in HMD mode
    if (shouldCaptureMouse()) {
        QMutexLocker locker(&_reticleLock);
        auto newPosition = QCursor::pos();
        auto changeInRealMouse = newPosition - _lastKnownRealMouse;
        auto newReticlePosition = _reticlePositionInHMD + toGlm(changeInRealMouse);
        setReticlePosition(newReticlePosition, sendFakeEvent);
        _ignoreMouseMove = true;
        QCursor::setPos(QPoint(_lastKnownRealMouse.x(), _lastKnownRealMouse.y())); // move cursor back to where it was
        return true;  // swallow the event
    } else {
        _lastKnownRealMouse = QCursor::pos();
    }
    return false; // let the caller know to process the event
}

glm::vec2 CompositorHelper::getReticlePosition() const {
    if (isHMD()) {
        QMutexLocker locker(&_reticleLock);
        return _reticlePositionInHMD;
    }
    return toGlm(QCursor::pos());
}

bool CompositorHelper::getReticleOverDesktop() const {
    // if the QML/Offscreen UI thinks we're over the desktop, then we are...
    // but... if we're outside of the overlay area, we also want to call ourselves
    // as being over the desktop.
    if (isHMD()) {
        QMutexLocker locker(&_reticleLock);
        glm::vec2 maxOverlayPosition = _currentDisplayPlugin->getRecommendedUiSize();
        static const glm::vec2 minOverlayPosition;
        if (glm::any(glm::lessThan(_reticlePositionInHMD, minOverlayPosition)) ||
            glm::any(glm::greaterThan(_reticlePositionInHMD, maxOverlayPosition))) {
            return true;
        }
    }
    return _isOverDesktop;
}

glm::vec2 CompositorHelper::getReticleMaximumPosition() const {
    glm::vec2 result;
    if (isHMD()) {
        result = VIRTUAL_SCREEN_SIZE;
    } else {
        QRect rec = qApp->screens()[0]->geometry();
        result = glm::vec2(rec.right(), rec.bottom());
    }
    return result;
}

void CompositorHelper::sendFakeMouseEvent() {
    if (qApp->thread() != QThread::currentThread()) {
        QMetaObject::invokeMethod(this, "sendFakeMouseEvent", Qt::BlockingQueuedConnection);
        return;
    }

    if (_renderingSurface) {
        // in HMD mode we need to fake our mouse moves...
        QPoint globalPos(_reticlePositionInHMD.x, _reticlePositionInHMD.y);
        auto button = Qt::NoButton;
        auto buttons = QGuiApplication::mouseButtons();
        auto modifiers = QGuiApplication::keyboardModifiers();
        QMouseEvent event(QEvent::MouseMove, globalPos, button, buttons, modifiers);
        _fakeMouseEvent = true;
        qApp->sendEvent(_renderingSurface, &event);
        _fakeMouseEvent = false;
    }
}

void CompositorHelper::setReticlePosition(const glm::vec2& position, bool sendFakeEvent) {
    if (isHMD()) {
        glm::vec2 maxOverlayPosition = _currentDisplayPlugin->getRecommendedUiSize();
        // FIXME don't allow negative mouseExtra
        glm::vec2 mouseExtra = (MOUSE_EXTENTS_PIXELS - maxOverlayPosition) / 2.0f;
        glm::vec2 minMouse = vec2(0) - mouseExtra;
        glm::vec2 maxMouse = maxOverlayPosition + mouseExtra;

        {
            QMutexLocker locker(&_reticleLock);
            _reticlePositionInHMD = glm::clamp(position, minMouse, maxMouse);
        }

        if (sendFakeEvent) {
            sendFakeMouseEvent();
        }
    } else {
        // NOTE: This is some debugging code we will leave in while debugging various reticle movement strategies,
        // remove it after we're done
        const float REASONABLE_CHANGE = 50.0f;
        glm::vec2 oldPos = toGlm(QCursor::pos());
        auto distance = glm::distance(oldPos, position);
        if (distance > REASONABLE_CHANGE) {
            qDebug() << "Contrller::ScriptingInterface ---- UNREASONABLE CHANGE! distance:" <<
                distance << " oldPos:" << oldPos.x << "," << oldPos.y << " newPos:" << position.x << "," << position.y;
        }

        QCursor::setPos(position.x, position.y);
    }
}

void CompositorHelper::computeHmdPickRay(const glm::vec2& cursorPos, glm::vec3& origin, glm::vec3& direction) const {
    auto surfacePointAt = sphereSurfaceFromOverlay(cursorPos); // in world space
    origin = vec3(_currentCamera[3]); 
    direction = glm::normalize(surfacePointAt - origin);
}

glm::mat4 CompositorHelper::getUiTransform() const {
    return _currentCamera * glm::inverse(_currentDisplayPlugin->getHeadPose());
}

//Finds the collision point of a world space ray
bool CompositorHelper::calculateRayUICollisionPoint(const glm::vec3& position, const glm::vec3& direction, glm::vec3& result) const {
    auto UITransform = getUiTransform();
    auto relativePosition4 = glm::inverse(UITransform) * vec4(position, 1);
    auto relativePosition = vec3(relativePosition4) / relativePosition4.w;
    auto relativeDirection = glm::inverse(glm::quat_cast(UITransform)) * direction;

    float uiRadius = _oculusUIRadius; // * myAvatar->getUniformScale(); // FIXME - how do we want to handle avatar scale

    float instersectionDistance;
    if (raySphereIntersect(relativeDirection, relativePosition, uiRadius, &instersectionDistance)){
        result = position + glm::normalize(direction) * instersectionDistance;
        return true;
    }

    return false;
}

glm::vec2 CompositorHelper::sphericalToOverlay(const glm::vec2& sphericalPos) const {
    glm::vec2 result = sphericalPos;
    result.x *= -1.0f;
    result /= _textureFov;
    result.x /= _textureAspectRatio;
    result += 0.5f;
    result *= _currentDisplayPlugin->getRecommendedUiSize();
    return result;
}

glm::vec2 CompositorHelper::overlayToSpherical(const glm::vec2& overlayPos) const {
    glm::vec2 result = overlayPos;
    result /= _currentDisplayPlugin->getRecommendedUiSize();
    result -= 0.5f;
    result *= _textureFov;
    result.x *= _textureAspectRatio;
    result.x *= -1.0f;
    return result;
}

glm::vec2 CompositorHelper::overlayFromSphereSurface(const glm::vec3& sphereSurfacePoint) const {
    auto UITransform = getUiTransform();
    auto relativePosition4 = glm::inverse(UITransform) * vec4(sphereSurfacePoint, 1);
    auto direction = vec3(relativePosition4) / relativePosition4.w;
    // FIXME use a GLMHelper cartesianToSpherical after fixing the rotation signs.
    glm::vec2 polar = glm::vec2(glm::atan(direction.x, -direction.z), glm::asin(direction.y)) * -1.0f;
    auto overlayPos = sphericalToOverlay(polar);
    return overlayPos;
}

glm::vec3 CompositorHelper::sphereSurfaceFromOverlay(const glm::vec2& overlay) const {
    auto spherical = overlayToSpherical(overlay);
    // FIXME use a GLMHelper sphericalToCartesian after fixing the rotation signs.
    auto sphereSurfacePoint = getPoint(spherical.x, spherical.y);
    auto UITransform = getUiTransform();
    auto position4 = UITransform * vec4(sphereSurfacePoint, 1);
    return vec3(position4) / position4.w;
}

void CompositorHelper::updateTooltips() {
    //if (_hoverItemId != _noItemId) {
    //    quint64 hoverDuration = usecTimestampNow() - _hoverItemEnterUsecs;
    //    if (_hoverItemEnterUsecs != UINT64_MAX && !_hoverItemTitle.isEmpty() && hoverDuration > TOOLTIP_DELAY) {
    //        // TODO Enable and position the tooltip
    //        _hoverItemEnterUsecs = UINT64_MAX;
    //        _tooltipId = Tooltip::showTip(_hoverItemTitle, _hoverItemDescription);
    //    }
    //}
}

static const float FADE_DURATION = 500.0f;
static const float FADE_IN_ALPHA = 1.0f;
static const float FADE_OUT_ALPHA = 0.0f;

void CompositorHelper::startFadeFailsafe(float endValue) {
    _fadeStarted = usecTimestampNow();
    _fadeFailsafeEndValue = endValue;

    const int SLIGHT_DELAY = 10;
    QTimer::singleShot(FADE_DURATION + SLIGHT_DELAY, [this]{
        checkFadeFailsafe();
    });
}

void CompositorHelper::checkFadeFailsafe() {
    auto elapsedInFade = usecTimestampNow() - _fadeStarted;
    if (elapsedInFade > FADE_DURATION) {
        setAlpha(_fadeFailsafeEndValue);
    }
}

void CompositorHelper::fadeIn() {
    _fadeInAlpha = true;

    _alphaPropertyAnimation->setDuration(FADE_DURATION);
    _alphaPropertyAnimation->setStartValue(_alpha);
    _alphaPropertyAnimation->setEndValue(FADE_IN_ALPHA);
    _alphaPropertyAnimation->start();

    // Sometimes, this "QPropertyAnimation" fails to complete the animation, and we end up with a partially faded
    // state. So we will also have this fail-safe, where we record the timestamp of the fadeRequest, and the target
    // value of the fade, and if after that time we still haven't faded all the way, we will kick it to the final
    // fade value
    startFadeFailsafe(FADE_IN_ALPHA);
}

void CompositorHelper::fadeOut() {
    _fadeInAlpha = false;

    _alphaPropertyAnimation->setDuration(FADE_DURATION);
    _alphaPropertyAnimation->setStartValue(_alpha);
    _alphaPropertyAnimation->setEndValue(FADE_OUT_ALPHA);
    _alphaPropertyAnimation->start();
    startFadeFailsafe(FADE_OUT_ALPHA);
}

void CompositorHelper::toggle() {
    if (_fadeInAlpha) {
        fadeOut();
    } else {
        fadeIn();
    }
}

glm::mat4 CompositorHelper::getReticleTransform(const glm::mat4& eyePose, const glm::vec3& headPosition) const {
    glm::mat4 result;
    if (isHMD()) {
        vec3 reticleScale = vec3(Cursor::Manager::instance().getScale() * reticleSize);
        auto reticlePosition = getReticlePosition();
        auto spherical = overlayToSpherical(reticlePosition);
        // The pointer transform relative to the sensor
        auto pointerTransform = glm::mat4_cast(quat(vec3(-spherical.y, spherical.x, 0.0f))) * glm::translate(mat4(), vec3(0, 0, -1));
        float reticleDepth = getReticleDepth();
        if (reticleDepth != 1.0f) {
            // Cursor position in UI space
            auto cursorPosition = vec3(pointerTransform[3]) / pointerTransform[3].w;
            // Ray to the cursor, in UI space
            auto cursorRay = glm::normalize(cursorPosition - headPosition) * reticleDepth;
            // Move the ray to be relative to the head pose
            pointerTransform[3] = vec4(cursorRay + headPosition, 1);
            // Scale up the cursor because of distance
            reticleScale *= reticleDepth;
        }
        glm::mat4 overlayXfm;
        pointerTransform = overlayXfm * pointerTransform;
        pointerTransform = glm::inverse(eyePose) * pointerTransform;
        result = glm::scale(pointerTransform, reticleScale);
    } else {
        static const float CURSOR_PIXEL_SIZE = 32.0f;
        const auto canvasSize = vec2(toGlm(_renderingSurface->size()));;
        vec2 mousePosition = toGlm(_renderingSurface->mapFromGlobal(QCursor::pos()));
        mousePosition /= canvasSize;
        mousePosition *= 2.0;
        mousePosition -= 1.0;
        mousePosition.y *= -1.0f;

        vec2 mouseSize = CURSOR_PIXEL_SIZE / canvasSize;
        result = glm::scale(glm::translate(glm::mat4(), vec3(mousePosition, 0.0f)), vec3(mouseSize, 1.0f));
    }
    return result;
}
