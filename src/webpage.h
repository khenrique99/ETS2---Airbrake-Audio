// The settings page served by web.h, embedded in the DLL. It is split into two
// literals because MSVC limits a single string literal to about 16 KB.
#pragma once

inline const char* webPageHead() {
    return R"HTML(<!doctype html>
<html lang="pt-BR"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Airbrake Audio</title>
<style>
:root{--bg:#f5f4ef;--card:#fff;--text:#1f1e1c;--mute:#6b6a65;--line:#e0ded6;--acc:#185fa5;--ok:#3b6d11;--okbg:#eaf3de;--bad:#a32d2d;--badbg:#fcebeb}
@media (prefers-color-scheme:dark){:root{--bg:#1d1d1b;--card:#2a2a27;--text:#ecebe6;--mute:#a3a29b;--line:#3d3d39;--acc:#85b7eb;--ok:#c0dd97;--okbg:#27500a;--bad:#f7c1c1;--badbg:#791f1f}}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--text);font:15px/1.5 system-ui,Segoe UI,sans-serif}
main{max-width:760px;margin:0 auto;padding:16px}
a{color:var(--acc)}
header{display:flex;flex-wrap:wrap;align-items:center;gap:10px;margin-bottom:4px}
h1{font-size:20px;font-weight:600;margin:0}
.badge{font-size:12px;padding:2px 10px;border-radius:8px;background:var(--okbg);color:var(--ok)}
.badge.off{background:var(--badbg);color:var(--bad)}
.sp{flex:1}
.credit{font-size:13px;color:var(--mute);margin:0 0 14px}
.credit a{margin-left:6px}
button{font:inherit;background:var(--card);color:var(--text);border:1px solid var(--line);border-radius:8px;padding:6px 12px;cursor:pointer}
button:hover{border-color:var(--mute)}
button.pri{background:var(--text);color:var(--bg);border-color:var(--text)}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:10px;margin-bottom:14px}
.m{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:10px 12px}
.m span{display:block;font-size:12px;color:var(--mute)}
.m b{font-size:22px;font-weight:600}
.bar{height:4px;background:var(--line);border-radius:2px;margin-top:6px}
.bar i{display:block;height:4px;width:0;background:var(--acc);border-radius:2px}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:14px 18px;margin-bottom:14px}
.card h2{font-size:15px;font-weight:600;margin:6px 0 10px}
.row{display:grid;grid-template-columns:minmax(120px,190px) 1fr 70px;gap:10px;align-items:center;margin-bottom:6px}
.row output{text-align:right;font-variant-numeric:tabular-nums}
input[type=range]{width:100%}
.actions{display:flex;gap:8px;justify-content:flex-end;flex-wrap:wrap;align-items:center}
#msg{font-size:13px;color:var(--mute);margin-right:auto}
.key{display:flex;gap:10px;align-items:center;flex-wrap:wrap}
kbd{background:var(--bg);border:1px solid var(--line);border-radius:6px;padding:2px 10px;font:600 14px ui-monospace,Consolas,monospace}
</style></head><body><main>
<header>
<h1>Airbrake Audio</h1><span class="badge" id="badge">...</span>
<span class="sp"></span>
<button id="lang">EN</button><button id="close"></button>
</header>
<p class="credit">Airbrake Audio - Developed by Kelvin Henrique
<a href="https://www.instagram.com/khenrique99/" target="_blank" rel="noopener">Instagram @khenrique99</a>
<a href="https://github.com/khenrique99" target="_blank" rel="noopener">GitHub khenrique99</a></p>
<div class="grid">
<div class="m"><span data-t="pedal"></span><b id="m-pedal">0%</b><div class="bar"><i id="m-bar"></i></div></div>
<div class="m"><span data-t="speed"></span><b id="m-speed">0</b></div>
<div class="m"><span data-t="held"></span><b id="m-held">0.0 s</b></div>
<div class="m"><span data-t="state"></span><b id="m-state">-</b></div>
</div>
<div class="card"><h2 data-t="hotkey"></h2><div class="key"><kbd id="key">-</kbd><button id="setkey"></button><span id="keyhint" style="color:var(--mute);font-size:13px"></span></div></div>
<div class="card" id="fields"></div>
<div class="actions"><span id="msg"></span><button id="reset"></button><button class="pri" id="save"></button></div>
)HTML";
}

