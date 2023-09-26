/** @file  webUI.c
 *  @author Klaus Deutschkämer (https://github.com/deklaus)
 *
 *  @brief Functions for WLAN User Interface (UI)
 *  @todo 
 */
/*  Change Log:
 *  26.09.2023 V0.12
 *  - First issue
 */

// *** data type, constant and macro definitions
// *** global variables
// *** private variables
// *** public function bodies

void handleRoot() {
  char  buf[64];
  String htmlPage;
  htmlPage.reserve(16000);  // prevent ram fragmentation

htmlPage = F(
  "<!DOCTYPE html>\n"
  "<html lang='de'>\n"
  "<head>\n"
  "<meta charset='utf-8'>\n"
  "	<link rel='icon' href='data:image/x-icon;base64,AAABAAEAGiACAAEAAQAwAQAAFgAAACgAAAAaAAAAQAAAAAEAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAzzMwAAAAAAP///\n"
  "	8D//4/A//4fwP/4H8D/8D/A/+A/wP/AP8D/wD/A/8A/wP/AP8Df4D/A5+A/wPPwP8D4+D/A/DwfwP4eD8D+Dw/A/weHwP8D48D/gfnA/4D8wP+A/0D/gP/A/4B/wP+A/8D/gP/A/wH/wP8D\n"
  "	/8D/B//A/h//wPz//8D////A////wP//j8D//h/A//gfwP/wP8D/4D/A/8A/wP/AP8D/wD/A/8A/wN/gP8Dn4D/A8/A/wPj4P8D8PB/A/h4PwP4PD8D/B4fA/wPjwP+B+cD/gPzA/4D/QP+\n"
  "	A/8D/gH/A/4D/wP+A/8D/Af/A/wP/wP8H/8D+H//A/P//wP///8A='>\n"
  "<title>ESP-FALMOT</title>\n"
  "<meta name='viewport' content='width=device-width, initial-scale=0.9'>\n"
  "<script>\n"
  "	let position = [-1, 0, 56, 36, 100]; \n"           // initial positions (javascript)
  " let set_pos = [-1, 0, 65, 36, 100 ]; \n"           // valve set_positions
  " let max_mA = [0.0, 11.0, 12.0, 13.0, 14.0 ]; \n"   // motor current limits [mA]  
  "\n"
  "setInterval(f_status, 1500); \n"
  "f_status(); \n"

  "	function get_vz() {\n"  // returns the 'value' of the selected checkbox: [1..4]
  "   var inputs = document.getElementsByName('vz');\n"
  "   for (var i = 0; i < inputs.length; i++) {\n"
  "     if (inputs[i].checked) return inputs[i].value;\n"
  "   }\n"
  "	}\n"

  " function VZselected(sel) { \n"
  "   document.getElementById('set_pos').value = set_pos[sel]; \n"
  "   document.getElementById('max_mA').value  = max_mA[sel]; \n"
  "	}\n"

  " function update_pos(sel) {\n"   // updates the position display of selected vz (sel = [1-4])
  "   var radios = document.getElementsByName('vz'); \n"
  "   var color = radios[sel-1].checked ? '#9f6' : '#494'; \n"
  " 	var id = 'vz'+ sel.toString(10); \n"   // bgcolor has to be set, even when position == set_pos
  " 	document.querySelector(id).innerText = position[sel].toString(10) + ' %'; \n"
  " 	id = 'VZ'+ sel.toString(10); \n"   // hbar (width)
  " 	var elem = document.getElementById(id); \n"
  " 	elem.setAttribute('style', 'width:' + position[sel].toString(10) + '%;background-color:' + color + ';'); \n"
  "	}\n"

  "	function update_allpos() {	\n"   // update aller Positionen
  "		for (var i = 1; i <= 4; i++) {\n"
  "     update_pos(i);\n"
  "		}	\n"
  "	}\n"

  "	function f_save() {\n"
  "	  var data = 'no response';\n"
  "		var query = 'ssid=' + document.getElementById('ssid').value + '&psk=' + document.getElementById('psk').value;\n"
  "   var xhr = new XMLHttpRequest();\n"
  "   xhr.open('GET', '/save?'+query, true);\n"
  "   xhr.onreadystatechange = function () {\n"
  "     if ((xhr.readyState == 4) && (xhr.status == 200)) {\n"
  "       data = xhr.responseText;\n"
  "       //alert(xhr.responseText);\n"
	"			  var info = document.createTextNode(data);\n"
	"	      var element = document.getElementById('status');\n"
  "       element.replaceChild(info, element.firstChild);\n"
  "     }\n"
  "   }\n"
  "   xhr.send();\n"
  "	}\n"

  " function f_move() {\n"
  "	  var data = 'no response';\n"
  "	  var sel = get_vz();\n"
  "   set_pos[sel] = parseInt(document.getElementById('set_pos').value);\n"
  "   max_mA[sel] = parseFloat(document.getElementById('max_mA').value);\n"
  "		var query = 'vz=' + sel + '&set_pos=' + set_pos[sel] + '&max_mA=' + max_mA[sel].toFixed(1);\n"
  "   var xhr = new XMLHttpRequest();\n"
  "   xhr.open('GET', '/move?'+query, true);\n"
  "   xhr.onreadystatechange = function () {\n"
  "     if ((xhr.readyState == 4) && (xhr.status == 200)) {\n"
  "       data = xhr.responseText;\n"
  "       //alert(xhr.responseText);\n"
	"			  var info = document.createTextNode(data);\n"
	"	      var element = document.getElementById('status');\n"
  "       element.replaceChild(info, element.firstChild);\n"
  "     }\n"
  "   }\n"
  "   xhr.send();\n"
  "   update_pos(sel);\n"
  " }\n"

  "	function f_home() {\n"
  "	  var data = 'no response';\n"
  "	  var sel = get_vz();\n"
  "	  max_mA[sel] = parseFloat(document.getElementById('max_mA').value);\n"
  "		var query = 'vz=' + sel + '&max_mA=' + max_mA[sel].toFixed(1);\n"
  "   var xhr = new XMLHttpRequest();\n"
  "   xhr.open('GET', '/home?'+query, true);\n"
  "   xhr.onreadystatechange = function () {\n"
  "     if ((xhr.readyState == 4) && (xhr.status == 200)) {\n"
  "       data = xhr.responseText;\n"
  "       //alert(xhr.responseText);\n"
	"			  var info = document.createTextNode(data);\n"
	"	      var element = document.getElementById('status');\n"
  "       element.replaceChild(info, element.firstChild);\n"
  "     }\n"
  "   }\n"
  "   xhr.send();\n"
  "	}\n"

  "	function f_status() {\n"
  "		fetch('/status?')\n"
  "			.then (function(response){\n"
  "			    if (!response.ok) {\n"
  "					  throw new Error('HTTP error ' + response.status);\n"
  "			    }\n"
  "				return response.json();\n"
  "			})\n"
  "			.then (function(data){ \n"
  "       var radios = document.getElementsByName('vz'); \n"
  "       mAmps = data.mAmps.valueOf(); \n"
  "       document.querySelector('mAmps').innerText = data.mAmps; \n" 
  "				position[1] = data.VZ1.Position; \n"
  "				position[2] = data.VZ2.Position; \n"
  "				position[3] = data.VZ3.Position; \n"
  "				position[4] = data.VZ4.Position; \n"
  "	      for (var i = 1; i <= 4; i++) {\n"
  "	 	      if (position[i] < 0 || position[i] > 100) position[i] = 50; \n"
  "	      } \n"
  "       update_allpos();"
  "			});\n"
  "	}\n"

  "	function f_info() {\n"
  "	  var data = 'no response';\n"
  "   var xhr = new XMLHttpRequest();\n"
  "   xhr.open('GET', '/info?', true);\n"
  "   xhr.onreadystatechange = function () {\n"
  "     if ((xhr.readyState == 4) && (xhr.status == 200)) {\n"
  "       data = xhr.responseText;\n"
  "       //alert(xhr.responseText);\n"
  "       var info = document.createTextNode(data);\n"
  "       var element = document.getElementById('status');\n"
  "       element.replaceChild(info, element.firstChild);\n"
  "     }\n"
  "   }\n"
  "   xhr.send();\n"
  " }\n"
 
 "</script>\n"
  "<style>\n"
  "	a { \n"
  "		text-decoration:none;\n"
  "		color:#488; \n"
  "	}\n"
  "	body {\n"
  "		min-width:360px;\n"
  "		text-align:center;\n"
  "		font-family:verdana,sans-serif;\n"
  "		background:#252525;\n"
  "	}\n"
  "	.page {\n"
  "		background:#252525;\n"
  "		margin: auto;\n"
  "		color:#eaeaea; \n"
  "		width:360px;\n"
  "		padding:10px;\n"
  "	}\n"
  "	/* Ventil-Positionen */\n"
  "	div.table {\n"
  "		width:360px;\n"
  "		font-size:0.9rem;\n"
  "	}\n"
  " div.tr { \n"
  "		display: flex;\n"
  "		align-items: center;\n"
  " }	\n"
  "	\n"
  " div.td1 { \n"
  "	  text-align:left;\n"
  "		width:20%;\n"
  " }\n"
  " div.td2 { \n"
  "		width:60%;\n"
  " }	\n"
  " div.td3 { \n"
  "	  text-align:right;\n"
  "	  padding-left:5px; \n"
  "	  width:15%;\n"
  " }\n"
  "	.hbar {\n"
  "		height:20px; \n"
  "		display: flex;\n"
  "		align-items: center;\n"
  "		background-color:#999;\n"
  "		margin-top: 0.5em;\n"
  "		padding: 0.1em;\n"
  "		-webkit-transition-duration:0.2s;\n"
  "		transition-duration:0.2s;\n"
  "		cursor:pointer;		\n"
  "	}\n"
  "	.btn, input[type=submit] {\n"
  "		border:0;\n"
  "		border-radius:0.3rem;\n"
  "		background:#29e;\n"
  "		color:#fff;\n"
  "		width:100%;\n"
  "		height:2rem;\n"
  "		font-size:0.9rem;\n"
  "		//margin-top:1rem;\n"
  "	}\n"
  "	.btn:hover { \n"
  "		background-color:#147; \n"
  "	}	\n"
  "	.btnlbl {\n"
  "		text-align:left;\n"
  "	}\n"
  "	.label {\n"
  "		padding-left:10px;\n"
  "	}\n"
  "	.vz-sel {\n"
  "	}	\n"
  "	input[type=text],input[type=password] {\n"
  "		width:130px;\n"
  "		text-align:center;\n"
  "		float:right;\n"
  "	}\n"
  "	input[type=radio] {\n"
  "		text-align:left;\n"
  "		float:left;\n"
  "	}\n"
  "	input[id=ssid],input[id=psk] {\n"
  "		width:190px;\n"
  "	}\n"
  "	form, p[id=status] {\n"
  "		text-align:left;\n"
  "		font-size:0.9rem;\n"
  "	}\n"
  "</style>\n"
  "</head>\n"

  "<body>\n"
  "	<div class='page'>\n"
  "	<noscript>Upps! Offenbar ist JavaScript nicht aktiv!<br>ESP-ValveControl funktioniert nur mit JavaScript.<br>Bitte Browser-Einstellungen überprüfen!<br>\n"
  "	</noscript>\n"
  "	\n"
  "	<h3>ESP-ValveControl</h3>\n"
  "	\n"
  "	<table style='width:100%; text-align:left;'>\n"
  "	  <tr>\n"
  "		<td style='width:55%; padding-left:10px;'><b>Aktueller Strom:</b></td>\n"
  "		<td><mAmps>0.0</mAmps> mA</td>\n"
  "	  </tr>  \n"
  "	</table>\n"
  "\n"
  "	<script>\n"
  "		'use strict';\n"
  "     document.querySelector('mAmps').innerText = '");  
htmlPage += mAmps;
htmlPage += F("';\n"
  "	</script>\n"
  "\n"
  "	<h2>Ventil-Positionen</h2>\n"
  "	<div class='table'>\n"
  "	<div class='tr'>\n"
  "		<div class='td1'><input type='radio' name='vz' id='radio1' value='1' onclick='VZselected(1)' checked>\n"
  "		  <label for='radio1'>VZ1</label>\n"
  "		</div>\n"
  "		<div class='td2'><div class='hbar' id='VZ1'; style='width:"); htmlPage += position[1]; htmlPage += F("%;'></div></div>\n"
  "		<div class='td3'><vz1>"); htmlPage += position[1]; htmlPage += F(" %</vz1></div>\n"
  "	</div>\n"
  "\n"
  "	<div class='tr'>\n"
  "		<div class='td1'><input type='radio' name='vz' id='radio2' value='2' onclick='VZselected(2)'>\n"
  "		  <label for='radio2'>VZ2</label>\n"
  "		</div>\n"
  "		<div class='td2'><div class='hbar' id='VZ2'; style='width:"); htmlPage += position[2]; htmlPage += F("%;'></div></div>\n"
  "		<div class='td3'><vz2>"); htmlPage += position[2]; htmlPage += F(" %</vz2></div>\n"
  "	</div>\n"
  "	\n"
  "	<div class='tr'>\n"
  "		<div class='td1'><input type='radio' name='vz' id='radio3' value='3' onclick='VZselected(3)'>\n"
  "		  <label for='radio3'>VZ3</label>\n"
  "		</div>\n"
  "		<div class='td2'><div class='hbar' id='VZ3'; style='width:"); htmlPage += position[3]; htmlPage += F("%;'></div></div>\n"
  "		<div class='td3'><vz3>"); htmlPage += position[3]; htmlPage += F(" %</vz3></div>\n"
  "	</div>\n"
  "	\n"
  "	<div class='tr'>\n"
  "		<div class='td1'><input type='radio' name='vz' id='radio4' value='4' onclick='VZselected(4)'>\n"
  "		  <label for='radio4'>VZ4</label>\n"
  "		</div>\n"
  "		<div class='td2'><div class='hbar' id='VZ4'; style='width:"); htmlPage += position[4]; htmlPage += F("%;'></div></div>\n"
  "		<div class='td3'><vz4>"); htmlPage += position[4]; htmlPage += F(" %</vz4></div>\n"
  "	</div>  \n"
  "	</div>\n"
  "	\n"
  "    <p class='label'>\n"
  "		<form> \n"
  "		  <p class='label'>Sollposition [%] \n"
  "		  <input type='text' id='set_pos' value='"); htmlPage += set_pos[1]; htmlPage += F("'>\n"
  "		  </p>\n"
  "		  <p class='label'>Strom-Grenzwert [mA]\n"
  "		  <input type='text' id='max_mA' value='"); sprintf(buf, "%.1f", max_mA[1]); htmlPage += buf; htmlPage += F("'>\n"
  "		  </p>\n"
  "		</form>\n"
  "	</p>\n"
  "		\n"
  "    <p><button class='btn' name='btn_move' onclick='f_move();'> Position anfahren</button></p>\n"
  "    <p><button class='btn' name='btn_home' onclick='f_home();'> Referenzieren</button></p>\n"
  "    <p><button class='btn' name='btn_info' onclick='f_info();'> Info</button></p>\n"
  "\n"
  "    <p class='label'>\n"
  "		<form> \n"
  "		  <p class='label'>WLAN-SSID\n"
  "       <input type='text' id='ssid' value='"); htmlPage += ssid; htmlPage += F("'>\n"
  "		  </p>\n"
  "		  <p class='label'>WLAN-Passwort\n" 
  "       <input type='password' id='psk' value='"); htmlPage += psk; htmlPage += F("'>\n"
  "		  </p>\n"
  "		</form>\n"
  "	</p>\n"
  "\n"
  "    <p><button class='btn' name='btn_save'   onclick='f_save();'>   Speichern</button></p>\n"
  "\n"
  "    <p id='status'> </p>\n"
  "	<script>\n"
  "		var elem = document.getElementsByName('vz');\n"
  "		for (var i = 0; i < 4; i++) {\n"
  "			elem[i].addEventListener('click', update_allpos);\n"
  "		}\n"
  "	</script>\n"
  "\n"
  "	<div style='text-align:right;font-size:10px;'>\n"
  "	<hr/>\n");
  htmlPage += ESPversion; htmlPage += F(" von <a href='https://github.com/deklaus/' target='_blank'>Klaus Deutschkämer</a>\n"
  "	</div>	\n"
  "</div>\n"
  "</body>\n"
  "</html>\n"
  "\n" );

  server.send(200, "text/html", htmlPage);
} // handleRoot()


