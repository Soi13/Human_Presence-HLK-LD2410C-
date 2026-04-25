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
<br/>
<h3>🔌 Connection Diagram</h3>
<b>Wiring table:</b>
<table>
  <tr>
    <th>LD2410C</th>
    <th>ESP32 Pin</th>
    <th>Description</th>
  </tr>
  <tr>
    <td>VCC (5V)</td>
    <td>5V/VIN</td>
    <td>Power supply</td>
  </tr>
  <tr>
    <td>GND</td>
    <td>GND</td>
    <td>Common ground</td>
  </tr>
 <tr>
    <td>TX</td>
    <td>RX (GPIO 16)</td>
    <td>Sensor - ESP32</td>
  </tr>
 <tr>
    <td>RX</td>
    <td>TX (GPIO 17)</td>
    <td>ESP32 - Sensor</td>
  </tr>
 <tr>
    <td>OUT</td>
    <td>GPIO optional</td>
    <td>Digital presence signal</td>
  </tr>
</table>
<br/>
<b>Diagram:</b>
<br/><br/>

![Picture of schema](LD210C-ESP32-Circuit.jpg)

            +----------------------+
          |     LD2410C          |
          |                      |
          |   VCC  ------------+------ 5V (ESP32)
          |   GND  ------------+------ GND
          |   TX   ------------+------ GPIO16 (RX)
          |   RX   <-----------+------ GPIO17 (TX)
          |   OUT  ------------+------ GPIO4 (optional)
          +----------------------+

                        ||
                        ||

          +----------------------+
          |       ESP32          |
          |                      |
          |   VIN / 5V           |
          |   GND                |
          |   GPIO16 (RX)        |
          |   GPIO17 (TX)        |
          |   GPIO4 (INPUT)      |
          +----------------------+
<br/>
<h3>⚙️Communication</h3>
<ul>
<li>Interface: <b>UART</b></li>
<li>Baud rate: <b>256000</b></li>
<li>Data format: binary frames (decoded in firmware)</li>
</ul>
The sensor provides:
<ul>
<li>Moving target distance</li>
<li>Stationary target distance</li>
<li>Energy levels per detection zone</li>
</ul>
<br/>
<h3>🚀Features</h3>
<ul>
<li>Real-time presence detection</li>
<li>Moving vs stationary classification</li>
<li>Distance measurement</li>
<li>MQTT integration (Home Assistant ready)</li>
<li>Optional fast trigger via OUT pin</li>
</ul>
<br/>
<h3>🧠How It Works</h3>
1. LD2410C continuously scans using mmWave radar<br/>
2. Internal DSP processes reflections<br/>
3. Data is sent via UART to ESP32<br/>
4. ESP32:
<ul>
<li>parses frames</li>
<li>extracts distance and presence</li>
<li>publishes results</li>
</ul>
<br/>
<h3>⚠️Important Notes</h3>
<ul>
<li><b>Common ground is required</b> between ESP32 and sensor</li>
<li>Sensor must be powered with <b>5V</b></li>
<li>UART lines must be <b>crossed (TX ↔ RX)</b></li>
<li>Ensure correct baud rate (256000)</li>
<li>Avoid blocking operations in firmware (important for MQTT stability)</li>
</ul>
<br/>
<h3>📦Configuration examples:</h3>

