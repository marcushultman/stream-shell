import { Terminal } from "https://esm.sh/@xterm/xterm";
import { FitAddon } from "https://esm.sh/@xterm/addon-fit";
import { Readline } from "https://esm.sh/xterm-readline"
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

const rl = new Readline();
term.loadAddon(rl);

term.open(document.getElementById("xterm"));
fitAddon.fit();

term.writeln(`{{welcome_message}}`);

let ctrlCHandler = () => {};

window.readline = (prompt) => Promise.race([
  rl.read(prompt),
  new Promise(resolve => (ctrlCHandler = () => resolve(null))),
]);

// keydown does not work for this...
document.onkeyup = (e) => {
  if (e.key.toLowerCase() === 'c' && e.ctrlKey) {
    ctrlCHandler();
  }
};
document.onkeydown = (e) => {
  e.key.toLowerCase() === 'k' && e.metaKey && term.clear();
};

await Module({
  stdout: (e) => term.write(new Uint8Array([e])),
  stderr: (e) => term.write(new Uint8Array([e])),
});
