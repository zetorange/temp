'use strict';

const cateyes = require('..');

const processName = process.argv[2];

const source = `'use strict';

rpc.exports = {
  listThreads: function () {
    return Process.enumerateThreadsSync();
  }
};
`;

async function main() {
  const systemSession = await cateyes.attach(0);
  const bytecode = await systemSession.compileScript(source, {
    name: 'bytecode-example'
  });

  const session = await cateyes.attach(processName);
  const script = await session.createScriptFromBytes(bytecode);
  script.message.connect(message => {
    console.log('[*] Message:', message);
  });
  await script.load();

  console.log('[*] Called listThreads() =>', await script.exports.listThreads());

  await script.unload();
}

main()
  .catch(e => {
    console.error(e);
  });