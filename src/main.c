#include <common.h>
#include <scir.h>
#include <allocate.h>
#include <vm.h>

#ifdef STREETCAST_PLATFORM_PSP_OPTION_

PSP_MODULE_INFO("Streetcast", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

// Three functions that just enable the home button lol.
int pspCallbackExit() {
    sceKernelExitGame();
    return 0;
}

int pspCallbackThread() {
    i32 callback_id = sceKernelCreateCallback("Exit Callback", pspCallbackExit, NULL);

    sceKernelRegisterExitCallback(callback_id);
    sceKernelSleepThreadCB();

    return 0;
}

i32 pspCallbackSetup() {
    i32 thread_id = sceKernelCreateThread("Update Thread", pspCallbackThread, 10, 0xFFF, 0, 0);

    sceKernelStartThread(thread_id, 0, NULL);
    return thread_id;
}

#endif

// We don't need malloc where we're going.
u8 streetcast_memory[STREETCAST_ALLOCATION_SIZE_INT_OPTION_]; // For emulator operations
u8 dreamcast_memory[16777216 + 8388608 + 2097152];     // 26 MiB, for emulated games

int main() {
    #ifdef STREETCAST_PLATFORM_PSP_OPTION_
    pspCallbackSetup();
    pspDebugScreenInit();
    #endif

    AllocateBuffer root_buffer = {
        .base = streetcast_memory,
        .capacity = sizeof(streetcast_memory),
    };
    AllocateBumpState root_buffer_state = allocateBufferBumpStateBind(&root_buffer);

    AllocateBuffer scir_buffer = allocateBump(&root_buffer_state, 1024 + 512 + 2048 + 2048, 16);
    ScirBlock scir_block = scirBlockAllocate(&scir_buffer, 1024, 2048, 2048);
    ScirBlockAppendState scir_state = scirBlockAppendStateBind(&scir_block);

    u16 const_offset0 = scirBlockConstAppend(&scir_state, (u32[]){3, 8, 12}, 3);
    u16 op_offset0 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_IMMLOAD | SCIR_OP_TYPE_GENERAL | SCIR_OP_WIDTH_32, (u16[]){const_offset0}, 1);
    u16 op_offset1 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_IMMLOAD | SCIR_OP_TYPE_GENERAL | SCIR_OP_WIDTH_32, (u16[]){const_offset0 + 1}, 1);
    u16 op_offset2 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_IMMLOAD | SCIR_OP_TYPE_GENERAL, (u16[]){const_offset0 + 2}, 1);
    u16 op_offset3 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI | SCIR_OP_TYPE_GENERAL, (u16[]){op_offset0, op_offset1}, 2);
    scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI | SCIR_OP_TYPE_GENERAL, (u16[]){op_offset2, op_offset3}, 2);     

    const u16 op_count = scirBlockAppendStateOpCount(&scir_state);

    sc_printf("Lifetimes:\n");
    for (usize i = 0; i < op_count; ++i) {
        sc_printf("[%zd]: %d\n", i, scir_block.op_lifetime_array[i]);
    }

    AllocateBuffer op_degree_buffer = allocateBump(&root_buffer_state, op_count * 2, sizeof(u16));
    scirBlockOpDegreeDetermine(&scir_block, &op_degree_buffer, op_count);

    sc_printf("Registers:\n");
    for (usize i = 0; i < op_count; ++i) {
        sc_printf("[%zd]: %d\n", i, ((u16*)op_degree_buffer.base)[i]);
    }

    vmCompileScirBlock(&scir_block, (u16*)op_degree_buffer.base, op_count);

    #ifdef STREETCAST_PLATFORM_PSP_OPTION_
    sceKernelSleepThread();
    #endif

    exit(EXIT_SUCCESS);
}