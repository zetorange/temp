'use strict';

const cateyes = require('..');

async function main() {
  const device = await cateyes.getUsbDevice();
  const applications = await device.enumerateApplications();
  console.log('[*] Applications:', applications);
}

main()
  .catch(e => {
    console.error(e);
  });