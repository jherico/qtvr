#include <UiApplication.h>

#include "shadertoy/Shadertoy.h"
#include "shadertoy/Renderer.h"

class Application : public UiApplication {
    Q_OBJECT
public:
    Application(int& argc, char** argv);

protected:
    virtual void paintGL() override;
    virtual void cleanupBeforeQuit() override;
    virtual void aboutToQuit() override;

private:
    shadertoy::Renderer _renderer;

};
