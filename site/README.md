# lwos.dev 站点

落地页在这里, 以后的 wiki 也在这里。Astro 静态输出, 由 Cloudflare Workers
当纯静态资源发布 —— 没有服务端渲染, 没有 adapter, 静态资源请求免费且不限量。

## 目录

    public/                 只有这里的内容会被发布
      assets/               图片与字体 (从原来那份单文件 HTML 的内联 base64 拆出来的)
    src/
      data/site.ts          所有 URL 的唯一出处
      layouts/              SiteLayout.astro —— Win95 版 IE 外壳
      pages/                index.astro (主页)、404.astro
      scripts/              外壳的交互逻辑 (菜单/地址栏/对话框/快捷键)
      styles/               chrome.css (外壳) + page.css (文档)
    tools/                  用 Pillow 重新生成图片资源的脚本
    wrangler.jsonc          Cloudflare Workers 配置
    astro.config.mjs        Astro 配置

## 常用命令

    pnpm install
    pnpm dev        本地开发, 带热更新
    pnpm build      产出到 dist/
    pnpm preview    预览 dist/
    pnpm deploy     构建并发布到 Cloudflare (本地需要先 wrangler login)

## 页面是怎么拼起来的

外壳 (标题栏、菜单、地址栏、工具栏、状态栏、对话框) 全部在
`src/layouts/SiteLayout.astro` 里, 页面只写 `.page` 里的内容:

    <SiteLayout title="..." windowTitle="..." path="..." sections={[...]}>
      ...页面正文...
    </SiteLayout>

以后加 wiki 就是再加一个用同一个 layout 的页面, 外壳一行都不用改。

URL 一律从 `src/data/site.ts` 取。仓库搬家的话只改那一个文件。
Go 菜单和地址栏的补全列表都由同一个 `homeSections` 生成, 不会各写一份然后对不上。

## 重新生成图片资源

`tools/` 里那三个脚本需要 Pillow:

    pip install pillow
    python3 tools/mkgif.py     # boot/globe/new/divider/construction/spin 以及 5 个 88x31 按钮
    python3 tools/mkmem.py     # flatmem.gif
    python3 tools/mkwm.py      # wordmark.png / wordmark-mini.png

它们直接写进 `public/assets/`, 不需要再手动拷贝一遍。
脚本里的字体和图标输入在 `tools/fonts/` 和 `public/assets/img/lwos.png`。

> **注意**: 脚本里有 `random.seed(7)`, 逻辑上是可复现的, 但 Pillow 的 GIF/PNG
> 编码器各版本并不完全一致。实测用 Pillow 12.3.0 跑出来的字节数和仓库里现存的
> 图片对不上 (肉眼看不出差别, 但 sha256 变了)。所以 **别因为顺手跑了一遍脚本
> 就把这些图片的改动提交上去**, 除非你确实想要新图。

`public/assets/img/gplv3-badge.svg` 是 **FSF 官方的 GPLv3 徽章**, 不是上面这些脚本
生成的 —— 别去 `tools/` 里找它。官方那份 (gpl-v3-logo.svg) 是一张 A4 画布, 里面
**红黑两套** lockup 加一行 "Free as in Freedom", 所以裁过: 只留红色徽章本体, 加了
viewBox。改了哪三处、公有领域声明、设计者和上游 sha256 都记在
`public/assets/img/LICENSE-gplv3-badge.txt`。用矢量是因为它在高分屏上不糊; 高度按
31px 渲染, 宽度因此是 78px —— 官方那个 88x31 的 PNG 是**另一种取景**, 两者不会重合。
这个徽章是 banner 底部那排里唯一的链接 (指向仓库根目录的 `LICENSE`)。

## 部署 (Cloudflare Workers)

`lwos.dev` 已经托管在 Cloudflare 上 (NS 是 kelly/todd.ns.cloudflare.com),
但 apex 还没有 A 记录, 所以目前什么都访问不到。

在 Cloudflare dashboard 里:

1. **Workers & Pages → Create → 连到 GitHub 仓库 `LWOS-dev/lwos`**
2. **Root directory 填 `site`** —— 仓库根目录是 OS 源码, 站点在子目录里
3. **Build command**: `pnpm build`
4. **Deploy command**: `npx wrangler deploy`
5. 建好之后, 在 Worker 的 **Settings → Domains & Routes** 里绑 `lwos.dev`

