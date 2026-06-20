const RAW_BUILD_VERSION = '__BOWLING_BUILD_VERSION__';
const BUILD_VERSION =
  RAW_BUILD_VERSION && RAW_BUILD_VERSION !== '__BOWLING_BUILD_VERSION_SENTINEL__' ? RAW_BUILD_VERSION : 'dev';
const CACHE_NAME = 'game-v' + BUILD_VERSION;
const PRECACHE_URLS = [
  './',
  'index.html',
  'index.js',
  'index.wasm',
  'index.data',
  'manifest.json',
  'sw.js',
  'icon-192.png',
  'icon-512.png'
];

function normalizeCacheKey(url) {
  const pathname = url.pathname || '/';
  if (pathname.endsWith('/'))
    return './';

  const filename = pathname.split('/').pop() || '';
  if (filename === '' || filename === 'index.html')
    return 'index.html';
  return filename;
}

async function cachedAppShellResponse(request) {
  const cache = await caches.open(CACHE_NAME);
  const cacheKey = request.mode === 'navigate'
    ? 'index.html'
    : normalizeCacheKey(new URL(request.url));

  const cached = await cache.match(cacheKey);
  if (cached)
    return cached;

  const response = await fetch(request, { cache: 'no-cache' });
  if (response && response.ok) {
    await cache.put(cacheKey, response.clone());
  }
  return response;
}

self.addEventListener('install', event => {
  event.waitUntil(
    caches.open(CACHE_NAME).then(cache => cache.addAll(
      PRECACHE_URLS.map(url => new Request(url, { cache: 'reload' }))
    )).then(() => {
      if (!self.registration.active) {
        return self.skipWaiting();
      }
    })
  );

  const newWorker = self.registration.installing;
  if (newWorker) {
    newWorker.addEventListener('statechange', async () => {
      if (newWorker.state !== 'installed')
        return;
      if (!self.registration.active)
        return;
      const clients = await self.clients.matchAll({ type: 'window', includeUncontrolled: true });
      if (!clients.length)
        return;
      for (const client of clients) {
        client.postMessage({ type: 'NEW_VERSION_AVAILABLE' });
      }
    });
  }
});

self.addEventListener('activate', event => {
  event.waitUntil(
    caches.keys().then(keys =>
      Promise.all(keys.map(key => (key === CACHE_NAME ? null : caches.delete(key))))
    ).then(() => self.clients.claim())
  );
});

self.addEventListener('message', event => {
  if (!event.data || event.data.type !== 'SKIP_WAITING')
    return;
  self.skipWaiting();
});

self.addEventListener('fetch', event => {
  if (event.request.method !== 'GET') return;
  const url = new URL(event.request.url);
  if (url.pathname.endsWith('/version.json')) {
    event.respondWith(fetch(event.request, { cache: 'no-store' }));
    return;
  }
  if (event.request.mode === 'navigate') {
    event.respondWith(cachedAppShellResponse(event.request));
    return;
  }

  const cacheKey = normalizeCacheKey(url);
  if (PRECACHE_URLS.includes(cacheKey)) {
    event.respondWith(cachedAppShellResponse(event.request));
    return;
  }

  event.respondWith(
    caches.match(event.request).then(cached => {
      if (cached) return cached;
      return fetch(event.request).then(response => {
        const responseClone = response.clone();
        caches.open(CACHE_NAME).then(cache => cache.put(event.request, responseClone));
        return response;
      }).catch(() => cached);
    })
  );
});
