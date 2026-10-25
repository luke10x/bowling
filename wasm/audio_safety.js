// Loaded with --pre-js, before SDL opens its Web Audio output.
(function () {
    'use strict';
    var output = null;
    var fatal = false;
    var lastPulse = -Infinity;
    var timeoutSeconds = 1.5;
    var fadeSeconds = 0.05;
    var lastResumeAttempt = -Infinity;
    var resumeNeeded = true;
    var wasBackgrounded = false;
    var returnedAt = 0;
    var callbackSeenAfterReturn = false;
    var reopenRequested = false;
    var reopenAttempts = 0;

    function clearBuffer(buffer) {
        for (var channel = 0; channel < buffer.numberOfChannels; ++channel)
            buffer.getChannelData(channel).fill(0);
    }

    function mute() {
        if (!output) return;
        resumeNeeded = true;
        var now = output.context.currentTime;
        output.gate.gain.cancelScheduledValues(now);
        output.gate.gain.setValueAtTime(0, now);
        lastPulse = -Infinity;
    }

    function stop(reason, isFatal) {
        fatal = fatal || !!isFatal;
        resumeNeeded = true;
        mute();
        if (output) {
            var old = output;
            output = null;
            old.node.onaudioprocess = function (event) { clearBuffer(event.outputBuffer); };
            old.node.disconnect();
            old.gate.disconnect();
            // SDL may still be feeding a dummy buffer while autoplay is blocked.
            if (old.audio.silenceTimer !== undefined) {
                clearInterval(old.audio.silenceTimer);
                old.audio.silenceTimer = undefined;
            }
            if (old.context.state !== 'closed') {
                var stopped = fatal ? old.context.close() : old.context.suspend();
                stopped.catch(function () {});
            }
        }
        if (reason) console.warn('[AudioSafety] ' + reason);
    }

    function pulse() {
        if (!output || fatal || document.hidden) return;
        if (returnedAt && !callbackSeenAfterReturn && !reopenRequested &&
                reopenAttempts < 2 && Date.now() - returnedAt > 1800) {
            console.warn('[AudioSafety] No audio callback after foreground; reopening SDL audio (Web Audio state: ' +
                output.context.state + ')');
            reopenRequested = true;
        }
        var now = output.context.currentTime;
        if (now - lastPulse < 0.25) return;
        lastPulse = now;
        var gain = output.gate.gain;
        gain.cancelScheduledValues(now);
        gain.setValueAtTime(1, now);
        gain.setValueAtTime(1, now + timeoutSeconds);
        gain.linearRampToValueAtTime(0, now + timeoutSeconds + fadeSeconds);
        // This deadline runs on the audio rendering thread. A JS timer alone
        // cannot silence a repeating buffer when the main thread is stuck.
    }

    function resume(fromGesture) {
        if (fatal || document.hidden || !resumeNeeded) return;
        var sdl = Module['SDL2'];
        var context = output ? output.context : sdl && sdl.audioContext;
        if (!context || context.state === 'closed') return;
        if (context.state === 'running') {
            resumeNeeded = false;
            pulse();
            return;
        }
        // Foreground events may fire before SDL has reopened its device.
        // Try again on attach and directly within the next touch gesture,
        // where mobile autoplay policies permit AudioContext.resume().
        var now = Date.now();
        if (!fromGesture && now - lastResumeAttempt < 250) return;
        lastResumeAttempt = now;
        try {
            context.resume().then(function () {
                if (context.state === 'running') {
                    resumeNeeded = false;
                    if (output && output.context === context) pulse();
                }
            }).catch(function (error) {
                console.warn('[AudioSafety] Web Audio resume failed:', error);
            });
        } catch (error) {
            console.warn('[AudioSafety] Web Audio resume failed:', error);
        }
    }

    function attach() {
        var sdl = Module['SDL2'];
        var audio = sdl && sdl.audio;
        var node = audio && audio.scriptProcessorNode;
        var context = sdl && sdl.audioContext;
        if (!node || !context) return;
        if (output && output.node === node) return;
        stop();
        var gate = context.createGain();
        gate.gain.setValueAtTime(0, context.currentTime);
        node.disconnect();
        node.connect(gate);
        gate.connect(context.destination);
        output = { context: context, audio: audio, node: node, gate: gate };
        resumeNeeded = true;
        var callback = node.onaudioprocess;
        node.onaudioprocess = function (event) {
            // Never leave the previous PCM block in the output after a trap.
            clearBuffer(event.outputBuffer);
            if (!output || output.node !== node || fatal) return;
            try {
                callback.call(node, event);
                if (returnedAt) {
                    callbackSeenAfterReturn = true;
                    returnedAt = 0;
                }
            } catch (error) {
                clearBuffer(event.outputBuffer);
                stop('Audio callback failed: ' + error, true);
                throw error;
            }
        };
        lastPulse = -Infinity;
        // SDL connects new devices straight to the speakers. Even after a
        // fatal error, intercept a PANIC/reopen attempt before it can play.
        if (fatal) stop();
        else {
            pulse();
            resume(false);
        }
    }

    function needsReopen() {
        if (!reopenRequested) return false;
        reopenRequested = false;
        reopenAttempts++;
        returnedAt = Date.now();
        callbackSeenAfterReturn = false;
        return true;
    }

    function markForeground() {
        if (document.hidden) return;
        if (wasBackgrounded) {
            wasBackgrounded = false;
            returnedAt = Date.now();
            callbackSeenAfterReturn = false;
            reopenRequested = false;
            reopenAttempts = 0;
        }
        resume(false);
    }

    Module['xfmAudioSafety'] = {
        attach: attach, pulse: pulse, mute: mute, stop: stop,
        resume: resume, needsReopen: needsReopen
    };
    var previousAbort = Module['onAbort'];
    Module['onAbort'] = function (reason) {
        stop('Wasm aborted: ' + reason, true);
        if (previousAbort) previousAbort(reason);
    };
    window.addEventListener('error', function (event) {
        // Resource load failures have no ErrorEvent message and need not mute.
        if (event.message) {
            stop('Uncaught error: ' + event.message, true);
        }
    });
    window.addEventListener('unhandledrejection', function (event) {
        stop('Unhandled rejection: ' + event.reason, true);
    });
    // These handlers never enter Wasm: they also work after a Wasm trap.
    document.addEventListener('visibilitychange', function () {
        if (document.hidden) {
            wasBackgrounded = true;
            mute();
        } else markForeground();
    });
    window.addEventListener('pagehide', function () { wasBackgrounded = true; stop(); });
    document.addEventListener('freeze', function () { wasBackgrounded = true; stop(); });
    window.addEventListener('pageshow', markForeground);
    window.addEventListener('focus', markForeground);
    window.addEventListener('pointerdown', function () { resume(true); }, true);
    window.addEventListener('touchstart', function () { resume(true); }, true);
})();
