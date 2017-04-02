#!/usr/bin/python

from __future__ import division

divs = [1, 2, 3, 4, 6, 8, 12, 16, 24, 32]

class Frequencies:
    def __init__(self, pll_mul, cpu_div, ram_div):
        self._pll_mul, self._cpu_div, self._ram_div = pll_mul, cpu_div, ram_div

    def pll(self):
        return 12 * self._pll_mul

    def pll_mul(self):
        return self._pll_mul

    def cpu(self):
        return self.pll() / self._cpu_div

    def cpu_div(self):
        return self._cpu_div

    def ram(self):
        return self.pll() / self._ram_div

    def ram_div(self):
        return self._ram_div

    def msd(self):
        return 24 * self.pll() / 360

    def msd2(self):
        return 48 * self.pll() / 360

def notes(f):
    if f.pll() > 100 and f.ram_div() == 1:
        return "Unstable: RAM div /1 > 100 MHz"
    elif f.pll() == 360 and f.cpu_div() == 1 and f.ram_div() == 3:
        return "The nominal clock speed"

    cpu_oc, ram_oc = f.cpu() / 360, f.ram() / 120
    similar = 0.999 <= cpu_oc / ram_oc <= 1.001

    stability = ""
    if cpu_oc > 1.2 or ram_oc > 1.6:
        stability = " (likely unstable)"
    elif cpu_oc > 1.1 or ram_oc > 1.5:
        stability = " (less stable)"
    elif cpu_oc > 1 or ram_oc > 1:
        stability = " (stable)"

    if cpu_oc > 1 and ram_oc > 1 and similar:
        return "Overclocked %4.1f%%%s" % ((cpu_oc - 1) * 100, stability)
    elif cpu_oc > 1 and ram_oc > 1:
        return "CPU +%4.1f%%, RAM +%4.1f%%%s" % ((cpu_oc - 1) * 100, (ram_oc - 1) * 100, stability)
    elif cpu_oc > 1:
        return "CPU overclocked %4.1f%%%s" % ((cpu_oc - 1) * 100, stability)
    elif ram_oc > 1:
        return "RAM overclocked %4.1f%%%s" % ((ram_oc - 1) * 100, stability)

    return ""

def fits_constraints(f):
    if f.cpu() > 468 or f.ram() > 204:
        return False  # Clock speeds assumed to be unstable a priori
    if f.pll() < 100 or f.pll() > 500:
        return False  # The PLL output must be between 100 MHz and 500 MHz
    if f.ram_div() % f.cpu_div() != 0:
        return False  # The RAM divider must be a multiple of the CPU divider
    if f.ram_div() / f.cpu_div() >= 24:
        return False  # The ratio between the CPU and RAM clocks must be < 24
    if f.cpu() >= 100:
        if f.cpu() == f.ram() and f.cpu_div() != 2:
            return False  # Do not use unstable memory divider /1 over 100 MHz
        elif f.cpu() != f.ram() and f.cpu_div() != 1:
            return False  # Only use CPU clock divider 1 otherwise
    else:
        # Only use the lowest CPU clock divider that would bring the PLL output
        # above 100 MHz
        usable_divs = [i for i in divs if 100 <= f.cpu() * i <= 500]
        if len(usable_divs) == 0 or f.cpu_div() != usable_divs[0]:
            return False

    return True

if __name__ == '__main__':
    fs = [Frequencies(pll_mul, cpu_div, ram_div)
          for pll_mul in xrange(9, 40)
          for cpu_div in divs
          for ram_div in divs
          if fits_constraints(Frequencies(pll_mul, cpu_div, ram_div))]

    fs = sorted(fs, key=Frequencies.ram, reverse=True)  # secondary sorting key
    fs = sorted(fs, key=Frequencies.cpu, reverse=True)  # primary sorting key

    print "  PLL *mul    CPU /div    RAM /div    mSD    SD2  Notes"
    for f in fs:
        note = notes(f)
        line = "%5.1f *%-2d   %5.1f /%-2d   %5.1f /%-2d   %5.1f   %5.1f" % (f.pll(), f.pll_mul(), f.cpu(), f.cpu_div(), f.ram(), f.ram_div(), f.msd(), f.msd2())
        if note:
            line = "%s  %s" % (line, note)
        print line
