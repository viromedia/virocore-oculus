/*
 * Copyright (c) 2017-present, Viro, Inc.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.example.virosample;

import android.app.Activity;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Color;
import android.os.Bundle;
import android.util.Log;

import com.viro.core.Node;

import com.viro.core.Scene;
import com.viro.core.Text;
import com.viro.core.Texture;
import com.viro.core.Vector;
import com.viro.core.ViroView;
import com.viro.core.ViroViewGVR;
import com.viro.core.ViroViewOVR;

import java.io.IOException;
import java.io.InputStream;

/**
 * A sample Android activity for creating GVR and/or OVR stereoscopic scenes.
 * <p>
 * This activity automatically handles the creation of the VR renderer based on the currently
 * selected build variant in Android Studio.
 * <p>
 * Extend and override onRendererStart() to start building your 3D scenes.
 */
public class ViroActivityVR extends Activity {

    private static final String TAG = ViroActivityVR.class.getSimpleName();
    private ViroView mViroView;
    private AssetManager mAssetManager;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (BuildConfig.VIRO_PLATFORM.equalsIgnoreCase("GVR")) {
            mViroView = createGVRView();
        } else if (BuildConfig.VIRO_PLATFORM.equalsIgnoreCase("OVR")) {
            mViroView = createOVRView();
        }
        setContentView(mViroView);
    }

    private ViroView createGVRView() {
        ViroViewGVR viroView = new ViroViewGVR(this, new ViroViewGVR.StartupListener() {
            @Override
            public void onSuccess() {
                onRendererStart();
            }

            @Override
            public void onFailure(ViroViewGVR.StartupError error, String errorMessage) {
                onRendererFailed(error.toString(), errorMessage);
            }
        }, new Runnable() {
            @Override
            public void run() {
                Log.e(TAG, "On GVR userRequested exit");
            }
        });
        return viroView;
    }

    private ViroView createOVRView() {
        ViroViewOVR viroView = new ViroViewOVR(this, new ViroViewOVR.StartupListener() {
            @Override
            public void onSuccess() {
                onRendererStart();
            }

            @Override
            public void onFailure(ViroViewOVR.StartupError error, String errorMessage) {
                onRendererFailed(error.toString(), errorMessage);
            }
        });
        return viroView;
    }

    private void onRendererStart() {
        // Create your scene here. We provide a simple Hello World scene as an example
        createHelloWorldScene();
    }

    public void onRendererFailed(String error, String errorMessage) {
        // Fail as you wish!
    }

    private void createHelloWorldScene() {
        // Create a new Scene and get its root Node
        Scene scene = new Scene();
        Node rootNode = scene.getRootNode();

        // Load the background image into a Bitmap file from assets
        Bitmap backgroundBitmap = bitmapFromAsset("guadalupe_360.jpg");

        // Add a 360 Background Texture if we were able to find the Bitmap
        if (backgroundBitmap != null) {
            Texture backgroundTexture = new Texture(backgroundBitmap, Texture.Format.RGBA8, true, true);
            scene.setBackgroundTexture(backgroundTexture);
        }

        // Create a Text geometry
        Text helloWorldText = new Text.TextBuilder().viroContext(mViroView.getViroContext()).
                textString("Hello World").
                fontFamilyName("Roboto").fontSize(50).
                color(Color.WHITE).
                width(4).height(2).
                horizontalAlignment(Text.HorizontalAlignment.CENTER).
                verticalAlignment(Text.VerticalAlignment.CENTER).
                lineBreakMode(Text.LineBreakMode.NONE).
                clipMode(Text.ClipMode.CLIP_TO_BOUNDS).
                maxLines(1).build();

        // Create a Node, position it, and attach the Text geometry to it
        Node textNode = new Node();
        textNode.setPosition(new Vector(0, 0, -2));
        textNode.setGeometry(helloWorldText);

        // Attach the textNode to the Scene's rootNode.
        rootNode.addChildNode(textNode);
        mViroView.setScene(scene);
    }

    private Bitmap bitmapFromAsset(String assetName) {
        if (mAssetManager == null) {
            mAssetManager = getResources().getAssets();
        }

        InputStream imageStream;
        try {
            imageStream = mAssetManager.open(assetName);
        } catch (IOException exception) {
            Log.w(TAG, "Unable to find image ["+assetName+"] in assets!");
            return null;
        }
        return BitmapFactory.decodeStream(imageStream);
    }

    @Override
    protected void onStart() {
        super.onStart();
        mViroView.onActivityStarted(this);
    }

    @Override
    protected void onResume() {
        super.onResume();
        mViroView.onActivityResumed(this);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mViroView.onActivityPaused(this);
    }

    @Override
    protected void onStop() {
        super.onStop();
        mViroView.onActivityStopped(this);
    }
}