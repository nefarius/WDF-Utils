# Driver Module Framework (DMF)

**Important:** this has been merged [upstream](https://github.com/Microsoft/DMF/blob/master/Dmf/Documentation/Driver%20Module%20Framework.md#simplifying-compilation-and-linking-with-dmf), yay ❤️

## Usage

To include and link against [DMF](https://github.com/microsoft/DMF) simply clone DMF to the same project parent directory **or** set the environment variable `DmfRootPath` to the DMF directory. Then add the following snippet to your driver project:

```xml
  <ImportGroup Label="PropertySheets">
    <Import Project="DMF.props" />
  </ImportGroup>
```

**Important:** if a `PropertySheets` group already exists, append the Import-Node(s) to that to avoid linker issues!

Drop the provided `DMF.props` into the project directory and all include and linker settings will be available. Works for both `KMDF` and `UMDF`.

Don't forget to build DMF at least once for all configurations and architectures your driver project uses, otherwise you're missing the required libraries to link against!
