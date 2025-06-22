
void _start()
{
    for (;;)
        __asm__ volatile ("int $0x81" : : "a" (0x00), "b"("Hello, World! from executable B"));
}
