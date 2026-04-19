# Modbus RTU Linux Kernel Module (Raspberry Pi 4)

A Linux kernel module implementing a complete **Modbus RTU master** stack over RS485 for Raspberry Pi 4. The driver runs entirely in kernel space, providing low-latency, deterministic communication with Modbus RTU slave devices (sensors, PLCs, controllers).

---

## Architecture

The project is split into two loadable kernel modules that communicate through a shared callback interface:

```
User Space
┌─────────────────────────────────────────────┐
│  read/write /dev/modbusclass/<device>       │
└────────────────────┬────────────────────────┘
                     │
Kernel Space
┌────────────────────▼────────────────────────┐
│  modbus_device_module.ko                    │
│  ├── Platform device driver                 │
│  ├── Character device file ops              │
│  └── Sysfs interface (/sys/class/modbusclass) │
└────────────────────┬────────────────────────┘
                     │  Modbus App Layer
┌────────────────────▼────────────────────────┐
│  Modbus RTU Stack (modbus_rtu/)             │
│  ├── Application state machine (modbus.c)  │
│  ├── RTU frame assembly/CRC (mbrtu.c)      │
│  └── LightModbus PDU library               │
└────────────────────┬────────────────────────┘
                     │  Callback interface
┌────────────────────▼────────────────────────┐
│  modbus_controller_module.ko                │
│  ├── Serdev UART driver (RS485)             │
│  ├── hrtimer for T3.5 silence detection     │
│  └── Tasklet-based RX event processing     │
└────────────────────┬────────────────────────┘
                     │
         RS485 Hardware (UART2)
```

### Key Modules

| Module | Source | Role |
|--------|--------|------|
| `modbus_controller_module.ko` | `modbuscontroller.c`, `modbuscontroller_timer.c`, `modbus_rtu/` | Serdev UART driver + Modbus RTU protocol stack |
| `modbus_device_module.ko` | `modbusdevice.c`, `modbusdevice_syscalls.c` | Platform device driver, char device, sysfs |

---

## Features

- **Full Modbus RTU master** in kernel space — no userspace Modbus library needed
- **Serdev driver** for UART/RS485 (115200 baud, no parity, no flow control)
- **hrtimer**-based T3.5 inter-frame silence detection
- **Tasklet** for deferred RX processing (no blocking in hard IRQ context)
- **Character device** per slave (`/dev/modbusclass/<device>`)
- **Device Tree driven** — slave addresses, registers, and permissions defined in DTS
- **Dynamic device discovery** via `devm_of_platform_populate()`
- **Thread-safe** access with `DEFINE_MUTEX(master_lock)`
- **CRC16-CCITT** frame validation
- Cross-compiled for ARM64, kernel 6.12

---

## Directory Structure

```
17_modbus_rtu/
├── Makefile                     # Build configuration
├── rs485_overlay.dts            # Device Tree overlay
├── modbuscontroller.c           # Serdev UART driver
├── modbuscontroller.h
├── modbuscontroller_timer.c     # hrtimer implementation
├── modbusdevice.c               # Platform device driver
├── modbusdevice.h
├── modbusdevice_syscalls.c      # File operations (open/read/write/llseek)
├── modbus_rtu/
│   ├── modbus.c                 # Modbus application state machine
│   ├── modbus.h
│   ├── mbrtu.c                  # RTU frame layer
│   ├── mbrtu.h
│   ├── mbcrc.c                  # CRC16 calculation
│   ├── mbcrc.h
│   ├── port_event.c             # Tasklet event queuing
│   ├── port_timer.c             # Timer abstraction layer
│   └── port.h
└── lightmodbus/                 # LightModbus library (PDU layer, kernel-ported)
```

---

## Prerequisites

### Host (cross-compilation)

- `aarch64-linux-gnu-gcc` cross-compiler
- Linux kernel source for Raspberry Pi (`~/linux_rasp-6.12/`)
- `dtc` (device tree compiler)

Install on Ubuntu/Debian:
```bash
sudo apt install gcc-aarch64-linux-gnu device-tree-compiler
```

