// Browser-only install guidance. Language IDs match TxlLanguage in txl_runtime.h.
// Kept in the web shell so the native game has no copy or browser-specific UI.
(function () {
    'use strict';

    const profiles = ['safariIos', 'otherIos', 'chromeAndroid', 'samsungAndroid',
        'firefoxAndroid', 'chromeDesktop', 'edgeDesktop', 'safariMac', 'other'];
    const names = {
        safariIos: 'Safari · iPhone/iPad', otherIos: 'Other browser · iPhone/iPad',
        chromeAndroid: 'Chrome · Android', samsungAndroid: 'Samsung Internet · Android',
        firefoxAndroid: 'Firefox · Android', chromeDesktop: 'Chrome · computer',
        edgeDesktop: 'Edge · computer', safariMac: 'Safari · Mac', other: 'Other browser'
    };
    const copy = {
        0: {
            title: 'Install Angel Balls',
            warning: 'Your progress is stored in this browser. If its site data is cleared, your progress can disappear. Install the game as an app and open it from its app icon.',
            browser: 'Installation steps for', close: 'Close and continue',
            steps: {
                safariIos: ['In Safari, open this page.', 'Tap the Page Menu, then Share.', 'Tap Add to Home Screen, enable Open as Web App, then tap Add.'],
                otherIos: ['Open this page in Safari.', 'Tap the Page Menu, then Share.', 'Tap Add to Home Screen, enable Open as Web App, then tap Add.'],
                chromeAndroid: ['Tap Chrome’s three-dot menu.', 'Tap Add to Home screen or Install app.', 'Confirm, then open the new icon from your home screen.'],
                samsungAndroid: ['Tap Samsung Internet’s menu.', 'Choose Add page to, then Home screen.', 'Confirm, then open the new icon from your home screen.'],
                firefoxAndroid: ['Tap Firefox’s three-dot menu.', 'Tap Add to Home Screen.', 'Confirm Add, then open the new icon from your home screen.'],
                chromeDesktop: ['Open Chrome’s three-dot menu.', 'Choose Cast, save, and share, then Install page as app. You can also use the install icon in the address bar.', 'Confirm the installation and launch the new app.'],
                edgeDesktop: ['Open Edge’s three-dot menu.', 'Choose More tools, Apps, then Install this site as an app. You can also use the app icon in the address bar.', 'Confirm the installation and launch the new app.'],
                safariMac: ['In Safari, open the File menu.', 'Choose Add to Dock.', 'Click Add, then launch the web app from the Dock.'],
                other: ['Open this page in Chrome or Edge on a computer, Chrome on Android, or Safari on iPhone, iPad, or Mac.', 'Select that browser above to see its installation steps.']
            }
        },
        1: {
            title: '安装 Angel Balls',
            warning: '游戏进度保存在此浏览器中。如果浏览器清除网站数据，进度可能丢失。请将游戏安装为应用，并从应用图标打开。',
            browser: '选择浏览器查看安装步骤', close: '关闭并继续',
            steps: {
                safariIos: ['在 Safari 中打开此页面。', '轻点页面菜单，然后轻点“共享”。', '轻点“添加到主屏幕”，开启“作为网页 App 打开”，然后轻点“添加”。'],
                otherIos: ['请先在 Safari 中打开此页面。', '轻点页面菜单，然后轻点“共享”。', '轻点“添加到主屏幕”，开启“作为网页 App 打开”，然后轻点“添加”。'],
                chromeAndroid: ['轻点 Chrome 的三点菜单。', '选择“添加到主屏幕”或“安装应用”。', '确认后，从主屏幕上的新图标打开游戏。'],
                samsungAndroid: ['轻点 Samsung Internet 的菜单。', '选择“添加页面到”，然后选择“主屏幕”。', '确认后，从主屏幕上的新图标打开游戏。'],
                firefoxAndroid: ['轻点 Firefox 的三点菜单。', '选择“添加到主屏幕”。', '确认添加后，从主屏幕上的新图标打开游戏。'],
                chromeDesktop: ['打开 Chrome 的三点菜单。', '选择“投放、保存和分享”，然后选择“将页面安装为应用”。也可以使用地址栏中的安装图标。', '确认安装并启动新应用。'],
                edgeDesktop: ['打开 Edge 的三点菜单。', '选择“更多工具”→“应用”→“将此站点安装为应用”。也可以使用地址栏中的应用图标。', '确认安装并启动新应用。'],
                safariMac: ['在 Safari 中打开“文件”菜单。', '选择“添加到程序坞”。', '点按“添加”，然后从程序坞启动网页应用。'],
                other: ['请用电脑上的 Chrome 或 Edge、Android 上的 Chrome，或 iPhone、iPad、Mac 上的 Safari 打开此页面。', '在上方选择该浏览器，查看安装步骤。']
            }
        },
        2: {
            title: 'Įdiekite Angel Balls',
            warning: 'Pažanga saugoma šioje naršyklėje. Jei jos svetainių duomenys bus išvalyti, pažanga gali dingti. Įdiekite žaidimą kaip programėlę ir atverkite jį per jos piktogramą.',
            browser: 'Diegimo veiksmai naršyklei', close: 'Uždaryti ir tęsti',
            steps: {
                safariIos: ['Atverkite šį puslapį „Safari“.', 'Palieskite puslapio meniu, tada „Bendrinti“.', 'Rinkitės „Įtraukti į pradžios ekraną“, įjunkite „Atidaryti kaip žiniatinklio programą“ ir palieskite „Įtraukti“.'],
                otherIos: ['Atverkite šį puslapį „Safari“ naršyklėje.', 'Palieskite puslapio meniu, tada „Bendrinti“.', 'Rinkitės „Įtraukti į pradžios ekraną“, įjunkite „Atidaryti kaip žiniatinklio programą“ ir palieskite „Įtraukti“.'],
                chromeAndroid: ['Palieskite „Chrome“ trijų taškų meniu.', 'Rinkitės „Pridėti prie pagrindinio ekrano“ arba „Įdiegti programą“.', 'Patvirtinkite ir atverkite žaidimą per naują piktogramą.'],
                samsungAndroid: ['Palieskite „Samsung Internet“ meniu.', 'Rinkitės „Pridėti puslapį prie“, tada „Pradžios ekranas“.', 'Patvirtinkite ir atverkite žaidimą per naują piktogramą.'],
                firefoxAndroid: ['Palieskite „Firefox“ trijų taškų meniu.', 'Rinkitės „Pridėti prie pradžios ekrano“.', 'Patvirtinkite ir atverkite žaidimą per naują piktogramą.'],
                chromeDesktop: ['Atverkite „Chrome“ trijų taškų meniu.', 'Rinkitės „Perduoti, išsaugoti ir bendrinti“, tada „Įdiegti puslapį kaip programą“. Arba spauskite diegimo piktogramą adreso juostoje.', 'Patvirtinkite diegimą ir paleiskite programėlę.'],
                edgeDesktop: ['Atverkite „Edge“ trijų taškų meniu.', 'Rinkitės „Daugiau įrankių“ → „Programos“ → „Įdiegti šią svetainę kaip programą“. Arba spauskite programos piktogramą adreso juostoje.', 'Patvirtinkite diegimą ir paleiskite programėlę.'],
                safariMac: ['„Safari“ atverkite meniu „Failas“.', 'Rinkitės „Pridėti į Dock“.', 'Spauskite „Pridėti“ ir paleiskite programėlę iš Dock.'],
                other: ['Atverkite šį puslapį su „Chrome“ arba „Edge“ kompiuteryje, „Chrome“ „Android“ įrenginyje arba „Safari“ „iPhone“, „iPad“ ar „Mac“ įrenginyje.', 'Viršuje pasirinkite šią naršyklę ir skaitykite diegimo veiksmus.']
            }
        },
        3: {
            title: 'Angel Balls をインストール',
            warning: '進行状況はこのブラウザー内に保存されます。サイトデータが消去されると失われる可能性があります。ゲームをアプリとしてインストールし、アプリアイコンから起動してください。',
            browser: 'ブラウザー別のインストール手順', close: '閉じて続ける',
            steps: {
                safariIos: ['Safari でこのページを開きます。', 'ページメニューから「共有」をタップします。', '「ホーム画面に追加」を選び、「Webアプリとして開く」をオンにして「追加」をタップします。'],
                otherIos: ['このページを Safari で開きます。', 'ページメニューから「共有」をタップします。', '「ホーム画面に追加」を選び、「Webアプリとして開く」をオンにして「追加」をタップします。'],
                chromeAndroid: ['Chrome の三点メニューをタップします。', '「ホーム画面に追加」または「アプリをインストール」を選びます。', '確認後、ホーム画面の新しいアイコンから起動します。'],
                samsungAndroid: ['Samsung Internet のメニューをタップします。', '「ページを追加」から「ホーム画面」を選びます。', '確認後、ホーム画面の新しいアイコンから起動します。'],
                firefoxAndroid: ['Firefox の三点メニューをタップします。', '「ホーム画面に追加」を選びます。', '追加を確認し、新しいアイコンから起動します。'],
                chromeDesktop: ['Chrome の三点メニューを開きます。', '「キャスト、保存、共有」から「ページをアプリとしてインストール」を選びます。アドレスバーのインストールアイコンも使えます。', 'インストールを確認してアプリを起動します。'],
                edgeDesktop: ['Edge の三点メニューを開きます。', '「その他のツール」→「アプリ」→「このサイトをアプリとしてインストール」を選びます。アドレスバーのアプリアイコンも使えます。', 'インストールを確認してアプリを起動します。'],
                safariMac: ['Safari の「ファイル」メニューを開きます。', '「Dockに追加」を選びます。', '「追加」を押し、Dock から Web アプリを起動します。'],
                other: ['パソコンでは Chrome または Edge、Android では Chrome、iPhone・iPad・Mac では Safari でこのページを開いてください。', '上の一覧からそのブラウザーを選び、手順をご覧ください。']
            }
        }
    };

    const overlay = document.getElementById('pwa-install-overlay');
    const selector = document.getElementById('pwa-install-browser');
    const steps = document.getElementById('pwa-install-steps');
    const close = document.getElementById('pwa-install-close');
    const canvas = document.getElementById('canvas');
    let locale = copy[0];
    let visible = false;
    let previousFocus = null;

    function standalone() {
        return !!((window.matchMedia && window.matchMedia('(display-mode: standalone)').matches) ||
            navigator.standalone);
    }

    function detectedBrowser() {
        const ua = navigator.userAgent || '';
        const ios = /iPhone|iPad|iPod/.test(ua) ||
            (navigator.platform === 'MacIntel' && navigator.maxTouchPoints > 1);
        if (ios) return /CriOS|FxiOS|EdgiOS|OPiOS/.test(ua) ? 'otherIos' : 'safariIos';
        if (/Android/.test(ua)) {
            if (/SamsungBrowser/.test(ua)) return 'samsungAndroid';
            if (/Firefox|Fennec/.test(ua)) return 'firefoxAndroid';
            return /Chrome|CriOS/.test(ua) ? 'chromeAndroid' : 'other';
        }
        if (/Edg\//.test(ua)) return 'edgeDesktop';
        if (/Chrome|Chromium/.test(ua)) return 'chromeDesktop';
        if (/Safari/.test(ua) && /Macintosh/.test(ua)) return 'safariMac';
        return 'other';
    }

    function renderSteps() {
        steps.replaceChildren();
        for (const line of locale.steps[selector.value]) {
            const item = document.createElement('li');
            item.textContent = line;
            steps.appendChild(item);
        }
    }

    function hide() {
        if (!visible) return;
        visible = false;
        overlay.hidden = true;
        canvas.inert = false;
        if (previousFocus && document.contains(previousFocus)) previousFocus.focus();
        else canvas.focus();
    }

    selector.addEventListener('change', renderSteps);
    close.addEventListener('click', hide);
    overlay.addEventListener('keydown', function (event) {
        if (event.key === 'Escape') { event.preventDefault(); hide(); }
        // Keep keyboard navigation inside the modal.
        if (event.key === 'Tab') {
            if (event.shiftKey && document.activeElement === selector) {
                event.preventDefault(); close.focus();
            } else if (!event.shiftKey && document.activeElement === close) {
                event.preventDefault(); selector.focus();
            }
        }
    });

    window.bowlingPwaInstallPrompt = {
        visible: function () { return visible; },
        show: function (languageId) {
            if (visible || standalone()) return;
            // Check each startup. A browser reload is a new launch, while an
            // installed PWA launch is skipped by the standalone check above.
            locale = copy[languageId] || copy[0];
            document.documentElement.lang = ({ 0: 'en', 1: 'zh', 2: 'lt', 3: 'ja' })[languageId] || 'en';
            document.getElementById('pwa-install-title').textContent = locale.title;
            document.getElementById('pwa-install-warning').textContent = locale.warning;
            document.getElementById('pwa-install-browser-label').textContent = locale.browser;
            close.textContent = locale.close;
            selector.replaceChildren();
            for (const profile of profiles) {
                const option = document.createElement('option');
                option.value = profile;
                option.textContent = names[profile];
                selector.appendChild(option);
            }
            selector.value = detectedBrowser();
            renderSteps();
            previousFocus = document.activeElement;
            canvas.inert = true;
            overlay.hidden = false;
            visible = true;
            selector.focus();
        }
    };
})();
