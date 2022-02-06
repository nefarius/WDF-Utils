# USB Device Emulation (UDE)

## Usage

To include and link against [UDE](https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-emulated-device--ude--architecture) simply add the following snippet to your driver project:

```xml
  <ImportGroup Label="PropertySheets">
    <Import Project="UDE.props" />
  </ImportGroup>
```

Drop the provided `UDE.props` into the project directory and all include and linker settings will be available. Works for `KMDF` only.
