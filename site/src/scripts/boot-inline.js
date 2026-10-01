/* 首页的开机入口: boot.gif 当海报 (街机吸引画面), 点击后原地把 GIF 换成
   真机 (v86 引导最新 CI 镜像).
   v86 + wasm 有好几 MB, 所以点了才动态 import —— 首页首屏不背这个体积,
   Vite 会把 vm.js 连同 v86 拆成独立 chunk, 落地页平时一个字节都不拉. */
import { IMAGE_GZ_URL, DEMO } from "../data/site";

var $ = function (id) { return document.getElementById(id) };

var poster = $("boot-poster");
if (poster) {
  var cap = $("boot-cap");
  poster.addEventListener("click", async function () {
    poster.disabled = true;
    cap.textContent = "fetching the boot image\u2026";
    try {
      var vm = await import("./vm.js");
      var buffer = await vm.fetchImage(vm.resolveImageUrl());
      // 原地长大: 虚拟机框比海报高一大截, 直接换会一下子撑开页面.
      // 先把框钉在海报的宽高上, 再动画展开到自然高度.
      var shell = $("boot-shell");
      var startW = poster.offsetWidth, startH = poster.offsetHeight;
      poster.hidden = true;
      shell.hidden = false;
      shell.style.width = startW + "px"; // 宽度钉住, 屏幕在里面居中
      shell.style.height = startH + "px";
      shell.style.overflow = "hidden";
      shell.getBoundingClientRect(); // 强制回流, 让起始尺寸生效
      var endH = shell.scrollHeight;
      var shrink = function () {
        shell.style.height = "";
        shell.style.overflow = "";
        shell.style.transition = "";
      };
      if (matchMedia("(prefers-reduced-motion: reduce)").matches) {
        shrink();
      } else {
        shell.style.transition = "height .45s ease";
        shell.style.height = endH + "px";
        shell.addEventListener("transitionend", shrink, { once: true });
        setTimeout(shrink, 700); // transitionend 偶发不触发的兜底
      }
      var emulator = vm.startVM($("bootscreen"), buffer);
      emulator.add_listener("emulator-ready", function () {
        if (document.fullscreenEnabled || document.webkitFullscreenEnabled)
          $("boot-fullscreen").disabled = false;
        $("boot-cad").disabled = false;
        cap.textContent = "LIVE — the real system, running the latest CI image. ";
        var a = document.createElement("a");
        a.href = DEMO;
        a.textContent = "Open the full demo page";
        cap.appendChild(a);
      });
      $("boot-fullscreen").onclick = function () {
        vm.goFullscreen(document.querySelector("#bootdemo .demo-bezel"));
      };
      // Ctrl+Alt+Del 的 make/break 扫描码
      $("boot-cad").onclick = function () {
        emulator.keyboard_send_scancodes([0x1D, 0x38, 0x53, 0xD3, 0xB8, 0xDD]);
      };
    } catch (e) {
      console.error(e);
      poster.disabled = false;
      cap.textContent = "Power-on failed: " + (e && e.message ? e.message : e) + " — ";
      var d = document.createElement("a");
      d.href = IMAGE_GZ_URL;
      d.textContent = "get the image directly";
      cap.appendChild(d);
    }
  });
}
