#define SHADER_DIR "./assets/shaders/"
#define TEXTURE_DIR "./assets/textures/"
#define MODEL_DIR "./assets/models/"

#include <vk_engine.h>

int main()
{
    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
