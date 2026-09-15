# macOS build support

`macos.cmake` builds Rulr Core/Nodes and their addons into one shared library,
so applications and dynamically loaded plugins share the same registries and state.
Include it from an application located under `openFrameworks/apps/<application>`.
It expects the unmodified openFrameworks 0.12.1 macOS release and its compiled
`libs/openFrameworksCompiled/lib/osx/openFrameworks.a`.

Build that archive using oF's supplied `openFrameworksLib.xcodeproj`, Release,
with the host architecture. Install CMake, Ninja, Ceres and NLopt; provide OpenCV
4.12 with contrib modules through `OpenCV_DIR`. OpenCV 5 is not supported.
The include defines `RulrLibrary` and `rulr_add_plugin(name)` for plugins in this
addon. Link application and plugin targets to `RulrLibrary`.

The application must copy `Application/bin/data/assets` and
`ofxCvGui/data/assets` to its `bin/data/assets`, build the application sources as
a macOS bundle, and place plugin dylibs beside that bundle in `bin`.
Use `@executable_path/../../..` in the application's build RPATH.
Supply `NSCameraUsageDescription` in its Info.plist for AVFoundation camera use.

This build enables the standard webcam, StillImages and VideoPlayer devices.
Canon (default on Windows), KinectV2OSX and Blackmagic integrations are optional
through `RULR_WITH_CANON`, `RULR_WITH_KINECT_V2_OSX` and `RULR_WITH_BLACKMAGIC`.
Enabling a macro also requires adding that SDK and addon to the build.
Only Apple Silicon has been verified. The output uses local dependency paths
and is intended for development, not redistribution as a standalone bundle.
