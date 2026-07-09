"""
串口写入 RA2E1 Flash — 详细通讯日志版
用法: python flash_write.py
"""
import serial
import struct

COM_PORT = 'COM19'
BAUD = 9600
NUM_PAGES = 4               # ← 写几页 (每页 512 字节)

def tx_hex(data: bytes) -> str:
    """字节转 HEX 字符串"""
    return ' '.join(f'{b:02X}' for b in data)

def main():
    ser = serial.Serial(COM_PORT, BAUD, timeout=2)

    # ============================================================
    # 第 1 步: 握手 (HANDSHAKE 'B')
    # ============================================================
    print("=" * 60)
    print("【步骤 1】握手 — 检测 Bootloader 是否在线")
    print("=" * 60)
    ser.write(b'\x42')
    print(f"  PC → MCU:  42                            ('B' 握手)")
    r = ser.read(1)
    if r == b'\x42':
        print(f"  MCU → PC:  42                            (握手成功)")
    else:
        print(f"  MCU → PC:  {tx_hex(r)}                            (握手失败!)")
        ser.close(); return

    # ============================================================
    # 第 2 步: 逐页写入 + 验证
    # ============================================================
    for page in range(NUM_PAGES):
        offset = page * 512
        data = bytes([page, (page + 1) & 0xFF] + [(i + page * 7) & 0xFF for i in range(510)])
        checksum = sum(data) & 0xFFFF

        # ---- 2a. SET_ADDRESS 'D' ----
        print()
        print("=" * 60)
        print(f"【步骤 2a】设地址 — 第{page+1}页, 偏移 0x{offset:04X} (绝对地址 0x{0x4000+offset:08X})")
        print("=" * 60)
        addr_bytes = bytes([(offset >> 8) & 0xFF, offset & 0xFF])
        ser.write(b'\x44' + addr_bytes)
        print(f"  PC → MCU:  44 {tx_hex(addr_bytes)}                   ('D' + 地址 = 0x{offset:04X})")
        r = ser.read(2)
        print(f"  MCU → PC:  {tx_hex(r)}                      (44 03 确认)")

        # ---- 2b. LOAD_FLASH 'M' ----
        print()
        print("=" * 60)
        print(f"【步骤 2b】写 Flash — 发送 {len(data)} 字节数据")
        print("=" * 60)
        ser.write(b'\x4D')
        print(f"  PC → MCU:  4D                            ('M' 写Flash)")

        len_bytes = b'\x02\x00'
        ser.write(len_bytes)
        print(f"  PC → MCU:  02 00                         (数据长度 = 512)")

        ser.write(data)
        print(f"  PC → MCU:  {tx_hex(data[:8])} ... {tx_hex(data[-8:])}  (512字节数据)")
        print(f"             (前8字节)                    (后8字节)")

        r = ser.read(6)
        expected = b'\x4D\x00\x02' + struct.pack('>H', checksum) + b'\x03'
        if r == expected:
            print(f"  MCU → PC:  {tx_hex(r)}                      (写入成功, 校验和=0x{checksum:04X})")
        else:
            print(f"  MCU → PC:  {tx_hex(r)}                      (写入失败! 期望 {tx_hex(expected)})")
            break

        # ---- 2c. 回读验证 SET_ADDRESS + READ_FLASH ----
        print()
        print("=" * 60)
        print(f"【步骤 2c】读回验证 — 重新设地址, 读回整页对比")
        print("=" * 60)

        # 设地址
        ser.write(b'\x44' + addr_bytes)
        ser.read(2)
        print(f"  PC → MCU:  D + 地址 0x{offset:04X}        (重新设地址)")

        # 读 Flash
        ser.write(b'\x4E')
        print(f"  PC → MCU:  4E                            ('N' 读Flash)")
        r = ser.read(516)
        print(f"  MCU → PC:  4E 02 00 [512字节数据] 03     (收到 {len(r)} 字节)")

        if len(r) < 516:
            print(f"             !! 接收不完整, 期望 516 字节, 实际 {len(r)} 字节")
            break

        back_data = r[3:-1]
        errors = sum(1 for a, b in zip(back_data, data) if a != b)
        if errors == 0:
            print(f"             >> 逐字节比对: 全部 512 字节正确 <<")
        else:
            print(f"             !! {errors} / 512 字节不匹配 !!")
            for i, (a, b) in enumerate(zip(back_data, data)):
                if a != b:
                    print(f"             Byte[{i}]: 写入={b:02X}  读回={a:02X}")
                    if i >= 10:   # 最多打印前10个错误
                        print(f"             ... (共 {errors} 处差异)")
                        break

    ser.close()
    print()
    print("=" * 60)
    print("通讯结束")
    print("=" * 60)

if __name__ == '__main__':
    main()
