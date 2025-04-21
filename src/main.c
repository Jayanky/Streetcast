#include <defines.h>
#include <common.h>
#include <scir.h>
#include <allocate.h>
#include <vm.h>

// We don't need malloc where we're going.
static u8 main_scir_block_memory[SC_SCIR_BLOCK_SIZE_INT_]; // For holding IR during compilation

#ifdef SC_PLATFORM_PSP_OPTION_

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

int main() {
    #ifdef SC_PLATFORM_PSP_OPTION_

    pspCallbackSetup();
    pspDebugScreenInit();

    #endif

    AllocateBuffer scir_block_buffer = {
        .base = main_scir_block_memory,
        .capacity = sizeof(main_scir_block_memory),
    };

    ScirBlock scir_block = scirBlockAllocate(&scir_block_buffer, SC_SCIR_BLOCK_OP_ARRAY_SIZE_INT_, SC_SCIR_BLOCK_USE_ARRAY_SIZE_INT_, SC_SCIR_BLOCK_CONST_ARRAY_SIZE_INT_);
    ScirBlockAppendState scir_state = scirBlockAppendStateBind(&scir_block);

    u16 const_offset0 = scirBlockConstAppend(&scir_state, (u32[]){3, 8, 12}, 3);
    u16 op_offset0 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_IMMLOAD |  SCIR_OP_WIDTH_32, (u16[]){const_offset0}, 1);
    u16 op_offset1 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_IMMLOAD |  SCIR_OP_WIDTH_32, (u16[]){const_offset0 + 1}, 1);
    u16 op_offset2 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_IMMLOAD | SCIR_OP_WIDTH_32, (u16[]){const_offset0 + 2}, 1);
    u16 op_offset3 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op_offset0, op_offset1}, 2);
    scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op_offset2, op_offset3}, 2);     

    const u16 op_count = scirBlockAppendStateOpElements(&scir_state);

    sc_printf("Lifetimes:\n");
    for (usize i = 0; i < op_count; ++i) {
        sc_printf("[%zd]: %d\n", i, scir_block.op_lifetime_array[i]);
    }

    vmScirBlockCompile(&scir_block, op_count);

    #ifdef SC_PLATFORM_PSP_OPTION_

    sceKernelSleepThread();

    #endif

    exit(EXIT_SUCCESS);
}