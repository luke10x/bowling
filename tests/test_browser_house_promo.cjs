// Run with: node --test tests/test_browser_house_promo.cjs
const { test } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const html = fs.readFileSync(path.join(__dirname, '../wasm/shell_itch_io.html'), 'utf8');
const start = html.indexOf('(function () {\n                if (!window.BOWLING_ADSENSE_CLIENT) return;');
const end = html.indexOf('})();', start) + 5;
assert.ok(start >= 0 && end > start);
const script = html.slice(start, end);

function browser(housePromo) {
    const timers = new Map();
    const scripts = [];
    let nextTimer = 1;
    const elements = [];
    function createElement(tag) {
        const listeners = new Map();
        const element = {
            tag, style: {}, dataset: {}, children: [], disabled: false,
            appendChild(child) { this.children.push(child); child.parent = this; },
            setAttribute(name, value) { this[name] = value; },
            focus() {},
            addEventListener(type, fn) { listeners.set(type, fn); },
            click() { if (!this.disabled) listeners.get('click')?.(); },
            remove() { elements.splice(elements.indexOf(this), 1); }
        };
        elements.push(element);
        return element;
    }
    const document = {
        visibilityState: 'visible', createElement,
        body: { appendChild() {} },
        head: { appendChild(element) { scripts.push(element); } },
        getElementById(id) { return elements.find(element => element.id === id); }
    };
    const window = {
        BOWLING_ADSENSE_CLIENT: 'ca-pub-1023905489122100',
        BOWLING_ADS_TEST: false, BOWLING_H5_HOUSE_PROMO: housePromo,
        BOWLING_MONEY_REWARD: 100, BOWLING_AD_FREQUENCY_HINT: '30s',
        location: { search: '' }
    };
    vm.runInNewContext(script, { window, document, URLSearchParams,
        encodeURIComponent, console, setInterval(fn) {
            const id = nextTimer++;
            timers.set(id, fn);
            return id;
        }, clearInterval(id) { timers.delete(id); },
        setTimeout() { return 0; }, clearTimeout() {} });
    function descendants(element) {
        return [element, ...element.children.flatMap(descendants)];
    }
    function button(label) {
        return descendants(document.getElementById('in-game-promo'))
            .find(element => element.tag === 'button' && element.textContent === label);
    }
    return { window, document, scripts, timers, button,
        tick(count) { for (let i = 0; i < count; i++) for (const fn of [...timers.values()]) fn(); } };
}

test('live browser promo gives the shop reward only after a completed view', () => {
    const app = browser(true);
    assert.equal(app.scripts.length, 0, 'pending mode must not load Google ads');
    app.window.bowlingShowMoneyRewardAd();
    assert.equal(app.document.getElementById('in-game-promo').children[0].children[0].textContent,
        'IN-GAME PROMO');
    app.button('Skip (no reward)').click();
    assert.equal(app.window.bowlingMoneyRewardEarnedCount || 0, 0);
    assert.equal(app.window.bowlingMoneyRewardFinishedCount, 1);
    app.window.bowlingShowMoneyRewardAd();
    app.document.visibilityState = 'hidden';
    app.tick(2);
    assert.equal(app.button('Collect $100'), undefined, 'hidden page must not advance view');
    app.document.visibilityState = 'visible';
    app.tick(6);
    app.button('Collect $100').click();
    assert.equal(app.window.bowlingMoneyRewardEarnedCount, 1);
    assert.equal(app.window.bowlingMoneyRewardFinishedCount, 2);
    assert.equal(app.window.bowlingMoneyRewardLastStatus, 'house-promo-viewed');
});

test('live interstitial completes through promo; switch off restores Google path', () => {
    const app = browser(true);
    app.window.bowlingShowGameAd('win', 'A word from our sponsors', 'Continue to ad');
    const overlay = app.document.getElementById('loss-ad-overlay');
    assert.equal(overlay.children[0].textContent, 'A word from our sponsors');
    assert.equal(overlay.children[1].textContent, 'Continue to ad');
    assert.match(overlay.children[1].style.cssText, /background:#2bc86f/);
    overlay.click();
    overlay.click();
    assert.ok(app.document.getElementById('in-game-promo'));
    app.tick(4);
    app.button('Continue').click();
    assert.equal(app.window.bowlingInterstitialShownCount, 1);
    assert.equal(app.window.bowlingInterstitialFinishedCount, 1);
    const google = browser(false);
    assert.equal(google.scripts.length, 1);
    assert.match(google.scripts[0].src, /adsbygoogle\.js/);
});
