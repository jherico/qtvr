#include <plugins/PluginApplication.h>

#include "shadertoy/Shadertoy.h"
#include "shadertoy/Renderer.h"
#include "shadertoy/ShaderModel.h"

class Application : public PluginApplication {
    Q_OBJECT
public:
    Application(int& argc, char** argv);

protected:
    virtual void paintGL() override;
    virtual void resizeGL() override;
    virtual void cleanupBeforeQuit() override;
    virtual void aboutToQuit() override;
    virtual void initializeUI(const QUrl& desktopUrl) override;

protected slots:
    void loadShader(const QString& shaderId);

private:
    shadertoy::Renderer _renderer;
    ShaderModel* _model{ nullptr };
};
