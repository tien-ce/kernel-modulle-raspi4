# Modbus RTU Linux Kernel Driver (Raspberry Pi 4)

A Linux kernel driver implementing a complete **Modbus RTU master** over RS485 for Raspberry Pi 4.
The full protocol stack runs in kernel space. Each connected slave is exposed as a character device
and a sysfs node. No userspace Modbus library is needed.

License: MIT · Target: ARM64, kernel 6.12

---

## Architecture

Two loadable kernel modules communicate through a shared callback interface:

```
User Space
┌──────────────────────────────────────────────────────────┐
│  /dev/co_sensor  /dev/pm_sensor                          │
│  /sys/class/modbusclass/<device>/...                     │
└───────────────────────────┬──────────────────────────────┘
                            │
┌───────────────────────────▼──────────────────────────────┐
│  modbus_device_module.ko                                 │
│  Platform device driver · char device · sysfs            │
└───────────────────────────┬──────────────────────────────┘
                            │  ModbusSend() / ModbusReceive()
┌───────────────────────────▼──────────────────────────────┐
│  modbus_controller_module.ko                             │
│  Serdev UART driver · Modbus RTU FSM · hrtimer (T3.5)    │
└───────────────────────────┬──────────────────────────────┘
                            │  UART2 / RS485
                     ┌──────┴──────┐
                     │  CO Sensor  │  │  PM Sensor  │
                     └─────────────┘  └─────────────┘
```

---

## Directory Structure

```
17_modbus_rtu/
├── Kconfig                      # menuconfig options
├── Kconfig.local                # Standalone config wrapper
├── Makefile                     # Dual-mode build rules
├── rs485_overlay.dts            # Device Tree overlay
├── modbus_controller/           # --- Controller Module ---
│   ├── modbus_controller.h      # Shared API
│   ├── modbuscontroller.c       # Serdev UART driver
│   ├── modbuscontroller_timer.c # hrtimer wrapper
│   └── modbus_rtu/              # --- Protocol Layer ---
│       ├── modbus.c             # Orchestration & Exported Symbols
│       ├── mbrtu.c              # RTU State Machine
│       ├── Include/             # Protocol headers
│       └── lightmodbus/         # LightModbus PDU library
└── modbus_device/               # --- Device Module ---
    ├── modbusdevice.c           # Platform device driver
    ├── modbusdevice_syscalls.c  # Syscalls & sysfs
    └── modbusdevice_sysfs.h     # Shared structs
```

---

## Build

**Prerequisites**
```bash
sudo apt install gcc-aarch64-linux-gnu device-tree-compiler
```
Kernel source must be at `~/linux_rasp-6.12/` (configure `KERNEL_SRC` in Makefile if different).

**Configuration**
You can configure the module features using the standard Linux menuconfig interface:
```bash
make menuconfig
```
In the menu:
1. Select **Modbus RTU Master Support** (press `M` to build as modules).
2. Enter the menu to configure specific features:
   - **Modbus RTU Controller Driver**: Select physical interface (UART/USB).
   - **Modbus RTU Device Support**: Enable sensor types.
   - **PM Sensor Options**: Enable **Include PM10 value register** if your sensor supports it.

**Compile**
```bash
make       # Builds both modules recursively
make dtb   # Device Tree overlay
make clean
```

Outputs: 
- `modbus_controller/modbus_controller_module.ko`
- `modbus_device/modbus_device_module.ko`
- `rs485_overlay.dtbo`

**Deploy to Raspberry Pi**
```bash
make install   # uses push_rpi alias, adjust to your setup
```

---

## Device Tree Configuration

The overlay plugs into UART2. The controller reads baudrate from `lsmy,baudrate`.
Each child node becomes one sensor device.

```dts
&uart2 {
    modbus_controller {
        compatible = "serdev,modbus_controller";
        #address-cells = <1>;
        #size-cells = <0>;
        lsmy,baudrate = <9600>;

        co_sensor@1 {
            status = "okay";
            compatible = "rs485,co_sensor";
            reg = <0x1>;                          /* Modbus slave address */
            lsmy,reg-addresses = <0x0006>;        /* Holding register to read */
        };

        pm_sensor@24 {
            compatible = "rs485,pm_sensor";
            reg = <0x24>;
            lsmy,reg-addresses = <0x0004 0x0009>; /* PM1.0, PM2.5 (add 3rd for PM10) */
        };
    };
};
```

Apply on the target:
```bash
sudo dtoverlay rs485_overlay.dtbo
```

---

## Loading and Unloading

Controller must be loaded first — device module depends on its exported symbols.

```bash
# Load
sudo insmod modbus_controller/modbus_controller_module.ko
sudo insmod modbus_device/modbus_device_module.ko

# Unload (reverse order)
sudo rmmod modbus_device_module
sudo rmmod modbus_controller_module
```

---

## User-Space Usage

See **USERGUIDE.txt** in this directory for the full interface reference.

Quick reference — sysfs:
```bash
cat /sys/class/modbusclass/co_sensor/co_value
cat /sys/class/modbusclass/pm_sensor/pm2_5_value
echo 2000 | sudo tee /sys/class/modbusclass/co_sensor/interval_time
```

For character device programming and full examples, see the companion user-space repository:
https://github.com/tien-ce/Linux---rasp-user-space/tree/main/05_device_modbus

---

## Author

Văn Tiến — tien11102004@gmail.com
