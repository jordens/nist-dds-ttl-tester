from migen.fhdl.std import *
from migen.bus import wishbone

from misoclib import gpio, spiflash
from misoclib.gensoc import GenSoC

class _CRG(Module):
	def __init__(self, platform):
		self.clock_domains.cd_sys = ClockDomain()
		clkgen_locked = Signal()
		self.specials += Instance("DCM_CLKGEN",
			p_CLKFXDV_DIVIDE=2, p_CLKFX_DIVIDE=2, p_CLKFX_MD_MAX=2.5, p_CLKFX_MULTIPLY=5,
			p_CLKIN_PERIOD=31.25, p_SPREAD_SPECTRUM="NONE", p_STARTUP_WAIT="FALSE",
			i_CLKIN=platform.request("clk32"), o_CLKFX=self.cd_sys.clk,
			o_LOCKED=clkgen_locked, i_FREEZEDCM=0, i_RST=0)
		self.specials += Instance("FD", p_INIT=1, i_D=~clkgen_locked, o_Q=self.cd_sys.rst, i_C=ClockSignal())

class DDSTTLTesterSoC(GenSoC):
	default_platform = "papilio_pro"

	def __init__(self, platform):
		GenSoC.__init__(self, platform,
			clk_freq=80*1000000,
			cpu_reset_address=0x60000)

		self.submodules.crg = _CRG(platform)

		# BIOS is in SPI flash
		self.submodules.spiflash = spiflash.SpiFlash(platform.request("spiflash2x"),
			cmd=0xefef, cmd_width=16, addr_width=24, dummy=4)
		self.register_rom(self.spiflash.bus)

		# Use block RAM instead of SDRAM for now
		sys_ram_size = 32*1024
		self.submodules.sys_ram = wishbone.SRAM(sys_ram_size)
		self.add_wb_slave(lambda a: a[27:29] == 2, self.sys_ram.bus)
		self.add_cpu_memory_region("sdram", 0x40000000, sys_ram_size)

		self.submodules.leds = gpio.GPIOOut(platform.request("user_led"))

default_subtarget = DDSTTLTesterSoC
