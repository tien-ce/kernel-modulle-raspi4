# Linux Kernel Modules — Raspberry Pi 4

A collection of Linux kernel module experiments for Raspberry Pi 4 (arm64, Linux 6.12), covering timer, memory-mapped I/O, and a full Modbus RTU driver stack with sysfs interface.

**Target**: Raspberry Pi 4, arm64, Linux 6.12  
**Cross-compiler**: `aarch64-linux-gnu-gcc`  
**Kernel source**: `~/linux_rasp-6.12/`

---

## Environment Setup

Source `arm64-env.sh` before building any module:

```bash
source arm64-env.sh
```

This sets `ARCH`, `CROSS_COMPILE`, `KERNEL_SRC`, and `KERNEL_OUT` for all cross-compilation targets.

---

## Modules Overview

| Folder | Module | Topic |
|--------|--------|-------|
| `07_timer` | `my_timer` | Periodic kernel timer |
| `08_timer_with_button` | `timer_button` | Debounced GPIO input via timer |
| `11_ledblink_by_mmio` | `hello_mmio` | Direct register access via MMIO |
| `17_modbus_rtu` | `modbus_controller_module` + `modbus_device_module` | Full Modbus RTU master over RS485 |
| `18_mod_sysfs` | `modbus_device_sysfs` | Modbus device driver with sysfs register interface |

---

## Timer

### 07 — Periodic Kernel Timer

A minimal module demonstrating the `timer_list` API. On load, schedules a callback that fires every 10 seconds and re-arms itself indefinitely using `mod_timer()`.

**Key APIs**: `timer_setup()`, `add_timer()`, `mod_timer()`, `del_timer_sync()`, `jiffies`

```
07_timer/
├── my_timer.c
└── Makefile
```

### 08 — Debounced Button with Timer

Extends the timer pattern to implement GPIO debouncing. A 10 ms periodic timer samples the button pin three times consecutively; a confirmed press toggles the LED output. No interrupts — pure polling in timer context.

**Key APIs**: `gpio_request()`, `gpio_direction_input/output()`, `gpio_get/set_value()`, `timer_list`

```
08_timer_with_button/
├── timer_button.c
└── Makefile
```

---

## Memory-Mapped I/O

### 11 — LED Blink by MMIO

Controls a GPIO pin by directly reading and writing the BCM2711 GPIO peripheral registers, bypassing the kernel GPIO subsystem entirely. Maps the physical register block at `0xfe200000` with `ioremap()` and manipulates `GPFSEL`, `GPSET`, and `GPCLR` registers via `readl()`/`writel()`.

Exposes a `/proc/lll-gpio` interface so userspace can trigger GPIO state changes without going through `/sys/class/gpio`.

**Key APIs**: `ioremap()`, `iounmap()`, `readl()`, `writel()`, `proc_create()`

**Reference**: [BCM2711 Peripherals Datasheet](https://datasheets.raspberrypi.com/bcm2711/bcm2711-peripherals.pdf)

> **Note**: Requires the SoC to be in *low peripherals mode*. See [this issue](https://github.com/raspberrypi/documentation/issues/2289) if the base address is wrong.

```
11_ledblink_by_mmio/
├── hello_mmio.c
└── Makefile
```

---

## Modbus RTU

### 17 — Full Modbus RTU Master

A complete Modbus RTU master stack running entirely in kernel space, built across two loadable modules:

```
User Space
┌───────────────────────────────────────────┐
│   read/write /dev/modbusclass/<device>    │
└──────────────────┬────────────────────────┘
                   │
Kernel Space
┌──────────────────▼────────────────────────┐
│  modbus_device_module.ko                  │
│  ├── Platform device driver (DT matching) │
│  ├── Character device file ops            │
│  └── Sysfs class /sys/class/modbusclass   │
└──────────────────┬────────────────────────┘
                   │  Modbus application layer
┌──────────────────▼────────────────────────┐
│  Modbus RTU Stack (modbus_rtu/)           │
│  ├── modbus.c  — application FSM          │
│  ├── mbrtu.c   — RTU frame assembly/CRC   │
│  └── LightModbus — PDU library (ported)   │
└──────────────────┬────────────────────────┘
                   │  Callback interface
┌──────────────────▼────────────────────────┐
│  modbus_controller_module.ko              │
│  ├── Serdev UART driver (RS485, 115200)   │
│  ├── hrtimer — T3.5 silence detection     │
│  └── Tasklet — deferred RX processing     │
└──────────────────┬────────────────────────┘
                   │
       RS485 hardware (UART2)
```

**Key features**:
- Serdev UART driver bound to the `modbus_controller` node in Device Tree
- `hrtimer` detects the Modbus T3.5 inter-frame silence
- Tasklet defers RX byte processing out of hard IRQ context
- `DEFINE_MUTEX(master_lock)` serializes concurrent userspace reads
- All slave addresses, registers, and permissions defined in the DTS overlay
- Device-managed allocations (`devm_*`) throughout

**Supported function code**: `0x03` — Read Holding Registers

**Device Tree** (`rs485_overlay.dts`):
```dts
&uart2 {
    modbus_controller {
        compatible = "serdev,modbus_controller";

        co_sensor@24 {
            reg = <0x24>;
            lsmy,reg-addresses = <0x0006 0x0100 0x0101>;
            lsmy,reg-names = "CO value", "Address", "Baudrate";
            lsmy,reg-perm = <1 1 1>;
        };
    };
};
```

**Usage**:
```bash
sudo insmod modbus_controller_module.ko
sudo insmod modbus_device_module.ko

cat /dev/modbusclass/co_sensor        # read all configured registers
echo "0x0064" > /dev/modbusclass/co_sensor  # write to a register

sudo rmmod modbus_device_module
sudo rmmod modbus_controller_module
```

For the full directory structure, build instructions, and protocol details see [`17_modbus_rtu/README.md`](17_modbus_rtu/README.md).

---

### 18 — Modbus Device Driver with sysfs Interface

Extends project 17's `modbus_device_module` by exposing each slave's registers as individual sysfs files instead of a single character device read. Each file is named from the DT `lsmy,reg-names` property and mode-controlled by `lsmy,reg-perm`.

```
/sys/class/modbusclass/<device>/
    interval_time          # polling interval in ms (read/write)
    Register-value/
        CO_value           # one file per lsmy,reg-names entry
        Address
        Baudrate
```

`interval_time` is readable and writable. Writes persist in the driver's private data and control how often the device should be polled by userspace.

**Private data retrieval in sysfs callbacks**: The `show`/`store` callbacks receive the sysfs `struct device *dev` — which is the device created by `device_create()`, not the platform device. The platform private data is on the parent:
```c
struct modev_private_data *dev_data = dev_get_drvdata(dev->parent);
```

**Per-register dynamic attributes**: At probe time, one `kobj_attribute` is allocated per DT register entry. Each wraps an index and a back-pointer to private data, recovered in callbacks via `container_of(attr, struct modev_reg_attr, kattr)`.

**Usage**:
```bash
sudo insmod modbus_controller_module.ko
sudo insmod modbus_device_sysfs.ko

cat /sys/class/modbusclass/co_sensor/CO_value
echo 500 > /sys/class/modbusclass/co_sensor/interval_time

sudo rmmod modbus_device_sysfs
sudo rmmod modbus_controller_module
```

For build and deploy instructions see [`18_mod_sysfs/README.md`](18_mod_sysfs/README.md).

---

## License

GPL v2

## Author

Văn Tiến — tien11102004@gmail.com
