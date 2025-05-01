
void _start()
{
    __asm__ volatile ("int $0x81" : : "a" (0x00), "b"("Hello, World!"));
}

