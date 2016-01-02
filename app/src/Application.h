#include <app/RenderableApplication.h>

class Application : public RenderableApplication {
    Q_OBJECT
public:
    Application(int& argc, char** argv, QElapsedTimer& startup_time) : RenderableApplication(argc, argv, startup_time) {}
};