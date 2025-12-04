#include "src/Application.h"
#include <iostream>

int main(int argc, char **argv)
{
    Application app;

    if (!app.initialize())
    {
        std::cerr << "Failed to initialize application" << std::endl;
        return EXIT_FAILURE;
    }

    app.run();
    app.cleanup();

    return 0;
}
