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
export const WIKI = `${ORIGIN}/wiki/`;
export const BOCHS = "https://bochs.sourceforge.io/";
export const QEMU = "https://www.qemu.org/";

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
