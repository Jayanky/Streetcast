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
    u16 op0 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_LOADIMM, NULL, 0, (u32[]){25}, 1);
    u16 op1 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_LOADIMM, NULL, 0, (u32[]){44}, 1);

    u16 op2 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op0, op1}, 2, NULL, 0);

    u16 op3 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op0, op2}, 2, NULL, 0);
    u16 op4 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op1, op3}, 2, NULL, 0);

    u16 op5 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op0, op4}, 2, NULL, 0);
    u16 op6 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op1, op5}, 2, NULL, 0);
    u16 op7 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op2, op6}, 2, NULL, 0);

    u16 op8  = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op0, op7 }, 2, NULL, 0);
    u16 op9  = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op1, op8 }, 2, NULL, 0);
    u16 op10 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op2, op9 }, 2, NULL, 0);
    u16 op11 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op3, op10}, 2, NULL, 0);

    u16 op12 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op0, op11}, 2, NULL, 0);
    u16 op13 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op1, op12}, 2, NULL, 0);
    u16 op14 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op2, op13}, 2, NULL, 0);
    u16 op15 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op3, op14}, 2, NULL, 0);
    u16 op16 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op4, op15}, 2, NULL, 0);

    u16 op17 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op0, op16}, 2, NULL, 0);
    u16 op18 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op1, op17}, 2, NULL, 0);
    u16 op19 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op2, op18}, 2, NULL, 0);
    u16 op20 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op3, op19}, 2, NULL, 0);
    u16 op21 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op4, op20}, 2, NULL, 0);
    u16 op22 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op5, op21}, 2, NULL, 0);

    u16 op23 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op0, op22}, 2, NULL, 0);
    u16 op24 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op1, op23}, 2, NULL, 0);
    u16 op25 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op2, op24}, 2, NULL, 0);
    u16 op26 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op3, op25}, 2, NULL, 0);
    u16 op27 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op4, op26}, 2, NULL, 0);
    u16 op28 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op5, op27}, 2, NULL, 0);
    u16 op29 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op6, op28}, 2, NULL, 0);

    u16 op30 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op0, op29}, 2, NULL, 0);
    u16 op31 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op1, op30}, 2, NULL, 0);
    u16 op32 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op2, op31}, 2, NULL, 0);
    u16 op33 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op3, op32}, 2, NULL, 0);
    u16 op34 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op4, op33}, 2, NULL, 0);
    u16 op35 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op5, op34}, 2, NULL, 0);
    u16 op36 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op6, op35}, 2, NULL, 0);
    u16 op37 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op7, op36}, 2, NULL, 0);

    u16 op38 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op8, op37}, 2, NULL, 0);
    u16 op39 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op7, op38}, 2, NULL, 0);
    u16 op40 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op6, op39}, 2, NULL, 0);
    u16 op41 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op5, op40}, 2, NULL, 0);
    u16 op42 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op4, op41}, 2, NULL, 0);
    u16 op43 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op3, op42}, 2, NULL, 0);
    u16 op44 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op2, op43}, 2, NULL, 0);
    u16 op45 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op1, op44}, 2, NULL, 0);
    u16 op46 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_ADDI, (u16[]){op0, op45}, 2, NULL, 0);

    u16 op47 = scirBlockOpAppend(&scir_state, SCIR_OP_CODE_STORE, (u16[]){op46}, 1, (u32[]){(usize)main_dreamcast_memory}, 1);

    
    vmCompileScirBlock(&scir_block, scir_state.op_code_array_position);

    sc_printf("main_dreamcast_memory[0]\n%d\n", ((u32*)main_dreamcast_memory)[0]);

    #ifdef SC_PLATFORM_PSP_OPTION_

    sceKernelSleepThread();

    #endif

    exit(EXIT_SUCCESS);
}