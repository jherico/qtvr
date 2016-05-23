//
//  GLBackendTransform.cpp
//  libraries/gpu/src/gpu
//
//  Created by Sam Gateau on 3/8/2015.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "GLBackend.h"

using namespace gpu;
using namespace gpu::gl;

// Transform Stage
void GLBackend::do_setModelTransform(Batch& batch, size_t paramOffset) {
}

void GLBackend::do_setViewTransform(Batch& batch, size_t paramOffset) {
    _transform._view = batch._transforms.get(batch._params[paramOffset]._uint);
    _transform._invalidView = true;
}

void GLBackend::do_setProjectionTransform(Batch& batch, size_t paramOffset) {
    memcpy(&_transform._projection, batch.editData(batch._params[paramOffset]._uint), sizeof(Mat4));
    _transform._invalidProj = true;
}

void GLBackend::do_setViewportTransform(Batch& batch, size_t paramOffset) {
    memcpy(&_transform._viewport, batch.editData(batch._params[paramOffset]._uint), sizeof(Vec4i));

    if (!_inRenderTransferPass && !isStereo()) {
        ivec4& vp = _transform._viewport;
        glViewport(vp.x, vp.y, vp.z, vp.w);
    }

    // The Viewport is tagged invalid because the CameraTransformUBO is not up to date and will need update on next drawcall
    _transform._invalidViewport = true;
}

void GLBackend::do_setDepthRangeTransform(Batch& batch, size_t paramOffset) {

    Vec2 depthRange(batch._params[paramOffset + 1]._float, batch._params[paramOffset + 0]._float);

    if ((depthRange.x != _transform._depthRange.x) || (depthRange.y != _transform._depthRange.y)) {
        _transform._depthRange = depthRange;
        
        glDepthRangef(depthRange.x, depthRange.y);
    }
}

void GLBackend::killTransform() {
    glDeleteBuffers(1, &_transform._objectBuffer);
    glDeleteBuffers(1, &_transform._cameraBuffer);
    glDeleteBuffers(1, &_transform._drawCallInfoBuffer);
    glDeleteTextures(1, &_transform._objectBufferTexture);
}

void GLBackend::syncTransformStateCache() {
    _transform._invalidViewport = true;
    _transform._invalidProj = true;
    _transform._invalidView = true;

    glGetIntegerv(GL_VIEWPORT, (GLint*) &_transform._viewport);

    glGetFloatv(GL_DEPTH_RANGE, (GLfloat*)&_transform._depthRange);

    Mat4 modelView;
    auto modelViewInv = glm::inverse(modelView);
    _transform._view = modelViewInv;
}

void GLBackend::TransformStageState::preUpdate(size_t commandIndex, const StereoState& stereo) {
    // Check all the dirty flags and update the state accordingly
    if (_invalidViewport) {
        _camera._viewport = glm::vec4(_viewport);
    }

    if (_invalidProj) {
        _camera._projection = _projection;
    }

    if (_invalidView) {
        // This is when the _view matrix gets assigned
        _view = glm::inverse(_camera._view);
    }

    if (_invalidView || _invalidProj || _invalidViewport) {
        size_t offset = _cameraUboSize * _cameras.size();
        _cameraOffsets.push_back(TransformStageState::Pair(commandIndex, offset));
        if (stereo._enable) {
            _cameras.push_back((_camera.getEyeCamera(0, stereo, _view)));
            _cameras.push_back((_camera.getEyeCamera(1, stereo, _view)));
        } else {
            _cameras.push_back((_camera.recomputeDerived(_view)));
        }

    }

    // Flags are clean
    _invalidView = _invalidProj = _invalidViewport = false;
}

void GLBackend::TransformStageState::update(size_t commandIndex, const StereoState& stereo) const {
    size_t offset = INVALID_OFFSET;
    while ((_camerasItr != _cameraOffsets.end()) && (commandIndex >= (*_camerasItr).first)) {
        offset = (*_camerasItr).second;
        _currentCameraOffset = offset;
        ++_camerasItr;
    }

    if (offset != INVALID_OFFSET) {
        if (!stereo._enable) {
            bindCurrentCamera(0);
        }
    }
    (void)CHECK_GL_ERROR();
}

void GLBackend::TransformStageState::bindCurrentCamera(int eye) const {
    if (_currentCameraOffset != INVALID_OFFSET) {
        glBindBufferRange(GL_UNIFORM_BUFFER, TRANSFORM_CAMERA_SLOT, _cameraBuffer, _currentCameraOffset + eye * _cameraUboSize, sizeof(CameraBufferElement));
    }
}

void GLBackend::updateTransform(const Batch& batch) {
    _transform.update(_commandIndex, _stereo);

    auto& drawCallInfoBuffer = batch.getDrawCallInfoBuffer();
    if (batch._currentNamedCall.empty()) {
        auto& drawCallInfo = drawCallInfoBuffer[_currentDraw];
        glDisableVertexAttribArray(gpu::Stream::DRAW_CALL_INFO); // Make sure attrib array is disabled
        glVertexAttribI2i(gpu::Stream::DRAW_CALL_INFO, drawCallInfo.index, drawCallInfo.unused);
    } else {
        glEnableVertexAttribArray(gpu::Stream::DRAW_CALL_INFO); // Make sure attrib array is enabled
        glBindBuffer(GL_ARRAY_BUFFER, _transform._drawCallInfoBuffer);
        glVertexAttribIPointer(gpu::Stream::DRAW_CALL_INFO, 2, GL_UNSIGNED_SHORT, 0,
                               _transform._drawCallInfoOffsets[batch._currentNamedCall]);
        glVertexAttribDivisor(gpu::Stream::DRAW_CALL_INFO, 1);
    }
    
    (void)CHECK_GL_ERROR();
}

void GLBackend::resetTransformStage() {
    
}