/** @brief Handler for MOVE. Arguments: <ESP_IP>/move?vz=[1..4]&set_pos=[0..100]&max_mA=[0.0 .. 100.0]
 */
void handleMove ()
{
  int error = 1;  // preset!
  char  buf[64];
  String htmlPage;
  htmlPage.reserve(128);  // prevent ram fragmentation
  int   sel;
  int   pos;
  float mA;

  if (server.hasArg("vz") && server.hasArg("set_pos") && server.hasArg("max_mA")) 
  {
    sel = server.arg("vz").toInt();
    pos = server.arg("set_pos").toInt();
    mA  = server.arg("max_mA").toFloat();
    if (sel >= 1 && sel <= 4 && pos >= 0 && pos <= 100 && mA >= 0.0 && mA <= 100.) 
    {
      htmlPage = F("{\n"
      "\"move\": {\n"
      "  \"vz\": ");  htmlPage += sel;  htmlPage += F(",\n"
      "  \"set_pos\": \""); htmlPage += pos;  htmlPage += F(",\n"
      "  \"max_mA\": \""); sprintf(buf, "%.1f", mA);  htmlPage += buf;  htmlPage += F("\"\n"
      "  }\n}");      
      vz = sel;
      set_pos[sel] = pos;
      max_mA[sel] = mA;
      flags.move = 1;   // set flag for triggering a MOVE command to PIC
      error = 0;
    }
  }
  if (error) 
  {
    htmlPage = F("Parameter error: \n");
    for (uint8_t i = 0; i < server.args(); i++) 
    {
      htmlPage += server.argName(i) + "=" + server.arg(i) + "\n";
    }
  }    
  server.send(200, "text/plain", htmlPage);

} // handleMove()


