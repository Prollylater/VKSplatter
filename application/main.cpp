#include "HelloTriangle.h"

int main()
{
    ContextCreateInfo info = ContextCreateInfo::Default();
    HelloTriangleApplication app(info);

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