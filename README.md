# 🚆 Train Water Tank Telemetry System (ESP32 + GSM + 4–20mA)

Industrial IoT device to monitor **railway coach water tank level** and upload data to the cloud using **cellular PPP internet**.

Built using **ESP32 + GSM modem + 4–20mA sensors** for real-world deployment.

---

## ⚙️ Hardware
- ESP32
- Quectel EC200U-CN (LTE Cat-1 GSM modem)
- 4–20mA level sensors (2 channels)
- 150Ω precision shunt resistor
- SIM card

---

## 🚀 Features
- 4–20mA → % conversion
- Dual sensor channels
- ADC averaging for noise reduction
- HTTPS REST API (token auth)
- PPP cellular internet
- NTP time sync
- Sends every 15 minutes
- Modem OFF after send (power saving)
- Designed for train deployment

---

## 🔄 Working Flow
Boot  
→ PPP Connect  
→ Sync Time  
→ Get Token  
→ Read Sensors  
→ Send JSON  
→ PPP OFF  
→ Wait 15 min  
→ Repeat  

---

## 📡 Example Payload
```json
{
  "coachNo": "233443",
  "waterTankPercentage": 72,
  "readingDateTime": "03/02/2026 16:46",
  "sensorLocation": "C"
}
```

---

## 🖥️ Sample Output
```
PPP CONNECTED
CH1: 72% | CH2: 41%
HTTP Code: 200
Data Saved Successfully
PPP closed
```

---

## 📁 Files
```
main.ino
README.md
```

---

## 🧠 Skills Demonstrated
Embedded C++ • ESP32 • GSM PPP • HTTPS APIs • 4–20mA sensors • Industrial IoT • Low-power design

---

## 👨‍💻 Author
Mohd Musharraf  
B.Tech ECE – Central University of Jammu
