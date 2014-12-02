/* Definitions for the ARM private memory region.
 */
#ifndef _arm_priv_h_
#define _arm_priv_h_

#include "config.h"

CONF_IMPORT(__arm_priv_physical_base__);
CONF_IMPORT(__arm_priv_base__);
CONF_IMPORT(__arm_priv_size__);

#define ARM_PRIV_BASE CONF_ADDRESS(__arm_priv_base__)
#define ARM_PRIV_PHYSICAL_BASE CONF_ADDRESS(__arm_priv_physical_base__)
#define ARM_PRIV_SIZE CONF_ADDRESS(__arm_priv_size__)

#define PERIPH(offset) ((volatile unsigned int *)((ARM_PRIV_BASE + (offset))))

// Snoop control unit.
#define SCU(offset) PERIPH(0x0000 + (offset))
#define SCUCONR SCU(0x000)      // SCU control register.
#define SCUCFGR SCU(0x004)      // SCU configuration register.
#define SCUPSR  SCU(0x004)      // SCU power status register.
#define SCUIARR SCU(0x00C)      // SCU invalidate all registers register.
#define SCUFSAR SCU(0x040)      // SCU filtering start address register.
#define SCUFEAR SCU(0x044)      // SCU filtering end address register.
#define SCUSACR SCU(0x050)      // SCU access control register.
#define SCUNSAR SCU(0x054)      // SCU non-secure access control register.

// Interrupt controller.
#define GIC(offset) PERIPH(0x0100 + (offset))
#define ICCICR  GIC(0x000)      // CPU interface control register.
#define ICCPMR  GIC(0x004)      // Interrupt priority mask register.
#define ICCBPR  GIC(0x008)      // Binary point register.
#define ICCIAR  GIC(0x00C)      // Interrupt acknowledge register.
#define ICCEOIR GIC(0x010)      // End of interrupt register.
#define ICCRPR  GIC(0x014)      // Running priority register.
#define ICCHPIR GIC(0x018)      // Highest pending interrupt register.
#define ICCAPBR GIC(0x01C)      // Aliased non-secure binary point register.
#define ICCIDR  GIC(0x0FC)      // CPU interface implementer identification

// Global timer.
#define GTM(offset) PERIPH(0x0200  + (offset))
#define GTLOR   GTM(0x000)      // Global timer low 32 bits register.
#define GTHIR   GTM(0x004)      // Global timer high 32 bits register.
#define GTCTRLR GTM(0x008)      // Global timer control register.
#define GTISRR  GTM(0x00C)      // Global timer interrupt status register.
#define GTCLOR  GTM(0x010)      // Global timer compare low register.
#define GTCHIR  GTM(0x014)      // Global timer compare high register.
#define GTAIR   GTM(0x018)      // Global timer auto-increment register.

// Private timers and watchdogs.
#define PTM(offset) PERIPH(0x0600 + (offset))
#define PTLDR   PTM(0x000)      // Private timer load register.
#define PTCTRR  PTM(0x004)      // Private timer counter register.
#define PTCONR  PTM(0x008)      // Private timer control register.
#define PTISR   PTM(0x00C)      // Private timer interrupt status register.
#define WDLDR   PTM(0x020)      // Watchdog load register.
#define WDCTRR  PTM(0x024)      // Watchdog counter register.
#define WDCONR  PTM(0x028)      // Watchdog control register.
#define WDISR   PTM(0x02C)      // Watchdog interrupt status register.
#define WDRSR   PTM(0x030)      // Watchdog reset status register.
#define WDDISR  PTM(0x034)      // Watchdog disable register.

// Interrupt distributor.
#define ICD(offset) PERIPH(0x1000 + (offset))
#define ICDDCR   ICD(0x000)     // Distributor control register.
#define ICDICRT  ICD(0x004)     // Interrupt controller type register.
#define ICDIIDR  ICD(0x008)     // Distributor implementer identification.
#define ICD1SRn  ICD(0x080)     // Interrupt security registers.
#define ICDISERn ICD(0x100)     // Interrupt set-enable registers.
#define ICDICERn ICD(0x180)     // Interrupt clear-enable registers.
#define ICDISPRn ICD(0x200)     // Interrupt set-pending registers.
#define ICDICPRn ICD(0x280)     // Interrupt clear-pending registers.
#define ICDABRn  ICD(0x300)     // Active bit registers.
#define ICDIPRn  ICD(0x400)     // Interrupt priority registers.
#define ICDIPTRn ICD(0x800)     // Interrupt processor target registers.
#define ICDICFRn ICD(0xC00)     // Interrupt configuration registers.
#define ICPPISR  ICD(0xD00)     // PPI status register.
#define ICSPISRn ICD(0xD00)     // SPI status registers.
#define ICDSGIR  ICD(0xF00)     // Software generated interrupt register.
#define ICPIDR0  ICD(0xFD0)     // Peripheral ID0 register.
#define ICPIDR1  ICD(0xFD4)     // Peripheral ID1 register.
#define ICPIDR2  ICD(0xFD8)     // Peripheral ID2 register.
#define ICPIDR3  ICD(0xFDC)     // Peripheral ID3 register.
#define ICPIDR4  ICD(0xFE0)     // Peripheral ID4 register.
#define ICPIDR5  ICD(0xFE4)     // Peripheral ID5 register.
#define ICPIDR6  ICD(0xFE8)     // Peripheral ID6 register.
#define ICPIDR7  ICD(0xFEC)     // Peripheral ID7 register.
#define ICCIDR0  ICD(0xFF0)     // Component ID0 register.
#define ICCIDR1  ICD(0xFF4)     // Component ID1 register.
#define ICCIDR2  ICD(0xFF8)     // Component ID2 register.
#define ICCIDR3  ICD(0xFFC)     // Component ID3 register.

#endif // _arm_priv_h_
