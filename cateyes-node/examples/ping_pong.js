'use strict';

const cateyes = require('..');

const processName = process.argv[2];

const source = `'use strict';

recv('poke', function onMessage(pokeMessage) {
  send('pokeBack');
});
`;

async function main() {
  const session = await cateyes.attach(processName);

  const script = await session.createScript(source);
  script.message.connect(message => {
    console.log('[*] Message:', message);
    script.unload();
  });
  await script.load();
  console.log('[*] Script loaded');

  script.post({ type: 'poke' });
}

main()
  .catch(e => {
    console.error(e);
  });