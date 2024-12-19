#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

// Wi-Fi credentials
const char *ssid = "Students";
const char *password = "Students";

WebServer server(80);

// Pin definitions
#define TRIGPIN 12
#define ECHOPIN 13
#define DHTPIN 15
#define potPin 34
#define led01Pin 2
#define led02Pin 4
#define led03Pin 14
#define led04Pin 21
#define buzzerPin 19

// DHT sensor
DHT dht(DHTPIN, DHT11);

// Variables
long duration;
int distance;
int potValue;
float pH;
float temperature;

// Function prototypes
void handleRoot();
void handleSensorData();
void measureSensors();
String getPhColor(int pH); // Function to return the pH color based on value

// Setup
void setup() {
  Serial.begin(115200);

  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);
  pinMode(led01Pin, OUTPUT);
  pinMode(led02Pin, OUTPUT);
  pinMode(led03Pin, OUTPUT);
  pinMode(led04Pin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  dht.begin();

  digitalWrite(led01Pin, LOW);
  digitalWrite(led02Pin, LOW);
  digitalWrite(led03Pin, LOW);
  digitalWrite(led04Pin, LOW);
  digitalWrite(buzzerPin, LOW);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/sensorData", handleSensorData);
  server.begin();
}

void loop() {
  server.handleClient();
  measureSensors();
}

void measureSensors() {
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);
  duration = pulseIn(ECHOPIN, HIGH);
  distance = duration * 0.034 / 2;

  // Read potentiometer value and map it to pH range (0 to 14)
  potValue = analogRead(potPin);
  pH = map(potValue, 0, 4095, 0, 14);  // Mapping pH from 0 to 14

  temperature = dht.readTemperature();

  digitalWrite(led01Pin, distance <= 5 ? HIGH : LOW);
  digitalWrite(led02Pin, temperature > 20 ? HIGH : LOW);
  digitalWrite(led03Pin, (pH < 5 || pH > 7) ? HIGH : LOW); // Adjust for pH range (out of normal range)
  if ((pH < 5 || pH > 7) && temperature >= 20 && distance <= 5) {
    digitalWrite(led04Pin, HIGH);
    digitalWrite(buzzerPin, HIGH);
  } else {
    digitalWrite(led04Pin, LOW);
    digitalWrite(buzzerPin, LOW);
  }

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C | pH value: ");
  Serial.print(pH);
  Serial.print(" | Water Level: ");
  Serial.println(distance);
}

String getPhColor(float pH) {
  if (pH >= 0 && pH <= 3) {
    return "red"; // Acidic range (0-3)
  } else if (pH > 3 && pH <= 6) {
    return "yellow"; // Slightly acidic range (4-6)
  } else if (pH == 7) {
    return "green"; // Neutral
  } else if (pH > 7 && pH <= 10) {
    return "blue"; // Slightly alkaline range (8-10)
  } else if (pH > 10 && pH <= 14) {
    return "purple"; // Alkaline range (11-14)
  }
  return "gray"; // Default if out of range
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang='en'>
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1.0'>
  <title>AquaTrack Monitor</title>
  <script src='https://cdn.jsdelivr.net/npm/chart.js'></script>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; background: #f4f6f8; }
    .data-card { padding: 20px; margin: 10px; background: white; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }
    canvas { max-width: 100%; }
  </style>
</head>
<body>
  <h1>AquaTrack</h1>
   <h2>Water level , Water temperature & pH value Monitor</h2>
  <div class='data-card'><h2>Temperature: <span id='temp'>--</span> °C</h2></div>
  <div class='data-card'><h2>pH Value: <span id='ph'>--</span></h2></div>
  <div class='data-card'><h2>Water Level: <span id='level'>--</span> cm</h2></div>
  
  <div class='data-card'>
    <h2>Real-Time pH Value Graph</h2>
    <canvas id='phChart'></canvas>
  </div>
  
  <div class='data-card'>
    <h2>Real-Time Temperature Graph</h2>
    <canvas id='tempChart'></canvas>
  </div>

  <script>
    const phCtx = document.getElementById('phChart').getContext('2d');
    const tempCtx = document.getElementById('tempChart').getContext('2d');

    const phChart = new Chart(phCtx, {
      type: 'line',
      data: {
        labels: [],
        datasets: [{
          label: 'pH Value',
          data: [],
          borderColor: 'rgba(75, 192, 192, 1)',
          backgroundColor: 'rgba(75, 192, 192, 0.2)',
          borderWidth: 2,
          fill: true,
          tension: 0.1
        }]
      },
      options: {
        responsive: true,
        scales: {
          x: { title: { display: true, text: 'Time' } },
          y: { title: { display: true, text: 'pH Value' }, min: 0, max: 14, stepSize: 1 }
        }
      }
    });

    const tempChart = new Chart(tempCtx, {
      type: 'line',
      data: {
        labels: [],
        datasets: [{
          label: 'Temperature (°C)',
          data: [],
          borderColor: 'rgba(255, 99, 132, 1)',
          backgroundColor: 'rgba(255, 99, 132, 0.2)',
          borderWidth: 2,
          fill: true,
          tension: 0.1
        }]
      },
      options: {
        responsive: true,
        scales: {
          x: { title: { display: true, text: 'Time' } },
          y: { title: { display: true, text: 'Temperature (°C)' }, min: 0, max: 50 }
        }
      }
    });

    setInterval(() => {
      fetch('/sensorData').then(res => res.json()).then(data => {
        const now = new Date();
        const timestamp = `${now.getHours()}:${now.getMinutes()}:${now.getSeconds()}`;

        document.getElementById('temp').textContent = data.temp;
        document.getElementById('ph').textContent = data.ph;
        document.getElementById('level').textContent = data.level;

        // Set the pH value color dynamically
        document.getElementById('ph').style.color = data.phColor;

        if (phChart.data.labels.length > 20) {
          phChart.data.labels.shift();
          phChart.data.datasets[0].data.shift();
        }
        if (tempChart.data.labels.length > 20) {
          tempChart.data.labels.shift();
          tempChart.data.datasets[0].data.shift();
        }

        phChart.data.labels.push(timestamp);
        phChart.data.datasets[0].data.push(data.ph);
        tempChart.data.labels.push(timestamp);
        tempChart.data.datasets[0].data.push(data.temp);

        phChart.update();
        tempChart.update();
      });
    }, 1000);
  </script>
</body>
</html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleSensorData() {
  String phColor = getPhColor(pH); // Get the color based on pH value
  String json = "{\"temp\":" + String(temperature) +
                ",\"ph\":" + String(pH) +
                ",\"level\":" + String(distance) +
                ",\"phColor\":\"" + phColor + "\"}";  // Send the color as part of the data
  server.send(200, "application/json", json);
}
