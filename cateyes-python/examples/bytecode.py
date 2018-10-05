# -*- coding: utf-8 -*-
from __future__ import print_function

import cateyes


system_session = cateyes.attach(0)
bytecode = system_session.compile_script(name="bytecode-example", source="""\
'use strict';

rpc.exports = {
  listThreads: function () {
    return Process.enumerateThreadsSync();
  }
};
""")

session = cateyes.attach("Twitter")
script = session.create_script_from_bytes(bytecode)
script.load()
api = script.exports
print("api.list_threads() =>", api.list_threads())
