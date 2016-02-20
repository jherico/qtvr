#include <HifiApplication.h>
#include <vulkan/VKWindow.h>

class Application : public HifiApplication {
    Q_OBJECT
public:
    Application(int& argc, char** argv);

    virtual void cleanupBeforeQuit() override;
    virtual void aboutToQuit() override;

private:
    VKWindow* _window { nullptr };
};