/** @brief Handler for HOME. Arguments: <ESP_IP>/home?vz=[1..4]&max_mA=[0.0 .. 100.0]
 */
void handleHome ()
{
  int error = 1;  // preset!
  char  buf[128];
  String htmlPage;
  htmlPage.reserve(128);  // prevent ram fragmentation
  int   sel;
  float mA;

  if (server.hasArg("vz") && server.hasArg("max_mA")) 
  {
    sel = server.arg("vz").toInt();
    mA  = server.arg("max_mA").toFloat();
    if (sel >= 1 && sel <= 4 && mA >= 0. && mA <= 100.) 
    {
      htmlPage = F("{\n"
      "\"home\": {\n"
      "  \"vz\": ");  htmlPage += sel;  htmlPage += F(",\n"
      "  \"max_mA\": \""); sprintf(buf, "%.1f", mA);  htmlPage += buf;  htmlPage += F("\"\n"
      "  }\n}");
      vz = sel;
      max_mA[sel] = mA;
      flags.home = 1;   // set flag for triggering a HOME command to PIC
      error = 0;
    }
  }
  if (error) 
  {
    htmlPage = F("Parameter error: \n");
    for (uint8_t i = 0; i < server.args(); i++) 
    {
       htmlPage += server.argName(i) + "=" + server.arg(i) + "\n";
    }    
  }
  server.send(200, "text/plain", htmlPage);

} // handleHome ()


