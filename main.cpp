
#include <WiFi.h>
#include <Wire.h>
#include <MPU6050.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <Adafruit_I2CDevice.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>


Adafruit_BMP280 bmp; // Używam I2C


// Wi-Fi credentials
const char *ssid = "Symulator lotu";
const char *password = "password";

//silnik
const int motorPin = 25;
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

// MPU6050
MPU6050 mpu;
int16_t ax, ay, az;
int16_t gx, gy, gz;

float filteredAx = 0, filteredAy = 0, filteredAz = 0;
float filteredGx = 0, filteredGy = 0, filteredGz = 0;
const float alpha = 0.8; // Dostosuj alpha do potrzeb (wartość między 0 a 1)
const float sensitivityFactor = 1; 

// Web server
WebServer server(80);


void handleRoot() {
String html = R"(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<style>
body {
    background-color: #A9A9A9;
    font-family: Arial, sans-serif;
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    height: 100%;
    margin: 0;
    padding: 0;
}
#container {
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    text-align: center;
}
#flightParams-title {
    font-size: 24px;
    font-weight: bold;
    margin-bottom: 10px;
}

   
#tempPressure-container,
#altitude-container,
#motorSpeed-container {
    display: flex;
    flex-direction: column;
    align-items: center;
    width: 100%;
    margin-bottom: 10px;
     
}

input[type=range] {
    -webkit-appearance: none;
    width: 100%;
    height: 10px;
    background: #d3d3d3;
    outline: none;
    opacity: 0.7;
    -webkit-transition: .2s;
    transition: opacity .2s;
}
input[type=range]::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 18px;
    height: 18px;
    background: #212421;
    cursor:pointer;
    border-radius: 50%;
}
.rect {
        border: 2px solid #000;
        padding: 10px;
        margin: 5px;
        border-radius: 10px;
        background-color: #d3d3d3; /* Metaliczny kolor tła */
        width: 200px;
        text-align: center;
    }
    #flightParams-container {
        display: flex;
        flex-direction: column;
        align-items: center;
    
    }
        #temperature-container,
    #pressure-container {
        display: flex;
        flex-direction: column;
        align-items: center;
        width: 100%;
        margin-bottom: 10px;
    }
  input[type=range] {
    -webkit-appearance: none;
    width: 80%; /* Zmniejszone do 80%, aby pasowało do prostokąta */
    height: 10px;
    background: #525050;
    outline: none;
    opacity: 0.7;
    -webkit-transition: .2s;
    transition: opacity .2s;
    margin: 0 auto; /* Wyśrodkowane w prostokącie */
}

.motor-button {
    color: black; }

</style>
</head>
<body>
<div id="container">
        <div id="flightParams-title">
            PARAMETRY LOTU
        </div>
        <div id="flightParams-container">
            <div class="rect" id="temperature-container">
                <span id="temperature-value">Temperatura: -- °C</span>
            </div>
            <div class="rect" id="pressure-container">
                <span id="pressure-value">Ciśnienie: -- hPa</span>
            </div>
            <div class="rect" id="altitude-container">
                <span id="altitude-value">Wysokość: -- m</span>
            </div>
            <div class="rect" id="motorControl-container">
             <button id="motorOn" class="motor-button">Włącz silnik</button>
            <button id="motorOff" class="motor-button">Wyłącz silnik</button>
            </div>

        <canvas id="horizon" width="600" height="600"></canvas>
    </div>
</div>
<script>



const updateMotorSpeed = async (value) => {
    try {
        await fetch(`/motor?speed=${value}`);
    } catch (error) {
        console.error("Failed to update motor speed:", error);
    }
};

document.getElementById("motorOn").addEventListener("click", () => updateMotorSpeed(255)); // 1 maksymalna wartość
document.getElementById("motorOff").addEventListener("click", () => updateMotorSpeed(0)); // 0 aby deaktywować silnik



