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

另外 `public/assets/img/gplv3-88x31.png` 是 **FSF 官方的 GPLv3 按钮**, 不是上面
这些脚本生成的 —— 别去 `tools/` 里找它。来源、公有领域声明和 sha256 记在
`public/assets/img/LICENSE-gplv3-88x31-logo.txt`。按钮行里只有它是链接 (指向
仓库根目录的 `LICENSE`), 其余几个纯粹是装饰。

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

**在线试玩** — 现在卡在体积上。`lwcnc.img` 是 131040 × 512 = **64 MiB**,
而 Cloudflare 单个静态资源文件的上限是 **25 MiB**, 直接传不上去。可选:

- gzip 之后当静态资源发布, 前端用 `DecompressionStream` 解压再喂给 v86
  (FAT 镜像大片是零, 压缩率会很高)
- 放 R2, 用 Worker 绑定绕开 25 MiB 限制
- 专门做一个精简镜像

另外 **还没有实测过 LWOS 的 MBR 在 v86 (SeaBIOS) 下能不能正常接管引导**,
这个得真跑一次才知道, 不能想当然。

**为什么 CSS 是两个文件** — `chrome.css` 是窗口外壳, `page.css` 是文档本体。
Astro 构建时会把它们合并成一个 CSS 文件, 所以运行时没有区别; 拆开纯粹是为了
源码上分得清哪块属于外壳、哪块属于页面。等 wiki 页面多起来, 如果不想让它再带上
落地页横幅那部分 CSS, 再把 `page.css` 拆成"文档通用"和"落地页专用"两块即可。
