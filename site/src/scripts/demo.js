/* 在线试玩页: 拉镜像 -> 解压 -> 引导 LWOS, 具体机制在共享的 vm.js 里.
   本地调试 (astro dev 没有 Worker 路由):
     pnpm dev  然后开  /demo/?img=/assets/demo/lwcnc.img.gz
     那份文件来自 make webimg, 在 .gitignore 里, 不进仓库. */
import { resolveImageUrl, fetchImage, startVM } from "./vm.js";
import { DEMO_INFO_PROXY, IMAGE_GZ_URL } from "../data/site";

var $ = function (id) { return document.getElementById(id) };

// 失败直接打在"屏幕"上, 沿用机器本身的观感
function fail(e) {
  console.error(e);
  var t = $("screen-text");
  t.style.display = "block"; // 图形文本模式下这个 div 被 v86 藏起来了
  t.textContent = "BOOT FAILED: " + (e && e.message ? e.message : e) + " — ";
  var a = document.createElement("a");
  a.href = IMAGE_GZ_URL;
  a.textContent = "download the image directly";
  a.style.color = "#7af";
  t.appendChild(a);
}

fetchImage(resolveImageUrl()).then(function (buffer) {
  const emulator = startVM($("screen"), buffer);
  emulator.add_listener("emulator-ready", function () {
    $("demo-restart").disabled = false;
    $("demo-fullscreen").disabled = false;
    $("demo-cad").disabled = false;
  });

  // 暖重启 (emulator.restart()) 之后 LWOS 在 v86 下键盘不再响应输入,
  // 冷启动没这个问题 —— 所以 Reset 用整页刷新, 顺带拉一次最新镜像
  $("demo-restart").onclick = function () { location.reload() };
  $("demo-fullscreen").onclick = function () { emulator.screen_go_fullscreen() };
  // Ctrl+Alt+Del 的 make/break 扫描码
  $("demo-cad").onclick = function () {
    emulator.keyboard_send_scancodes([0x1D, 0x38, 0x53, 0xD3, 0xB8, 0xDD]);
  };
}, fail);

// 当前镜像构建到哪个 commit; 本地 override 时没有意义
if (!new URLSearchParams(location.search).get("img")) {
  fetch(DEMO_INFO_PROXY, { cache: "no-store" })
    .then(function (r) { return r.ok ? r.json() : null })
    .then(function (info) {
      if (info && info.commit)
        $("demo-build").textContent = "image " + String(info.commit).slice(0, 7) +
          " — built " + info.date + " — rolling GitHub release";
    })
    .catch(function () { });
}
