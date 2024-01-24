/* customize.js
   Upload this script into the ESP filesystem.
   It replaces some default headers and labels by the given texts.
*/
function CustomizeLabels ()
{
  document.getElementById('header1').innerHTML = 'Erdgeschoss';         // -> OpenValveControl
  document.getElementById('header2').innerHTML = 'Ventil-Positionen';   // -> Valve Positions
  
  document.getElementById('radio1').labels[0].innerHTML = 'VZ1';  
  document.getElementById('radio2').labels[0].innerHTML = 'VZ2';  
  document.getElementById('radio3').labels[0].innerHTML = '3-Büro';     // -> VZ3  
  document.getElementById('radio4').labels[0].innerHTML = 'VZ4';  
}

