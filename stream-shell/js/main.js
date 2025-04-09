import { Terminal } from "https://esm.sh/@xterm/xterm";
import { FitAddon } from "https://esm.sh/@xterm/addon-fit";
import Module from "./wasm-cc.js";

const term = new Terminal({
  convertEol: true,
  fontFamily: 'Source Code Pro, monospace',
  fontSize: 12,
  macOptionIsMeta: false,
  theme: {
    background: "#282c34"
  }
});

const fitAddon = new FitAddon();
term.loadAddon(fitAddon);

term.open(document.getElementById("xterm"));
fitAddon.fit();

term.writeln(`{{welcome_message}}`);

let state = {};

window.getline = () => new Promise(resolve => {
  state = {
    buffer: '',
    resolve,
    sub: term.onKey(e => {
      if (e.key === '\x7F') {
        if (e.domEvent.metaKey) {
          term.write('\x1b[2K\r');
        } else if ([...state.buffer].length) {
          term.write('\b \b');
          state.buffer = [...state.buffer].slice(0, -1).join('');
        }
        return;
      }
      if (['{', '}', '[', ']', '|'].includes(e.domEvent.key)) {
        term.write(e.domEvent.key);
        state.buffer += e.domEvent.key;
      } else {
        term.write(e.key);
        state.buffer += e.key;
      }
      if (e.key == '\r') {
        term.write('\n');

        state.resolve(state.buffer.trim());
        state.sub.dispose();
        state = {};
      }
    }),
  };
});

document.onkeyup = (e) => {
  if (e.key.toLowerCase() === 'c' && e.ctrlKey && state.resolve) {
    term.write('\n');
    state.resolve(null);
    state.sub.dispose();
    state = {};
  }
}
document.onkeydown = (e) => {
  if (e.key.toLowerCase() === 'k' && e.metaKey) {
    term.clear();
  }
}

await Module({
  stdout: (e) => term.write(new Uint8Array([e])),
  stderr: (e) => term.write(new Uint8Array([e])),
});
