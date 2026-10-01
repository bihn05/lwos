/**
 * 站点全部 URL 的唯一出处。
 * 仓库要是搬家了, 只需要改这一个文件。
 */

/** 对外域名。也是地址栏里显示的那个。 */
export const ORIGIN = "https://lwos.dev";

export const REPO = "https://github.com/LWOS-dev/lwos";
export const SPECS = `${REPO}/tree/main/spec2`;
export const REPORT = `${REPO}/tree/main/report`;
export const ROADMAP = `${REPO}/blob/main/spec2/90-roadmap.md`;
export const DEVLOG = `${REPO}/blob/main/spec2/99-log.md`;
export const LICENSE_URL = `${REPO}/blob/main/LICENSE`;
export const WIKI = `${ORIGIN}/wiki/`;
export const BOCHS = "https://bochs.sourceforge.io/";
export const QEMU = "https://www.qemu.org/";

/** 在线试玩页 (v86). */
export const DEMO = `${ORIGIN}/demo/`;

/**
 * 在线试玩用的启动镜像不进仓库: CI 每次 push 到 main 都重新构建, 把产物覆盖
 * 挂在固定 tag 的 Release 上 (滚动更新). Release 资产没有 CORS 头, 浏览器直拉
 * 会被拦, 所以 demo 页实际请求的是同源代理路由 (worker.js 转发), 下面这个是
 * 给人看的直链. 本地调试时 demo 页支持 ?img= 覆盖.
 */
export const IMAGE_RELEASE_TAG = "latest-image";
export const IMAGE_GZ_URL = `${REPO}/releases/download/${IMAGE_RELEASE_TAG}/lwcnc.img.gz`;

/** 同源代理路由, 由 site/worker.js 提供; astro dev 下没有, 用 ?img= 顶替. */
export const DEMO_IMAGE_PROXY = "/demo-image";
export const DEMO_INFO_PROXY = "/demo-image-info";

/** 页面作者 (前端部分), 出现在页脚署名里。账号和链接分开写, 只改账号就够了。 */
export const AUTHOR = "No-22-Github";
export const AUTHOR_URL = `https://github.com/${AUTHOR}`;

export interface NavItem {
  /** 页内锚点, 例如 "#about" */
  href: string;
  label: string;
}

/**
 * 主页的各个小节。Go 菜单和地址栏的下拉补全都由它生成,
 * 所以这两处不会各写一份然后对不上。
 */
export const homeSections: NavItem[] = [
  { href: "#about", label: "What is LWOS?" },
  { href: "#memory", label: "Flat memory, hardware tasks" },
  { href: "#works", label: "What works today" },
  { href: "#build", label: "Build & run" },
];
