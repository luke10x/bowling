package org.libsdl.app;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.StandardCharsets;
import com.google.android.gms.ads.AdError;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.FullScreenContentCallback;
import com.google.android.gms.ads.LoadAdError;
import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.ads.interstitial.InterstitialAd;
import com.google.android.gms.ads.interstitial.InterstitialAdLoadCallback;

/** Android document picker bridge for tracker songs. No storage permission is needed. */
public class TrackerActivity extends SDLActivity {
    private static final String TAG = "TrackerSongFiles";
    private static final int OPEN_SONG = 4101;
    private static final int SAVE_SONG = 4102;
    private static final int MAX_SONG_BYTES = 8 * 1024 * 1024;
    private byte[] pendingExport;
    // A loaded ad is consumed once and replenished on close.
    private InterstitialAd lossAd;
    private boolean lossAdLoading;
    private boolean lossAdReturnPending;
    private static native void nativeInterstitialShown();
    private static native void nativeInterstitialFinished();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        MobileAds.initialize(this, status -> runOnUiThread(this::loadLossAd));
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (lossAdReturnPending) restoreAfterLossAd();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus && lossAdReturnPending) restoreAfterLossAd();
    }

    private void loadLossAd() {
        if (lossAd != null || lossAdLoading) return;
        lossAdLoading = true;
        InterstitialAd.load(this, getString(R.string.admob_loss_unit), new AdRequest.Builder().build(),
            new InterstitialAdLoadCallback() {
                @Override public void onAdLoaded(InterstitialAd ad) {
                    lossAdLoading = false;
                    lossAd = ad;
                }
                @Override public void onAdFailedToLoad(LoadAdError error) {
                    lossAdLoading = false;
                    Log.w(TAG, "Loss interstitial unavailable: " + error.getMessage());
                }
            });
    }

    // Called at a charged win or retry. Never wait for a load: the game
    // remains usable when offline or when Google has no ad.
    public void showGameInterstitial() {
        runOnUiThread(() -> {
            InterstitialAd ad = lossAd;
            if (ad == null) {
                nativeInterstitialFinished();
                loadLossAd();
                return;
            }
            lossAd = null;
            ad.setFullScreenContentCallback(new FullScreenContentCallback() {
                @Override public void onAdShowedFullScreenContent() {
                    Log.i(TAG, "Game interstitial opened");
                    nativeInterstitialShown();
                }
                @Override public void onAdDismissedFullScreenContent() {
                    Log.i(TAG, "Game interstitial dismissed");
                    nativeInterstitialFinished();
                    lossAdReturnPending = true;
                    restoreAfterLossAd();
                    loadLossAd();
                }
                @Override public void onAdFailedToShowFullScreenContent(AdError error) {
                    nativeInterstitialFinished();
                    lossAdReturnPending = true;
                    restoreAfterLossAd();
                    Log.w(TAG, "Loss interstitial could not open: " + error.getMessage());
                    loadLossAd();
                }
            });
            ad.show(this);
        });
    }

    private void restoreAfterLossAd() {
        // The ad has its own Activity. Let Android finish returning to ours before
        // reconciling SDL's surface/focus state. Its lifecycle already pauses and
        // resumes audio; touching the SDL audio device from this UI callback races
        // with the game thread closing that device on SDL_WINDOWEVENT_MINIMIZED.
        getWindow().getDecorView().post(() -> {
            Log.i(TAG, "Loss ad return: activityResumed=" + mIsResumedCalled +
                " windowFocus=" + hasWindowFocus() +
                " surfaceReady=" + (mSurface != null && mSurface.mIsSurfaceReady) +
                " SDL=" + mCurrentNativeState + "->" + mNextNativeState);
            if (isFinishing() || !hasWindowFocus() || !mIsResumedCalled ||
                mSurface == null || !mSurface.mIsSurfaceReady) return;
            mSurface.requestFocus();
            mHasFocus = true;
            mNextNativeState = NativeState.RESUMED;
            handleNativeState();
            lossAdReturnPending = false;
        });
    }

    private static native void nativeSongLoaded(String filename, byte[] contents);
    private static native void nativeSongPickerClosed();
    private static native void nativeSongSaved(boolean success);

    public void openTrackerSong() {
        runOnUiThread(() -> {
            try {
                Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                // Providers do not agree on a MIME type for .h files.
                intent.setType("*/*");
                startActivityForResult(intent, OPEN_SONG);
            } catch (ActivityNotFoundException | SecurityException e) {
                Log.e(TAG, "Could not open document picker", e);
                nativeSongPickerClosed();
            }
        });
    }

    public void saveTrackerSong(String filename, byte[] contents) {
        runOnUiThread(() -> {
            if (pendingExport != null) {
                nativeSongSaved(false);
                return;
            }
            pendingExport = contents;
            try {
                Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.setType("text/plain");
                intent.putExtra(Intent.EXTRA_TITLE, filename);
                startActivityForResult(intent, SAVE_SONG);
            } catch (ActivityNotFoundException | SecurityException e) {
                Log.e(TAG, "Could not open save picker", e);
                pendingExport = null;
                nativeSongSaved(false);
            }
        });
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == OPEN_SONG) {
            if (resultCode != Activity.RESULT_OK || data == null || data.getData() == null) {
                nativeSongPickerClosed();
                return;
            }
            Uri uri = data.getData();
            String name = getDocumentName(uri);
            try (InputStream stream = getContentResolver().openInputStream(uri)) {
                if (stream == null) throw new java.io.IOException("Empty document stream");
                ByteArrayOutputStream buffer = new ByteArrayOutputStream();
                byte[] chunk = new byte[8192];
                int count;
                while ((count = stream.read(chunk)) != -1) {
                    if (buffer.size() + count > MAX_SONG_BYTES)
                        throw new java.io.IOException("Song file is too large");
                    buffer.write(chunk, 0, count);
                }
                byte[] bytes = buffer.toByteArray();
                // Reject invalid UTF-8 before passing text to the existing song parser.
                String text = StandardCharsets.UTF_8.newDecoder()
                    .onMalformedInput(CodingErrorAction.REPORT)
                    .onUnmappableCharacter(CodingErrorAction.REPORT)
                    .decode(ByteBuffer.wrap(bytes)).toString();
                if (text.indexOf('\0') >= 0)
                    throw new CharacterCodingException();
                nativeSongLoaded(name, bytes);
            } catch (Exception e) {
                Log.e(TAG, "Could not read tracker song", e);
                nativeSongLoaded(name, null);
            }
        } else if (requestCode == SAVE_SONG) {
            byte[] bytes = pendingExport;
            pendingExport = null;
            if (resultCode != Activity.RESULT_OK || data == null || data.getData() == null || bytes == null) {
                nativeSongSaved(false);
                return;
            }
            try (OutputStream stream = getContentResolver().openOutputStream(data.getData(), "wt")) {
                if (stream == null) throw new java.io.IOException("Empty document stream");
                stream.write(bytes);
                stream.flush();
                nativeSongSaved(true);
            } catch (Exception e) {
                Log.e(TAG, "Could not save tracker song", e);
                nativeSongSaved(false);
            }
        }
    }

    private String getDocumentName(Uri uri) {
        try (Cursor cursor = getContentResolver().query(uri, null, null, null, null)) {
            if (cursor != null && cursor.moveToFirst()) {
                int column = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                if (column >= 0 && !cursor.isNull(column))
                    return cursor.getString(column);
            }
        } catch (Exception e) {
            Log.w(TAG, "Document name unavailable", e);
        }
        return "song.h";
    }
}
