# 🛡️ Water Meter Anti-Theft System

**An ESP32-based security system that stopped water meter theft in a residential building through vibration detection, instant alarms, and community Telegram alerts.**

---

## 🚨 The Problem

Water meters in my neighborhood (New Obour, Egypt) were getting stolen constantly. They're kept in locked boxes outside buildings, but thieves would just break the locks and take them. Residents would wake up with no water and a hefty replacement bill.

I decided to do something about it.

---

## ⚡ What I Built

A complete security system that:

- 🎯 **Monitors 24 apartments** simultaneously using vibration sensors on each water meter
- 🔊 **Triggers loud sirens** the moment someone touches a meter
- 📱 **Sends Telegram messages** to residents instantly - different alerts based on how close you are to the theft
- 🔍 **Detects tampering** with the system itself (if thieves cut wires, everyone gets notified)
- 🌐 **Has a web interface** for managing everything without needing to touch the hardware

---

## 🔄 How It Actually Works

When someone starts messing with a water meter, the vibration sensor trips. Immediately:

1. 🚨 **Siren blares** - Can't miss it, impossible to ignore
2. 📲 **Owner gets an urgent Telegram message** - "Your meter is being stolen RIGHT NOW"
3. ⚠️ **Neighbors in the same box get alerts** - "Meter next to yours is being hit"
4. ℹ️ **Other nearby residents get notifications** - "FYI, theft attempt happening"

The whole building knows within seconds. Thieves can't work quietly anymore.

The system also watches its own wiring. If someone tries to disable it by cutting sensor cables, it detects that too and alerts everyone.

---

## 🧩 Technical Challenges I Solved

### ⚙️ Managing 24 sensors with limited pins
I couldn't wire all 24 sensors independently - not enough GPIO pins. So I built a power-cycling system that groups sensors and switches between them. The ESP32 powers one side of the building, reads those sensors, then switches to the other side. Happens so fast that nothing gets missed.

### 🔄 Keeping alarms working while sending messages
Telegram messages can take time, especially with spotty internet. I built a message queue system so notifications get sent in the background while the alarm keeps doing its job. If a message fails, it retries automatically without affecting anything else.

### 💻 Making it actually usable
No command line, no SSH, no technical knowledge needed. I built a complete web interface where residents can configure their Telegram notifications, and admins can enable/disable apartments, check system status, and change settings. Just connect to the device's WiFi and open a browser.

### 📡 Handling WiFi drops
The ESP32 runs both as a WiFi client (connecting to the building's network) and as an access point (so admins can always reach it) simultaneously. Even if the internet goes down, local alarms still work and you can still access the configuration page.

---

## 🛠️ The Stack

| Component | Technology |
|-----------|------------|
| 🧠 **Brain** | ESP32 microcontroller |
| 🔌 **Sensor Control** | Custom switching circuit (NPN transistors) |
| 📳 **Detection** | 801S vibration sensors (24 units) |
| 🔊 **Alarms** | Relay-controlled sirens |
| 💬 **Notifications** | Telegram Bot API |
| 🌐 **Interface** | Web server running on ESP32 |

All code written in **C++** for Arduino framework. No external servers needed - everything runs locally on the device.

---

## 🎯 What Makes This Work

The **graduated alert system** is the key innovation. Everyone gets notified, but the message changes based on where you are:

| Distance | Alert Type | Icon |
|----------|------------|------|
| **Your meter** | 🚨 Emergency alert | Critical |
| **Same box** | ⚠️ High priority warning | Urgent |
| **Adjacent box** | ⚠️ Security notice | Important |
| **Other side** | ℹ️ General FYI | Informational |

This means people actually pay attention instead of ignoring constant identical alerts. And it creates layers of response - the closer you are, the more urgent your notification, the more likely you are to check or intervene.

---

## ✅ Real Results

It works. Theft attempts now should drop dramatically. When the siren goes off and everyone's phones start buzzing simultaneously, thieves bail.

The community alert aspect turned out to be more powerful than I expected. It's not just an alarm system - it's a coordinated response system.

---

## 💡 What I Learned

This project taught me that good embedded systems design isn't just about making hardware work - it's about handling real-world chaos. Power outages, bad WiFi, people entering wrong settings, thieves cutting wires - the system has to keep working anyway.

I also learned that the best security systems are the ones people actually want to use. That's why I spent as much time on the web interface as on the core functionality. If residents can't easily configure their alerts, they won't use it, and the whole thing is pointless.

---

## 🌟 Key Features

```
✨ Real-time monitoring          🔔 Multi-level alerting
🌍 Bilingual support (AR/EN)    🔐 Tamper detection
📊 Live web dashboard            💾 Persistent configuration
🔄 Auto-recovery systems         📱 Mobile-friendly interface
⚡ Non-blocking architecture     🎨 Intuitive admin panel
```

---

## 📸 System Overview

```
🏢 Building (24 Apartments)
    ├─ 🔌 Right Side (12 apartments)
    │   ├─ 📦 Right Box (6 sensors)
    │   └─ 📦 Left Box (6 sensors)
    │
    ├─ 🔌 Left Side (12 apartments)
    │   ├─ 📦 Right Box (6 sensors)
    │   └─ 📦 Left Box (6 sensors)
    │
    └─ 🧠 ESP32 Control Unit
        ├─ 🔊 Dual Sirens
        ├─ 📡 WiFi Module
        ├─ 💬 Telegram Bot
        └─ 🌐 Web Server
```

---

<div align="center">

### 👨‍💻 Built by Omar Raafat

**2025** | Practical solution to a real problem in my community

🏠 New Obour, Egypt

---

**Technologies Used**

![ESP32](https://img.shields.io/badge/ESP32-000000?style=for-the-badge&logo=espressif&logoColor=white)
![C++](https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![Arduino](https://img.shields.io/badge/Arduino-00979D?style=for-the-badge&logo=arduino&logoColor=white)
![Telegram](https://img.shields.io/badge/Telegram-26A5E4?style=for-the-badge&logo=telegram&logoColor=white)

</div>
