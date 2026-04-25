<h1>ESP32 + LD2410C Presence Detection System</h1>
<p>This project implements a <b>room presence detection system</b>b> using an ESP32 microcontroller and the HLK-LD2410C. It detects both <b>moving and stationary</b> people (e.g., someone sitting still), which makes it far more reliable than PIR sensors for smart home and surveillance use cases.</p>
<br/>
<h3>📡Overview</h3>

The system uses:
<ul>
<li><b>ESP32</b> for processing, networking (MQTT / Wi-Fi), and integration</li>
<li><b>LD2410C radar sensor</b> for accurate human presence detection</li>
</ul>

Typical workflow:
<ul>
<li>Sensor continuously scans the environment</li>
<li>ESP32 reads detection data via UART</li>
<li>Presence / distance is processed</li>
<li>Data is published (e.g., MQTT, Home Assistant)</li>
</ul>
<br>
<h3>🔍 About the Sensor (LD2410C)</h3>

The HLK-LD2410C is a <b>millimeter-wave radar module</b> designed for indoor human detection.🧰 Hardware Requirements
ESP32 development board
LD2410C radar sensor
5V power supply
Jumper wires

<b>Key features</b>
<ul>
<li>Detects <b>motion and stationary presence</b></li>
<li>Adjustable <b>detection range (up to ~6–8 meters)</b></li>
<li>Multi-zone (gate) sensitivity configuration</li>
<li>Real-time distance measurement</li>
<li>Works through:</li>
 <ul>
 <li>glass</li>
 <li>plastic</li>
 <li>thin walls (limited)</li>
 </ul>
</ul>
<b>Advantages over PIR</b>
<ul>
<li>Detects <b>still humans</b> (breathing, micro-movements)</li>
<li>Not affected by temperature</li>
<li>Higher sensitivity and stability</li>
</ul>
<br/>
<h3>🧰 Hardware Requirements</h3>
<ul>
<li>ESP32 development board</li>
<li>LD2410C radar sensor</li>
<li>5V power supply</li>
<li>Jumper wires</li>
</ul>
