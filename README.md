<p align="center">
<img src="https://github.com/dthian/virocore-oculus/blob/master/Logo.png">
</p>

ViroCore Oculus
=====================

ViroCore Oculus is SceneKit for Android, a 3D framework for developers to build immersive applications using Java. ViroCore combines a high-performance rendering engine with a descriptive API for creating 3D, AR, and VR apps. While lower-level APIs like OpenGL require you to precisely implement complex rendering algorithms, ViroCore requires only high-level scene descriptions, and code for the interactivity and animations you want your application to perform.

The repository contains both the rendering source code, and as well as the ViroCore platform, primarily for the Oculus Quest platform. Both are free to use with no limits on distribution.

To report bugs/issues related to Viro-Oculus, please file new issues on this repository.

### Releases
ViroCore is automatically built by our piplines here: ![Viro Renderer CI Pipeline](https://github.com/dthian/virocore/workflows/Viro%20Renderer%20CI%20Pipeline/badge.svg)

## Quick Start
### Running sample code instructions:
You can get up and running with the latest stable release of ViroCore! To do so, simply:
1. Clone the repo into your workspace with git: `git clone https://github.com/viromedia/virocore.git`.
2. Go to the code-sample directory for a list of current samples.
3. Choose the code sample you wish to deploy, and open the root directory in Android studio. 
4. Build and deploy.
5. You should now be in the application! Enjoy!

## Manual Building of the Renderer

If you would like to modify / make changes to the renderer directly. These are the instructions for building the renderer and ViroCore platform. 

### Building the ViroCore platform:
1. Follow the same prerequisite directions above from our [Quick start guide](https://virocore.viromedia.com/docs/getting-started).
2. Clone the repo into your workspace with git: `git clone https://github.com/viromedia/virocore.git`.
3. Execute the following commands to build the ViroCore platform library
   ```
   $ cd android
   $ ./gradlew :virocore:assembleRelease
   ```
4. If the above gradle build succeeded, verify you see a `virocore-*.aar` file (* for the version number) at `android/virocore/build/outputs/aar/virocore-*.aar`
5. To run ViroCore tests, open the android project at `android/app` in Android Studio and run `releasetest` target on your android device.
6. To use this updated / newly built `virocore-*.aar` in your own project copy the aar file to `viro_core/` in your project and modify your `viro_core/build.gradle` to point to the new file.

## More Information

Viro Media Website: https://viromedia.com/

ViroCore Documentation: https://virocore.viromedia.com/

API Reference(Java Docs): https://developer.viromedia.com/

Check out our [blog](https://blog.viromedia.com/) for tutorials, news, and updates.
