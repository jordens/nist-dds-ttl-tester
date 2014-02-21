from migen.fhdl.std import *
from migen.bus import wishbone
from mibuild.generic_platform import *

from misoclib import gpio, spiflash
from misoclib.gensoc import GenSoC

_tester_io = [
	("user_led", 1, Pins("B:7"), IOStandard("LVTTL33")),
	("ttl", 0,
		Subsignal("d", Pins("C:11 C:10 C:9 C:8 C:7 C:6 C:5 C:4 C:3 C:2 C:1 C:0 B:4 A:11 B:5 A:10")),
		Subsignal("tx_l", Pins("A:9")),
		Subsignal("tx_h", Pins("B:6")),
		IOStandard("LVTTL33")),
	("dds", 0,
		Subsignal("a", Pins("A:5 B:10 A:6 B:9 A:7 B:8")),
		Subsignal("d", Pins("A:12 B:3 A:13 B:2 A:14 B:1 A:15 B:0")),
		Subsignal("sel", Pins("A:2 B:14 A:1 B:15 A:0")),
		Subsignal("p", Pins("A:8 B:12")),
		Subsignal("fud", Pins("B:11")),
		Subsignal("wr_n", Pins("A:4")),
		Subsignal("rd_n", Pins("B:13")),
		Subsignal("reset", Pins("A:3")),
		IOStandard("LVTTL33")),
	("pmt", 0, Pins("C:13"), IOStandard("LVTTL33")),
	("pmt", 1, Pins("C:14"), IOStandard("LVTTL33")),
	("pmt", 2, Pins("C:15"), IOStandard("LVTTL33")),
	("xtrig", 0, Pins("C:12"), IOStandard("LVTTL33")),
]

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
		platform.add_extension(_tester_io)

		self.submodules.crg = _CRG(platform)

		# BIOS is in SPI flash
		self.submodules.spiflash = spiflash.SpiFlash(platform.request("spiflash2x"),
			cmd=0xefef, cmd_width=16, addr_width=24, dummy=4)
		self.flash_boot_address = 0x70000
		self.register_rom(self.spiflash.bus)

		# Use block RAM instead of SDRAM for now
		sys_ram_size = 32*1024
		self.submodules.sys_ram = wishbone.SRAM(sys_ram_size)
		self.add_wb_slave(lambda a: a[27:29] == 2, self.sys_ram.bus)
		self.add_cpu_memory_region("sdram", 0x40000000, sys_ram_size)

		self.submodules.leds = gpio.GPIOOut(platform.request("user_led"))

default_subtarget = DDSTTLTesterSoC
