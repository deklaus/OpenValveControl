# This folder contains the ESP Firmware
This project was compiled on **Linux Mint 21** with the **Arduino IDE 2.1.0**, <br>
it uses a ESP8266 board "**LOLIN(WEMOS) D1  mini (clone)**" and
the following Arduino libraries:

- **ESP8266WiFi**, Version 1.0
- **ESP8266WebServer**, Version 1.0
- **U8g2**, Version 2.34.22

## ESP-ValveControl.ino
This is the Arduino project file and must reside in a folder with the same name (ESP-ValveControl).
As usual, it contains the **setup()** and the main **loop()** blocs.

#### setup
- Initialize Serial (UART0) to 38400 Bd. <br>
  Serial uses UART0 which is mapped to pins GPIO1 (TX) and GPIO3 (RX).  <br>
  For communication with the PIC-µC, we remap UART0 to GPIO15 (TX) and GPIO13 (RX) by Serial.swap(). 
  Calling swap again maps UART0 back to GPIO1 and GPIO3. (Serial1/UART1 can not be used to receive).
- Initialize OLED Status Display <br>
  Show local IP address and initial valve positions.
- Initialize WiFi <br>
  When connected, show local IP address on OLED, so we can connect to the webserver UI.
- Read the PIC Firmware Version
- Setup Webserver <br>
  This implements the client request handlers. You can append the commands and parameters to the URI, e.g. <br>
  ``` http://192.168.2.108/move?vz=1&set_pos=25&max_mA=30 ``` <br>
  ``` http://192.168.2.108/status? ```

#### loop
- Process request handler and execute commands from UI
- Periodically read status from PIC (valve controller)
- Update OLED status display

## OLED.ino

![grafik](https://github.com/deklaus/OpenValveControl/assets/134941062/381b864e-4c95-4f8c-b542-b32fa9c08f5e)

#### OLED_init
- Initializes the Display.

#### OLED_show
- Updates OLED **row** with text message **char \*s**.

#### OLED_update_status
- Updates the OLED status display:
  - graphical position (horizontal bars)
  - numeric position (if reference is set)
  - actual drive current 

## webUI.ino
This file contains the webserver part of the ESP software. Actually it constructs the HTML Page (called by the *client*) 
as String, but in the future it shall be placed into *LittleFS*. The basic structure of an html web page is assumed to be known. 
The essential features of a **web server** are:
- Input of parameters (values)
- Activation of requests
- Display of data (measured values), if possible in real time.

#### Input of parameters
Input parameters can be realized in HTML as <forms>.
If the page is reloaded, we currently generate in the module handleRoot() the HTML code 
with the value to be displayed, in the example the current target position. <br>
Example:
``` 
<form> 
	<p class='label'>Sollposition [%] 
	<input type='text' id='set_pos' value='35'>
	</p>
</form>
```
However, this is impractical if the page is to be loaded from an ESP file system, 
then you would have to search the HTML code for parameters and replace the values.
But if you give the input element an ID, e.g. ```id='set_pos' ```, then you can change 
the *value* via JavaScript function without reloading the page. <br>
Example:

```
function VZselected(sel) 
{
  document.getElementById('set_pos').value = set_pos[sel];
}
```
Wait - this is only the initial value displayed. But how does a new input value reach the server,
resp. how does the ESP get the input? In fact, this is not necessary for this application! <br>
Only if an action is triggered, e.g. when the button 'Move to position' gets pressed, the momentary *value* of the 
set position is read from the form using ``` document.getElementById('set_pos').value ``` and sent to the 
server together with the move command and other required parameters. See next paragraph.

#### Activation of requests
In our application we want to initiate a request by pressing a button, e.g. to run the valve to a desired 
position, after the parameters (valve index, set position and max. current) have been configured.
When the button gets pressed, we can call a javascript function, which handles the necessary actions.
To achieve this, we must pass the name of the function as argument of the *onclick* item in the definition of *button*: <br>
``` <button class='btn' name='btn_move' onclick='f_move();'> Position anfahren</button> ```

Example:
```
function f_move() {
  var data = 'no response';	// init result string

  // get the selected valve zone, set position and stop current from UI:
  var sel = get_vz();	
  set_pos[sel] = parseInt(document.getElementById('set_pos').value);
  max_mA[sel] = parseFloat(document.getElementById('max_mA').value);

  // build the argument string, which will be appended to the URI and the request (/move?) and send to the ESP:
  var query = 'vz=' + sel + '&set_pos=' + set_pos[sel] + '&max_mA=' + max_mA[sel].toFixed(1);

  // send a XML HTTP request to the server, which can be evaluated by the "server.on" function (see the *setup* chapter).  
  var xhr = new XMLHttpRequest();
  xhr.open('GET', '/move?'+query, true);

  // the next paragraph installs an event handler, which is executed when the request has been answered,
  // but doesn't stall the execution of this function:
  xhr.onreadystatechange = function ()
  {
    if ((xhr.readyState == 4) && (xhr.status == 200))
    {
      data = xhr.responseText;
      alert(xhr.responseText);

      // install a section at the bottom of the site where we can show the response
      var info = document.createTextNode(data);
      var element = document.getElementById('status');
      element.replaceChild(info, element.firstChild);
    }
  }
  xhr.send();  // this will send the request
}
```

#### Display of measured values

Similar to request activation, we can use XMLHttpRequest (short: XHR) to dynamically request data from 
the web server without having to reload the HTML page.


```
setInterval(f_status, 1500);
f_status();
```






