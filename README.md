# WISADEV — Claude Code Monitor

แสดง **Claude Code usage limits** แบบ real-time บนจอ OLED  
เห็น Session % และ Weekly % ได้ตลอดโดยไม่ต้องพิมพ์ `/usage`

```
PLAN Pro       14:32
─────────────────────
Ses:15%        3h38m
All:53%       Wed8AM
```

---

## สิ่งที่ต้องมี

- ESP32 DevKit + OLED SSD1306 0.91" (128×32) — *มาพร้อม firmware แล้ว*
- Mac ที่ติดตั้ง **Claude Code** และ login แล้ว
- Python 3.8+

---

## ติดตั้ง

**1. ติดตั้ง library**
```bash
pip3 install bleak
```

**2. รัน script**
```bash
python3 push_plan.py
```

เชื่อม Bluetooth และส่งข้อมูลอัตโนมัติ ไม่ต้อง pair ใน System Settings  
ครั้งแรกถ้ามีหลายอุปกรณ์จะถามให้เลือก แล้วจำไว้สำหรับครั้งต่อไป

**3. รันทิ้งไว้**  
script อัพเดทหน้าจอทุก 1 นาที กด `Ctrl+C` เมื่อต้องการหยุด

---

## การต่อสาย

| OLED | ESP32 |
|------|-------|
| GND | GND |
| VCC | 3.3V |
| SCL | GPIO 22 |
| SDA | GPIO 21 |

---

## วิธีทำงาน

```
Claude Code Keychain → Anthropic API → push_plan.py → Bluetooth → ESP32 → OLED
```

ดึง token จาก macOS Keychain อัตโนมัติ ไม่ต้องใส่ API key ใดๆ

---

## คำถามที่พบบ่อย

**Q: ต้องเสียบสายต่อคอมตลอดไหม?**  
ไม่ครับ ใช้ USB adapter ธรรมดาเสียบปลั๊กได้เลย

**Q: ล็อคจอ Mac แล้ว script หยุดไหม?**  
ไม่หยุดครับ ทำงานต่อ background แต่ถ้าปิดฝา Mac จะหยุดชั่วคราว และ reconnect อัตโนมัติเมื่อเปิดฝากลับมา

**Q: รองรับ Windows หรือ Linux ไหม?**  
macOS เท่านั้น เพราะใช้ macOS Keychain

---

## License

MIT — ใช้และแก้ไขได้อิสระ  
made with ♥ by [WISADEV](https://github.com/wisadev)
