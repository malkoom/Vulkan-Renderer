#include "Engine.hpp"

int main()
{
    if (g_EnableValidationLayers == true) std::cout << "Capas de validación puestas" << std::endl;

    try
    {
        Engine app;
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "EXCEPCIÓN CAPTURADA: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    glfwTerminate();

    return EXIT_SUCCESS;
}