/* Win95 版 Internet Explorer 外壳的行为: 菜单、带补全的地址栏、
   页内前进后退、模态对话框, 以及键盘快捷键。
   普通 IIFE, 无依赖; 由 Astro 打包。 */
(function(){
  var $=function(id){return document.getElementById(id)};
  var st=$('status'), addr=$('addr'), list=$('addrlist'), drop=$('addrdrop');
  // 由 SiteLayout.astro 提供, 这样同一套外壳在 / 和 /wiki/ 下都能用
  var win=document.querySelector('.win'), ORIGIN=win.dataset.origin, BASE=win.dataset.base;
  var HOST=new URL(ORIGIN).host, HOME=new URL(BASE).pathname.replace(/\/+$/,'');
  var HOME_RE=new RegExp('^(?:www\\.)?'+HOST.replace(/\\./g,'\\\\.')
    +HOME.replace(/[.*+?^${}()|[\]\\]/g,'\\$&')+'/?(?:#([\\w-]*))?$','i');
  var hist=['#top'], hi=0;
  function setStatus(t){st.textContent=t}
  function external(url){
    var a=document.createElement('a');a.href=url;a.target='_blank';a.rel='noopener';
    document.body.appendChild(a);a.click();a.remove();
    setStatus('Opening '+url+'...');setTimeout(function(){setStatus('Done')},1200);
  }
  function sync(){
    $('t-back').disabled=hi<=0;$('t-fwd').disabled=hi>=hist.length-1;
    document.querySelector('[data-act="back"]').disabled=hi<=0;
    document.querySelector('[data-act="fwd"]').disabled=hi>=hist.length-1;
  }
  function show(hash,push){
    var el=(hash!=='#top'&&document.querySelector(hash))||$('top');
    if(hash==='#top')window.scrollTo({top:0});else el.scrollIntoView({block:'start'});
    addr.value=hash==='#top'?BASE:BASE+hash;
    if(push!==false&&hist[hi]!==hash){hist=hist.slice(0,hi+1);hist.push(hash);hi=hist.length-1}
    sync();
  }
  function back(){if(hi>0){hi--;show(hist[hi],false)}}
  function fwd(){if(hi<hist.length-1){hi++;show(hist[hi],false)}}

  // dialog
  var icon=document.querySelector('.titlebar img').src, lastFocus=null;
  function dialog(title,html){
    lastFocus=document.activeElement;
    $('dlg-title').textContent=title;$('dlg-text').innerHTML=html;$('dlg-icon').src=icon;
    $('veil').hidden=false;$('dlg-ok').focus();
  }
  function closeDlg(){$('veil').hidden=true;if(lastFocus&&lastFocus.focus)lastFocus.focus()}
  $('dlg-ok').onclick=closeDlg;$('dlg-x').onclick=closeDlg;
  function esc(t){return t.replace(/[&<>"]/g,function(c){return{'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]})}

  // address bar: lwos.dev anchors scroll in place, anything else opens in a new tab
  function navigate(raw){
    var v=raw.trim(); if(!v){show('#top');return}
    var m=v.replace(/^https?:\/\//i,'');
    var home=m.match(HOME_RE);
    if(home){var h=home[1]||'';show(h&&document.getElementById(h)?'#'+h:'#top');return}
    var bare=v.replace(/^#/,'');
    if(/^[\w-]+$/.test(bare)&&document.getElementById(bare)){show('#'+bare);return}
    if(/^https?:\/\//i.test(v)||/^[\w-]+(\.[\w-]+)+(\/\S*)?$/.test(m)){
      var url=/^https?:/i.test(v)?v:'https://'+m; addr.value=url; external(url); return;
    }
    dialog('Microsoft Internet Explorer','Internet Explorer cannot open the Internet site <b>'+esc(v)+'</b>.<br><br>A connection with the server could not be established.');
  }
  function openList(){list.hidden=false;drop.setAttribute('aria-expanded','true')}
  function closeList(){list.hidden=true;drop.setAttribute('aria-expanded','false');list.querySelectorAll('.act').forEach(function(x){x.classList.remove('act')})}
  $('addrform').addEventListener('submit',function(e){e.preventDefault();closeList();navigate(addr.value);addr.blur()});
  drop.onclick=function(){if(list.hidden)openList();else closeList()};
  list.addEventListener('click',function(e){var li=e.target.closest('li');if(!li)return;addr.value=li.dataset.url;closeList();navigate(li.dataset.url)});
  addr.addEventListener('keydown',function(e){
    var items=[].slice.call(list.children),i=items.findIndex(function(x){return x.classList.contains('act')});
    if(e.key==='ArrowDown'||e.key==='ArrowUp'){
      e.preventDefault();openList();if(i>-1)items[i].classList.remove('act');
      i=e.key==='ArrowDown'?Math.min(items.length-1,i+1):Math.max(0,i-1);
      items[i].classList.add('act');addr.value=items[i].dataset.url;
    } else if(e.key==='Escape'){closeList()}
  });
  addr.addEventListener('focus',function(){addr.select()});

  // menus
  var bar=$('menubar'), openMi=null;
  function closeMenus(){if(openMi){openMi.classList.remove('open');openMi.querySelector('.menu').hidden=true;openMi=null}}
  function openMenu(mi){closeMenus();closeList();mi.classList.add('open');mi.querySelector('.menu').hidden=false;openMi=mi}
  bar.querySelectorAll('.mi').forEach(function(mi){
    var t=mi.querySelector('.mt');
    t.addEventListener('click',function(e){e.stopPropagation();if(openMi===mi)closeMenus();else{openMenu(mi);var f=mi.querySelector('.menu button:not([disabled])');if(e.detail===0&&f)f.focus()}});
    t.addEventListener('mouseenter',function(){if(openMi&&openMi!==mi)openMenu(mi)});
  });
  bar.addEventListener('keydown',function(e){
    if(!openMi)return;
    var items=[].slice.call(openMi.querySelectorAll('.menu button:not([disabled])')),i=items.indexOf(document.activeElement);
    if(e.key==='ArrowDown'){e.preventDefault();items[(i+1)%items.length].focus()}
    if(e.key==='ArrowUp'){e.preventDefault();items[(i-1+items.length)%items.length].focus()}
    if(e.key==='ArrowRight'||e.key==='ArrowLeft'){
      var mis=[].slice.call(bar.querySelectorAll('.mi')),j=mis.indexOf(openMi);
      j=(j+(e.key==='ArrowRight'?1:-1)+mis.length)%mis.length;openMenu(mis[j]);
      var f=mis[j].querySelector('.menu button:not([disabled])');if(f)f.focus();
    }
  });
  function toggle(btn,el){btn.classList.toggle('on');el.hidden=!btn.classList.contains('on');btn.setAttribute('aria-checked',btn.classList.contains('on'))}
  function font(cls,btn){var p=$('top');p.classList.remove('fs-l','fs-s');if(cls)p.classList.add(cls);btn.parentNode.querySelectorAll('.rad').forEach(function(b){b.classList.toggle('on',b===btn)})}
  function refresh(){
    setStatus('Opening page '+addr.value+'...');
    var t=document.querySelector('.throbber');t.classList.add('busy');
    setTimeout(function(){t.classList.remove('busy');setStatus('Done')},700);
  }
  bar.addEventListener('click',function(e){
    var b=e.target.closest('.menu button');if(!b||b.disabled)return;
    closeMenus();
    if(b.dataset.go)return show(b.dataset.go);
    if(b.dataset.href)return external(b.dataset.href);
    switch(b.dataset.act){
      case 'open':addr.focus();break;
      case 'props':dialog('Properties','<b>LWOS Home Page</b><br>Address: '+esc(addr.value)+'<br>Type: HTML Document<br>Protocol: HyperText Transfer Protocol<br>Size: every byte by hand');break;
      case 'copy':
        var u=addr.value;
        var p=navigator.clipboard?navigator.clipboard.writeText(u):Promise.reject();
        p.then(function(){setStatus('Copied '+u)},function(){addr.focus();addr.select();setStatus('Press Ctrl+C to copy the address')});break;
      case 'selall':var r=document.createRange();r.selectNodeContents($('top'));var sel=getSelection();sel.removeAllRanges();sel.addRange(r);break;
      case 'tg-toolbar':toggle(b,$('toolbar'));break;
      case 'tg-addr':toggle(b,$('addrbar'));break;
      case 'tg-status':toggle(b,$('statusbar'));break;
      case 'font-l':font('fs-l',b);break;
      case 'font-m':font('',b);break;
      case 'font-s':font('fs-s',b);break;
      case 'refresh':refresh();break;
      case 'back':back();break;
      case 'fwd':fwd();break;
      case 'about':dialog('About LWOS','<b>LWOS v2</b><br>A hobby OS for CNC milling machines.<br>Flat 32-bit protected mode. No BIOS, no paging.<br><br>Copyleft 2026 &middot; GPL-3.0-only<br>https://lwos.dev');break;
    }
  });
  document.addEventListener('click',function(e){
    if(!e.target.closest('#menubar'))closeMenus();
    if(!e.target.closest('#addrform'))closeList();
  });
  document.addEventListener('keydown',function(e){
    if(e.key==='Escape'){closeMenus();closeList();if(!$('veil').hidden)closeDlg()}
    if(e.key==='F5'){e.preventDefault();refresh()}
    if((e.ctrlKey||e.metaKey)&&e.key.toLowerCase()==='l'){e.preventDefault();addr.focus()}
    if(e.altKey&&e.key==='ArrowLeft'){e.preventDefault();back()}
    if(e.altKey&&e.key==='ArrowRight'){e.preventDefault();fwd()}
  });

  // in-page links go through our history; status bar shows where a link goes
  document.querySelectorAll('a[href]').forEach(function(a){
    var h=a.getAttribute('href');
    a.addEventListener('mouseenter',function(){setStatus('Shortcut to '+(h.charAt(0)==='#'?BASE+(h==='#top'?'':h):a.href))});
    a.addEventListener('mouseleave',function(){setStatus('Done')});
    if(h.charAt(0)==='#')a.addEventListener('click',function(e){e.preventDefault();show(h)});
  });
  $('t-back').onclick=back;$('t-fwd').onclick=fwd;
  $('t-home').onclick=function(){show('#top')};
  $('t-stop').onclick=function(){setStatus('Stopped')};
  $('t-refresh').onclick=refresh;
  sync();
})();
