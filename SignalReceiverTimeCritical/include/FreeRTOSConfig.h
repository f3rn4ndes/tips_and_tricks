#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#define configUSE_PREEMPTION 0
#define configUSE_IDLE_HOOK 0     // Disable the idle hook
#define configUSE_TICK_HOOK 0     // Disable the tick hook
#define configTICK_RATE_HZ (1000) // Adjust the tick rate as necessary
#define configMAX_PRIORITIES (25) // Ensure you have enough priority levels
#define configMINIMAL_STACK_SIZE ((unsigned short)130)
#define configTOTAL_HEAP_SIZE ((size_t)(16 * 1024))
#define configMAX_TASK_NAME_LEN (16)
#define configUSE_TRACE_FACILITY 1
#define configUSE_16_BIT_TICKS 0
#define configIDLE_SHOULD_YIELD 1

// Optional: Define these if not already defined
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configSUPPORT_STATIC_ALLOCATION 0

// Hook function related definitions
#define configCHECK_FOR_STACK_OVERFLOW 2
#define configUSE_MALLOC_FAILED_HOOK 1

#endif /* FREERTOS_CONFIG_H */
