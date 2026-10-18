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

/** Android document picker bridge for tracker songs. No storage permission is needed. */
public class TrackerActivity extends SDLActivity {
    private static final String TAG = "TrackerSongFiles";
    private static final int OPEN_SONG = 4101;
    private static final int SAVE_SONG = 4102;
    private static final int MAX_SONG_BYTES = 8 * 1024 * 1024;
    private byte[] pendingExport;

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
