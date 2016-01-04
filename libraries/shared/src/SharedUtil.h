//
//  SharedUtil.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 2/22/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SharedUtil_h
#define hifi_SharedUtil_h

#include <memory>
#include <mutex>
#include <math.h>
#include <stdint.h>

#ifndef _WIN32
#include <unistd.h> // not on windows, not needed for mac or windows
#endif

#include <QtCore/QDebug>
#include <QtCore/QCoreApplication>

// Provides efficient access to a named global type.  By storing the value
// in the QApplication by name we can implement the singleton pattern and 
// have the single instance function across DLL boundaries.  
template <typename T, typename... Args>
T* globalInstance(const char* propertyName, Args&&... args) {
    static std::unique_ptr<T> instancePtr;
    static T* resultInstance { nullptr };
    static std::mutex mutex;
    if (!resultInstance) {
        std::unique_lock<std::mutex> lock(mutex);
        if (!resultInstance) {
            auto variant = qApp->property(propertyName);
            if (variant.isNull()) {
                // Since we're building the object, store it in a shared_ptr so it's 
                // destroyed by the destructor of the static instancePtr
                instancePtr = std::unique_ptr<T>(new T(std::forward<Args>(args)...));

                void* voidInstance = &(*instancePtr);
                variant = QVariant::fromValue(voidInstance);
                qApp->setProperty(propertyName, variant);
            }
            void* returnedVoidInstance = variant.value<void*>();
            resultInstance = static_cast<T*>(returnedVoidInstance);
        }
    }
    return resultInstance;
}


// Equivalent to time_t but in usecs instead of secs
quint64 usecTimestampNow(bool wantDebug = false);
void usecTimestampNowForceClockSkew(int clockSkew);

// Number of seconds expressed since the first call to this function, expressed as a float
// Maximum accuracy in msecs
float secTimestampNow();

float randFloat();
int randIntInRange (int min, int max);
float randFloatInRange (float min,float max);
float randomSign(); /// \return -1.0 or 1.0
unsigned char randomColorValue(int minimum = 0);
bool randomBoolean();
bool shouldDo(float desiredInterval, float deltaTime);

const char* getCmdOption(int argc, const char * argv[],const char* option);
bool cmdOptionExists(int argc, const char * argv[],const char* option);

#ifdef _WIN32
void usleep(int waitTime);
#endif

/// \return bool is the float NaN
bool isNaN(float value);

template <typename T>
uint qHash(const std::shared_ptr<T>& ptr, uint seed = 0)
{
    return qHash(ptr.get(), seed);
}

#endif // hifi_SharedUtil_h
