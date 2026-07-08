xlair_add_siv3d_application(XLAIR
    BUNDLE_IDENTIFIER "dev.xlair.XLAIR"
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
        MACOS   EMBED "icon.icns"
        WINDOWS EMBED "icon.ico"
)

target_link_libraries(XLAIR PRIVATE Siv3D::Siv3D)
