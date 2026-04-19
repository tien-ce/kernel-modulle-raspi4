# 18 — Modbus Device Driver with sysfs Interface

Platform driver that matches Modbus device nodes from the Device Tree, allocates a character device per matched device, and exposes each device's registers as individual sysfs files — named and permission-controlled via DT properties.

Builds on project 17 by adding sysfs on top of the existing cdev interface.

| Item | Detail |
|---|---|
| Module | `modbus_device_sysfs` |
| Target | Raspberry Pi 4 (arm64, Linux 6.12) |
| sysfs class | `/sys/class/modbusclass/` |
| Character devices | `/dev/<dt-node-name>` |

After loading, each matched device exposes:

```
/sys/class/modbusclass/<device>/
    interval_time
    Register-value/
        <reg-name-0>     # one file per lsmy,reg-names entry
        <reg-name-1>
        ...
```

Register file names, addresses, and permissions (`RD_ONLY=0x01`, `WR_ONLY=0x10`, `RD_WR=0x11`) are all read from the Device Tree properties `lsmy,reg-names`, `lsmy,reg-addresses`, and `lsmy,reg-perm`.

---

## File Structure

```
18_mod_sysfs/
├── modbusdevice_sysfs.c      # Platform driver: probe/remove, sysfs setup
├── modbusdevice_sysfs.h      # Shared structs, macros, and prototypes
├── modbusdevice_syscalls.c   # File operations: open/read/write/llseek/release
└── Makefile
```

---

## Build & Deploy

```bash
# Cross-compile for Raspberry Pi 4
make

# Compile Device Tree overlay
make dtb

# Push module to the board
make install

# On the Raspberry Pi
sudo dtoverlay mod_sysfs_overlay
sudo insmod modbus_device_sysfs.ko

# Remove
sudo rmmod modbus_device_sysfs
```