const horizonCanvas = document.getElementById("horizon");
const horizonCtx = horizonCanvas.getContext("2d");
const mpuDataElem = document.getElementById("mpuData");
function drawHorizon(pitch, roll) {
const centerX = horizonCanvas.width / 2;
const centerY = horizonCanvas.height / 2;
const radius = 300;



horizonCtx.clearRect(0, 0, horizonCanvas.width, horizonCanvas.height);

// Draw the instrument housing
horizonCtx.fillStyle = "gray";
horizonCtx.beginPath();
horizonCtx.arc(centerX, centerY, radius, 0, 2 * Math.PI);
horizonCtx.fill();

// Set clipping region to the circle for the sky and earth
horizonCtx.beginPath();
horizonCtx.arc(centerX, centerY, radius - 10, 0, 2 * Math.PI); // 10 is the thickness of the housing
horizonCtx.clip();

// Rotate the canvas
horizonCtx.translate(centerX, centerY);
horizonCtx.rotate(roll * Math.PI / 180);
horizonCtx.translate(-centerX, -centerY);



// Draw the sky
horizonCtx.fillStyle = "skyblue";
horizonCtx.beginPath();
horizonCtx.arc(centerX, centerY, radius, Math.PI, 0, false);
horizonCtx.lineTo(centerX + radius, centerY + pitch);
horizonCtx.lineTo(centerX - radius, centerY + pitch);
horizonCtx.closePath();
horizonCtx.fill();

// Draw the ground
horizonCtx.fillStyle = "saddlebrown";
horizonCtx.beginPath();
horizonCtx.arc(centerX, centerY, radius, 0, Math.PI, false);
horizonCtx.lineTo(centerX - radius, centerY + pitch);
horizonCtx.lineTo(centerX + radius, centerY + pitch);
horizonCtx.closePath();
horizonCtx.fill();

// Draw auxiliary lines and labels
horizonCtx.strokeStyle = "white";
horizonCtx.lineWidth = 4;
horizonCtx.font = "20px Arial Black";
horizonCtx.fillStyle = "white";

// Clear the canvas
horizonCtx.clearRect(0, 0, horizonCanvas.width, horizonCanvas.height);

// Set clipping region to the circle
horizonCtx.beginPath();
horizonCtx.arc(centerX, centerY, radius, 0, 2 * Math.PI);
horizonCtx.clip();

// Rotate the canvas
horizonCtx.translate(centerX, centerY);
horizonCtx.rotate(roll * Math.PI / 180);
horizonCtx.translate(-centerX, -centerY);

// Draw the sky
horizonCtx.fillStyle = "skyblue";
horizonCtx.fillRect(0, 0, horizonCanvas.width, centerY + pitch);

// Draw the ground
horizonCtx.fillStyle = "saddlebrown";
horizonCtx.fillRect(0, centerY + pitch, horizonCanvas.width, horizonCanvas.height - (centerY + pitch));

// Draw auxiliary lines and labels
horizonCtx.strokeStyle = "white";
horizonCtx.lineWidth = 4;
horizonCtx.font = "bold 20px Arial Black"; // Pogrubiona czcionka
horizonCtx.fillStyle = "white";

// 1
horizonCtx.beginPath();
horizonCtx.moveTo(centerX - radius * 3 / 8, centerY + pitch - 5 * 20);
horizonCtx.lineTo(centerX + radius * 3 / 8, centerY + pitch - 5 * 20);
horizonCtx.closePath();
horizonCtx.stroke();

// 2
horizonCtx.beginPath();
horizonCtx.moveTo(centerX - radius * 3 / 4, centerY + pitch - 4 * 20);
horizonCtx.lineTo(centerX + radius * 3 / 4, centerY + pitch - 4 * 20);
horizonCtx.closePath();
horizonCtx.stroke();

// 3
horizonCtx.beginPath();
horizonCtx.moveTo(centerX - radius * 3 / 8, centerY + pitch - 3 * 20);
horizonCtx.lineTo(centerX + radius * 3 / 8, centerY + pitch - 3 * 20);
horizonCtx.fillText((20) + "°", centerX - radius * 3 / 4 - 50, centerY + pitch - 3 * 20);
horizonCtx.fillText((20) + "°", centerX + radius * 3 / 4 + 10, centerY + pitch - 3 * 20);
horizonCtx.closePath();
horizonCtx.stroke();

// 4
horizonCtx.beginPath();
horizonCtx.moveTo(centerX - radius * 3 / 4, centerY + pitch - 2 * 20);
horizonCtx.lineTo(centerX + radius * 3 / 4, centerY + pitch - 2 * 20);
horizonCtx.closePath();
horizonCtx.stroke();

// 5
horizonCtx.beginPath();
horizonCtx.moveTo(centerX - radius * 3 / 8, centerY + pitch - 1 * 20);
horizonCtx.lineTo(centerX + radius * 3 / 8, centerY + pitch - 1 * 20);
horizonCtx.fillText((10) + "°", centerX - radius * 3 / 4 - 50, centerY + pitch - 1 * 20);
horizonCtx.fillText((10) + "°", centerX + radius * 3 / 4 + 10, centerY + pitch - 1 * 20);
horizonCtx.closePath();
horizonCtx.stroke();

// 6
horizonCtx.beginPath();
horizonCtx.moveTo(centerX - radius * 3 / 8, centerY + pitch + 1 * 20);
horizonCtx.lineTo(centerX + radius * 3 / 8, centerY + pitch + 1 * 20);
horizonCtx.fillText((10) + "°", centerX - radius * 3 / 4 - 50, centerY + pitch + 1 * 20);
horizonCtx.fillText((10) + "°", centerX + radius * 3 / 4 + 10, centerY + pitch + 1 * 20);
horizonCtx.closePath();
horizonCtx.stroke();

// 7
horizonCtx.beginPath();
horizonCtx.moveTo(centerX - radius * 3 / 4, centerY + pitch + 2 * 20);
horizonCtx.lineTo(centerX + radius * 3 / 4, centerY + pitch + 2 * 20);
horizonCtx.closePath();
horizonCtx.stroke();

// 8
horizonCtx.beginPath();
horizonCtx.moveTo(centerX - radius * 3 / 8, centerY + pitch + 3 * 20);
horizonCtx.lineTo(centerX + radius * 3 / 8, centerY + pitch + 3 * 20);
horizonCtx.fillText((20) + "°", centerX - radius * 3 / 4 - 50, centerY + pitch + 3 * 20);
horizonCtx.fillText((20) + "°", centerX + radius * 3 / 4 + 10, centerY + pitch + 3 * 20);
horizonCtx.closePath();
horizonCtx.stroke();

// 9
horizonCtx.beginPath();
horizonCtx.moveTo(centerX - radius * 3 / 4, centerY + pitch + 4 * 20);
horizonCtx.lineTo(centerX + radius * 3 / 4, centerY + pitch + 4 * 20);
horizonCtx.closePath();
horizonCtx.stroke();

// 10
horizonCtx.beginPath();
horizonCtx.moveTo(centerX - radius * 3 / 8, centerY + pitch + 5 * 20);
horizonCtx.lineTo(centerX + radius * 3 / 8, centerY + pitch + 5 * 20);
horizonCtx.closePath();
horizonCtx.stroke();

// Reset rotation
horizonCtx.setTransform(1, 0, 0, 1, 0, 0);


for (let i = -2; i <= 2; i++) {
// Skip the horizon line
if (i === 0) continue;
// Translate and rotate the canvas
horizonCtx.translate(centerX, centerY);
horizonCtx.rotate(roll * Math.PI / 180);
horizonCtx.translate(-centerX, -centerY);

// Reset rotation
horizonCtx.translate(centerX, centerY);
horizonCtx.rotate(-roll * Math.PI / 180);
horizonCtx.translate(-centerX, -centerY);
}


// Reset rotation
horizonCtx.translate(centerX, centerY);
horizonCtx.rotate(-roll * Math.PI / 180);
horizonCtx.translate(-centerX, -centerY);

// Draw auxiliary lines and labels
horizonCtx.strokeStyle = "white";
horizonCtx.lineWidth = 4;
horizonCtx.font = "bold 20px Arial Black"; // Pogrubiona czcionka
horizonCtx.fillStyle = "white";

// Set clipping region to the circle
horizonCtx.beginPath();
horizonCtx.arc(centerX, centerY, radius, 0, 2 * Math.PI);
horizonCtx.clip();

// Rotate the canvas
horizonCtx.translate(centerX, centerY);
horizonCtx.rotate(roll * Math.PI / 180);
horizonCtx.translate(-centerX, -centerY);

// Reset rotation
horizonCtx.setTransform(1, 0, 0, 1, 0, 0);
// Draw the circle
horizonCtx.strokeStyle = "white";
horizonCtx.lineWidth = 2;
horizonCtx.beginPath();
horizonCtx.arc(centerX, centerY, radius, 0, 2 * Math.PI);
horizonCtx.stroke();

// Draw the center airplane silhouette
horizonCtx.strokeStyle = "yellow";
horizonCtx.lineWidth = 8;
horizonCtx.beginPath();
horizonCtx.moveTo(centerX - 80, centerY);
horizonCtx.lineTo(centerX - 40, centerY);
horizonCtx.moveTo(centerX + 40, centerY);
horizonCtx.lineTo(centerX + 80, centerY);
horizonCtx.moveTo(centerX, centerY - 10);
horizonCtx.lineTo(centerX, centerY + 10);
horizonCtx.moveTo(centerX - 10, centerY + 10);
horizonCtx.lineTo(centerX + 10, centerY + 10);
horizonCtx.stroke();




}
async function fetchData() {
try {
const response = await fetch("/data");
const data = await response.text();
const [ax, ay, az, gx, gy, gz] = data.split(",").map(Number);

// Calculate pitch and roll
const pitch = Math.atan2(ax, Math.sqrt(ay * ay + az * az)) * 180 / Math.PI*4;
const roll = Math.atan2(ay, Math.sqrt(ax * ax + az * az)) * 180 / Math.PI;


// Update horizon and MPU data
drawHorizon(pitch, roll);
mpuDataElem.textContent = `Odczyty MPU: ax=${ax}, ay=${ay}, az=${az}, gx=${gx}, gy=${gy}, gz=${gz}`;
} catch (error) {
console.error("Failed to fetch data:", error);
}
}
function startFetchingData() {
setInterval(fetchData, 200); // Aktualizuj co  200ms
}

 function updateTemperature(temperature) {
        document.getElementById('temperature-value').innerText = `Temperatura: ${temperature} `;
    }
    
    function updatePressure(pressure) {
        document.getElementById('pressure-value').innerText = `Ciśnienie: ${pressure} `;
    }
    
 function updateAltitude(temperature, pressure) {
    const P0 = 101325;  
    const P = parseFloat(pressure) * 100;  // Konwersja z hPa na Pa

    // Przeliczanie ciśnienia na wysokość przy użyciu równania atmosfery wzorcowej
    const h = (1 - Math.pow((P / P0), 0.190263)) * 44330.8; // Wzór na wysokość w metrach
    document.getElementById('altitude-value').innerText = `Wysokość: ${h.toFixed(2)} m`;  
}


