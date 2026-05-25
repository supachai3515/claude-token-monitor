#!/usr/bin/env python3
"""
push_plan.py — ดึง Claude Code usage แล้วส่งเข้า ESP32 ผ่าน Bluetooth
รัน: python3 push_plan.py
"""

import asyncio
import subprocess
import json
import sys
import os
import urllib.request
from datetime import datetime, timezone

try:
    from bleak import BleakClient, BleakScanner
except ImportError:
    print("[ERROR] กรุณาติดตั้ง: pip3 install bleak")
    sys.exit(1)

DEVICE_NAME  = "WISADEV"
RX_UUID      = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
CONFIG_FILE  = os.path.join(os.path.dirname(__file__), ".wisadev_address")


def get_token():
    try:
        result = subprocess.run(
            ["security", "find-generic-password", "-s", "Claude Code-credentials", "-w"],
            capture_output=True, text=True
        )
        creds = json.loads(result.stdout.strip())
        return creds["claudeAiOauth"]["accessToken"]
    except Exception as e:
        print(f"[ERROR] ดึง token ไม่ได้: {e}")
        return None


def fetch_usage(token):
    req = urllib.request.Request(
        "https://api.anthropic.com/api/oauth/usage",
        headers={
            "Authorization":  f"Bearer {token}",
            "anthropic-beta": "oauth-2025-04-20",
            "User-Agent":     "claude-code/2.0.32",
            "Accept":         "application/json",
        }
    )
    with urllib.request.urlopen(req, timeout=10) as resp:
        return json.loads(resp.read())


def format_remaining(resets_at):
    try:
        dt   = datetime.fromisoformat(resets_at.replace("Z", "+00:00"))
        diff = dt - datetime.now(timezone.utc)
        secs = int(diff.total_seconds())
        if secs <= 0:
            return "reset"
        h, m = divmod(secs // 60, 60)
        return f"{h}h{m:02d}m" if h else f"{m}m"
    except Exception:
        return "--"


def format_day(resets_at):
    try:
        dt    = datetime.fromisoformat(resets_at.replace("Z", "+00:00"))
        local = dt.astimezone()
        return local.strftime("%a") + local.strftime("%-I%p")
    except Exception:
        return "--"


def load_address():
    try:
        return open(CONFIG_FILE).read().strip()
    except Exception:
        return None

def save_address(addr):
    open(CONFIG_FILE, "w").write(addr)

async def pick_device():
    print("กำลังค้นหาอุปกรณ์ WISADEV...")
    devices = await BleakScanner.discover(timeout=10)
    found = [d for d in devices if d.name and DEVICE_NAME in d.name]

    if not found:
        print("[ERROR] ไม่พบ WISADEV — ตรวจสอบว่า ESP32 เปิดอยู่")
        return None

    if len(found) == 1:
        return found[0]

    # หลายตัว — ให้เลือก
    print("\nพบหลายอุปกรณ์ เลือกตัวที่ต้องการ:")
    for i, d in enumerate(found):
        print(f"  [{i+1}] {d.name} ({d.address})")
    choice = int(input("เลือกหมายเลข: ")) - 1
    return found[choice]

async def send_ble(payload):
    address = load_address()

    if address:
        print(f"ใช้อุปกรณ์เดิม ({address})")
    else:
        device = await pick_device()
        if not device:
            return False
        address = device.address
        save_address(address)
        print(f"บันทึก address แล้ว: {address}")

    async with BleakClient(address) as client:
        await asyncio.sleep(1)
        await client.write_gatt_char(RX_UUID, payload.encode(), response=True)
        await asyncio.sleep(0.5)
        print("[OK] ส่งข้อมูลสำเร็จ")
    return True


def main():
    print("=== WISADEV push_plan ===")

    token = get_token()
    if not token:
        sys.exit(1)

    try:
        data = fetch_usage(token)
    except Exception as e:
        print(f"[ERROR] เรียก API ไม่ได้: {e}")
        sys.exit(1)

    five_hour = data.get("five_hour") or {}
    seven_day = data.get("seven_day") or {}

    ses_pct   = int(five_hour.get("utilization", 0))
    ses_reset = format_remaining(five_hour.get("resets_at") or "")
    all_pct   = int(seven_day.get("utilization", 0))
    all_reset = format_day(seven_day.get("resets_at") or "")
    time_str  = datetime.now().strftime("%H:%M")

    print(f"Session : {ses_pct}% (reset ใน {ses_reset})")
    print(f"Weekly  : {all_pct}% (reset {all_reset})")

    payload = json.dumps({
        "ses":  ses_pct,
        "sr":   ses_reset,
        "all":  all_pct,
        "ar":   all_reset,
        "time": time_str,
    })

    asyncio.run(send_ble(payload))


if __name__ == "__main__":
    import time
    INTERVAL = 60  # 1 นาที
    while True:
        try:
            main()
        except Exception as e:
            print(f"[RETRY] {e}")
        print(f"รอ {INTERVAL//60} นาที...\n")
        time.sleep(INTERVAL)
