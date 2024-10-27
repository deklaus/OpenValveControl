/* customize.js v0.9 (oct 26, 2024)
   Upload this script into the ESP filesystem.
   It replaces some default headers and labels by the given texts.
*/
function CustomizeLabels ()
{
  document.getElementById('header1').innerHTML = '1<sup>st</sup> Floor'; // -> OpenValveControl
  document.getElementById('header2').innerHTML = 'Ventil-Positionen';    // -> Valve Positions
  
  document.getElementById('radio1').labels[0].innerHTML = 'VZ1';  
  document.getElementById('radio2').labels[0].innerHTML = 'VZ2';  
  document.getElementById('radio3').labels[0].innerHTML = '3-Office';    // -> VZ3  
  document.getElementById('radio4').labels[0].innerHTML = 'VZ4';  
}

// Error messages may be changed or translated if required.
// When you change the no. of items in this array, don't forget to adjust the function f_status() in index.html!
const ErrorMessage = [
  "AD converter timeout",				  // -127	PIC errors
  "EEPROM write: argument count != 2",    // -9
  "EEPROM write address out of range",    // -8
  "MOVE command when HOME is active",	  // -7
  "MOVE command: argument count != 3",    // -6
  "Reference not set",					  // -5
  "Undefined command",					  // -4
  "Out of range: mA_max",				  // -3
  "Out of range: set_pos",				  // -2
  "Out of range: vz",					  // -1
  "No error",							  //  0
  "INI file not found",					  //  1		ESP errors
  "Identifier not found",				  //  2
  "PIC timeout",						  //  3
  "Move not acknowledged by PIC",		  //  4
  "Home not acknowledged by PIC",		  //  5
  "Status not acknowledged by PIC",		  //  6
  "EEPROM write not acknowledged by PIC", //  7
  "EEPROM write: address out of range",   //  8
  "EEPROM write: argument count != 2",    //  9
  "EEPROM write: data out of range"       // 10
]  