/** @brief Handler for INFO request.
 */
void handleInfo ()
{
  char  buf[128];
  String htmlPage;
  htmlPage.reserve(500);  // prevent ram fragmentation

  htmlPage = F(
  "{ \n"
  "\"ESP\": \"" ); htmlPage += ESPversion; htmlPage += F("\",\n"
  "\"PIC\": \"" ); htmlPage += PICversion; htmlPage += F("\",\n"
  "\"SSID\": \""); htmlPage += ssid;       htmlPage += F("\",\n"
  "}\n");

    //server.sendHeader("Access-Control-Allow-Origin","*"); 
    server.send(200, "text/plain", htmlPage);

} // handleInfo ()


/** @brief Handler for "SAVE" (@todo: POST). Arguments: <ESP_IP>/save?ssid=<string>&psk=<password>
 */
void handleSave ()
{
  int error = 1;  // preset!
  char  buf[128];
  String htmlPage;
  htmlPage.reserve(256);  // prevent ram fragmentation
  String  id;
  String  pwd;

  if (server.hasArg("ssid") && server.hasArg("psk")) 
  {
    id = server.arg("ssid");
    pwd = server.arg("psk");
    htmlPage = F("{\n"
    "\"save\": {\n"
    "  \"ssid\": \""); htmlPage += id;  htmlPage += F("\",\n"
//    "  \"psk\": \"");  htmlPage += pwd; htmlPage += F("\"\n"
    "  \"psk\": \"");  htmlPage += "***"; htmlPage += F("\"\n"
    "  }\n}");

    strncpy(ssid, id.c_str(),  sizeof(ssid)); 
    ssid[sizeof(ssid)-1] = '\0';
    strncpy(psk,  pwd.c_str(), sizeof(pwd));
    ssid[sizeof(pwd)-1] = '\0';

    flags.save = 1;   /** @todo set flag for new WLAN credentials (loop: save in Flash) */
    error = 0;
  }
  if (error) 
  {
    htmlPage = F("Parameter error: \n");
    for (uint8_t i = 0; i < server.args(); i++) 
    {
       htmlPage += server.argName(i) + "=" + server.arg(i) + "\n";
    }    
  }
  server.send(200, "text/plain", htmlPage);

} // handleSave ()