function fetchTempPressure() {

    updateTemperature(temperature);
    updatePressure(pressure);
    updateAltitude(temperature, pressure);
}


    async function fetchTempPressure() {
        try {
            const response = await fetch("/tempPressure");
            const data = await response.text();
            const [temperature, pressure] = data.split(",");
            updateTemperature(temperature);
            updatePressure(pressure);
            updateAltitude(parseFloat(temperature), parseFloat(pressure));
        } catch (error) {
            console.error("Failed to fetch temperature and pressure data:", error);
        }
    }


function startFetchingTempPressure() {
    setInterval(fetchTempPressure, 2000); // Fetch data every 2000ms (2 seconds)
}



startFetchingData();
startFetchingTempPressure();
</script>
</body>
</html>

)";
server.send(200, "text/html", html);
}

void updateFilteredReadings() {
    int16_t ax, ay, az;
    int16_t gx, gy, gz;

    // Odczytaj dane z MPU6050
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    // Zastosuj filtr dolnoprzepustowy
    filteredAx = alpha * ax + (1 - alpha) * filteredAx;
    filteredAy = alpha * ay + (1 - alpha) * filteredAy;
    filteredAz = alpha * az + (1 - alpha) * filteredAz;
    filteredGx = alpha * gx + (1 - alpha) * filteredGx;
    filteredGy = alpha * gy + (1 - alpha) * filteredGy;
    filteredGz = alpha * gz + (1 - alpha) * filteredGz;
}

