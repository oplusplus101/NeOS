
#include <KNeOS.h>

INT IOCreate(sDriverObject *pObject, sIORequestPacket *pIRP)
{
    KNeoPrintString("Create signal received!\n");
    return 0;
}

INT DriverMain(sDriverObject *pObject)
{
    pObject->arrIOHandlers[IO_CREATE] = IOCreate;

    sDeviceObject *pDevice;
    KNeoCreateDevice(pObject, L"Devices\\TEST2", 0, &pDevice);
    
    return 0;
}
