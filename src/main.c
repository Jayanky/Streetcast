#include <defines.h>
#include <common.h>
#include <scir.h>
#include <allocate.h>
#include <vm.h>

// We don't need malloc where we're going.
static u8 main_dreamcast_memory[SC_DC_MEMORY_SIZE_INT_];     // 26 MiB, for emulated games
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

    ScirBlock scir_block = scirBlockAllocate(&scir_block_buffer, SC_SCIR_BLOCK_MAX_OPS_INT_, SC_SCIR_BLOCK_MAX_USES_INT_, SC_SCIR_BLOCK_MAX_CONSTS_INT_);
    ScirBlockAppendState scir_state = scirBlockAppendStateBind(&scir_block);

    u16 op0 = scirBlockOpAppendLoadimmi(&scir_state, 25);
    u16 op1 = scirBlockOpAppendLoadimmi(&scir_state, 44);
    u16 op2 = scirBlockOpAppendAddimmi(&scir_state, op0, 50);
    u16 op3 = scirBlockOpAppendAddi(&scir_state, op2, op1);
    u16 op4 = scirBlockOpAppendLoadimmi(&scir_state, (uptr)main_dreamcast_memory);
    u16 op5 = scirBlockOpAppendStore(&scir_state, op3, op4, 0);
    
    vmCompileScirBlock(&scir_block, scir_state.op_code_array_position);

    scPrintf("main_dreamcast_memory[0]\n%d\n", ((u32*)main_dreamcast_memory)[0]);

    #ifdef SC_PLATFORM_PSP_OPTION_

    sceKernelSleepThread();

    #endif

    exit(EXIT_SUCCESS);
}