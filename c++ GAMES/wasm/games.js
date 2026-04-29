// wasm/games.js
export default async function createModule(options = {}) {
  const wasmUrl = options.wasmUrl || new URL("./games.wasm", import.meta.url);

  const response = await fetch(wasmUrl);
  if (!response.ok) {
    throw new Error("Failed to load games.wasm");
  }

  const bytes = await response.arrayBuffer();

  const wasmModule = await WebAssembly.instantiate(bytes, {
    env: {
      memory: new WebAssembly.Memory({ initial: 256 }),
      table: new WebAssembly.Table({ initial: 0, element: "anyfunc" }),
      abort: () => console.error("WASM aborted"),
    },
  });

  return wasmModule.instance.exports;
}