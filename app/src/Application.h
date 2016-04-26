#include <plugins/PluginApplication.h>
#include <QSortFilterProxyModel>

#include "shadertoy/Shadertoy.h"
#include "shadertoy/Renderer.h"
#include "shadertoy/Model.h"

class Application : public PluginApplication {
    Q_OBJECT
    using Parent = PluginApplication;
public:
    Application(int& argc, char** argv);

protected:
    void paintGL() override;
    void resizeGL() override;
    void aboutToQuit() override;
    void initializeUI(const QUrl& desktopUrl) override;

//protected slots:
//    void loadShaderObject(const QVariant& shader);
//    void loadShader(const QString& shaderId);
//    void onUpdateCode(const QString& code);

private:
    shadertoy::Renderer _renderer;
//    std::shared_ptr<QSortFilterProxyModel> _proxy;
//    std::shared_ptr<shadertoy::Model> _model;
};
