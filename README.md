# iousbpoc
### PoC for a OS X kernel vulnerability in IOUSBFamily

This bug was patched in High Sierra 10.13.2, but at the time of discovery, the source code for IOUSBFamily still hadn't been updated, which is how I was able to find it.

The bug actually exists in two different classes: `IOUSBInterfaceUserClientV2` and `IOUSBDeviceUserClientV2`, both containing the vulnerable method `SetAsyncPort`. This PoC targets `IOUSBInterfaceUserClientV2::SetAsyncPort`, but I could have just as easily targeted `IOUSBDeviceUserClientV2::SetAsyncPort`.

Let's take a look at this vulnerable method:

```
IOReturn IOUSBInterfaceUserClientV2::SetAsyncPort(mach_port_t port)
{
	USBLog(6,"+IOUSBInterfaceUserClientV2[%p]::SetAsyncPort %p", this, port);
	if (fWakePort != MACH_PORT_NULL)
	{
		super::releaseNotificationPort(fWakePort);
		fWakePort = MACH_PORT_NULL;
	}
	
    if (!fOwner)
        return kIOReturnNotAttached;
	
    fWakePort = port;
    return kIOReturnSuccess;
}
```

In a single-threaded application, this method would be totally valid. MIG automatically increments the refcount of incoming mach_ports so IOUSBInterfaceUserClientV2 will always hold a reference on `fWakePort` (an instance property). The old port is released before being overriden, preventing reference leaks.

The kernel is, however, not a single-threaded application, and this method is not thread-safe. There is no locking implemented in this method, meaning two threads in parallel are able to race to both drop a ref on `fWakePort` (via `releaseNotificationPort`) before its value is set to `MACH_PORT_NULL`. This means it's possible to drop 2 refs on a mach port when only 1 was held, which could lead to a UaF due to the userclient holding a dangling pointer on `fWakePort`.

`IOUSBInterfaceUserClientV2::SetAsyncPort` is accessible via IOUSBDeviceUserClientV2's external method 22, with your target port as `wake_port`. This PoC aims to over-release a mach port, triggering a kernel panic, but it is theoretically possible to exploit this UaF to gain arbitrary code execution with kernel privileges.

If you have any questions about this bug, feel free to drop me a message on twitter [@Muirey03](https://twitter.com/Muirey03).