/** @brief Handler for STATUS request. \n
 *  Contains all readings and status' from PIC µC.
 *  May be extended, because data is parsed as JSON object (name:value).
 */
void handleStatus ()
{
  char  buf[128];
  String htmlPage;
  htmlPage.reserve(1000);  // prevent ram fragmentation

  flags.status = 1;   // set flag for status request  OBSOLETE - we do this every loop cycle

  htmlPage = F(
  "{ \n"
  "\"mAmps\": \""); sprintf(buf, "%.1f", mAmps); htmlPage += buf; htmlPage += F("\",\n"

  "\"VZ1\": {\n"
  "    \"Position\": "); htmlPage += position[1]; htmlPage += F(",\n"
  "    \"Set_Pos\": ");  htmlPage += set_pos[1];  htmlPage += F(",\n"
  "    \"Ref_Set\": ");  htmlPage += refset[1];   htmlPage += F(",\n"
  "    \"max_mA\": \""); sprintf(buf, "%.1f", max_mA[1]); htmlPage += buf;   htmlPage += F("\"\n"
  "  },\n"

  "\"VZ2\": {\n"
  "    \"Position\": "); htmlPage += position[2]; htmlPage += F(",\n"
  "    \"Set_Pos\": ");  htmlPage += set_pos[2];  htmlPage += F(",\n"
  "    \"Ref_Set\": ");  htmlPage += refset[2];   htmlPage += F(",\n"
  "    \"max_mA\": \""); sprintf(buf, "%.1f", max_mA[2]); htmlPage += buf;   htmlPage += F("\"\n"
  "  },\n"

  "\"VZ3\": {\n"
  "    \"Position\": "); htmlPage += position[3]; htmlPage += F(",\n"
  "    \"Set_Pos\": ");  htmlPage += set_pos[3];  htmlPage += F(",\n"
  "    \"Ref_Set\": ");  htmlPage += refset[3];   htmlPage += F(",\n"
  "    \"max_mA\": \""); sprintf(buf, "%.1f", max_mA[3]); htmlPage += buf;   htmlPage += F("\"\n"
  "  },\n"

  "\"VZ4\": {\n"
  "    \"Position\": "); htmlPage += position[4]; htmlPage += F(",\n"
  "    \"Set_Pos\": ");  htmlPage += set_pos[4];  htmlPage += F(",\n"
  "    \"Ref_Set\": ");  htmlPage += refset[4];   htmlPage += F(",\n"
  "    \"max_mA\": \""); sprintf(buf, "%.1f", max_mA[4]); htmlPage += buf;   htmlPage += F("\"\n"
  "  }\n"

  "}\n");

  server.send(200, "text/plain", htmlPage);

} // handleStatus ()


/** @brief Handler for all NotFound requests (invalid URIs)
 */
void handleNotFound () 
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) 
  { 
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n"; 
  }
  server.send(404, "text/plain", message);

} // handleNotFound ()

// *** private function bodies

/**
 End of File
 */