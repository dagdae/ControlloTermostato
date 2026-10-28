// Pagina web della centralina (HTML + CSS + JS in un unico file, servita da /).
#pragma once

#include <Arduino.h>

static const char INDEX_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="it"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Controllo termosifone</title>
<style>
:root{color-scheme:light;--bg:#f9f9f7;--card:#fcfcfb;--ink:#0b0b0b;--ink2:#52514e;--muted:#898781;--grid:#e1e0d9;--axis:#c3c2b7;--line:#2a78d6;--wash:rgba(137,135,129,.18);--border:rgba(11,11,11,.10);--good:#0ca30c;--warn:#fab219;--btn:#efeee9;--sel:#2a78d6;--selink:#fff}
@media (prefers-color-scheme:dark){:root{color-scheme:dark;--bg:#0d0d0d;--card:#1a1a19;--ink:#f0efec;--ink2:#c3c2b7;--muted:#898781;--grid:#2c2c2a;--axis:#383835;--line:#3987e5;--wash:rgba(195,194,183,.16);--border:rgba(255,255,255,.10);--btn:#2c2c2a;--sel:#3987e5;--selink:#fff}}
*{box-sizing:border-box}
[hidden]{display:none!important}
body{margin:0;background:var(--bg);color:var(--ink);font:15px/1.45 system-ui,-apple-system,"Segoe UI",sans-serif}
header,main,footer{max-width:760px;margin:0 auto;padding:0 16px}
header{display:flex;align-items:center;justify-content:space-between;gap:8px;padding-top:16px}
h1{font-size:18px;margin:0}
h2{font-size:15px;margin:0 0 10px}
.card{background:var(--card);border:1px solid var(--border);border-radius:12px;padding:16px;margin:12px 0}
.muted{color:var(--muted)}.ink2{color:var(--ink2)}
.hero{display:flex;align-items:baseline;gap:6px}
.hero b{font-size:56px;font-weight:600;line-height:1}
.hero span{font-size:20px;color:var(--ink2)}
.badge{display:inline-flex;align-items:center;gap:6px;font-weight:600}
.dot{width:10px;height:10px;border-radius:50%;background:var(--muted);flex:none}
.dot.good{background:var(--good)}.dot.warn{background:var(--warn)}
.row{display:flex;flex-wrap:wrap;gap:8px 16px;align-items:center;margin-top:8px}
.warnbox{margin-top:10px;padding:8px 10px;border-radius:8px;background:rgba(250,178,25,.14)}
button{font:inherit;padding:8px 12px;border-radius:8px;border:1px solid var(--border);background:var(--btn);color:var(--ink);cursor:pointer}
button.on{background:var(--sel);color:var(--selink);border-color:transparent}
button:disabled{opacity:.5;cursor:default}
.btns{display:flex;flex-wrap:wrap;gap:8px;align-items:center}
input,select{font:inherit;padding:6px 8px;border-radius:8px;border:1px solid var(--border);background:var(--bg);color:var(--ink);max-width:100%}
input[type=number]{width:96px}
select{width:100%;max-width:330px}
.scroll{overflow-x:auto}
fieldset{border:1px solid var(--border);border-radius:10px;margin:0 0 12px;padding:10px 12px;min-width:0}
legend{font-weight:600;padding:0 4px}
label{display:flex;flex-wrap:wrap;justify-content:space-between;align-items:center;gap:6px 12px;margin:6px 0}
label small{flex-basis:100%;color:var(--muted)}
table{border-collapse:collapse;width:100%;font-variant-numeric:tabular-nums}
th,td{text-align:left;padding:4px 6px;border-bottom:1px solid var(--grid);font-size:13px}
#chart{position:relative;margin-top:6px}
#chart svg{display:block;width:100%;touch-action:none}
#chart svg:focus{outline:2px solid var(--sel);outline-offset:2px}
.legend{display:flex;gap:16px;font-size:13px;color:var(--ink2);margin-top:4px}
.key{display:inline-block;vertical-align:middle;margin-right:6px}
.key.line{width:16px;height:2px;background:var(--line)}
.key.band{width:12px;height:12px;background:var(--wash);border-radius:2px}
.tip{position:absolute;pointer-events:none;background:var(--card);border:1px solid var(--border);border-radius:8px;padding:6px 8px;font-size:13px;white-space:nowrap;box-shadow:0 2px 8px rgba(0,0,0,.12)}
.tip b{display:block;font-size:15px}
.ax{fill:var(--muted);font-size:11px;font-variant-numeric:tabular-nums}
.thr{fill:var(--ink2);font-size:11px}
ol{margin:0;padding-left:18px}li{margin:2px 0}
footer{padding-bottom:24px;font-size:13px}
a{color:var(--sel)}
</style></head><body>
<header><h1>Controllo termosifone</h1><span id="conn" class="muted">...</span></header>
<main>
<section class="card" aria-live="polite">
 <div class="hero"><b id="temp">--</b><span>&deg;C</span></div>
 <div id="src" class="ink2"></div>
 <div class="row"><span class="badge"><span id="outDot" class="dot"></span><span id="out">...</span></span><span id="mode" class="ink2"></span></div>
 <div class="row"><span id="state"></span><span id="timer" class="ink2"></span></div>
 <div id="reason" class="muted" style="margin-top:6px"></div>
 <div id="err" class="warnbox" hidden></div>
</section>

<section class="card">
 <h2>Comandi</h2>
 <div class="btns">
  <button data-cmd="auto" id="bAuto">Automatico</button>
  <button data-cmd="bypass" id="bBypass">Solo termostato</button>
  <button data-cmd="forceoff" id="bOff">Spento forzato</button>
  <button data-cmd="resume" id="bResume">Riaccendi ora</button>
 </div>
 <div class="btns" style="margin-top:8px">
  <button data-cmd="forceon" id="bOn">Acceso forzato per</button>
  <input type="number" id="onMin" value="60" min="1" max="1440" aria-label="minuti"> <span class="muted">min</span>
 </div>
 <div id="pressSync" class="btns" style="margin-top:8px" hidden>
  <span class="ink2">Il termostato adesso &egrave;:</span>
  <button data-cmd="thermo_on">Acceso</button><button data-cmd="thermo_off">Spento</button>
 </div>
</section>

<section class="card">
 <h2>Temperatura del termosifone, ultime 24 ore</h2>
 <div class="legend"><span><i class="key line"></i>Temperatura</span><span><i class="key band"></i>Riscaldamento bloccato</span></div>
 <div id="chart"></div>
 <details style="margin-top:8px"><summary class="ink2">Tabella oraria</summary><div id="table"></div></details>
</section>

<section class="card" id="nodesCard" hidden>
 <h2>Nodi sensore a batteria</h2><div id="nodes"></div>
</section>

<section class="card">
 <h2>Impostazioni</h2>
 <form id="cfg">
 <fieldset><legend>Ciclo</legend>
  <label>Spegni quando il termosifone arriva a (&deg;C) <input name="t_off" type="number" step="0.5" min="25" max="90"></label>
  <label>Ritardo dopo la soglia (min) <input name="overrun_min" type="number" min="0" max="240"><small>0 = spegne subito. Esempio: 15 = a 50 &deg;C parte un timer di 15 minuti, poi spegne.</small></label>
  <label>Riaccensione <select name="restart">
   <option value="0">Quando scende sotto la soglia</option><option value="1">Dopo il timer</option>
   <option value="2">Timer o soglia (il primo)</option><option value="3">Timer e soglia (entrambi)</option>
   <option value="4">Mai: resta spento fino a "Riaccendi ora"</option></select></label>
  <label>Riaccendi sotto (&deg;C) <input name="t_on" type="number" step="0.5" min="10" max="89"></label>
  <label>Timer di pausa (min) <input name="off_min" type="number" min="1" max="1440"><small>Usato dalle modalit&agrave; di riaccensione a timer (90 = 1 h 30 min).</small></label>
  <label>Limite di sicurezza (&deg;C) <input name="t_max" type="number" step="0.5" min="26" max="95"><small>Sopra questo valore spegne sempre, anche durante il ritardo e l'acceso forzato.</small></label>
 </fieldset>
 <fieldset><legend>Sensore</legend>
  <label>Sorgente <select name="src"><option value="0">Infrarossi MLX90614 (senza contatto)</option><option value="1">Sonda DS18B20 a filo</option><option value="2">Nodo a batteria (ESP-NOW)</option></select></label>
  <label class="ir">Emissivit&agrave; efficace <input name="ir_eps" type="number" step="0.01" min="0.2" max="1"><small>Si imposta da sola con la calibrazione qui sotto.</small></label>
  <label>Correzione (&deg;C) <input name="offset" type="number" step="0.1" min="-20" max="20"></label>
  <label>Conferma soglia (s) <input name="confirm_s" type="number" min="0" max="600"><small>Per quanto tempo la soglia deve restare superata prima di agire (filtra i disturbi).</small></label>
  <label>Se il sensore si guasta <select name="fault"><option value="0">Lascia comandare il termostato</option><option value="1">Blocca il riscaldamento</option></select></label>
  <label>Guasto dopo (s), sensore locale <input name="to_loc" type="number" min="10" max="3600"></label>
  <label class="rem">Guasto dopo (s), nodo a batteria <input name="to_rem" type="number" min="60" max="3600"></label>
  <label class="rem">Misura del nodo vicino alle soglie (s) <input name="n_fast" type="number" min="10" max="600"></label>
  <label class="rem">Misura del nodo a riposo (s) <input name="n_slow" type="number" min="10" max="1800"></label>
 </fieldset>
 <fieldset><legend>Attuatore</legend>
  <label>Tipo <select name="act"><option value="0">Rel&egrave; NC in serie al termostato (consigliato)</option><option value="1">Rel&egrave; NO in serie al termostato</option><option value="2">Servo a due posizioni</option><option value="3">Servo che preme un pulsante</option></select></label>
  <label class="rly">Modulo rel&egrave; attivo basso <input name="rly_low" type="checkbox"></label>
  <label class="svpos">Angolo "consenti" <input name="sv_allow" type="number" min="0" max="180"></label>
  <label class="svpos">Angolo "blocca" <input name="sv_block" type="number" min="0" max="180"></label>
  <label class="svpr">Angolo di riposo <input name="sv_rest" type="number" min="0" max="180"></label>
  <label class="svpr">Angolo di pressione <input name="sv_press" type="number" min="0" max="180"></label>
  <label class="svpr">Durata pressione (ms) <input name="sv_ms" type="number" min="100" max="3000"></label>
 </fieldset>
 <div class="btns"><button type="submit">Salva</button><span id="cfgMsg" class="ink2"></span></div>
 </form>
</section>

<section class="card" id="calCard">
 <h2>Calibrazione infrarossi</h2>
 <p class="ink2" style="margin-top:0">Con il termosifone caldo e stabile misura la sua temperatura con un termometro a contatto e scrivila qui: l'emissivit&agrave; viene calcolata e salvata.</p>
 <div class="btns"><input type="number" id="realT" step="0.1" placeholder="es. 52.0" aria-label="temperatura reale"><button id="bCal">Calibra</button><span id="calMsg" class="ink2"></span></div>
</section>

<section class="card">
 <h2>Wi-Fi e accesso</h2>
 <form id="wifi">
  <label>Rete (SSID) <input name="ssid" maxlength="32" autocomplete="off"></label>
  <label>Password Wi-Fi <input name="pass" type="password" maxlength="64" autocomplete="new-password"></label>
  <label>Nome del dispositivo <input name="host" maxlength="31"><small>La pagina sar&agrave; anche su http://nome.local</small></label>
  <label>Password della pagina (utente admin) <input name="webpass" type="password" maxlength="32" autocomplete="new-password"><small>Vuota = lascia quella attuale. Protegge anche l'aggiornamento firmware.</small></label>
  <label>Togli la password della pagina <input name="webpass_clear" type="checkbox"></label>
  <div class="btns"><button type="submit">Salva e riavvia</button><span id="wifiMsg" class="ink2"></span></div>
 </form>
</section>

<section class="card">
 <h2>Eventi</h2><ol id="events"></ol>
</section>
</main>
<footer class="muted"><span id="foot"></span> &middot; <a href="/update">Aggiorna firmware</a> &middot; <a href="#" id="bReboot">Riavvia</a> &middot; <a href="#" id="bFactory">Ripristino di fabbrica</a></footer>
<script>
const $=id=>document.getElementById(id);
let cfg=null,st=null,hist=null,hover=-1;
async function api(url,data){
 const r=await fetch(url,data?{method:'POST',body:new URLSearchParams(data)}:{});
 const txt=await r.text(); let j={};
 try{j=JSON.parse(txt)}catch(e){j={error:txt}}
 if(!r.ok)throw new Error(j.error||r.statusText);
 return j;
}
function dur(s){s=Math.max(0,Math.round(s));if(s<60)return s+' s';const m=Math.floor(s/60);if(m<60)return m+' min';const h=Math.floor(m/60);return h+' h'+(m%60?' '+(m%60)+' min':'');}
function num(v,d){return v===null||v===undefined?'--':v.toFixed(d);}
function setText(id,t){$(id).textContent=t;}

async function refresh(){
 try{st=await api('/api/status');setText('conn',st.wifi.ok?'Wi-Fi '+st.wifi.ssid+' ('+st.wifi.rssi+' dBm)':(st.wifi.ap?'Rete di configurazione '+st.wifi.apSsid:'Wi-Fi non connesso'));}
 catch(e){setText('conn','Dispositivo non raggiungibile');return;}
 setText('temp',st.tv?num(st.t,1):'--');
 let src=st.srcName;
 if(st.src===0&&st.amb!==null)src+=' · sensore a '+num(st.amb,1)+' °C, lettura grezza '+num(st.raw,1)+' °C';
 if(st.src===2)src+=' · nodi attivi: '+st.nodes.filter(n=>n.age<=st.toRem).length;
 setText('src',src);
 $('outDot').className='dot '+(st.fault?'warn':(st.allowed?'good':''));
 setText('out',st.allowed?'Riscaldamento consentito':'Riscaldamento bloccato');
 setText('mode','Modalità: '+st.modeName);
 setText('state',st.mode===0?st.stateName+' da '+dur(st.inState):'');
 let tm='';
 const below='riaccende sotto '+st.tOn+' °C';
 if(st.mode===3&&st.rem>0)tm='torna in automatico tra '+dur(st.rem);
 else if(st.mode===0&&st.state===0)tm='spegne a '+st.tOff+' °C';
 else if(st.mode===0&&st.state===1)tm='spegne tra '+dur(st.rem);
 else if(st.mode===0&&st.state===2){
  if(st.restart===0)tm=below;
  else if(st.restart===1)tm='riaccende tra '+dur(st.rem);
  else if(st.restart===2)tm='riaccende tra '+dur(st.rem)+' o '+below;
  else if(st.restart===3)tm=st.rem>0?'pausa minima: ancora '+dur(st.rem)+', poi '+below:below;
 }
 setText('timer',tm);
 setText('reason','Ultimo evento: '+st.reason);
 const err=st.fault?'⚠ Sensore guasto o assente: '+(st.err||'nessuna lettura')+'. '+(st.allowed?'Comanda il termostato.':'Riscaldamento bloccato per sicurezza.'):(st.err?'⚠ '+st.err:'');
 $('err').hidden=!err;setText('err',err);
 const cur={0:'bAuto',1:'bBypass',2:'bOff',3:'bOn'}[st.mode];
 for(const id of['bAuto','bBypass','bOff','bOn'])$(id).classList.toggle('on',id===cur);
 $('bResume').disabled=!(st.mode===0&&(st.state===2||st.state===3));
 $('pressSync').hidden=st.act!==3;
 $('calCard').hidden=st.src!==0;
 renderNodes();
 setText('foot','Firmware '+st.fw+' · acceso da '+dur(st.up)+(st.wifi.ok?' · IP '+st.wifi.ip+' · canale '+st.wifi.ch:'')+(st.wifi.ap?' · rete di configurazione '+st.wifi.apSsid+' ('+st.wifi.apIp+')':''));
}

function renderNodes(){
 $('nodesCard').hidden=!(st.nodes.length||st.src===2);
 const box=$('nodes');box.textContent='';
 if(!st.nodes.length){box.textContent='Nessun nodo ricevuto finora.';box.className='muted';return;}
 box.className='scroll';
 const t=document.createElement('table');
 t.innerHTML='<tr><th>Nodo</th><th>Temperatura</th><th>Batteria</th><th>Segnale</th><th>Ultimo dato</th></tr>';
 for(const n of st.nodes){
  const r=t.insertRow();
  const cells=[n.mac.slice(-8),n.tv?num(n.t,2)+' °C':'errore sensore',(n.batt?(n.batt/1000).toFixed(2)+' V':'--')+(n.low?' ⚠ bassa':''),n.rssi+' dBm',dur(n.age)+' fa'];
  for(const c of cells)r.insertCell().textContent=c;
 }
 box.appendChild(t);
}

async function loadCfg(){
 cfg=await api('/api/config');
 const f=$('cfg');
 for(const el of f.elements){if(!el.name||!(el.name in cfg))continue;if(el.type==='checkbox')el.checked=!!cfg[el.name];else el.value=cfg[el.name];}
 const w=$('wifi');w.ssid.value=cfg.ssid;w.host.value=cfg.host;
 w.pass.placeholder=cfg.passSet?'(invariata)':'';w.webpass.placeholder=cfg.webpassSet?'(impostata)':'(nessuna)';
 showFields();
}
function showFields(){
 const f=$('cfg'),src=+f.src.value,act=+f.act.value;
 const vis=(cls,on)=>document.querySelectorAll('#cfg .'+cls).forEach(e=>e.hidden=!on);
 vis('ir',src===0);vis('rem',src===2);vis('rly',act<2);vis('svpos',act===2);vis('svpr',act===3);
}
function formData(f){const d={};for(const el of f.elements){if(!el.name)continue;d[el.name]=el.type==='checkbox'?(el.checked?'1':'0'):el.value;}return d;}

$('cfg').addEventListener('change',showFields);
$('cfg').addEventListener('submit',async e=>{
 e.preventDefault();setText('cfgMsg','Salvataggio...');
 try{await api('/api/config',formData(e.target));await loadCfg();setText('cfgMsg','Salvato.');refresh();loadHist();}
 catch(err){setText('cfgMsg','Errore: '+err.message);}
});
$('wifi').addEventListener('submit',async e=>{
 e.preventDefault();
 if(!confirm('Salvare e riavviare? Se cambi rete, la pagina sarà su un nuovo indirizzo.'))return;
 try{await api('/api/wifi',formData(e.target));setText('wifiMsg','Riavvio in corso...');}
 catch(err){setText('wifiMsg','Errore: '+err.message);}
});
document.querySelectorAll('[data-cmd]').forEach(b=>b.addEventListener('click',async()=>{
 const d={c:b.dataset.cmd};if(d.c==='forceon')d.min=$('onMin').value;
 try{await api('/api/cmd',d);}catch(err){alert(err.message);}
 refresh();loadEvents();
}));
$('bCal').addEventListener('click',async()=>{
 try{const j=await api('/api/cmd',{c:'calibrate',real:$('realT').value});setText('calMsg','Emissività impostata a '+j.eps.toFixed(3));loadCfg();}
 catch(err){setText('calMsg','Errore: '+err.message);}
});
$('bReboot').addEventListener('click',async e=>{e.preventDefault();if(confirm('Riavviare il dispositivo?'))await api('/api/reboot',{}).catch(()=>{});});
$('bFactory').addEventListener('click',async e=>{e.preventDefault();if(confirm('Cancellare TUTTE le impostazioni, Wi-Fi compreso?'))await api('/api/factory',{}).catch(()=>{});});

async function loadEvents(){
 try{const ev=await api('/api/events');const ol=$('events');ol.textContent='';
  for(const e of ev){const li=document.createElement('li');li.textContent=dur(e.ago)+' fa: '+e.text;ol.appendChild(li);}}
 catch(e){}
}

const SVGNS='http://www.w3.org/2000/svg';
function el(tag,attrs,parent){const e=document.createElementNS(SVGNS,tag);for(const k in attrs)e.setAttribute(k,attrs[k]);if(parent)parent.appendChild(e);return e;}
function agoLabel(s){if(s<=0)return 'ora';return '-'+(s>=3600?Math.round(s/3600)+' h':Math.round(s/60)+' min');}

async function loadHist(){try{hist=await api('/api/history');drawChart();drawTable();}catch(e){}}

function drawChart(){
 const box=$('chart');box.textContent='';
 if(!hist||hist.t.length<2){const p=document.createElement('p');p.className='muted';p.textContent='Il grafico compare dopo qualche minuto di funzionamento.';box.appendChild(p);return;}
 const W=Math.max(260,box.clientWidth),H=230,m={l:34,r:96,t:12,b:26};
 const n=hist.t.length,step=hist.step,pw=W-m.l-m.r,ph=H-m.t-m.b;
 let lo=Infinity,hi=-Infinity;
 for(const v of hist.t)if(v!==null){lo=Math.min(lo,v/10);hi=Math.max(hi,v/10);}
 const tOff=cfg?cfg.t_off:null,tOn=cfg?cfg.t_on:null;
 if(tOff!==null){lo=Math.min(lo,tOn);hi=Math.max(hi,tOff);}
 if(!isFinite(lo)){lo=10;hi=60;}
 lo=Math.floor((lo-2)/10)*10;hi=Math.ceil((hi+2)/10)*10;
 const x=i=>m.l+i/(n-1)*pw,y=v=>m.t+(1-(v-lo)/(hi-lo))*ph;
 const svg=el('svg',{width:W,height:H,viewBox:'0 0 '+W+' '+H,tabindex:0,role:'img','aria-label':'Grafico della temperatura del termosifone nelle ultime 24 ore'},box);
 for(let v=lo;v<=hi;v+=10){el('line',{x1:m.l,x2:m.l+pw,y1:y(v),y2:y(v),stroke:v===lo?'var(--axis)':'var(--grid)','stroke-width':1},svg);
  el('text',{x:m.l-6,y:y(v)+4,'text-anchor':'end',class:'ax'},svg).textContent=v;}
 // fasce in cui il riscaldamento era bloccato
 let s=-1;
 for(let i=0;i<=n;i++){const blocked=i<n&&!(hist.f[i]&1);
  if(blocked&&s<0)s=i;
  if(!blocked&&s>=0){el('rect',{x:x(Math.max(0,s-0.5)),y:m.t,width:Math.max(1,x(Math.min(n-1,i-0.5))-x(Math.max(0,s-0.5))),height:ph,fill:'var(--wash)'},svg);s=-1;}}
 // soglie con etichetta diretta
 if(tOff!==null){const yOff=y(tOff),yOn=Math.max(y(tOn),yOff+12);
  for(const [v,ly,txt] of [[tOff,yOff,'spegne '+tOff+'°'],[tOn,yOn,'riaccende '+tOn+'°']]){
  el('line',{x1:m.l,x2:m.l+pw,y1:y(v),y2:y(v),stroke:'var(--ink2)','stroke-width':1,opacity:.55},svg);
  el('text',{x:m.l+pw+6,y:ly+4,class:'thr'},svg).textContent=txt;}}
 // linea della temperatura (interrotta dove mancano i dati)
 let d='',pen=false;
 for(let i=0;i<n;i++){const v=hist.t[i];if(v===null){pen=false;continue;}d+=(pen?'L':'M')+x(i).toFixed(1)+' '+y(v/10).toFixed(1);pen=true;}
 el('path',{d:d,fill:'none',stroke:'var(--line)','stroke-width':2,'stroke-linejoin':'round','stroke-linecap':'round'},svg);
 // asse dei tempi
 const span=(n-1)*step,tick=span>=12*3600?6*3600:span>=4*3600?3600:span>=3600?900:300;
 for(let a=0;a<=span;a+=tick){const i=n-1-a/step;el('text',{x:x(i),y:H-8,'text-anchor':a===0?'end':'middle',class:'ax'},svg).textContent=agoLabel(a);}
 const lastI=(()=>{for(let i=n-1;i>=0;i--)if(hist.t[i]!==null)return i;return -1;})();
 if(lastI>=0)el('circle',{cx:x(lastI),cy:y(hist.t[lastI]/10),r:4,fill:'var(--line)',stroke:'var(--card)','stroke-width':2},svg);
 // mirino e tooltip
 const cross=el('line',{y1:m.t,y2:m.t+ph,stroke:'var(--ink2)','stroke-width':1,visibility:'hidden'},svg);
 const mark=el('circle',{r:4,fill:'var(--line)',stroke:'var(--card)','stroke-width':2,visibility:'hidden'},svg);
 const tip=document.createElement('div');tip.className='tip';tip.hidden=true;box.appendChild(tip);
 function show(i){
  hover=i=Math.max(0,Math.min(n-1,i));const v=hist.t[i],cx=x(i);
  cross.setAttribute('x1',cx);cross.setAttribute('x2',cx);cross.setAttribute('visibility','visible');
  if(v!==null){mark.setAttribute('cx',cx);mark.setAttribute('cy',y(v/10));mark.setAttribute('visibility','visible');}else mark.setAttribute('visibility','hidden');
  tip.textContent='';const b=document.createElement('b');b.textContent=v===null?'nessun dato':(v/10).toFixed(1)+' °C';tip.appendChild(b);
  tip.appendChild(document.createTextNode((i===n-1?'adesso':dur((n-1-i)*step)+' fa')+' · '+((hist.f[i]&1)?'consentito':'bloccato')));
  tip.hidden=false;const tw=tip.offsetWidth;tip.style.left=Math.min(Math.max(0,cx-tw/2),W-tw)+'px';tip.style.top='0px';
 }
 function hide(){hover=-1;cross.setAttribute('visibility','hidden');mark.setAttribute('visibility','hidden');tip.hidden=true;}
 svg.addEventListener('pointermove',e=>{const r=svg.getBoundingClientRect();show(Math.round((e.clientX-r.left-m.l)/pw*(n-1)));});
 svg.addEventListener('pointerleave',hide);svg.addEventListener('blur',hide);
 svg.addEventListener('focus',()=>show(n-1));
 svg.addEventListener('keydown',e=>{if(e.key==='ArrowLeft'||e.key==='ArrowRight'){e.preventDefault();show((hover<0?n-1:hover)+(e.key==='ArrowLeft'?-5:5));}});
}

function drawTable(){
 const box=$('table');box.textContent='';box.className='scroll';
 if(!hist||!hist.t.length)return;
 const per=Math.round(3600/hist.step),n=hist.t.length,t=document.createElement('table');
 t.innerHTML='<tr><th>Ora</th><th>Min</th><th>Media</th><th>Max</th><th>Bloccato</th></tr>';
 for(let end=n;end>0;end-=per){
  let lo=Infinity,hi=-Infinity,sum=0,c=0,bl=0;const start=Math.max(0,end-per);
  for(let i=start;i<end;i++){const v=hist.t[i];if(!(hist.f[i]&1))bl++;if(v===null)continue;lo=Math.min(lo,v);hi=Math.max(hi,v);sum+=v;c++;}
  const r=t.insertRow(),a=(n-end)*hist.step,b=(n-start)*hist.step;
  const cells=[(a?'-'+dur(a):'ora')+' … -'+dur(b),c?(lo/10).toFixed(1):'--',c?(sum/c/10).toFixed(1):'--',c?(hi/10).toFixed(1):'--',Math.round(bl*hist.step/60)+' min'];
  for(const x of cells)r.insertCell().textContent=x;
 }
 box.appendChild(t);
}

window.addEventListener('resize',()=>{if(hist)drawChart();});
loadCfg().then(()=>{refresh();loadHist();loadEvents();}).catch(()=>refresh());
setInterval(refresh,3000);setInterval(loadEvents,10000);setInterval(loadHist,60000);
</script></body></html>)HTML";

static const char UPDATE_HTML[] PROGMEM = R"HTML(<!doctype html>
<html lang="it"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Aggiorna firmware</title>
<style>:root{color-scheme:light dark}body{margin:0 auto;max-width:560px;padding:16px;font:15px/1.45 system-ui,-apple-system,"Segoe UI",sans-serif}
button,input{font:inherit}progress{width:100%;height:12px}</style></head><body>
<h1 style="font-size:18px">Aggiorna firmware</h1>
<p>Scegli il file <code>firmware.bin</code> generato da PlatformIO (<code>.pio/build/&lt;ambiente&gt;/firmware.bin</code>) o da Arduino IDE (Sketch &rarr; Esporta binario compilato).</p>
<input type="file" id="f" accept=".bin"> <button id="go">Carica</button>
<p><progress id="p" max="100" value="0" hidden></progress></p><p id="msg"></p>
<p><a href="/">&larr; Torna al controllo</a></p>
<script>
document.getElementById('go').onclick=()=>{
 const f=document.getElementById('f').files[0],msg=document.getElementById('msg'),p=document.getElementById('p');
 if(!f){msg.textContent='Scegli prima un file.';return;}
 const fd=new FormData();fd.append('firmware',f,f.name);
 const x=new XMLHttpRequest();x.open('POST','/update');p.hidden=false;
 x.upload.onprogress=e=>{if(e.lengthComputable)p.value=Math.round(e.loaded*100/e.total);};
 x.onload=()=>{msg.textContent=x.responseText;if(x.status===200)setTimeout(()=>location.href='/',8000);};
 x.onerror=()=>{msg.textContent='Errore di rete durante il caricamento.';};
 msg.textContent='Caricamento... non spegnere il dispositivo.';x.send(fd);
};
</script></body></html>)HTML";
