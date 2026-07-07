xlair_add_siv3d_application(SheetsViewer
    BUNDLE_IDENTIFIER "dev.xlair.SheetsViewer"
    RESOURCE_ENTRIES
        ALL     EMBED "engine/font"
        MACOS   EMBED "engine/lib/soundtouch"
        WINDOWS EMBED "engine/shader/d3d11"
        MACOS   EMBED "engine/shader/msl"
        LINUX   EMBED "engine/shader/essl"
        ALL     EMBED "engine/shader/glsl"
        ALL     EMBED "engine/soundfont"
        ALL     EMBED "engine/texture"
        WINDOWS COPY  "dll"
)

target_link_libraries(SheetsViewer PRIVATE XLAIR::SheetsAnalyzer)
