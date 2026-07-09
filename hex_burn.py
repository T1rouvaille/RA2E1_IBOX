"""
Intel HEX 烧录脚本 — RA2E1 Bootloader
用法: python hex_burn.py
"""
import serial
import struct
import os

# ============================================================
# 配置
# ============================================================
COM_PORT = 'COM19'
BAUD = 9600
HEX_FILE = 'Hawkeye_V0.1.hex'

APP_START = 0x00004000       # APP 起始地址
APP_END   = 0x00020000       # APP 结束地址
PAGE_SIZE = 512              # 每页字节数
ERASE_BLOCK = 2048           # 擦除块大小
OTA_FLAG_ADDR = 0x0001FFFC
CRC_ADDR = 0x0001FFF8

# ============================================================
# Intel HEX 解析
# ============================================================
def parse_hex(filepath):
    """解析 Intel HEX, 返回 {地址: 字节} 字典"""
    data = {}
    extended_addr = 0

    with open(filepath, 'r') as f:
        for line in f:
            line = line.strip()
            if not line.startswith(':'):
                continue

            byte_count = int(line[1:3], 16)
            address = int(line[3:7], 16)
            record_type = int(line[7:9], 16)

            if record_type == 0x00:         # 数据记录
                addr = extended_addr + address
                hex_data = line[9:9 + byte_count * 2]
                for i in range(byte_count):
                    data[addr + i] = int(hex_data[i * 2:i * 2 + 2], 16)

            elif record_type == 0x02:       # 扩展段地址
                extended_addr = int(line[9:13], 16) << 4

            elif record_type == 0x04:       # 扩展线性地址
                extended_addr = int(line[9:13], 16) << 16

            elif record_type == 0x01:       # EOF
                break

    return data

# ============================================================
# UART 通讯函数
# ============================================================
def set_address(ser, addr):
    """D 命令: 设地址 (一次性发送 D + 偏移量)"""
    ser.write(b'\x44')
    offset = addr - APP_START
    ser.write(bytes([(offset >> 8) & 0xFF, offset & 0xFF]))
    ser.read(2)  # 读 44 03

def write_page(ser, data):
    """M 命令: 写一页 512 字节, MCU 计算并回复校验和"""
    checksum = sum(data) & 0xFFFF
    ser.write(b'\x4D')
    ser.write(b'\x02\x00')
    ser.write(data)
    r = ser.read(6)
    expected = b'\x4D\x00\x02' + struct.pack('>H', checksum) + b'\x03'
    if r != expected:
        r_chk = (r[3] << 8) | r[4] if len(r) >= 5 else 0
        print(f"  ! 校验和: 本地=0x{checksum:04X}, MCU=0x{r_chk:04X}")
    return r == expected

def read_page(ser, addr):
    """N 命令: 读一页 512 字节"""
    set_address(ser, addr)
    ser.write(b'\x4E')
    r = ser.read(516)
    if len(r) < 516:
        return None
    return r[3:-1]