```c++
/////////##Configuration section for LD2410C sensor##//////////////

//Sending request
void ld2410_send(uint8_t *cmd, size_t len) {
    uart_write_bytes(UART_NUM_2, (const char*)cmd, len);
}

//Reading response
int ld2410_read(uint8_t *buf, size_t max_len) {
    return uart_read_bytes(UART_NUM_2, buf, max_len, pdMS_TO_TICKS(200));
}

//Enter in configuration mode
uint8_t enter_config[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x04, 0x00,
    0xFF, 0x00,
    0x01, 0x00,
    0x04, 0x03, 0x02, 0x01
};

//Exit from configuration mode
uint8_t exit_config[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x02, 0x00,
    0xFE, 0x00,
    0x04, 0x03, 0x02, 0x01
};

//Read firmware version. Command: 0xA0
uint8_t read_version[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x02, 0x00,
    0xA0, 0x00,
    0x04, 0x03, 0x02, 0x01
};

//Read all parameters. Command: 0x61
//This returns: max distance, gate sensitivities, thresholds
uint8_t read_all[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x02, 0x00,
    0x61, 0x00,
    0x04, 0x03, 0x02, 0x01
};

//Enabling engineering mode. Command: 0x62
uint8_t enable_engineering_mode[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x02, 0x00,
    0x62, 0x00,
    0x04, 0x03, 0x02, 0x01
};

//Disabling engineering mode. Command: 0x63
uint8_t disable_engineering_mode[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x02, 0x00,
    0x63, 0x00,
    0x04, 0x03, 0x02, 0x01
};

//Set max detection distance. Command: 0x60
//We set here maximum distance gate 8 (stationary and motion). No one duration is 5 sec.
uint8_t max_distance_gate[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x14, 0x00,
    0x60, 0x00,
    0x00, 0x00,
    0x08, 0x00, 0x00, 0x00,
    0x01, 0x00,
    0x08, 0x00, 0x00, 0x00,
    0x02, 0x00,
    0x05, 0x00, 0x00, 0x00,
    0x04, 0x03, 0x02, 0x01
};

//Set gate sensetivity. Command: 0x64
void set_gate(uint8_t gate, uint8_t moving, uint8_t stat) {
    uint8_t cmd[] = {
        0xFD, 0xFC, 0xFB, 0xFA,
        0x14, 0x00,
        0x64, 0x00,
        0x00, 0x00,
        gate, 0x00, 0x00, 0x00,
        0x01, 0x00,
        moving, 0x00, 0x00, 0x00,
        0x02, 0x00,
        stat, 0x00, 0x00, 0x00,
        0x04, 0x03, 0x02, 0x01
    };

    ld2410_send(cmd, sizeof(cmd));
}

//Set all gates at once
void configure_all_gates() {
    // Near field → high sensitivity
    set_gate(0, 50, 100);
    set_gate(1, 50, 100);
    set_gate(2, 40, 40);

    // Mid range
    set_gate(3, 30, 40);
    set_gate(4, 20, 30);

    // Far range → suppress noise
    set_gate(5, 15, 30);
    set_gate(6, 15, 20);
    set_gate(7, 15, 20);
    set_gate(8, 15, 20);
}

//Start config main method
void ld2410_start_config() {

    uint8_t rx[256];

    uart_flush(UART_PORT);

    //Enter config mode
    ld2410_send(enter_config, sizeof(enter_config));
    vTaskDelay(pdMS_TO_TICKS(200));

    //ld2410_send(read_all, sizeof(read_all));
    configure_all_gates();
    vTaskDelay(pdMS_TO_TICKS(200));

    //Enable engineering mode
    ld2410_send(enable_engineering_mode, sizeof(enable_engineering_mode));
    vTaskDelay(pdMS_TO_TICKS(200));

    //Disable engineering mode
    ld2410_send(disable_engineering_mode, sizeof(disable_engineering_mode));
    vTaskDelay(pdMS_TO_TICKS(200));

    //Check response after executing configuration comand
    int len = ld2410_read(rx, sizeof(rx));

    if (len > 0) {
        ESP_LOGI(TAG, "Received %d bytes", len);

        for (int i = 0; i < len; i++) {
            printf("%02X ", rx[i]);
        }
        printf("\n");
    }

    //Exit config mode
    ld2410_send(exit_config, sizeof(exit_config));
    vTaskDelay(pdMS_TO_TICKS(200));
}

////////##End of configuration section for LD2410C sensor##////////

...
...
...

void app_main(void)
{
    ...
    ...
    ld2410_start_config();
    ...
    ...
}

```
