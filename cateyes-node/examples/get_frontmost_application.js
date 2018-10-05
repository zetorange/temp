'use strict';

const cateyes = require('..');

async function main() {
  const device = await cateyes.getUsbDevice();
  const application = await device.getFrontmostApplication();
  console.log('[*] Frontmost application:', application);
}

main()
  .catch(e => {
    console.error(e);
  });