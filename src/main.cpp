#include <vk_engine.h>

int main()
{
    VenomApp* app = new VenomApp();

    try
    {
        app->run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
