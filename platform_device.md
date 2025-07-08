# Explanation of platform-related structs in linux/platform_device.h

The Linux kernel provides a flexible platform bus mechanism implemented in linux/platform_device.h. Platform devices and drivers are used to represent and bind embedded or SoC-specific hardware resources to their respective drivers.

---

## struct platform_device

The struct platform_device represents a single hardware device registered on the platform bus. It contains the device's identifying name, resources, and other metadata required to initialize it properly.

### Fields of struct platform_device:

- const char *name
  The device's name, used to match it with a compatible driver.

- int id
  An instance ID to distinguish between multiple identical devices.

- bool id_auto
  Flag indicating whether the ID was automatically assigned.

- struct device dev
  Embedded generic device structure, integrating the platform device into the Linux device model.

- u64 platform_dma_mask
  The DMA mask specifying which memory addresses the device can access.

- struct device_dma_parameters dma_parms
  Additional parameters for DMA configuration.

- u32 num_resources
  The number of hardware resources (I/O regions, interrupts) the device uses.

- struct resource *resource
  Pointer to an array of resource structures describing the hardware resources.

- const struct platform_device_id *id_entry
  Optional table of supported device IDs used for matching.

- const char *driver_override
  Optional field to forcibly bind this device to a specific driver, bypassing normal matching.

- struct mfd_cell *mfd_cell
  Pointer to the multi-function device cell if this is part of an MFD.

- struct pdev_archdata archdata
  Architecture-specific extensions for this device.

---

## struct platform_driver

The struct platform_driver represents the driver for one or more platform devices. It provides callbacks and a driver description to bind to compatible devices.

### Fields of struct platform_driver:

- int (*probe)(struct platform_device *)
  Callback invoked when a matching device is found. Initializes the device.

- int (*remove)(struct platform_device *)
  Callback invoked when the device is removed or the driver is unbound. Cleans up resources.

- void (*shutdown)(struct platform_device *)
  Optional callback for shutting down the device gracefully, e.g., during system halt.

- int (*suspend)(struct platform_device *, pm_message_t state)
  Callback for suspending the device for power management.

- int (*resume)(struct platform_device *)
  Callback for resuming the device after suspension.

- struct device_driver driver
  Embedded generic driver structure, linking this platform driver to the Linux driver model.

- const struct platform_device_id *id_table
  Optional table of supported device IDs for non-Device Tree platforms.

- bool prevent_deferred_probe
  If set, prevents deferred probe logic for this driver.

- bool driver_managed_dma
  Set by special drivers (like VFIO) to indicate that they manage their own DMA mappings instead of relying on the kernel DMA API.

---

## Notes

Platform devices are typically declared by firmware (Device Tree, ACPI) or registered at board setup.
Drivers and devices are matched by name, compatible, or id_table.
Drivers should implement at least probe and remove callbacks.

---