void handleData() {
    updateFilteredReadings(); // Wywołanie funkcji aktualizującej odczyty z czujnika 
    String data = String(filteredAx) + "," + String(filteredAy) + "," + String(filteredAz) + "," +
                  String(filteredGx) + "," + String(filteredGy) + "," + String(filteredGz);
    server.send(200, "text/plain", data);
}


void handleMotor() {
    int speed = server.arg("speed").toInt(); // Pobiera wartość prędkości z parametru żądania HTTP

    // Sprawdza, czy prędkość jest w dopuszczalnym zakresie, a następnie ustawia prędkość silnika
    if (speed >= 0 && speed <= 255) {
        ledcWrite(ledChannel, speed); // Ustawia prędkość silnika wykorzystując PWM
    }
    server.send(200, "text/plain", "OK"); // Wysyła potwierdzenie do użytkownika
}





void handleTempPressure() {
  float temperature = bmp.readTemperature();
  float pressure = bmp.readPressure() / 100.0F;  // Konwertuje na hPa
  String data = String(temperature) + "°C," + String(pressure) + "hPa";
  server.send(200, "text/plain", data);
}


void setup() {
   
Serial.begin(115200);

// Konfiguracja kanału PWM
  ledcSetup(ledChannel, freq, resolution);

  // Przypisanie GPIO do kanału PWM
  ledcAttachPin(motorPin, ledChannel);

 // Konfiguracja kanału PWM
    ledcSetup(ledChannel, freq, resolution);
    ledcAttachPin(motorPin, ledChannel);



// Initialize MPU6050
Wire.begin(18, 22);
 mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  filteredAx = ax;
  filteredAy = ay;
  filteredAz = az;
  filteredGx = gx;
  filteredGy = gy;
  filteredGz = gz;
  
  if (!bmp.begin(0x76)) {  // Sprawdź adres swojego czujnika BMP280, może być 0x76 lub 0x77
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
  }


mpu.initialize();
// Configure Wi-Fi

WiFi.mode(WIFI_AP);
WiFi.softAP(ssid, password);


// Configure web server
server.on("/", handleRoot);
server.on("/data", handleData);
 server.on("/tempPressure", handleTempPressure); 
  server.on("/motor", handleMotor); 
server.begin();
}
void loop() {
server.handleClient();
}