### Target (Raspberry Pi 4)

- Raspberry Pi 4 running Linux 6.12 (arm64)
- RS485 transceiver connected to UART2
- `insmod`/`modprobe` access (root)

---

## Build

```bash
# Build kernel modules
make all

# Compile Device Tree overlay
make dtb

# Clean build artifacts
make clean

# Push modules and DTB to Raspberry Pi (via scp)
make install

# Build for host system (testing)
make host
```

Build outputs:
- `modbus_controller_module.ko`
- `modbus_device_module.ko`
- `rs485_overlay.dtbo`

---

## Device Tree Configuration

The `rs485_overlay.dts` overlay defines the controller and slave devices under UART2:

```dts
&uart2 {
    modbus_controller {
        compatible = "serdev,modbus_controller";

        co_sensor@24 {
            reg = <0x24>;                          /* Slave address */
            lsmy,reg-addresses = <0x0006 0x0100 0x0101>;
            lsmy,reg-names = "CO value", "Address", "Baudrate";
            lsmy,reg-perm = <1 1 1>;               /* 1=read, 2=write */
        };
    };
};
```

**Properties**:

| Property | Description |
|----------|-------------|
| `reg` | Modbus slave address (1–247) |
| `lsmy,reg-addresses` | Holding register offsets to access |
| `lsmy,reg-names` | Human-readable names for each register |
| `lsmy,reg-perm` | Per-register permissions (`1` = read, `2` = write) |

Load the overlay on the target:
```bash
sudo dtoverlay rs485_overlay.dtbo
```

---

## Usage

### Load modules

```bash
sudo insmod modbus_controller_module.ko
sudo insmod modbus_device_module.ko
```

### Access slave registers

Each slave device appears as a character device under `/dev/modbusclass/`:

```bash
# Read all configured registers from a slave
cat /dev/modbusclass/co_sensor

# Write a value to a writable register
echo "0x0064" > /dev/modbusclass/co_sensor
```

### Sysfs interface (18_mod_sysfs variant)

An enhanced version with per-register sysfs attributes is available in `18_mod_sysfs/`:

```bash
# Read individual registers
cat /sys/class/modbusclass/co_sensor/CO_value
cat /sys/class/modbusclass/co_sensor/Address

# Configure polling interval (ms)
echo 500 > /sys/class/modbusclass/co_sensor/interval_time
```

### Unload modules

```bash
sudo rmmod modbus_device_module
sudo rmmod modbus_controller_module
```

---

## Protocol Details

### Modbus RTU Frame Format

```
┌──────────────┬───────────────┬──────────────────┬─────────────┐
│ Slave Addr   │ Function Code │ Data             │ CRC16       │
│ 1 byte       │ 1 byte        │ 0–252 bytes      │ 2 bytes     │
└──────────────┴───────────────┴──────────────────┴─────────────┘
```

- **Supported function code**: `0x03` — Read Holding Registers
- **Frame delimiter**: T3.5 silence (3.5 character intervals at configured baud rate), detected via hrtimer
- **Error detection**: CRC16-CCITT

### Finite State Machine

**RX states**: `RX_INIT` → `RX_IDLE` → `RX_RCV` → `RX_ERROR`

**TX states**: `TX_IDLE` → `TX_XMIT`

The FSM is driven by serial RX callbacks and timer expiry events, processed in tasklet context.

---

## Implementation Notes

- **Tasklet RX processing**: Serial receive interrupts enqueue bytes and schedule a tasklet; `ModbusRun()` FSM executes in soft-IRQ context, avoiding blocking in the hard IRQ handler.
- **Mutex serialization**: `master_lock` serializes concurrent `read()` calls from multiple user processes.
- **Device-managed allocation**: All allocations use `devm_kzalloc()` / `devm_kcalloc()` for automatic cleanup on driver unbind.
- **LightModbus**: A header-only Modbus PDU library adapted for kernel space (no stdlib, no dynamic memory at parse time).

---

## License

GPL v2

---

## Author

Văn Tiến — tien11102004@gmail.com
