/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.apache.android.media;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.net.URLConnection;
import java.util.HashMap;
import android.app.Activity;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.webkit.URLUtil;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.Toast;
import android.widget.VideoView;

public class VideoViewDemo extends Activity {
	private static final String TAG = "VideoViewDemo";

	private VideoView mVideoView;
	private EditText mPath;
	private ImageButton mPlay;
	private ImageButton mPause;
	private ImageButton mReset;
	private ImageButton mStop;
	private String current;
	private Thread _thread;

	@Override
	public void onCreate(Bundle icicle) {
		super.onCreate(icicle);
		setContentView(R.layout.main);
		mVideoView = (VideoView) findViewById(R.id.surface_view);

		mPath = (EditText) findViewById(R.id.path);
		//mPath.setText("file:///sdcard/foo.3gp");

		final String host = "rtsp://localhost";
		final int port = 4321;

		//mPath.setText(stringFromJNI( host, port ));
		//mpath.setText("rtsp://192.168.1.14:5554/test.sdp");
		//rtsp://video2.multicasttech.com/AFTVSciFiH264250.sdp");

		_thread = new Thread() {
			public void run() {
			    EnvRun("0.0.0.0", 5544);
			}
		    };
		_thread.start();


		mPlay = (ImageButton) findViewById(R.id.play);
		mPause = (ImageButton) findViewById(R.id.pause);
		mReset = (ImageButton) findViewById(R.id.reset);
		mStop = (ImageButton) findViewById(R.id.stop);

		mPlay.setOnClickListener(new OnClickListener() {
			public void onClick(View view) {
				playVideo();
			}
		});
		mPause.setOnClickListener(new OnClickListener() {
			public void onClick(View view) {
// 				if (mVideoView != null) {
// 					mVideoView.pause();
// 				}
			    startVideo();
			}
		});
		mReset.setOnClickListener(new OnClickListener() {
			public void onClick(View view) {
				if (mVideoView != null) {
					mVideoView.seekTo(0);
				}
			}
		});
		mStop.setOnClickListener(new OnClickListener() {
			public void onClick(View view) {
				if (mVideoView != null) {
					current = null;
					mVideoView.stopPlayback();
				}
			}
		});
		runOnUiThread(new Runnable(){
			public void run() {
			    //playVideo();
				
			}
			
		});
	}

    private void startVideo() {
	HashMap<Object,Object> contextHandle = ContextCreate();
	Log.v(TAG, "contextcreate: " + contextHandle.toString());
	HashMap<Object,Object> result;
	result = (HashMap<Object,Object>)contextHandle.get("response");
	result = (HashMap<Object,Object>)result.get("parameters");
	int contextId = ((Long)result.get("contextId")).intValue();
	Log.v(TAG, "contextId: " + contextId);

	HashMap<Object,Object> playResult = CommandPlay(contextId, "http://mediadownloads.mlb.com/mlbam/2010/06/29/9505835_m3u8/128/dropf_9505835_100m_128K.m3u8", "", "");
	Log.v(TAG, "playResult: " + playResult.toString());
    }

	private void playVideo() {
		try {
			final String path = mPath.getText().toString();
			Log.v(TAG, "path: " + path);
			if (path == null || path.length() == 0) {
				Toast.makeText(VideoViewDemo.this, "File URL/path is empty",
						Toast.LENGTH_LONG).show();

			} else {
				// If the path has not changed, just start the media player
				if (path.equals(current) && mVideoView != null) {
					mVideoView.start();
					mVideoView.requestFocus();
					return;
				}
				current = path;

				mVideoView.setVideoURI(Uri.parse( path ));
				mVideoView.start();
				mVideoView.requestFocus();

			}
		} catch (Exception e) {
			Log.e(TAG, "error: " + e.getMessage(), e);
			if (mVideoView != null) {
				mVideoView.stopPlayback();
			}
		}
	}

	private native void EnvRun(String ip, int port);
	private native void EnvStop();
	private native HashMap<Object,Object> ContextCreate();
	private native HashMap<Object,Object> ContextList();
	private native HashMap<Object,Object> ContextClose(int contextId);
	private native HashMap<Object,Object> ContextCloseAll();
	private native HashMap<Object,Object> CommandPlay(int contextId, String m3u8Uri, String httpSessionId, String keyPassword);
    private native HashMap<Object,Object> CommandPause(int contextId);
	private native HashMap<Object,Object> CommandResume(int contextId);
	static {
        System.loadLibrary("crtmpserver_dynamic");
    }
}

