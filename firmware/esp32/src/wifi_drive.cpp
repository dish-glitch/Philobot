#include "wifi_drive.h"

#ifdef WIFI_DRIVE
#include <WiFi.h>
#include <WebServer.h>

#define WIFI_SSID  "Philo-Robot"
#define WIFI_PASS  "philo1234"     // 8+ chars required by WPA2

static WebServer server(80);
static volatile bool    has_cmd = false;
static int              cmd_l = 0, cmd_r = 0;
static uint8_t          cmd_f = 0;

// Joystick page. Sends /cmd?l=..&r=..&f=.. ~10x/s while touched, and once
// more with 0s on release. The ESP32 watchdog stops the motors if the
// phone goes quiet for 500 ms (e.g. walks out of WiFi range).
static const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head>
<meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no">
<title>Philo</title><style>
body{margin:0;font-family:sans-serif;background:#111;color:#eee;text-align:center;touch-action:none;user-select:none}
#pad{width:80vw;height:80vw;max-width:360px;max-height:360px;margin:16px auto;border-radius:50%;background:#222;position:relative;border:2px solid #444}
#knob{width:30%;height:30%;border-radius:50%;background:#2a9d8f;position:absolute;left:35%;top:35%}
button{font-size:22px;padding:14px 40px;margin:8px;border:0;border-radius:12px;background:#e63946;color:#fff}
label{display:block;margin:10px}</style></head><body>
<h2>Philo</h2><div id="pad"><div id="knob"></div></div>
<button id="stop">STOP</button>
<label>Max speed <span id="mv">200</span><br><input id="max" type="range" min="130" max="255" value="200"></label>
<p id="st">touch the circle to drive</p>
<script>
const pad=document.getElementById('pad'),knob=document.getElementById('knob'),st=document.getElementById('st');
const maxIn=document.getElementById('max'),mv=document.getElementById('mv');
const MIN=120;let x=0,y=0,active=false,brake=false;
maxIn.oninput=()=>mv.textContent=maxIn.value;
function scale(v){if(v==0)return 0;const m=+maxIn.value;const s=MIN+Math.abs(v)*(m-MIN);return Math.round(v>0?s:-s);}
function move(e){const r=pad.getBoundingClientRect(),t=e.touches?e.touches[0]:e;
 let dx=(t.clientX-r.left)/r.width*2-1,dy=(t.clientY-r.top)/r.height*2-1;const d=Math.hypot(dx,dy);if(d>1){dx/=d;dy/=d;}
 x=Math.abs(dx)<0.15?0:dx;y=Math.abs(dy)<0.15?0:-dy;
 knob.style.left=(35+dx*35)+'%';knob.style.top=(35+dy*35)+'%';e.preventDefault();}
function start(e){active=true;brake=false;move(e);}
function end(){active=false;x=0;y=0;knob.style.left='35%';knob.style.top='35%';send();}
pad.addEventListener('touchstart',start,{passive:false});pad.addEventListener('touchmove',move,{passive:false});
pad.addEventListener('touchend',end);pad.addEventListener('mousedown',start);
window.addEventListener('mousemove',e=>{if(active)move(e);});window.addEventListener('mouseup',()=>{if(active)end();});
document.getElementById('stop').onclick=()=>{brake=true;active=false;x=0;y=0;send();};
function send(){const l=Math.max(-1,Math.min(1,y+x)),r=Math.max(-1,Math.min(1,y-x));
 const q=brake?'l=0&r=0&f=2':'l='+scale(l)+'&r='+scale(r)+'&f=0';
 fetch('/cmd?'+q).then(()=>st.textContent='connected').catch(()=>st.textContent='not connected — join WiFi Philo-Robot');}
setInterval(()=>{if(active)send();},100);
</script></body></html>)HTML";

static void handle_cmd() {
    cmd_l = constrain(server.arg("l").toInt(), -255, 255);
    cmd_r = constrain(server.arg("r").toInt(), -255, 255);
    cmd_f = (uint8_t)server.arg("f").toInt();
    has_cmd = true;
    server.send(200, "text/plain", "ok");
}

void wifi_drive_init() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_SSID, WIFI_PASS);
    server.on("/", []() { server.send_P(200, "text/html", PAGE); });
    server.on("/cmd", handle_cmd);
    server.begin();
    Serial.printf("WiFi drive: join \"%s\" (password %s), open http://%s\n",
                  WIFI_SSID, WIFI_PASS, WiFi.softAPIP().toString().c_str());
}

bool wifi_drive_poll(int &left, int &right, uint8_t &flags) {
    server.handleClient();
    if (!has_cmd) return false;
    has_cmd = false;
    left = cmd_l; right = cmd_r; flags = cmd_f;
    return true;
}

#else
void wifi_drive_init() {}
bool wifi_drive_poll(int &, int &, uint8_t &) { return false; }
#endif
