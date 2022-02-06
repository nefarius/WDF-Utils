# Driver Module Framework (DMF)

## Usage

To include and link against [DMF](https://github.com/microsoft/DMF) simply clone DMF to the same project parent directory and add the following snippet to your driver project:

```xml
  <ImportGroup Label="PropertySheets">
    <Import Project="DMF.props" />
  </ImportGroup>
```

Drop the provided `DMF.props` into the project directory and all include and linker settings will be available. Works for both `KMDF` and `UMDF`.
