<p align="center">
<img src="https://github.com/dthian/virocore-oculus/blob/master/Logo.png">
</p>

ViroCore Oculus
=====================

ViroCore Oculus is SceneKit for Android, a 3D framework for developers to build immersive applications using Java. ViroCore combines a high-performance rendering engine with a descriptive API for creating 3D, AR, and VR apps. While lower-level APIs like OpenGL require you to precisely implement complex rendering algorithms, ViroCore requires only high-level scene descriptions, and code for the interactivity and animations you want your application to perform.

The repository contains both the rendering source code, and as well as the ViroCore platform, primarily for the Oculus Quest platform. Both are free to use with no limits on distribution.

To report bugs/issues related to Viro-Oculus, please file new issues on this repository.

### Releases
Built releases can be seen under the [build folder](https://github.com/dthian/virocore-oculus/tree/master/build).
To actually re-build the renderer yourself, look at the Manual building steps shown below.

## Quick Start
There's 2 ways to get started with ViroCore.

### Option 1: Running sample code instructions
With this option, you can simply get up and running quickly with a skeletal hello world demo.
1. Clone the repo into your workspace with git: `git clone https://github.com/viromedia/virocore.git`.
2. Go to the code-sample directory to access the HelloWorld code sample, and open the root directory in Android studio. 
3. Build and deploy.
4. You should now be in the application! Enjoy!

### Option 2: Running Render Activity instructions
With this option, you can actually re-compile the renderer itself and run an Android activity that manually tests certain 3D components.
1. Clone the repo into your workspace with git: `git clone https://github.com/viromedia/virocore.git`.
2. Go to the /android directory from within the project's root dir. Open this directory in Android Studio.
3. Ensure that you are building to "rendertest". 
4. Target your device and build. This should target [this activity](https://github.com/dthian/virocore-oculus/blob/master/android/renderertest/src/main/java/com/viromedia/renderertest/ViroActivity.java).

## Manual Re-Building of the Renderer
If you would like to modify / make changes to the renderer directly. These are the instructions for building the renderer and ViroCore platform. 

### Building the renderer to be used in react-viro platform:
1. Follow the same prerequisite directions above from our [Quick start guide](https://virocore.viromedia.com/docs/getting-started).
2. Clone this repo into your workspace with git: `git clone https://github.com/viromedia/virocore.git`.
3. Clone the react-viro repo (named viro) in the same workspace (same parent directory as virocore) with git: `https://github.com/viromedia/viro.git`
4. Execute the following commands to build the ViroCore platform library
   ```
   $ cd android
   $ ./gradlew :viroreact:assembleRelease
   ```
5. If the above gradle build succeeded, verify you see a new `viroreact-release.aar` file at `viro/android/viro_renderer/viro_renderer-release.aar`. The build instructions outlined in [ViroReact Oculus](https://github.com/dthian/viroreact-oculus) repo will walk you through steps involved in building the react-viro bridge using this built renderer.

Note that a copy of the built renderer is also seen under the [build folder](https://github.com/dthian/virocore-oculus/tree/master/build). 

### Building the ViroCore platform:
1. Follow the same prerequisite directions above from our [Quick start guide](https://virocore.viromedia.com/docs/getting-started).
2. Clone this repo into your workspace with git: `git clone https://github.com/viromedia/virocore.git`.
3. Execute the following commands to build the ViroCore platform library
   ```
   $ cd android
   $ ./gradlew :virocore:assembleRelease
   ```
4. If the above gradle build succeeded, verify you see a `virocore-*.aar` file (* for the version number) at `android/virocore/build/outputs/aar/virocore-*.aar`
5. To use this updated / newly built `virocore-*.aar` in your own project copy the aar file to `viro_core/` in your project and modify your `viro_core/build.gradle` to point to the new file.

Note: ViroCore tests are *not* situable for running (As they were tailored for GVR / AR environemnt like testing).

## More Information

Viro Media Website: https://viromedia.com/

ViroCore Documentation: https://virocore.viromedia.com/

API Reference(Java Docs): https://developer.viromedia.com/

Check out our [blog](https://blog.viromedia.com/) for tutorials, news, and updates.
