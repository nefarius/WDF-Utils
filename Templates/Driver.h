/* The following snippet in your Driver.h gets all DMF components included */

#pragma warning(disable:5040)
#include <DmfModules.Library.h>
#pragma warning(default:5040)

#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>
#include <ntstrsafe.h>

// the rest of your includes