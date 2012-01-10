#include <omap4_hs.h>
#include <common.h>

U32 SEC_ENTRY_Std_Ppa_Call (U32 appl_id, U32 inNbArg, ...)
{
	U32 result = 0;
	U32 Param[4];
	va_list ap;

	va_start(ap, inNbArg);

	switch (inNbArg)
	{
		case 0:
			result = PUBLIC_SEC_ENTRY_Pub2SecDispatcher(
					appl_id,
					0,  // TBDJL needs to be explained
					0x7,
					inNbArg);
			break;

		case 1:
			Param[0] = va_arg(ap, U32);
			result = PUBLIC_SEC_ENTRY_Pub2SecDispatcher(
					appl_id,
					0,  // TBDJL needs to be explained
					0x7,
					inNbArg, Param[0]);
			break;

		case 2:
			Param[0] = va_arg(ap, U32);
			Param[1] = va_arg(ap, U32);
			result = PUBLIC_SEC_ENTRY_Pub2SecDispatcher(
					appl_id,
					0,  // TBDJL needs to be explained
					0x7,
					inNbArg, Param[0], Param[1]);
			break;

		case 3:
			Param[0] = va_arg(ap, U32);
			Param[1] = va_arg(ap, U32);
			Param[2] = va_arg(ap, U32);
			result = PUBLIC_SEC_ENTRY_Pub2SecDispatcher(
					appl_id,
					0,  // TBDJL needs to be explained
					0x7,
					inNbArg, Param[0], Param[1], Param[2]);
			break;
		case 4:
			Param[0] = va_arg(ap, U32);
			Param[1] = va_arg(ap, U32);
			Param[2] = va_arg(ap, U32);
			Param[3] = va_arg(ap, U32);
			result = PUBLIC_SEC_ENTRY_Pub2SecDispatcher(
					appl_id,
					0,  // TBDJL needs to be explained
					0x7,
					inNbArg, Param[0], Param[1], Param[2], Param[3]);
			break;
		default:
			printf("[ERROR] [SEC_ENTRY] Number of arguments not supported \n");
			return 1;
	}
	va_end(ap);
	if (result != 0)
		printf("[ERROR] [SEC_ENTRY] Call to Secure HAL failed!\n");
	return result;
}

U32 PL310aux(U32 appl_id, U32 value)
{
	__asm__ __volatile__("stmfd   sp!, {r2-r12, lr}");
	__asm__ __volatile__("mov    r12, r0");
	__asm__ __volatile__("ldr    r0, [r1]");
	__asm__ __volatile__("orr    r0, r0, #0x1E000000");
	__asm__ __volatile__("dsb");
	__asm__ __volatile__("smc #0");
	//__asm__ __volatile__(".word  0xE1600071");
	__asm__ __volatile__("ldmfd   sp!, {r2-r12, pc}");
	return 0;
}

