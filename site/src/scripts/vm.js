/* 在线试玩共享核心: 镜像获取 (gzip -> DecompressionStream) + v86 启动 +
   键盘门控. demo 页和首页的 inline 开机入口都走这里.
   镜像不进仓库: 默认走同源代理路由, CI 滚动发布在 GitHub Release 上;
   本地调试 (astro dev 没有 Worker 路由): 页面 URL 加 ?img=/assets/demo/lwcnc.img.gz
   (make webimg 的产物, gitignored). */
import { V86 } from "v86";
import wasmUrl from "v86/build/v86.wasm?url";
import { DEMO_IMAGE_PROXY } from "../data/site";

export function resolveImageUrl() {
  const override = new URLSearchParams(location.search).get("img");
  return override || DEMO_IMAGE_PROXY;
}

// 代理路由靠 content-type 认 gzip (?img= 指向的本地文件靠扩展名)
const IS_GZ = (url) => /\.gz(?:$|\?)/.test(url);

export async function fetchImage(url) {
  const res = await fetch(url, { cache: "no-store" });
  if (!res.ok) throw new Error("HTTP " + res.status + " fetching " + url);
  // content-encoding: gzip 时浏览器已经透明解压过了 (astro dev 对 .gz 就这样),
  // 再过一遍 DecompressionStream 会报 incorrect header check
  const encoded = /gzip/i.test(res.headers.get("content-encoding") || "");
  const gz = (IS_GZ(url) || /gzip/i.test(res.headers.get("content-type") || "")) && !encoded;
  let body = res.body;
  if (!body) throw new Error("empty response body");
  if (gz) {
    if (typeof DecompressionStream === "undefined")
      throw new Error("browser lacks DecompressionStream");
    body = body.pipeThrough(new DecompressionStream("gzip"));
  }
  // 分块读完拼成一个 ArrayBuffer 给 v86
  const reader = body.getReader(), chunks = [];
  let got = 0;
  for (;;) {
    const r = await reader.read();
    if (r.done) break;
    chunks.push(r.value); got += r.value.byteLength;
  }
  const out = new Uint8Array(got);
  let off = 0;
  for (const c of chunks) { out.set(c, off); off += c.byteLength }
  return out.buffer;
}

export function startVM(container, buffer) {
  const emulator = new V86({
    wasm_path: wasmUrl,
    memory_size: 128 * 1024 * 1024,
    vga_memory_size: 8 * 1024 * 1024,
    // 用 canvas + VGA 字模渲染文本模式: DOM 文本渲染器对"只改属性/同格重写"
    // 的差量刷新有漏, guest 里退格这类改动会留在屏幕上
    screen: { container, use_graphical_text: true },
    bios: { url: "/vendor/v86/bios/seabios.bin" },
    vga_bios: { url: "/vendor/v86/bios/vgabios.bin" },
    hda: { buffer },
    autostart: true,
  });
  window.__emu = emulator; // 调试用

  // v86 默认把整页的按键都截给虚拟机, 这里只让屏幕区域拿到焦点时才送键
  // (focusin 会从隐形 input 冒泡上来), 地址栏和 IE 外壳的快捷键才能正常工作.
  emulator.keyboard_set_enabled(false);
  container.addEventListener("focusin", () => emulator.keyboard_set_enabled(true));
  container.addEventListener("focusout", () => emulator.keyboard_set_enabled(false));

  // 移动端软键盘: 屏幕是 div, 点它拉不起键盘, 得聚焦一个隐形 input.
  // v86 在 window 上监听 input 事件并按 inputType 转按键 (insertText ->
  // simulate_char, deleteContentBackward -> 退格, insertLineBreak -> 回车),
  // 前提是事件目标带 phone_keyboard class 且门控开着 —— 我们只负责清空 value.
  const pk = container.querySelector(".phone_keyboard");
  if (pk) {
    container.addEventListener("click", () => pk.focus({ preventScroll: true }));
    pk.addEventListener("input", () => { pk.value = "" });
  }
  return emulator;
}

/* 全屏: v86 自带的 screen_go_fullscreen 写死了 getElementById("screen_container"),
   对我们的容器直接拿到 null, 所以自己实现. 全屏对象是 bezel (保留 CRT 黑框).
   iPhone 的 Safari 不给 div 提供 requestFullscreen, 返回 false 由调用方处理. */
export function goFullscreen(container) {
  const req = container.requestFullscreen || container.webkitRequestFullscreen;
  if (!req) return false;
  const p = req.call(container);
  if (p && p.catch) p.catch(() => {});
  // Chrome 的 Keyboard Lock: 全屏下 Esc 归虚拟机用
  try { if (navigator.keyboard && navigator.keyboard.lock) navigator.keyboard.lock().catch(() => {}) } catch (e) {}
  const pk = container.querySelector(".phone_keyboard");
  if (pk) pk.focus({ preventScroll: true });
  return true;
}
