
#include "./Engine/FGEngine.hpp"

extern "C" {
void android_main(struct android_app* state);
};

void android_main(struct android_app* app) {
    app_dummy(); // Make sure glue isn't stripped.
    FGEngine *engine = new FGEngine(app);
    engine->GameLoop();
    delete engine;
}