inline const char* webPageScript() {
    return R"HTML(<script>
var T={pt:{pedal:"Pedal",speed:"Velocidade",held:"Freando há",state:"Som tocando",hotkey:"Tecla para abrir e fechar esta tela",
setkey:"Alterar tecla",press:"Aperte a nova tecla (Esc cancela)",save:"Salvar no .ini",reset:"Restaurar padrão",close:"Fechar",
on:"conectado",off:"desconectado",saved:"Salvo no .ini.",fail:"Não foi possível salvar o .ini (pasta sem permissão?).",reseted:"Valores padrão aplicados. Clique em salvar para manter.",
none:"nenhum",brake:"freio",air:"saída de ar",
g_output:"Saída",g_brake:"Freio",g_air_release:"Saída de ar",
"output.boost_db":["Ganho (boost)","dB",1,.5,1],
"brake.volume_min":["Volume mínimo","%",100,1,0],"brake.volume_max":["Volume máximo","%",100,1,0],
"brake.min_speed_kmh":["Mudo abaixo de","km/h",1,1,0],"brake.fade_in_ms":["Fade de entrada","ms",1,10,0],"brake.fade_out_ms":["Fade de saída","ms",1,10,0],
"air_release.volume":["Volume","%",100,1,0],"air_release.full_brake_time_s":["Freada para o som inteiro","s",1,.5,1],
"air_release.short_volume":["Volume na freada curta","%",100,1,0],"air_release.short_length_s":["Duração na freada curta","s",1,.05,2],
"air_release.audible_length_s":["Corte máximo antes do inteiro","s",1,.05,2],"air_release.cut_fade_ms":["Fade do corte","ms",1,5,0],
"air_release.min_brake_time_ms":["Freada mínima","ms",1,50,0],"air_release.cooldown_ms":["Intervalo entre saídas","ms",1,100,0]},
en:{pedal:"Pedal",speed:"Speed",held:"Braking for",state:"Playing",hotkey:"Key that opens and closes this page",
setkey:"Change key",press:"Press the new key (Esc cancels)",save:"Save to .ini",reset:"Restore defaults",close:"Close",
on:"connected",off:"disconnected",saved:"Saved to .ini.",fail:"Could not save the .ini (folder not writable?).",reseted:"Defaults applied. Click save to keep them.",
none:"none",brake:"brake",air:"air release",
g_output:"Output",g_brake:"Brake",g_air_release:"Air release",
"output.boost_db":["Gain (boost)"],"brake.volume_min":["Minimum volume"],"brake.volume_max":["Maximum volume"],
"brake.min_speed_kmh":["Muted below"],"brake.fade_in_ms":["Fade in"],"brake.fade_out_ms":["Fade out"],
"air_release.volume":["Volume"],"air_release.full_brake_time_s":["Brake time for the full sound"],
"air_release.short_volume":["Volume after a short brake"],"air_release.short_length_s":["Length after a short brake"],
"air_release.audible_length_s":["Longest early cut"],"air_release.cut_fade_ms":["Cut fade"],
"air_release.min_brake_time_ms":["Minimum brake time"],"air_release.cooldown_ms":["Time between releases"]}};
var lang="pt",st=null,capturing=false,timers={};
try{lang=localStorage.getItem("lang")||((navigator.language||"").slice(0,2)=="pt"?"pt":"en")}catch(e){}
function $(i){return document.getElementById(i)}
function tr(k){return T[lang][k]!==undefined?T[lang][k]:T.pt[k]}
function meta(id){var m=T.pt[id],l=T[lang][id];if(!m)return {};return {label:(l||m)[0],unit:m[1],scale:m[2],step:m[3],dec:m[4]}}
function keyName(c){if(c=="Minus")return"-";if(c=="Equal")return"=";if(/^Key[A-Z]$/.test(c))return c[3];if(/^Digit[0-9]$/.test(c))return c[5];return c}
function post(path,body){return fetch(path,{method:"POST",headers:{"X-Airbrake":"1","Content-Type":"application/x-www-form-urlencoded"},body:body||""}).then(function(r){return r.json()})}
function say(t){$("msg").textContent=t}
function texts(){
 document.documentElement.lang=lang=="pt"?"pt-BR":"en";
 document.querySelectorAll("[data-t]").forEach(function(e){e.textContent=tr(e.getAttribute("data-t"))});
 $("setkey").textContent=tr("setkey");$("save").textContent=tr("save");$("reset").textContent=tr("reset");$("close").textContent=tr("close");
 $("lang").textContent=lang=="pt"?"EN":"PT";
 if(st){$("badge").textContent=tr("on");$("key").textContent=keyName(st.hotkey)}
}
function build(){
 var box=$("fields"),group="";box.innerHTML="";
 st.fields.forEach(function(f){
  var m=meta(f.id);if(!m.label)return;
  var g=f.id.split(".")[0];
  if(g!=group){group=g;var h=document.createElement("h2");h.setAttribute("data-t","g_"+g);box.appendChild(h)}
  var row=document.createElement("div");row.className="row";
  var lab=document.createElement("label");lab.textContent=m.label;
  var inp=document.createElement("input");inp.type="range";inp.id="f-"+f.id;
  inp.min=f.min*m.scale;inp.max=f.max*m.scale;inp.step=m.step;inp.value=f.value*m.scale;
  var out=document.createElement("output");
  function show(){out.textContent=(+inp.value).toFixed(m.dec)+" "+m.unit}
  function send(){post("/api/config",encodeURIComponent(f.id)+"="+(inp.value/m.scale)).then(refresh)}
  inp.addEventListener("input",function(){show();if(f.id!="output.boost_db"){clearTimeout(timers[f.id]);timers[f.id]=setTimeout(send,80)}});
  inp.addEventListener("change",function(){if(f.id=="output.boost_db")send()});
  show();row.appendChild(lab);row.appendChild(inp);row.appendChild(out);box.appendChild(row);
 });
 texts();
}
function refresh(){
 return fetch("/api/state").then(function(r){return r.json()}).then(function(s){
  var first=!st;st=s;$("badge").className="badge";
  if(first)build();
  var m=s.metrics;
  $("m-pedal").textContent=Math.round(m.pedal*100)+"%";$("m-bar").style.width=Math.round(m.pedal*100)+"%";
  $("m-speed").textContent=Math.round(m.speed)+" km/h";$("m-held").textContent=m.held.toFixed(1)+" s";
  $("m-state").textContent=tr(["none","brake","air"][m.state]);
  $("badge").textContent=tr("on");if(!capturing)$("key").textContent=keyName(s.hotkey);
 }).catch(function(){$("badge").className="badge off";$("badge").textContent=tr("off")});
}
$("lang").onclick=function(){lang=lang=="pt"?"en":"pt";try{localStorage.setItem("lang",lang)}catch(e){}if(st)build();else texts()};
$("close").onclick=function(){window.close()};
$("save").onclick=function(){post("/api/save").then(function(r){say(r.ok?tr("saved"):tr("fail"))}).catch(function(){say(tr("fail"))})};
$("reset").onclick=function(){post("/api/reset").then(function(){st=null;say(tr("reseted"));return refresh()})};
$("setkey").onclick=function(){capturing=true;$("key").textContent="...";$("keyhint").textContent=tr("press")};
document.addEventListener("keydown",function(e){
 if(capturing){
  e.preventDefault();capturing=false;$("keyhint").textContent="";
  if(e.code!="Escape"){post("/api/config","hotkey="+encodeURIComponent(e.code)).then(refresh)}else{$("key").textContent=keyName(st.hotkey)}
  return;
 }
 if(st&&e.code==st.hotkey&&!e.repeat){e.preventDefault();window.close()}
});
texts();refresh();setInterval(refresh,500);
</script></main></body></html>
)HTML";
}
