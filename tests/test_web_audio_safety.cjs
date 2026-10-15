// Run with: node --test tests/test_web_audio_safety.cjs
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');
const source = fs.readFileSync(require('node:path').join(__dirname, '../wasm/audio_safety.js'), 'utf8');

function setup(previousAbort) {
    const listeners = {};
    const clearedTimers = [];
    const events = target => ({ addEventListener(name, callback) {
        (listeners[target + name] ||= []).push(callback);
    } });
    const document = { ...events('document:'), hidden: false };
    const Module = { onAbort: previousAbort };
    vm.runInNewContext(source, { Module, document, window: events('window:'),
        console: { warn() {} }, clearInterval(id) { clearedTimers.push(id); } });
    function open(callback = () => {}) {
        const schedule = [];
        const gain = {
            cancelScheduledValues(time) {
                for (let i = schedule.length - 1; i >= 0; --i)
                    if (schedule[i].time >= time) schedule.splice(i, 1);
            },
            setValueAtTime(value, time) { schedule.push({ value, time }); },
            linearRampToValueAtTime(value, time) { schedule.push({ value, time }); }
        };
        const gate = { gain, connect() {}, disconnect() { this.disconnected = true; } };
        const context = { currentTime: 0, state: 'running', destination: {},
            createGain: () => gate,
            suspend() { this.state = 'suspended'; return Promise.resolve(); },
            close() { this.state = 'closed'; return Promise.resolve(); } };
        const node = { onaudioprocess: callback, connections: [],
            connect(to) { this.connections.push(to); }, disconnect() { this.connections = []; } };
        const audio = { scriptProcessorNode: node, silenceTimer: 42 };
        Module.SDL2 = { audioContext: context, audio };
        Module.xfmAudioSafety.attach();
        return { context, node, gate, audio, schedule };
    }
    return { Module, document, open, clearedTimers,
        emit(target, name, event = {}) {
            for (const listener of listeners[target + ':' + name] || []) listener(event);
        } };
}

function buffer() {
    const channels = [new Float32Array([1, 1]), new Float32Array([1, 1])];
    return { channels, numberOfChannels: channels.length, getChannelData: n => channels[n] };
}

test('audio engine has a zero-gain deadline even if no further JS runs', () => {
    const app = setup();
    const { schedule, node, gate } = app.open();
    assert.deepEqual(node.connections, [gate]);
    assert.equal(schedule.at(-2).value, 1);
    assert.equal(schedule.at(-1).value, 0);
    assert.equal(schedule.at(-1).time, 1.55);
});

test('only game heartbeats extend the deadline; audio callbacks do not', () => {
    const app = setup();
    const out = app.open();
    out.context.currentTime = 1;
    app.Module.xfmAudioSafety.pulse();
    assert.equal(out.schedule.at(-1).time, 2.55);
    out.context.currentTime = 2;
    out.node.onaudioprocess({ outputBuffer: buffer() });
    assert.equal(out.schedule.at(-1).time, 2.55);
    assert.ok(out.schedule.length <= 6);
});

test('a trapped callback clears partial output and disconnects audio without Wasm', () => {
    const app = setup();
    const error = new Error('Wasm trap');
    const out = app.open(event => {
        event.outputBuffer.getChannelData(0).fill(0.8);
        throw error;
    });
    const output = buffer();
    assert.throws(() => out.node.onaudioprocess({ outputBuffer: output }), error);
    assert.ok(output.channels.every(channel => channel.every(sample => sample === 0)));
    assert.deepEqual(out.node.connections, []);
    assert.equal(out.context.state, 'closed');
    assert.deepEqual(app.clearedTimers, [42]);
    const count = out.schedule.length;
    app.Module.xfmAudioSafety.pulse();
    assert.equal(out.schedule.length, count);
    const reopened = app.open();
    assert.equal(reopened.context.state, 'closed');
    assert.deepEqual(reopened.node.connections, []);
});

test('PANIC cleanup is repeatable and a fresh SDL device can play', () => {
    const app = setup();
    const first = app.open();
    app.Module.xfmAudioSafety.stop();
    app.Module.xfmAudioSafety.stop();
    assert.equal(first.gate.disconnected, true);
    assert.equal(first.audio.silenceTimer, undefined);
    let callbacks = 0;
    const second = app.open(() => { ++callbacks; });
    second.node.onaudioprocess({ outputBuffer: buffer() });
    assert.equal(callbacks, 1);
    assert.equal(second.schedule.at(-1).value, 0);
    assert.equal(second.node.connections.length, 1);
});

test('hidden pages mute immediately and cannot renew the deadline', () => {
    const app = setup();
    const out = app.open();
    app.document.hidden = true;
    app.emit('document', 'visibilitychange');
    assert.equal(out.schedule.at(-1).value, 0);
    const count = out.schedule.length;
    out.context.currentTime = 10;
    app.Module.xfmAudioSafety.pulse();
    assert.equal(out.schedule.length, count);
});

for (const [target, event, payload] of [
    ['window', 'error', { message: 'out of bounds' }],
    ['window', 'unhandledrejection', { reason: 'abort' }],
    ['window', 'pagehide', {}],
    ['document', 'freeze', {}]
]) {
    test(event + ' silences output without calling into the game', () => {
        const app = setup();
        const out = app.open();
        app.emit(target, event, payload);
        assert.equal(out.context.state, event === 'error' || event === 'unhandledrejection' ? 'closed' : 'suspended');
        assert.deepEqual(out.node.connections, []);
    });
}

test('Wasm abort hook silences audio and preserves the existing hook', () => {
    let reason;
    const app = setup(value => { reason = value; });
    const out = app.open();
    app.Module.onAbort('OOM');
    assert.equal(reason, 'OOM');
    assert.equal(out.context.state, 'closed');
    assert.deepEqual(out.node.connections, []);
});