# ============================================================
# 主流程
# ============================================================
def main():
    # 检查 hex 文件
    if not os.path.exists(HEX_FILE):
        print(f"找不到文件: {HEX_FILE}")
        return

    # 解析 hex
    print(f"解析 {HEX_FILE} ...")
    hex_data = parse_hex(HEX_FILE)
    if not hex_data:
        print("HEX 文件为空!")
        return

    start_addr = min(hex_data.keys())
    end_addr = max(hex_data.keys())
    print(f"数据范围: 0x{start_addr:08X} - 0x{end_addr:08X}")

    # 筛选 APP 区域内的数据
    app_data = {k: v for k, v in hex_data.items() if APP_START <= k < APP_END}
    if not app_data:
        print("HEX 文件中没有 APP 区域数据!")
        return

    app_start = min(app_data.keys())
    app_end = max(app_data.keys())
    print(f"APP  范围: 0x{app_start:08X} - 0x{app_end:08X}")

    # 按页对齐
    first_page = (app_start - APP_START) // PAGE_SIZE * PAGE_SIZE
    last_page = (app_end - APP_START) // PAGE_SIZE * PAGE_SIZE
    total_pages = (last_page - first_page) // PAGE_SIZE + 1
    total_bytes = total_pages * PAGE_SIZE
    print(f"需要写入: {total_pages} 页 ({total_bytes / 1024:.1f} KB)")

    # 确认
    ans = input("\n开始烧录? (y/n): ")
    if ans.lower() != 'y':
        return

    # 打开串口
    ser = serial.Serial(COM_PORT, BAUD, timeout=5)
    print(f"\n串口 {COM_PORT} 已打开")

    # 握手
    ser.write(b'\x42')
    if ser.read(1) != b'\x42':
        print("握手失败! 检查连接和波特率.")
        ser.close(); return
    print("握手 OK")

    # ============================================================
    # 逐页烧录
    # ============================================================
    success = 0
    fail = 0
    last_erased_block = None

    for i, page in enumerate(range(first_page, last_page + 1, PAGE_SIZE)):
        addr = APP_START + page
        page_num = page // PAGE_SIZE
        pct = (i + 1) * 100 // total_pages

        # 检查是否需要擦除新块
        block = addr // ERASE_BLOCK
        will_erase = (addr % ERASE_BLOCK == 0)

        # 构建页面数据 (未定义的字节填 0xFF)
        buf = bytearray(PAGE_SIZE)
        for j in range(PAGE_SIZE):
            buf[j] = app_data.get(addr + j, 0xFF)

        # 设地址 + 写入
        set_address(ser, addr)
        ok = write_page(ser, bytes(buf))

        if ok:
            success += 1
            flag = "[擦除+写]" if will_erase else "[写]"
            print(f"  [{pct:3d}%] 页{page_num:3d}  0x{addr:08X}  {flag}  OK")
        else:
            fail += 1
            print(f"  [{pct:3d}%] 页{page_num:3d}  0x{addr:08X}  写入失败!")
            ans = input("  继续? (y/n): ")
            if ans.lower() != 'y':
                break

    print(f"\n写入完成: {success} 页成功, {fail} 页失败")

    # ============================================================
    # 回读验证
    # ============================================================
    print("\n" + "=" * 60)
    print("回读验证...")
    print("=" * 60)

    verify_ok = 0
    verify_err = 0

    for i, page in enumerate(range(first_page, last_page + 1, PAGE_SIZE)):
        addr = APP_START + page
        page_num = page // PAGE_SIZE
        pct = (i + 1) * 100 // total_pages

        # 期望数据
        expected = bytearray(PAGE_SIZE)
        for j in range(PAGE_SIZE):
            expected[j] = app_data.get(addr + j, 0xFF)

        # 读回
        actual = read_page(ser, addr)
        if actual is None:
            verify_err += 1
            print(f"  [{pct:3d}%] 页{page_num:3d}  回读失败")
            continue

        errors = sum(1 for a, b in zip(actual, expected) if a != b)
        if errors == 0:
            verify_ok += 1
        else:
            verify_err += 1
            print(f"  [{pct:3d}%] 页{page_num:3d}  {errors} 字节不匹配")

        if (i + 1) % 10 == 0:
            print(f"  [{pct:3d}%] 已验证 {i+1}/{total_pages} 页 ...")

    print(f"\n验证完成: {verify_ok} 页正确, {verify_err} 页错误")

    # ============================================================
    # 写入 OTA 标志位 (0x55555555 @ 0x0001FFFC)
    # ============================================================
    if fail == 0 and verify_err == 0:
        print("\n" + "=" * 60)
        print("写入 OTA 标志位 (0x55555555 @ 0x0001FFFC)...")
        print("=" * 60)

        ota_page_addr = OTA_FLAG_ADDR & ~(PAGE_SIZE - 1)  # 页对齐: 0x0001FE00
        ota_offset = OTA_FLAG_ADDR - ota_page_addr        # 页内偏移: 508

        # 构建页面数据 (全部 0xFF, 仅 OTA 位置写 0x55555555)
        ota_buf = bytearray([0xFF] * PAGE_SIZE)
        ota_buf[ota_offset:ota_offset + 4] = b'\x55\x55\x55\x55'

        set_address(ser, ota_page_addr)
        ok = write_page(ser, bytes(ota_buf))
        if ok:
            print(f"  OTA 标志位写入成功 (页 0x{ota_page_addr:08X}, 偏移 {ota_offset})")
            # 验证 OTA 标志位
            actual = read_page(ser, ota_page_addr)
            if actual is not None:
                flag_bytes = actual[ota_offset:ota_offset + 4]
                if flag_bytes == b'\x55\x55\x55\x55':
                    print(f"  OTA 标志位验证: OK (0x55555555)")
                else:
                    print(f"  OTA 标志位验证: 失败! 读回={flag_bytes.hex().upper()}")
        else:
            print("  OTA 标志位写入失败!")

    # ============================================================
    # 总结
    # ============================================================
    print()
    print("=" * 60)
    if fail == 0 and verify_err == 0:
        print(">> 烧录成功! 所有页面写入正确 <<")
    else:
        print(">> 烧录有错误, 请检查 <<")
    print("=" * 60)

    # ============================================================
    # 跳转 APP
    # ============================================================
    if fail == 0 and verify_err == 0:
        ans = input("\n发送 NACK (0x01) 跳转至 APP? (y/n): ")
        if ans.lower() == 'y':
            ser.write(b'\x01')
            print("已发送 0x01, MCU 正在跳转至 APP ...")
    else:
        input("\n按回车键退出...")

    ser.close()

if __name__ == '__main__':
    main()