之后每次 push 到 main 都会自动构建发布。

`site/node_modules/`、`site/dist/`、`site/.astro/` 已经在根 `.gitignore` 里,
不会进仓库。

## 几个以后会碰到的点

**wiki** — 说好是按 markdown 生成。到时候:

    src/content/wiki/*.md            放 markdown
    src/pages/wiki/[...slug].astro   渲染

用 Astro 的内容集合 (content collections) 就行。导航要变的话给 SiteLayout
传 `sections` 和 `path`, 外壳本身不用动。

**在线试玩** — 已做成 (2026-10-02, `site-demo` 分支起家):

- **镜像不进仓库**。`.github/workflows/image.yml` 每次 push 到 main 重新
  `make webimg`, 把 gzip 镜像 (64 MiB 压到 ~80 KB, FAT 大片是零) 和
  `build-info.json` 覆盖挂到固定 tag `latest-image` 的 Release 上 —— URL
  永远不变, 匿名可拉, 滚动更新。
- **首页的开机入口**: 主页 "What is LWOS?" 里的 boot.gif 是海报 (街机吸引
  画面), 上面浮一个 POWER ON 按钮, 点击后才动态 `import("./vm.js")` 把
  GIF 原地换成真机 —— v86 连 wasm 有 2.4 MB, 首页首屏 JS 只有 4 KB,
  chunk 平时不拉。共享逻辑在 `src/scripts/vm.js` (fetch 镜像 + 解压 +
  startVM + 键盘门控), demo 页和首页都走它。
- **Release 资产没有 CORS 头** (release-assets.githubusercontent.com 不回
  `Access-Control-Allow-Origin`), 浏览器直拉会被拦, 所以 `worker.js` 提供
  `/demo-image` 和 `/demo-image-info` 两条同源代理路由, 边缘缓存 5 分钟。
  wrangler 配置加了 `main`, 但匹配到静态资源的请求仍然直发不进脚本。
- demo 页 (`src/pages/demo.astro`) 用 `DecompressionStream` 解压后把 64 MiB
  buffer 喂给 v86 当 IDE 硬盘。v86 走 npm 包, wasm 由 Vite `?url` 打包;
  BIOS (npm 包不带) vendored 在 `public/vendor/v86/bios/`, `.gitignore`
  里有对应的 `!` 反排除规则。
- 本地调试: `make webimg` 生成 `site/public/assets/demo/lwcnc.img.gz`
  (gitignored), `pnpm dev` 然后开 `/demo/?img=/assets/demo/lwcnc.img.gz`。
  astro dev 对 .gz 会加 `Content-Encoding: gzip`, 浏览器自动解压一层,
  demo 页检测到 content-encoding 后会跳过手动解压, 两种来源都能吃。
- **已实测** (v86 0.5.465, SeaBIOS): LWOS 的 MBR → STAGE2 → LOADER (自带
  FAT32/ATA 驱动) → MONITOR v2 全链路正常引导, 8042 键盘探测 PASS,
  monitor 命令可交互。已知边界:
  ① demo 页用 `use_graphical_text` (canvas + VGA 字模) 渲染文本模式,
  比 DOM 文本渲染器更接近真机;
  ② monitor 的退格只回移输入下标, 屏幕不擦、缓冲里已删的字符仍会进命令
  (LWOS 侧的问题, type abc → 退格×3 → `h` 回车 → 执行的是 `hbc`);
  ③ 暖重启 (v86 restart) 后 guest 键盘不再响应, 冷启动正常,
  所以 Reset 按钮用整页刷新代替;
  ④ v86 不支持 task gates 和保护模式 far call, P7 做 TSS 任务切换时再评估。

**为什么 CSS 是两个文件** — `chrome.css` 是窗口外壳, `page.css` 是文档本体。
Astro 构建时会把它们合并成一个 CSS 文件, 所以运行时没有区别; 拆开纯粹是为了
源码上分得清哪块属于外壳、哪块属于页面。等 wiki 页面多起来, 如果不想让它再带上
落地页横幅那部分 CSS, 再把 `page.css` 拆成"文档通用"和"落地页专用"两块即可。
