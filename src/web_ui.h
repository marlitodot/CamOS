#pragma once
// Web-Oberflaeche (Single Page), wird unter "/" ausgeliefert.
static const char INDEX_HTML[] = R"HTML(<!DOCTYPE html>
<html lang="de"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>CamOS</title>
<style>
:root{--bg:#0f1115;--card:#181b22;--line:#2a2f3a;--txt:#e6e8ec;--mut:#8b93a3;--acc:#4f8cff;--ok:#2ecc71;--bad:#e74c3c}
*{box-sizing:border-box}body{margin:0;font:14px/1.45 system-ui,Segoe UI,Roboto,sans-serif;background:var(--bg);color:var(--txt)}
header{display:flex;align-items:center;gap:8px;padding:10px 16px;background:var(--card);border-bottom:1px solid var(--line);flex-wrap:wrap;position:sticky;top:0;z-index:5}
header b{font-size:17px;margin-right:6px}
.pill{padding:2px 9px;border-radius:10px;background:#252a34;color:var(--mut);font-size:12px}
.pill.on{background:#14361f;color:var(--ok)}.pill.alarm{background:#4a1a16;color:#ff8577}
nav{display:flex;gap:4px;overflow-x:auto;padding:8px 16px;border-bottom:1px solid var(--line);background:var(--bg)}
nav button{background:none;border:0;color:var(--mut);padding:8px 12px;border-radius:8px;cursor:pointer;font:inherit;white-space:nowrap}
nav button.act{background:var(--acc);color:#fff}
main{padding:16px;max-width:1150px;margin:auto}
section{display:none}section.act{display:block}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:14px;margin-bottom:14px}
.card h3{margin:0 0 10px;font-size:15px}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:14px}
.split{display:grid;grid-template-columns:2fr 1fr;gap:14px}@media(max-width:800px){.split{grid-template-columns:1fr}}
.row{display:flex;align-items:center;gap:10px;margin:8px 0;flex-wrap:wrap}
.row>label:first-child{min-width:140px;color:var(--mut)}
input,select,textarea{background:#0d0f13;color:var(--txt);border:1px solid var(--line);border-radius:8px;padding:7px 9px;font:inherit;max-width:100%}
input[type=range]{flex:1;min-width:120px;padding:0;accent-color:var(--acc)}input[type=checkbox]{accent-color:var(--acc);width:18px;height:18px}
input[type=text],input[type=password]{flex:1;min-width:160px}
textarea{width:100%;font:13px/1.4 ui-monospace,Consolas,monospace}
output{min-width:38px;text-align:right;color:var(--mut)}
.btn{background:#252a34;color:var(--txt);border:1px solid var(--line);border-radius:8px;padding:7px 12px;cursor:pointer;font:inherit}
.btn:hover{border-color:var(--acc)}.btn.p{background:var(--acc);border-color:var(--acc);color:#fff}.btn.d{background:#5a1f1a;border-color:#7a2a22}
.view{background:#000;border-radius:10px;overflow:hidden;aspect-ratio:4/3;display:flex;align-items:center;justify-content:center;color:var(--mut)}
.view img{width:100%;height:100%;object-fit:contain}
#log{height:58vh;overflow:auto;background:#0b0d10;border:1px solid var(--line);border-radius:8px;padding:8px;font:12px/1.45 ui-monospace,Consolas,monospace;white-space:pre-wrap;margin:0 0 8px}
.bar{height:12px;background:#0d0f13;border-radius:6px;overflow:hidden;flex:1;position:relative}.bar i{display:block;height:100%;background:var(--acc);width:0;transition:width .3s}
table{width:100%;border-collapse:collapse}td,th{padding:6px;border-bottom:1px solid var(--line);text-align:left;vertical-align:top}
.mut{color:var(--mut)}code{background:#0d0f13;padding:1px 5px;border-radius:4px;font-size:12px}
.kv{display:grid;grid-template-columns:auto 1fr;gap:3px 14px}.kv span:nth-child(odd){color:var(--mut)}
#toast{position:fixed;bottom:16px;right:16px;background:var(--card);border:1px solid var(--acc);padding:10px 14px;border-radius:10px;display:none;max-width:80vw;word-break:break-all;z-index:9}
canvas{width:100%;height:120px;background:#0b0d10;border-radius:8px}
a.btn{text-decoration:none;display:inline-block}.ex{cursor:pointer}.ex:hover{background:#20242d}
</style></head><body>
<header><b>&#128247; CamOS</b><span id="hdev" class="pill">-</span><span id="hwifi" class="pill">WLAN</span><span id="hmot" class="pill">Bewegung</span><span id="hflash" class="pill">Blitz</span><span id="htime" class="pill">-</span></header>
<nav id="nav">
<button data-t="live" class="act">Live</button><button data-t="cam">Kamera</button><button data-t="mot">Bewegung</button><button data-t="rules">Regeln</button><button data-t="con">Konsole</button><button data-t="files">Dateien</button><button data-t="plug">Plugins</button><button data-t="sys">System</button>
</nav>
<main>

<section id="live" class="act"><div class="split">
<div class="card"><div class="view" id="view">Stream gestoppt</div>
<div class="row"><button class="btn p" id="bstream" onclick="toggleStream()">Stream starten</button>
<button class="btn" onclick="window.open('/capture?_='+Date.now())">Foto</button>
<button class="btn" onclick="window.open('/capture?flash=1&_='+Date.now())">Foto mit Blitz</button>
<button class="btn" onclick="cmd('Photo')" title="Mit SD-Karte: speichern auf SD. Ohne: Download auf den PC">Foto &rarr; SD / PC</button></div>
<div class="row"><label style="min-width:0"><input type="checkbox" id="autodl"> Fotos ohne SD-Karte automatisch auf den PC laden</label><button class="btn" onclick="dlPhoto()">Letztes RAM-Foto laden</button></div>
<p class="mut" style="margin:0">Stream-URL: <code id="surl"></code> &middot; Foto: <code>/capture</code></p></div>
<div>
<div class="card"><h3>Blitz</h3>
<div class="row"><input type="range" min="0" max="255" id="flr" data-k="flash.level" data-c="Flash"><output></output></div>
<div class="row"><button class="btn" onclick="cmd('Flash 255',1).then(status)">An</button><button class="btn" onclick="cmd('Flash 0',1).then(status)">Aus</button><button class="btn" onclick="cmd('Flash 40',1).then(status)">Dimm</button><button class="btn" onclick="cmd('FlashPulse 300',1)">Puls</button></div>
<div class="row"><label>Blitz bei Fotos</label><input type="checkbox" data-c="SnapFlash" data-k="flash.snap"></div>
<div class="row"><label>Foto-Blitz Stärke</label><input type="range" min="0" max="255" data-c="SnapLevel" data-k="flash.snaplevel"><output></output></div>
</div>
<div class="card"><h3>Status</h3><div class="kv" id="kvlive"></div></div>
</div></div></section>

<section id="cam"><div class="grid">
<div class="card"><h3>Bild</h3>
<div class="row"><label>Auflösung</label><select data-c="FrameSize" data-k="cam.framesize">
<option value="QQVGA">QQVGA 160x120</option><option value="QVGA">QVGA 320x240</option><option value="CIF">CIF 400x296</option><option value="HVGA">HVGA 480x320</option><option value="VGA">VGA 640x480</option><option value="SVGA">SVGA 800x600</option><option value="XGA">XGA 1024x768</option><option value="HD">HD 1280x720</option><option value="SXGA">SXGA 1280x1024</option><option value="UXGA">UXGA 1600x1200</option><option value="FHD">FHD 1920x1080</option><option value="QXGA">QXGA 2048x1536 (3 MP)</option><option value="QSXGA">QSXGA 2560x1920 (5 MP)</option></select></div>
<div class="row"><label>Sensor</label><span id="camsensor">-</span></div>
<div class="row"><label>JPEG Qualität (klein=besser)</label><input type="range" min="4" max="63" data-c="Quality" data-k="cam.quality"><output></output></div>
<div class="row"><label>Helligkeit</label><input type="range" min="-2" max="2" data-c="Brightness" data-k="cam.brightness"><output></output></div>
<div class="row"><label>Kontrast</label><input type="range" min="-2" max="2" data-c="Contrast" data-k="cam.contrast"><output></output></div>
<div class="row"><label>Sättigung</label><input type="range" min="-2" max="2" data-c="Saturation" data-k="cam.saturation"><output></output></div>
</div>
<div class="card"><h3>Ausrichtung &amp; Effekte</h3>
<div class="row"><label>Vertikal spiegeln</label><input type="checkbox" data-c="VFlip" data-k="cam.vflip"></div>
<div class="row"><label>Horizontal spiegeln</label><input type="checkbox" data-c="HMirror" data-k="cam.hmirror"></div>
<div class="row"><label>Auto-Weissabgleich</label><input type="checkbox" data-c="AWB" data-k="cam.awb"></div>
<div class="row"><label>Effekt</label><select data-c="Effect" data-k="cam.effect"><option value="0">Kein</option><option value="1">Negativ</option><option value="2">Graustufen</option><option value="3">Rot</option><option value="4">Grün</option><option value="5">Blau</option><option value="6">Sepia</option></select></div>
<p class="mut">Unterstützt: OV2640 (max. UXGA), OV3660 (max. QXGA 3 MP), OV5640 (max. QSXGA 5 MP). Wird automatisch erkannt. Ohne PSRAM max. SVGA. Hohe Auflösung = weniger FPS.</p>
</div></div></section>

<section id="mot"><div class="grid">
<div class="card"><h3>Bewegungserkennung</h3>
<div class="row"><label>Aktiv</label><input type="checkbox" data-c="Motion" data-k="motion.enabled"></div>
<div class="row"><label>Pixel-Schwelle</label><input type="range" min="1" max="100" data-c="MotionThreshold" data-k="motion.threshold"><output></output></div>
<div class="row"><label>Auslösen ab % Pixel</label><input type="range" min="0.1" max="30" step="0.1" data-c="MotionSensitivity" data-k="motion.sensitivity"><output></output></div>
<div class="row"><label>Prüf-Intervall ms</label><input type="range" min="100" max="3000" step="50" data-c="MotionInterval" data-k="motion.interval"><output></output></div>
<div class="row"><label>Nachlaufzeit s</label><input type="range" min="1" max="120" data-c="MotionCooldown" data-k="motion.cooldown"><output></output></div>
<div class="row"><label>Foto auf SD bei Bewegung</label><input type="checkbox" data-c="MotionSave" data-k="motion.save"></div>
</div>
<div class="card"><h3>Live</h3>
<div class="row"><label>Aktuell</label><div class="bar"><i id="mbar"></i></div><output id="mlvl"></output></div>
<canvas id="mgraph" width="600" height="120"></canvas>
<div class="kv" style="margin-top:10px"><span>Zustand</span><span id="mstate">-</span><span>Erkennungen</span><span id="mcount">-</span></div>
<p class="mut">Events: <code>Motion#Detected</code> (Wert = %) und <code>Motion#End</code>. Reaktionen im Tab <b>Regeln</b> festlegen.</p>
</div></div></section>

<section id="rules">
<div id="rulebox"></div>
<div class="grid">
<div class="card"><h3>Beispiele (klicken = in Rule1 übernehmen)</h3><table id="extab"></table></div>
<div class="card"><h3>Variablen &amp; Timer</h3><table id="vartab"></table>
<p class="mut">Syntax: <code>ON &lt;Event&gt;[=|&gt;|&lt;|!=wert] DO &lt;Befehl&gt; ENDON</code><br>
Events: System#Boot, Motion#Detected, Motion#End, Wifi#Connected, Wifi#Disconnected, Time#Initialized, Time#Minute (Minute des Tages), Rules#Timer=n, Flash#State, SD#Saved, Event#&lt;name&gt;, PIR#State, Gpio&lt;n&gt;#State, WebSend#Code ...<br>
Platzhalter: %value% %var1% %mem1% %time% %date% %timestamp% %uptime% %motion% %motioncount% %flash% %ip% %device%<br>
Mehrere Befehle: <code>Backlog Flash 255; Delay 20; Flash 0</code> (Delay in 0.1 s)</p></div>
</div></section>

<section id="con"><div class="card"><h3>Konsole</h3>
<pre id="log"></pre>
<div class="row"><input type="text" id="cin" placeholder="Befehl eingeben, z.B. Help, Status, Flash 100  (Pfeiltasten = Verlauf)" autocomplete="off" spellcheck="false">
<button class="btn p" onclick="sendCin()">Senden</button><button class="btn" onclick="$('#log').textContent=''">Leeren</button></div>
<div class="row"><button class="btn" onclick="runC('Help')">Help</button><button class="btn" onclick="runC('Status')">Status</button><button class="btn" onclick="runC('Plugins')">Plugins</button><button class="btn" onclick="runC('WifiScan')">WifiScan</button><button class="btn" onclick="runC('Time')">Time</button></div>
</div></section>

<section id="files"><div class="card" id="fcard"><h3>SD-Karte</h3>
<div class="row"><div class="bar" style="height:16px"><i id="sdbar"></i></div><span class="mut" id="sdinfo"></span></div>
<div class="row" id="crumb"></div>
<div class="row"><button class="btn" onclick="loadFiles()">Aktualisieren</button><button class="btn" onclick="mkDir()">Neuer Ordner</button>
<label class="btn p" style="min-width:0;color:#fff">Dateien hochladen<input type="file" multiple id="upf" style="display:none" onchange="upload(this.files)"></label>
<button class="btn" onclick="cmd('Photo').then(()=>setTimeout(loadFiles,1000))">Foto aufnehmen</button><button class="btn" onclick="cmd('SDInit').then(()=>loadFiles())">SD neu einbinden</button></div>
<div class="row" id="uprow" style="display:none"><progress id="upp" max="100" value="0" style="flex:1"></progress><span id="upinfo" class="mut"></span></div>
<p class="mut" style="margin:4px 0">Tipp: Dateien einfach hierher ziehen (Drag &amp; Drop).</p>
<table id="ftab"></table></div></section>

<section id="plug"><div class="card"><h3>Plugins</h3><table id="ptab"></table>
<p class="mut">Eigene Plugins: neue .cpp Datei in <code>src/plugins/</code> anlegen &ndash; Anleitung in <code>PLUGINS_AI.md</code> (für KI optimiert).</p></div></section>

<section id="sys"><div class="grid">
<div class="card"><h3>WLAN (optional)</h3>
<div class="row"><label>SSID</label><input type="text" id="wssid" list="wlist"><datalist id="wlist"></datalist><button class="btn" onclick="scan()">Suchen</button></div>
<div class="row"><label>Passwort</label><input type="password" id="wpass" placeholder="leer = unverändert"></div>
<div class="row"><button class="btn p" onclick="saveWifi()">Speichern &amp; verbinden</button><button class="btn" onclick="clearWifi()">WLAN entfernen</button></div>
<div class="kv" id="kvwifi"></div></div>
<div class="card"><h3>Access Point</h3>
<div class="row"><label>AP SSID</label><input type="text" id="apssid"></div>
<div class="row"><label>AP Passwort</label><input type="password" id="appass" placeholder="leer = unverändert, min. 8"></div>
<div class="row"><label>AP Modus</label><select id="apmode"><option value="0">Immer an</option><option value="1">Nur wenn kein WLAN</option></select></div>
<div class="row"><button class="btn p" onclick="saveAP()">Speichern &amp; anwenden</button></div></div>
<div class="card"><h3>Allgemein</h3>
<div class="row"><label>Hostname</label><input type="text" id="host"></div>
<div class="row"><label>Zeitzone</label><input type="text" id="tz" list="tzl"><datalist id="tzl"><option value="CET-1CEST,M3.5.0,M10.5.0/3">Mitteleuropa</option><option value="GMT0BST,M3.5.0/1,M10.5.0">London</option><option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Osteuropa</option><option value="EST5EDT,M3.2.0,M11.1.0">New York</option><option value="UTC0">UTC</option></datalist></div>
<div class="row"><label>NTP Server</label><input type="text" id="ntp"></div>
<div class="row"><label>Web-Passwort</label><input type="password" id="wpw" placeholder="Benutzer admin; leer lassen = unverändert"></div>
<div class="row"><button class="btn p" onclick="saveGen()">Speichern</button><button class="btn" onclick="cmd('WebPassword &quot;&quot;')">Passwort entfernen</button></div>
<p class="mut">Hostname wirkt nach Neustart.</p></div>
<div class="card"><h3>Taste</h3>
<div class="row"><label>Tasten-Pin</label><select data-c="ButtonPin" data-k="button.pin"><option value="-1">Aus</option><option value="0">IO0-Taste (GPIO0)</option><option value="12">GPIO12</option><option value="13">GPIO13</option><option value="14">GPIO14</option><option value="15">GPIO15</option></select></div>
<div class="row"><label>Kurz drücken = Foto</label><input type="checkbox" data-c="ButtonPhoto" data-k="button.photo"></div>
<p class="mut">Mit SD-Karte wird das Foto auf der SD gespeichert. Ohne SD wird es auf den PC heruntergeladen, wenn diese Seite offen ist. Events für Regeln: <code>Button#Short</code> und <code>Button#Long</code> (&gt;1,5 s).</p></div>
<div class="card"><h3>Firmware-Update (OTA)</h3>
<p class="mut">Datei <code>CamOS-esp32cam-ota.bin</code> wählen.</p>
<div class="row"><input type="file" id="otaf" accept=".bin"><button class="btn p" onclick="ota()">Hochladen</button></div>
<progress id="otap" max="100" value="0" style="width:100%"></progress></div>
<div class="card"><h3>Gerät</h3><div class="kv" id="kvsys"></div>
<div class="row"><button class="btn" onclick="if(confirm('Neu starten?'))cmd('Restart 1')">Neustart</button>
<button class="btn d" onclick="if(confirm('ALLE Einstellungen, Regeln und WLAN löschen?'))cmd('Reset 1')">Werkseinstellungen</button></div></div>
</div></section>
</main>
<div id="toast"></div>
<script>
const $=s=>document.querySelector(s),$$=s=>[...document.querySelectorAll(s)];
let st={},streaming=false,logFrom=0,hist=[],hi=0,lv=[],first=true;
const esc=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
function toast(t){const e=$('#toast');e.textContent=t;e.style.display='block';clearTimeout(e._t);e._t=setTimeout(()=>e.style.display='none',3000)}
async function cmd(c,quiet){try{const r=await fetch('/api/cmd',{method:'POST',body:c});const t=await r.text();if(!quiet)toast(t);return t}catch(e){toast('Fehler: '+e)}}
const get=(o,p)=>p.split('.').reduce((a,k)=>a==null?a:a[k],o);
$$('#nav button').forEach(b=>b.onclick=()=>{$$('#nav button').forEach(x=>x.classList.remove('act'));b.classList.add('act');$$('section').forEach(s=>s.classList.toggle('act',s.id==b.dataset.t));onTab(b.dataset.t)});
function onTab(t){if(t=='rules')loadRules();if(t=='files')loadFiles();if(t=='plug')loadPlugins();if(t=='con'){$('#cin').focus();const l=$('#log');l.scrollTop=l.scrollHeight}if(t=='sys')fillSys()}
$$('[data-c]').forEach(el=>{el.addEventListener('change',()=>{const v=el.type=='checkbox'?(el.checked?1:0):el.value;cmd(el.dataset.c+' '+v,true).then(status)});
 if(el.type=='range')el.addEventListener('input',()=>{const o=el.parentNode.querySelector('output');if(o)o.textContent=el.value})});
function fill(){$$('[data-k]').forEach(el=>{if(document.activeElement===el)return;const v=get(st,el.dataset.k);if(v===undefined)return;if(el.type=='checkbox')el.checked=!!v;else el.value=v;const o=el.parentNode.querySelector('output');if(o)o.textContent=v})}
const fmtUp=s=>{const d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);return(d?d+'d ':'')+h+'h '+m+'m'};
const kv=(id,o)=>$(id).innerHTML=Object.entries(o).map(([k,v])=>`<span>${esc(k)}</span><span>${esc(v)}</span>`).join('');
async function status(){let r;try{r=await fetch('/api/status');st=await r.json()}catch(e){$('#hdev').className='pill alarm';$('#hdev').textContent='offline';return}
 fill();const w=st.wifi,m=st.motion;
 $('#hdev').className='pill';$('#hdev').textContent=st.name;
 $('#hwifi').className='pill'+(w.connected?' on':'');$('#hwifi').textContent=w.connected?('WLAN '+w.ip):(w.ap?'nur AP':'kein WLAN');
 $('#hmot').className='pill'+(m.active?' alarm':(m.enabled?' on':''));$('#hmot').textContent=m.enabled?(m.active?'BEWEGUNG!':'Bewegung: ruhig'):'Bewegung aus';
 $('#hflash').className='pill'+(st.flash.level?' on':'');$('#hflash').textContent='Blitz '+st.flash.level;
 $('#htime').textContent=st.time||('Uptime '+fmtUp(st.uptime));
 $('#surl').textContent=streamUrl();
 $('#camsensor').textContent=st.cam.ok?st.cam.sensor+' (max. '+st.cam.maxfs+')':'nicht erkannt';
 {const fs=[...$('[data-c=FrameSize]').options].map(o=>o.value),mx=fs.indexOf(st.cam.maxfs);if(mx>=0)[...$('[data-c=FrameSize]').options].forEach((o,i)=>{o.disabled=i>mx;o.hidden=i>mx})}
 kv('#kvlive',{'Kamera':st.cam.ok?st.cam.sensor+' ('+st.cam.framesize+')':'FEHLER','WLAN':w.connected?w.ssid+' ('+w.rssi+' dBm)':'-','IP':w.connected?w.ip:'-','Access Point':w.ap?w.apssid+' ('+w.clients+' Clients)':'aus','SD-Karte':st.sd.ok?st.sd.total+' MB':'keine (Fotos → PC)','Taste':st.button.pin<0?'aus':'GPIO'+st.button.pin,'Stream-Clients':st.streams,'Uptime':fmtUp(st.uptime),'Freier RAM':Math.round(st.heap/1024)+' KB'});
 $('#mbar').style.width=Math.min(100,m.level/Math.max(m.sensitivity,0.1)*50)+'%';$('#mbar').style.background=m.active?'var(--bad)':'var(--acc)';
 $('#mlvl').textContent=m.level+'%';$('#mstate').textContent=m.active?'Bewegung erkannt':'ruhig';$('#mcount').textContent=m.count;
 lv.push(m.level);if(lv.length>120)lv.shift();graph();checkPhoto();
 if(first){first=false;fillSys();if(st.cam.ok)toggleStream()}}
function graph(){const c=$('#mgraph'),x=c.getContext('2d'),W=c.width,H=c.height,s=st.motion.sensitivity;const mx=Math.max(s*2,...lv,1);x.clearRect(0,0,W,H);
 x.strokeStyle='#e74c3c';x.setLineDash([4,4]);x.beginPath();x.moveTo(0,H-s/mx*H);x.lineTo(W,H-s/mx*H);x.stroke();x.setLineDash([]);
 x.strokeStyle='#4f8cff';x.lineWidth=2;x.beginPath();lv.forEach((v,i)=>{const px=i/119*W,py=H-v/mx*H;i?x.lineTo(px,py):x.moveTo(px,py)});x.stroke()}
function streamUrl(){return location.protocol+'//'+location.hostname+':81/stream'+(st.token?'?t='+st.token:'')}
function toggleStream(){const v=$('#view');if(streaming){v.innerHTML='Stream gestoppt';streaming=false}else{v.innerHTML='<img alt="stream" src="'+streamUrl()+'">';streaming=true}$('#bstream').textContent=streaming?'Stream stoppen':'Stream starten'}
async function pollLog(){try{const t=await (await fetch('/api/log?from='+logFrom)).text();const i=t.indexOf('\n');const n=parseInt(t.slice(0,i));if(!isNaN(n))logFrom=n;addLog(t.slice(i+1))}catch(e){}}
function addLog(s){if(!s)return;const l=$('#log');const bottom=l.scrollTop+l.clientHeight>=l.scrollHeight-30;l.textContent+=s;if(l.textContent.length>80000)l.textContent=l.textContent.slice(-60000);if(bottom)l.scrollTop=l.scrollHeight}
async function runC(c){addLog('> '+c+'\n');const r=await cmd(c,true);let out=r;try{out=JSON.stringify(JSON.parse(r),null,1);const j=JSON.parse(r);if(j.Help)out=j.Help}catch(e){}addLog(out+'\n')}
function sendCin(){const e=$('#cin'),c=e.value.trim();if(!c)return;hist.push(c);hi=hist.length;e.value='';runC(c)}
$('#cin').addEventListener('keydown',e=>{if(e.key=='Enter')sendCin();else if(e.key=='ArrowUp'){if(hi>0){hi--;e.target.value=hist[hi]}e.preventDefault()}else if(e.key=='ArrowDown'){if(hi<hist.length-1){hi++;e.target.value=hist[hi]}else{hi=hist.length;e.target.value=''}e.preventDefault()}});
const EX=[['Blitz bei Bewegung 5s','ON Motion#Detected DO Backlog Flash 255; Delay 50; Flash 0 ENDON'],
['Foto bei Bewegung auf SD','ON Motion#Detected DO Snapshot motion ENDON'],
['Push via ntfy.sh','ON Motion#Detected DO WebSend https://ntfy.sh/MEIN_TOPIC_123?m=Bewegung+%value%25 ENDON'],
['Nachts Bewegung aus (22-6 Uhr)','ON Time#Minute=1320 DO Motion 0 ENDON ON Time#Minute=360 DO Motion 1 ENDON'],
['Zähler in Var1','ON Motion#Detected DO Add1 1 ENDON ON Time#Minute=0 DO Var1 0 ENDON'],
['Blinken beim Start','ON System#Boot DO Backlog Flash 30; Delay 3; Flash 0; Delay 3; Flash 30; Delay 3; Flash 0 ENDON'],
['Timer: alle 60s Foto','ON System#Boot DO RuleTimer1 60 ENDON ON Rules#Timer=1 DO Backlog Snapshot tl; RuleTimer1 60 ENDON'],
['Eigenes Event','ON Event#licht DO Flash %value% ENDON   (Konsole: Event licht=120)'],
['Nur starke Bewegung','ON Motion#Detected>10 DO Echo Starke Bewegung: %value% Prozent ENDON']];
function loadEx(){$('#extab').innerHTML=EX.map((e,i)=>`<tr class="ex" onclick="useEx(${i})"><td><b>${esc(e[0])}</b><br><code>${esc(e[1])}</code></td></tr>`).join('')}
function useEx(i){const t=$('#rt1');t.value=(t.value?t.value+'\n':'')+EX[i][1].replace(/\s{3}\(.*\)$/,'');t.scrollIntoView({behavior:'smooth'});toast('In Rule1 eingefügt - Speichern nicht vergessen')}
async function loadRules(){const r=await (await fetch('/api/rules')).json();const box=$('#rulebox');box.innerHTML='';
 r.rules.forEach((x,i)=>{const n=i+1,d=document.createElement('div');d.className='card';
 d.innerHTML=`<div class="row" style="margin-top:0"><h3 style="margin:0">Rule${n}</h3><label><input type="checkbox" id="ron${n}" ${x.on?'checked':''}> aktiv</label><button class="btn p" onclick="saveRule(${n})">Speichern</button></div><textarea id="rt${n}" rows="5" spellcheck="false" placeholder="ON Motion#Detected DO Flash 255 ENDON"></textarea>`;
 box.appendChild(d);$('#rt'+n).value=x.text});
 let h='<tr><th>#</th><th>Var</th><th>Mem</th><th>Timer</th></tr>';for(let i=0;i<8;i++)h+=`<tr><td>${i+1}</td><td>${esc(r.vars[i])}</td><td>${esc(r.mems[i])}</td><td>${r.timers[i]||''}</td></tr>`;$('#vartab').innerHTML=h;loadEx()}
async function saveRule(n){const r=await fetch('/api/rule?n='+n+'&on='+($('#ron'+n).checked?1:0),{method:'POST',body:$('#rt'+n).value});toast(await r.text())}
let cwd='/',fents=[];const enc=encodeURIComponent,join=(d,n)=>(d.endsWith('/')?d:d+'/')+n;
const fmtB=b=>b<1024?b+' B':b<1048576?(b/1024).toFixed(1)+' KB':b<1073741824?(b/1048576).toFixed(1)+' MB':(b/1073741824).toFixed(2)+' GB';
const isImg=n=>/\.(jpe?g|png|gif|bmp)$/i.test(n);
async function loadFiles(d){if(d!==undefined)cwd=d;let r;try{r=await (await fetch('/api/fs?dir='+enc(cwd))).json()}catch(e){return toast('Fehler beim Laden')}
 if(!r.ok){$('#ftab').innerHTML='<tr><td class="mut">Keine SD-Karte gefunden. Karte (FAT32) einlegen und "SD neu einbinden" klicken.</td></tr>';$('#sdinfo').textContent='';$('#crumb').innerHTML='';$('#sdbar').style.width='0';return}
 const tot=r.total*1024,used=r.used*1024;$('#sdbar').style.width=(tot?used/tot*100:0)+'%';$('#sdinfo').textContent=`${fmtB(used)} von ${fmtB(tot)} belegt · ${fmtB(tot-used)} frei`;
 if(r.error){toast(r.error);if(cwd!='/')return loadFiles('/')}
 cwd=r.dir;let acc='',cr=`<a href="#" onclick="loadFiles('/');return false" style="color:var(--acc)">SD</a>`;
 cwd.split('/').filter(x=>x).forEach(p=>{acc+='/'+p;const a=acc;cr+=` / <a href="#" style="color:var(--acc)" onclick='loadFiles(${JSON.stringify(a).replace(/'/g,"&#39;")});return false'>${esc(p)}</a>`});$('#crumb').innerHTML=cr;
 fents=r.entries.sort((a,b)=>(b.d-a.d)||a.n.localeCompare(b.n));
 let h='<tr><th>Name</th><th>Größe</th><th style="text-align:right">Aktionen</th></tr>';
 if(cwd!='/'){const up=cwd.slice(0,cwd.lastIndexOf('/'))||'/';h+=`<tr><td colspan="3"><a href="#" style="color:var(--acc)" onclick='loadFiles(${JSON.stringify(up).replace(/'/g,"&#39;")});return false'>&#11025; ..</a></td></tr>`}
 fents.forEach((f,i)=>{const p=join(cwd,f.n);
  if(f.d)h+=`<tr><td><a href="#" style="color:var(--acc)" onclick="openDir(${i});return false">&#128193; ${esc(f.n)}</a></td><td class="mut">Ordner</td><td style="text-align:right"><button class="btn d" onclick="rmF(${i})">Löschen</button></td></tr>`;
  else h+=`<tr><td>${isImg(f.n)?'&#128444;':'&#128196;'} <a style="color:var(--txt)" href="/api/dl?inline=1&f=${enc(p)}" target="_blank">${esc(f.n)}</a></td><td>${fmtB(f.s)}</td><td style="text-align:right;white-space:nowrap"><a class="btn" href="/api/dl?f=${enc(p)}" download>&#11015; Download</a> <button class="btn d" onclick="rmF(${i})">Löschen</button></td></tr>`});
 if(!fents.length)h+='<tr><td colspan="3" class="mut">Ordner ist leer</td></tr>';$('#ftab').innerHTML=h}
function openDir(i){loadFiles(join(cwd,fents[i].n))}
async function rmF(i){const f=fents[i],p=join(cwd,f.n);if(!confirm((f.d?'Ordner "'+f.n+'" mit ALLEM Inhalt':'"'+f.n+'"')+' löschen?'))return;const r=await fetch('/api/rm?f='+enc(p),{method:'POST'});toast(await r.text());loadFiles()}
async function mkDir(){const n=prompt('Name des neuen Ordners:');if(!n)return;if(/[\/\\]/.test(n))return toast('Kein / im Namen');const r=await fetch('/api/mkdir?d='+enc(join(cwd,n.trim())),{method:'POST'});toast(await r.text());loadFiles()}
async function upload(files){files=[...files];if(!files.length)return;$('#uprow').style.display='flex';let i=0,okc=0;
 for(const f of files){i++;$('#upinfo').textContent=`${i}/${files.length}: ${f.name}`;
  okc+=await new Promise(res=>{const x=new XMLHttpRequest();x.open('POST','/api/upload?f='+enc(join(cwd,f.name)));x.upload.onprogress=e=>{$('#upp').value=e.loaded/e.total*100};
   x.onload=()=>{let ok=false;try{ok=JSON.parse(x.responseText).Upload=='OK'}catch(e){}if(!ok)toast(f.name+': '+x.responseText);res(ok?1:0)};x.onerror=()=>{toast(f.name+': Upload fehlgeschlagen');res(0)};x.send(f)})}
 $('#uprow').style.display='none';$('#upf').value='';toast(okc+' von '+files.length+' Datei(en) hochgeladen');loadFiles()}
const fc=$('#fcard');fc.addEventListener('dragover',e=>{e.preventDefault();fc.style.borderColor='var(--acc)'});fc.addEventListener('dragleave',()=>fc.style.borderColor='');
fc.addEventListener('drop',e=>{e.preventDefault();fc.style.borderColor='';upload(e.dataTransfer.files)});
let pseq=-1;const ad=$('#autodl');try{ad.checked=localStorage.getItem('autodl')!=='0'}catch(e){ad.checked=true}ad.onchange=()=>{try{localStorage.setItem('autodl',ad.checked?'1':'0')}catch(e){}};
function dlPhoto(){const a=document.createElement('a');a.href='/api/lastphoto?_='+Date.now();a.download=(st.photo&&st.photo.name)||'photo.jpg';document.body.appendChild(a);a.click();a.remove()}
function checkPhoto(){if(!st.photo)return;if(pseq>=0&&st.photo.seq>pseq&&ad.checked){dlPhoto();toast('Foto wird heruntergeladen: '+st.photo.name)}pseq=st.photo.seq}
async function loadPlugins(){const r=await (await fetch('/api/plugins')).json();
 $('#ptab').innerHTML='<tr><th>Name</th><th>Beschreibung</th><th>Status</th><th></th></tr>'+r.map(p=>`<tr><td><b>${esc(p.name)}</b></td><td>${esc(p.desc)}</td><td><code>${p.status?esc(JSON.stringify(p.status)):'-'}</code></td><td><button class="btn ${p.enabled?'':'p'}" onclick="cmd('Plugin ${p.name} ${p.enabled?0:1}').then(loadPlugins)">${p.enabled?'Deaktivieren':'Aktivieren'}</button></td></tr>`).join('')}
function fillSys(){if(!st.wifi)return;$('#wssid').value=st.wifi.ssid;$('#apssid').value=st.wifi.apssid;$('#apmode').value=st.wifi.apmode;$('#host').value=st.name;$('#tz').value=st.sys.tz;$('#ntp').value=st.sys.ntp;
 kv('#kvwifi',{'Status':st.wifi.connected?'verbunden':'nicht verbunden','IP':st.wifi.ip||'-','Signal':st.wifi.connected?st.wifi.rssi+' dBm':'-','mDNS':'http://'+st.name+'.local'});
 kv('#kvsys',{'Firmware':'CamOS '+st.version,'Geräte-ID':st.device,'Freier RAM':Math.round(st.heap/1024)+' KB','Freies PSRAM':Math.round(st.psram/1024)+' KB','Web-Passwort':st.sys.webpass?'aktiv':'aus','Uptime':fmtUp(st.uptime)})}
async function scan(){toast('Suche WLANs...');const r=await (await fetch('/api/scan')).json();$('#wlist').innerHTML=(r.WifiScan||[]).map(n=>`<option value="${esc(n.ssid)}">${n.rssi} dBm${n.enc?' 🔒':''}</option>`).join('');toast((r.WifiScan||[]).length+' Netze gefunden - ins SSID-Feld klicken')}
async function saveWifi(){const s=$('#wssid').value.trim(),p=$('#wpass').value;if(!s)return toast('SSID fehlt');await cmd('WifiSSID '+s,1);if(p)await cmd('WifiPass '+p,1);await cmd('WifiConnect');$('#wpass').value=''}
async function clearWifi(){if(!confirm('WLAN-Daten löschen? Die Kamera ist dann nur noch per Access Point erreichbar.'))return;await cmd('WifiSSID ""',1);await cmd('WifiPass ""',1);await cmd('WifiConnect')}
async function saveAP(){const s=$('#apssid').value.trim(),p=$('#appass').value;if(s)await cmd('APSSID '+s,1);if(p){const r=await cmd('APPass '+p,1);if(r.includes('mind'))return toast(r)}await cmd('APMode '+$('#apmode').value,1);if(confirm('Jetzt anwenden? Die Verbindung kann kurz abbrechen.'))cmd('WifiConnect')}
async function saveGen(){await cmd('Hostname '+$('#host').value.trim(),1);await cmd('Timezone '+$('#tz').value.trim(),1);await cmd('NtpServer '+$('#ntp').value.trim(),1);const p=$('#wpw').value;if(p){await cmd('WebPassword '+p,1);$('#wpw').value=''}toast('Gespeichert')}
function ota(){const f=$('#otaf').files[0];if(!f)return toast('Datei wählen');const x=new XMLHttpRequest();x.open('POST','/api/ota');x.upload.onprogress=e=>{$('#otap').value=e.loaded/e.total*100};x.onload=()=>toast(x.responseText);x.onerror=()=>toast('Upload fehlgeschlagen');x.send(f)}
status();pollLog();setInterval(status,1500);setInterval(pollLog,1200);
</script></body></html>)HTML";
