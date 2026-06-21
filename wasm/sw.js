const BUILD_VERSION = '__BUILD_VERSION__';
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

self.addEventListener('install', event => {
  event.waitUntil(
    caches.open(CACHE_NAME).then(cache => cache.addAll(PRECACHE_URLS)).then(() => {
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
  if (url.pathname.endsWith('/version.json') || url.pathname === '/version.json') {
    event.respondWith(
      fetch(event.request, { cache: 'no-store' }).catch(() =>
        new Response('{"buildVersion":""}', {
          status: 503,
          headers: { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' }
        })
      )
    );
    return;
  }
  if (event.request.mode === 'navigate') {
    event.respondWith(
      caches.match('index.html').then(cached => cached || fetch(event.request).catch(() => cached))
    );
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
