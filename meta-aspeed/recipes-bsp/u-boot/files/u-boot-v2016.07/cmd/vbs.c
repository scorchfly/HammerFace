/*
 * (C) Copyright 2016-Present, Facebook, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <crc.h>
#include <tpm.h>

#include <asm/io.h>
#include <asm/arch/ast_scu.h>
#include <asm/arch/vbs.h>

static int do_vbs(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
  volatile struct vbs *vbs = (volatile struct vbs*)AST_SRAM_VBS_BASE;
  if (argc == 3) {
    ulong t = simple_strtoul(argv[1], NULL, 10);
    ulong c = simple_strtoul(argv[2], NULL, 10);
    vbs->error_type = t;
    vbs->error_code = c;
    return 0;
  }

  if (argc == 2) {
    if (strncmp(argv[1], "disable", sizeof("disable")) == 0) {
#ifdef CONFIG_ASPEED_ENABLE_WATCHDOG
      /* This will disable the WTD1. */
      writel(readl(AST_WDT_BASE + 0x0C) & ~1, AST_WDT_BASE + 0x0C);
#endif
      vbs->rom_handoff = 0x0;
      return 0;
    } else if (strncmp(argv[1], "clear", sizeof("clear")) == 0) {
#ifdef CONFIG_ASPEED_TPM
      return tpm_nv_define_space(VBS_TPM_ROLLBACK_INDEX,
          TPM_NV_PER_GLOBALLOCK | TPM_NV_PER_PPWRITE, 0);
#endif
    } else {
      printf("Unknown vbs command\n");
    }
  }

  uint16_t crc = vbs->crc;
  uint32_t handoff = vbs->rom_handoff;
  bool crc_valid = false;

  /* Check CRC value */
  vbs->crc = 0;
  vbs->rom_handoff = 0x0;
  if (crc == crc16_ccitt(0, (uchar*)vbs, sizeof(struct vbs))) {
    crc_valid = true;
  }
  vbs->crc = crc;
  vbs->rom_handoff = handoff;

  printf("ROM executed from:       0x%08x\n", vbs->rom_exec_address);
  printf("ROM KEK certificates:    0x%08x\n", vbs->rom_keys);
  printf("ROM handoff marker:      0x%08x\n", vbs->rom_handoff);
  printf("U-Boot executed from:    0x%08x\n", vbs->uboot_exec_address);
  printf("U-Boot certificates:     0x%08x\n", vbs->subordinate_keys);
  printf("\n");
  printf("Certificates fallback:   %u\n", vbs->subordinate_last);
  printf("Certificates time:       %u\n", vbs->subordinate_current);
  printf("U-Boot fallback:         %u\n", vbs->uboot_last);
  printf("U-Boot time:             %u\n", vbs->uboot_current);
  printf("Kernel fallback:         %u\n", vbs->kernel_last);
  printf("Kernel time:             %u\n", vbs->kernel_current);
  printf("\n");
  printf("Flags force_recovery:    %d\n", (vbs->force_recovery) ? 1 : 0);
  printf("Flags hardware_enforce:  %d\n", (vbs->hardware_enforce) ? 1 : 0);
  printf("Flags software_enforce:  %d\n", (vbs->software_enforce) ? 1 : 0);
  printf("Flags recovery_boot:     %d\n", (vbs->recovery_boot) ? 1 : 0);
  printf("Flags recovery_retries:  %u\n", vbs->recovery_retries);
  printf("\n");
  printf("TPM status:  %u\n", vbs->error_tpm);
  printf("CRC valid:   %d (%hu)\n", (crc_valid) ? 1 : 0, crc);
  printf("Status: type (%d) code (%d)\n", vbs->error_type, vbs->error_code);

	return 0;
}

U_BOOT_CMD(
	vbs,	3,	1,	do_vbs,
	"print verified-boot status",
	"type code - set the vbs error type and code\n"
	"disable - disable the watchdog timer and ROM handoff check"
);
