# RegisterPowerStateChangeCallback

```c
NTSTATUS RegisterPowerStateChangeCallback(_Inout_ DEVICE_EXTENSION* pDeviceExtension)
{
#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " ---> RegisterPowerStateChangeCallback()\n");

#endif /// DBG
   
   NT_ASSERT(pDeviceExtension);

   NTSTATUS          status           = STATUS_SUCCESS;
   OBJECT_ATTRIBUTES objectAttributes = {0};
   UNICODE_STRING    unicodeString    = {0};

   RtlInitUnicodeString(&unicodeString,
                        L"\\Callback\\PowerState");

   InitializeObjectAttributes(&objectAttributes,
                              &unicodeString,
                              OBJ_CASE_INSENSITIVE,
                              0,
                              0);

   status = ExCreateCallback(&(pDeviceExtension->pCallbackObject),
                             &objectAttributes,
                             FALSE,                                  /// Do not create as the system should already have done this
                             TRUE);                                  /// Allow multiple callbacks
   if(status != STATUS_SUCCESS)
   {
      DbgPrintEx(DPFLTR_IHVNETWORK_ID,
                 DPFLTR_ERROR_LEVEL,
                 " !!!! RegisterPowerStateChangeCallback : ExCreateCallback() [status: %#x]\n",
                 status);

      HLPR_BAIL;
   }

   pDeviceExtension->pRegistration = ExRegisterCallback(pDeviceExtension->pCallbackObject,
                                                        PowerStateCallback,
                                                        pDeviceExtension);

   HLPR_BAIL_LABEL:

#if DBG
   
   DbgPrintEx(DPFLTR_IHVNETWORK_ID,
              DPFLTR_INFO_LEVEL,
              " <--- RegisterPowerStateChangeCallback() [status: %#x]\n",
              status);

#endif /// DBG
   
   return status;
}
```
