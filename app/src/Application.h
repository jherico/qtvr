#include <plugins/PluginApplication.h>
#include <QSortFilterProxyModel>

#include "shadertoy/Shadertoy.h"
#include "shadertoy/Renderer.h"
#include "shadertoy/Model.h"

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
    void onUpdateCode(const QString& code);

private:
    shadertoy::Renderer _renderer;
    std::shared_ptr<QSortFilterProxyModel> _proxy;
    std::shared_ptr<shadertoy::Model> _model;
};
