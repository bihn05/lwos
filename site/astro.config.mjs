import { defineConfig } from "astro/config";

/**
 * 只产出静态文件。Cloudflare Workers 直接把 dist/ 当静态资源发布
 * (见 wrangler.jsonc), 所以既没有 adapter 也没有服务端渲染。
 *
 * format: "directory" 让 URL 不带扩展名 ——
 * src/pages/wiki/foo.astro 会变成 /wiki/foo/ 而不是 /wiki/foo.html。
 */
export default defineConfig({
  site: "https://lwos.dev",
  build: { format: "directory" },
});
