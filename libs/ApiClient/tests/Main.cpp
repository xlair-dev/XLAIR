#define CATCH_CONFIG_MAIN
#include <ThirdParty/Catch2/catch.hpp>

// Siv3D's Windows library references its application entry point even though
// Catch2 supplies the console entry point for this headless test executable.
void Main() {}
