#include "first_app.hpp"
#include <spdlog/spdlog.h>

int main()
{
    lve::FirstApp app{};
    try
    {
        app.run();
    }
    catch (std::exception &e)
    {
        spdlog::error(e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}