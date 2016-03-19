//
//  ThreadHelpers.h
//
//  Created by Bradley Austin Davis on 2015-04-04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_ThreadHelpers_h
#define hifi_ThreadHelpers_h

#include <exception>
#include <QMutex>
#include <QMutexLocker>

#include "HifiApplication.h"

template <typename L, typename F>
void withLock(L lock, F function) {
    throw std::exception();
}

template <typename F>
void withLock(QMutex& lock, F function) {
    QMutexLocker locker(&lock);
    function();
}

class LambdaEvent : public QEvent {
    std::function<void()> _fun;
public:
    LambdaEvent(const std::function<void()> & fun) :
        QEvent(static_cast<QEvent::Type>(HifiApplication::Lambda)), _fun(fun) {
    }
    LambdaEvent(std::function<void()> && fun) :
        QEvent(static_cast<QEvent::Type>(HifiApplication::Lambda)), _fun(fun) {
    }
    void call() { _fun(); }
};


#endif
