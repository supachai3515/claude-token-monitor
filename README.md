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

## วิธีอัพโหลด Firmware ลง ESP32

> สำหรับผู้ที่ต้องการ compile และ flash firmware เอง (ปกติบอร์ดมี firmware มาให้แล้ว)

### สิ่งที่ต้องมี

- Python 3.8+ และ `pip3`
- USB cable เสียบ ESP32 กับ Mac

### 1. ติดตั้ง PlatformIO และแก้ PATH (ทำครั้งเดียว)

```bash
pip3 install platformio
```

หลังติดตั้งแล้ว ถ้ารัน `pio` แล้วขึ้น `command not found` ให้เพิ่ม PATH:

```bash
echo 'export PATH="$HOME/Library/Python/3.9/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

> ถ้าใช้ Python เวอร์ชันอื่น ให้เปลี่ยน `3.9` เป็นเวอร์ชันที่ใช้ (ดูได้จาก `python3 --version`)

### 2. แก้ python symlink (ทำครั้งเดียว)

Build script ของ ESP32 เรียก `python` แต่ macOS มีแค่ `python3` ต้องสร้าง symlink:

```bash
mkdir -p ~/.local/bin
ln -sf /usr/bin/python3 ~/.local/bin/python
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

### 2. ถอดสาย OLED ออกก่อน

**สำคัญ:** ถอดสาย OLED และสายอื่นๆ ทั้งหมดออกก่อนอัพโหลดเสมอ เพราะอาจรบกวนสัญญาณ

### 3. เข้า Bootloader Mode (ทำทุกครั้งก่อนอัพโหลด)

บอร์ดนี้ใช้ชิป CH340 ซึ่ง **ไม่รองรับ auto-reset** ต้องกด manual ทุกครั้ง:

```
1. กดปุ่ม BOOT ค้างไว้
2. กดปุ่ม EN (reset) แล้วปล่อย EN
3. ปล่อยปุ่ม BOOT
   → บอร์ดอยู่ใน bootloader mode แล้ว (มีเวลาประมาณ 3 วินาที)
```

### 4. อัพโหลด

รันคำสั่งนี้ **ทันที** หลังจากกดปุ่มข้างบน:

```bash
pio run --target upload
```

รอจนขึ้น `Hard resetting via RTS pin...` แสดงว่าสำเร็จ

### 5. ต่อสาย OLED กลับ

เมื่ออัพโหลดเสร็จแล้วค่อยต่อสาย OLED กลับตามตารางการต่อสายด้านบน

### หมายเหตุ platformio.ini

```ini
upload_port = /dev/cu.usbserial-0001   ; port ของ CH340 บน Mac
upload_speed = 115200
upload_flags =
    --before no_reset                  ; ไม่ให้ reset อัตโนมัติก่อน upload
    --after hard_reset                 ; reset หลัง upload เสร็จ
    --no-stub                          ; ต้องใช้กับ esptool เวอร์ชันนี้
```

ถ้า port ไม่ตรงให้รัน `ls /dev/cu.*` เพื่อหา port ที่ถูกต้อง

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
