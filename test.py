import gdb
import time
import os

    # // PLIC
    # for (pa_t pa = 0xC000000; pa < 0xC400000; pa += 4096)
    #     init_map(pa, pa, PTE_R | PTE_W);
    
    # // UART 0
    # for (pa_t pa = 0x10000000; pa < 0x10001000; pa += 4096)
    #     init_map(pa, pa, PTE_R | PTE_W);

    # // VIRTO Disk
    # for (pa_t pa = 0x10001000; pa < 0x10002000; pa += 4096)
    #     init_map(pa, pa, PTE_R | PTE_W);
    
    # // kernel code (text)
    # asm ("de:");
    # for (pa_t pa = 0x80000000; pa < (pa_t)_text_end; pa += 4096)
    #     init_map(pa, pa, PTE_R | PTE_W | PTE_X);
    
    # // kernel data
    # for (pa_t pa = (pa_t)_text_end; pa < (pa_t)_ram_end; pa += 4096)
    #     init_map(pa, pa, PTE_R | PTE_W);

class TestVM (gdb.Command):

    def __init__(self):
        super(TestVM, self).__init__("testvm", gdb.COMMAND_USER)
        self.errlis = []
    
    def prterr(self):
        print(f"\n{5 - len(self.errlis)}/{5} correct")
        for err in self.errlis:
            print(err)
    
    def eval(self, name):
        return int(gdb.parse_and_eval(name).__str__().split(' ')[0], 16)

    def testrange(self, name, start, end):
        try:
            print(f"test {name}")
            pa = start
            while pa < end:
                gdb.execute(f"x/x {pa}")
                pa = pa + 4096
        except Exception as msg:
            self.errlis.append(msg)
            if len(self.errlis) > 5:
                print("Too many errors")
                self.prterr()

    def invoke(self, arg, from_tty):
        _text_end = self.eval("_text_end")
        _bss_end = self.eval("_bss_end")
        _ram_end = self.eval("_ram_end")
        self.testrange("plic", 0xC000000, 0xC400000)
        self.testrange("uart", 0x10000000, 0x10001000)
        self.testrange("disk", 0x10001000, 0x10002000)
        self.testrange("ktext", 0x80000000, _text_end)
        self.testrange("kdata", _text_end, _bss_end)
        self.testrange("dram", _bss_end, _ram_end)
        self.prterr()


class TestAT(gdb.Command):
    def __init__(self):
        super(TestAT, self).__init__("testat", gdb.COMMAND_USER)
    
        
    def get_addr(self, name):
        return int(gdb.parse_and_eval(name).__str__().split(' ')[0], 16)
    
    def getstr(self, name):
        return gdb.parse_and_eval(name).__str__()
    
    def extract_ppn(self, str):
        return int(str.split(',')[9].split(' ')[3]) << 12

    def vpn1(self, va):
        return (va & 0b111111111000000000000) >> 12
    
    def vpn2(self,va):
        return (va & 0b111111111000000000000000000000) >> (12 + 9)
    
    def vpn3(self, va):
        return (va & 0b111111111000000000000000000000000000000) >> (12 + 9 * 2)
    
    def invoke(self, va, from_tty):

        va = int(va, 16)

        pte1 = self.getstr(f"kernelpt->arr[{self.vpn3(va)}]")
        pt2 = self.extract_ppn(pte1)

        pte2 = self.getstr(f"((pt_t *){pt2})->arr[{self.vpn2(va)}]")
        pt3 = self.extract_ppn(pte2)

        pte3 = self.getstr(f"((pt_t *){pt3})->arr[{self.vpn1(va)}]")
        pa = self.extract_ppn(pte3)

        print("recursive lookup:")
        print("pt2 =", hex(pt2));
        print("pt3 =", hex(pt3));
        print("pa =", hex(pa))

        
TestVM()
TestAT()
