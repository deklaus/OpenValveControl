# This folder contains the ESP Firmware
This project was compiled on **Linux Mint 21** with the **Arduino IDE 2.2.1**, <br>
it uses a ESP8266 board "**LOLIN(WEMOS) D1  mini (clone)**", core 'esp8266' 3.1.2 <br>
and the following Arduino libraries:

- **ESP8266WiFi**, Version 1.0
- **ESP8266WebServer**, Version 1.0
- **ESP8266HTTPUpdateServer**, Version 1.0
- **LittleFS**, Version 0.1.0
- **DallasTemperature**, Version 3.9.0
- **MAX31850 OneWire**, Version 1.1.1
- **U8g2**, Version 2.34.22
- **SPI**, Version 1.0
- **Wire**, Version 1.0 

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
This file contains the webserver part of the ESP software. It implements all the callbacks (called by the *client*). 
The User Interface by itself is a HTML Page (index.html), which is stored in *LittleFS* and can be invoked by the client 
on demand.<br>
The corresponding callback ``` bool handleFile(String &&path) ``` can be found in **LittleFS.ino**.

The basic structure of an HTML web page is assumed to be known. 
The essential features of a **web server** are:
- Input of parameters (values)
- Activation of requests
- Display of data (measured values), if possible in real time.

#### Input of parameters
Input parameters can be implemented in HTML as \<form\>.<br>
An initial value could be assigned with the expression ``` value='xxx' ```.

Example:
``` html
<form> 
	<p class='label'>Set Position [%] 
	<input type='text' id='set_pos' value='35'>
	</p>
</form>
```
However, the value should match the current target position. When we switch to a different
valve zone, we want to replace the value by the set position of the selected valve zone.
This can be achieved by giving the input element an ID, e.g. ```id='set_pos' ```, then we 
can change the *value* on the fly via a JavaScript function.

Example:
``` javascript
function VZselected(sel) 
{
  document.getElementById('set_pos').value = set_pos[sel];
}
```
But wait - this is only the initial value displayed. But how does a new input value is stored and reach 
the server, resp. how does the ESP get the input?<br>
When in our application an action is triggered, e.g. when the button 'Move VZ' gets pressed, an assigned
JavaScript function gets called. This function reads the current *value* of the form using 
``` document.getElementById('set_pos').value ``` and stores it in the JavaScript variable 
``` set_pos[sel] ```. Finally, a **XML Http Request (XHR)** is sent to the server, initiating the move command and 
transfering all required parameters (see next paragraph).

#### Activation of requests
In our application we want to initiate a request by pressing a button, e.g. to run the valve to a desired 
position, after the parameters (valve index, set position and max. current) have been configured.
When the button gets pressed, we can call a JavaScript function, which handles the necessary actions.
To achieve this, we must pass the name of the function as argument of the *onclick* item in the definition of *button*: <br>
``` <button class='btn' name='btn_move' onclick='f_move();'> Move VZ</button> ```

Example:
``` javascript
function f_move() {
  var data = 'no response';	// init result string

  // get the selected valve zone, set position and stop current from UI:
  var sel = get_vz();	
  set_pos[sel] = parseInt(document.getElementById('set_pos').value);
  max_mA[sel] = parseFloat(document.getElementById('max_mA').value);

  // build the argument string, which will be appended to the URI and the request (/move?) before being sent to the ESP:
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

Slightly different to the request activation, we can use **fetch** to dynamically request data from 
the web server. For this purpose, we define a JavaScript function **f_status()**. See the comments for
an explanation of how it works:

``` javascript
function f_status() {
  fetch('/status?')  // this is the fetch request, which can be evaluated by the "server.on" function (see the *setup* chapter).
  // it returns a 'Promise' that resolves to the Response to that request — as soon as the server responds with headers
  .then (function(response) {
    if (!response.ok) {	// if the server response is an HTTP error status:
      throw new Error('HTTP error ' + response.status);
    }
    return response.json();
  })
  // The Response object is the API wrapper for the fetched resource. We can parse the response body text as JSON.
  .then (function(data) {
    // parse mAmps and set value in user interface
    mAmps = data.mAmps.valueOf();
    document.querySelector('mAmps').innerText = data.mAmps;

    // parse position data and set the global JavaScript variables 
    position[1] = data.VZ1.Position;
    position[2] = data.VZ2.Position;
    position[3] = data.VZ3.Position;
    position[4] = data.VZ4.Position;
    update_allpos();  // this will update all sliders and numeric position display
  });
}
```

To periodically update the measured values we can use the JavaScript method **setInterval**:
``` setInterval(f_status, 1500); ```


