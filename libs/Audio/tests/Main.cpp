#define CATCH_CONFIG_MAIN
#include <ThirdParty/Catch2/catch.hpp>

// The Windows SDK references Siv3D's entry point even in headless console tests.
void Main() {}
