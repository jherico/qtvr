//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_UiApplication_h
#define hifi_UiApplication_h

#include <plugins/PluginApplication.h>

class OffscreenUi;

class UiApplication : public PluginApplication  {
    Q_OBJECT
    
public:
    UiApplication(int& argc, char** argv);
    virtual ~UiApplication();

    void initializeUI();
    void resizeGL();

    OffscreenUi* getOffscreenUi() { return _offscreenUi; }
    const OffscreenUi* getOffscreenUi() const { return _offscreenUi; }

protected:
    virtual void cleanupBeforeQuit() override;

private:
    OffscreenUi* _offscreenUi { nullptr };
    uint32_t _uiTexture { 0 };
    bool _reticleClickPressed { false };
};

#if defined(qApp)
#undef qApp
#endif
#define qApp (static_cast<UiApplication*>(QCoreApplication::instance()))

#endif // hifi_Application_h
