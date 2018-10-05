'use strict';

const cateyes = require('..');

const source = `'use strict';

recv(onMessage);

function onMessage(message) {
  send({ name: 'pong', payload: message });

  recv(onMessage);
}`;

async function spawnExample() {
  const pid = await cateyes.spawn(['/bin/cat', '/etc/resolv.conf']);

  console.log(`[*] Spawned pid=${pid}`);

  // This is where you could attach (see below) and instrument APIs before you call resume()
  await cateyes.resume(pid);
  console.log('[*] Resumed');
}

async function attachExample() {
  const session = await cateyes.attach('cat');
  console.log('[*] Attached:', session);
  session.detached.connect(onDetached);

  const script = await session.createScript(source);
  console.log('[*] Script created');
  script.message.connect(message => {
    console.log('[*] Message:', message);
  });
  await script.load();
  console.log('[*] Script loaded');

  process.on('SIGTERM', stop);
  process.on('SIGINT', stop);

  const timer = setInterval(() => {
    script.post({ name: 'ping' });
  }, 1000);

  function stop() {
    clearInterval(timer);
    script.unload();
  }

  function onDetached(reason) {
    console.log(`[*] onDetached(reason=${reason})`);
    clearInterval(timer);
  }
}

async function usbExample() {
  const device = await cateyes.getUsbDevice({ timeout: null });
  console.log('[*] USB device:', device);

  // Now call spawn(), attach(), etc. on `device` just like the above calls on `cateyes`
}

attachExample()
  .catch(e => {
    console.error(e);
  });
