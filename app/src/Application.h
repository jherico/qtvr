#include <UiApplication.h>

class Application : public UiApplication {
    Q_OBJECT
public:
    Application(int& argc, char** argv) : UiApplication(argc, argv) {}
};