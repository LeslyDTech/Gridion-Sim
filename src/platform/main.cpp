#include "App.h"

#if defined(__EMSCRIPTEN__)
#include <memory>
#endif

int main() {
#if defined(__EMSCRIPTEN__)
    // In the browser run() never returns: the page calls App::frame() for as long as the tab is
    // open. App lives on the heap so it outlives main()'s stack frame; the browser frees
    // everything when the tab closes.
    static std::unique_ptr<gridiron::App> app = std::make_unique<gridiron::App>();
    app->run();
#else
    gridiron::App app;
    app.run();
#endif
    return 0;
}
