# Bus Filter Framework

## Usage

To include and link against [BFF](https://github.com/abysdom/bus-filter-framework) simply add the following snippet to your driver project:

```xml
  <ImportGroup Label="PropertySheets">
    <Import Project="BFF.props" />
  </ImportGroup>
```

**Important:** if a `PropertySheets` group already exists, append the Import-Node(s) to that to avoid linker issues!

Drop the provided `BFF.props` into the project directory and all include and linker settings will be available. Works for `KMDF` only.
