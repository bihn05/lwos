/**
 * 静态站点 + 一条同源代理路由。
 *
 * /demo-image 和 /demo-image-info: 把 GitHub Release 上滚动更新的启动镜像
 * 代理给 demo 页。Release 资产 (release-assets.githubusercontent.com) 不带
 * Access-Control-Allow-Origin, 浏览器直拉会被 CORS 拦掉; 在 Worker 里 fetch
 * 没有这个问题, 浏览器只看到同源请求。
 *
 * 注意部署模型: 匹配到 dist/ 里静态资源的请求根本不会进这个脚本
 * (Cloudflare 直发, 免费不限量); 只有 /demo-image* 这类匹配不到的路径
 * 才会跑到这里, 消耗的是普通 Worker 请求额度 (免费 10 万次/天)。
 */

const TAG = "latest-image";
const UPSTREAM = {
  "/demo-image":
    "https://github.com/LWOS-dev/lwos/releases/download/" + TAG + "/lwcnc.img.gz",
  "/demo-image-info":
    "https://github.com/LWOS-dev/lwos/releases/download/" + TAG + "/build-info.json",
};

/** 边缘缓存 5 分钟: 滚动发布的镜像最多晚 5 分钟可见, 换来对 GitHub 的克制。 */
const EDGE_CACHE = { cacheEverything: true, cacheTtl: 300 };

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    const upstream = UPSTREAM[url.pathname];

    if (!upstream) return env.ASSETS.fetch(request);

    const r = await fetch(upstream, { redirect: "follow", cf: EDGE_CACHE });
    if (!r.ok) {
      return new Response(
        "upstream " + r.status + " — the rolling release may not exist yet",
        { status: 502 },
      );
    }
    const headers = new Headers(r.headers);
    headers.delete("content-disposition"); // 别让浏览器把它当成下载文件
    if (url.pathname === "/demo-image-info") {
      headers.set("content-type", "application/json");
    } else {
      // content-type 里带 gzip, demo 页据此决定要不要过 DecompressionStream
      headers.set("content-type", "application/gzip");
    }
    headers.set("access-control-allow-origin", "*"); // 本地 astro dev 跨域调试用
    return new Response(r.body, { status: r.status, headers });
  },
};
