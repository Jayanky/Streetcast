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

    // Intentionally overloading the register allocator
    u16 op0 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_LOADIMMI, NULL, 0, (u32[]){25}, 1);
    u16 op1 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_LOADIMMI, NULL, 0, (u32[]){44}, 1);
    u16 op2 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_LOADIMMI, NULL, 0, (u32[]){44}, 1);
    u16 op3 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_LOADIMMI, NULL, 0, (u32[]){44}, 1);
    u16 op4 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_LOADIMMI, NULL, 0, (u32[]){44}, 1);
    u16 op5 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_LOADIMMI, NULL, 0, (u32[]){44}, 1);
    u16 op6 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_LOADIMMI, NULL, 0, (u32[]){44}, 1);
    u16 op7 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_LOADIMMI, NULL, 0, (u32[]){44}, 1);
    u16 op8 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_LOADIMMI, NULL, 0, (u32[]){44}, 1);
    u16 op9 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_LOADIMMI, NULL, 0, (u32[]){44}, 1);
    u16 op10 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_LOADIMMI, NULL, 0, (u32[]){44}, 1);
    u16 op11 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_LOADIMMI, NULL, 0, (u32[]){44}, 1);


    u16 op12 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op11, op10, op9, op8, op7, op6, op5, op4, op3, op2, op1, op0}, 12, NULL, 0);
    u16 op13 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_STORE, (u16[]){op12}, 1, (u32[]){(uptr)main_dreamcast_memory}, 1);
    
    vmCompileScirBlock(&scir_block, scir_state.op_code_array_position);

    sc_printf("main_dreamcast_memory[0]\n%d\n", ((u32*)main_dreamcast_memory)[0]);

    #ifdef SC_PLATFORM_PSP_OPTION_

    sceKernelSleepThread();

    #endif

    exit(EXIT_SUCCESS);
}