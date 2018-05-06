import sys, os, string
import binascii
import struct
from sys import argv 
from Verilog_VCD import parse_vcd
from types import SimpleNamespace as New


def make_num(txt):
    if txt.startswith('0x'):
        return int(txt[2:], 16)
    elif txt.startswith('$'):
        return int(txt[1:], 16)
    elif txt.startswith('0b'):
        return int(txt[2:], 2)
    elif txt.startswith('%'):
        return int(txt[1:], 2)
    else:
        return int(txt)


str2int = make_num

  
def getfunc(addr): 
    maxkey = max(address_proc.keys()) 
    def cost(val):
        diff = addr - val 
        if diff >= 0:
            return(diff)
        else:
            return(maxkey + 1)
    minkey = min(address_proc.keys(), key=cost)
    return address_proc[minkey] 

def checkfuncstart(adr):    
    if adr in address_proc.keys():
        return(True)
    else:
        return(False)
    

# --- debug information
def read_debug_symbols(file):
    global maxfileline
    maxfileline = 0
    for line in open(file):
        if line.startswith('typedef:'):
            split = line.split('#')
            typename, typenum = split[1].split(':')            
            typenum = int(typenum)            
            typedesc = split[2]
            subtypes = {}
            subtypedesc = typedesc.split(';')          
            if subtypedesc[0][1] == 's':
                subtypedesc = subtypedesc[1:-1]
                for type in subtypedesc:
                    name, type, bitoffset = type.split(',')
                    byteoffset = str2int(bitoffset) // 8
                    subtypes[name] = (type, byteoffset)
            typedef[typenum] = (typename, typedesc, subtypes)
         
    for line in open(file):
         split = line.split()
         if split[0] in ['line:', 'function:', 'reglocal:', 'stacklocal', 'global:', 'regparam:', 'stackparam']:
             address = int(split[-1][2:], 16)
           
         if split[0] == 'line:':
             colon = split[1].rfind(':')
             filename, line = split[1][1:colon-1], int(split[1][colon+1:])
             if os.path.basename(filename) == srcname:
                 if address not in address_line or address_line[address] < line: 
                     line_address[line] = address
                     address_line[address] = line
                 if line>maxfileline:
                     maxfileline = line

         elif split[0] == 'reglocal:':
             data = bytes.fromhex(split[-1][2:])
             res, = struct.unpack('>i', data)
             proc, var = split[1].split(':')
             typenum = int(split[2])
             local_var.setdefault(proc, {})[var] = (res, typenum, 'reglocal') 
       
         elif split[0] == 'stacklocal:':
             data = bytes.fromhex(split[-1][2:])
             res, = struct.unpack('>i', data)
             proc, var = split[1].split(':')
             typenum = int(split[2])
             local_var.setdefault(proc, {})[var] = (res, typenum, 'stacklocal')

         elif split[0] == 'function:':
             name = split[1]             
             proc_address[name] = address             

         elif split[0] == 'regparam:': 
             proc, var = split[1].split(':')
             typenum = int(split[2])
             local_var.setdefault(proc, {})[var] = (address, typenum, 'regparam') 
        
         elif split[0] == 'stackparam:':
             proc, var = split[1].split(':')
             typenum = int(split[2])
             local_var.setdefault(proc, {})[var] = (address, typenum, 'stackparam') 

         elif split[0] == 'global:':
             typenum = int(split[2])
             global_var[split[1]] = (address, typenum)



def load_debug_information(module):
    global typedef, local_var, global_var, proc_address, address_proc, address_line, line_address, srcname
    typedef, local_var, global_var, proc_address, address_line, line_address = {}, {}, {}, {}, {}, {}
    maxline = 0
    srcname = module + '.c'
    read_debug_symbols(module)
    
    address_proc = dict([(v,k) for k,v in proc_address.items()])
    
    address_global_var = dict([(b[0],a) for a,b in global_var.items()])

def getsigvalstr(name):
    global signals
    signal = signals[name]
    curvalstr = signal.timevals[signal.timeslot][1]
    if  curvalstr != 'x':
        curvalstr = '{0:08x}'.format(int(curvalstr, 2))
    return(curvalstr)

if __name__ == '__main__':
   # --- load debug information
    load_debug_information(argv[1]+'.deb')
    table = parse_vcd('system.vcd',siglist = \
    ['system_tb.clk', \
    'system_tb.uut.picorv32_core.reg_pc[31:0]', \
    'system_tb.uut.picorv32_core.\cpuregs[10][31:0]', \
    'system_tb.uut.picorv32_core.\cpuregs[12][31:0]', \
    'system_tb.uut.picorv32_core.mem_addr[31:0]', \
    'system_tb.uut.picorv32_core.mem_rdata[31:0]', \
    'system_tb.uut.picorv32_core.mem_wdata[31:0]', \
    'system_tb.uut.picorv32_core.mem_wstrb[3:0]', \
    'system_tb.uut.picorv32_core.mem_valid',
    'system_tb.uut.picorv32_core.mem_ready'])    
    codes = list(table.keys())
    globals = ['x']
    global signals
    signals = {}
    pctimevals = []
    watchadr = {}
    for var in globals:
        watchadr[global_var[var][0]] = var
    for code in table.keys():
        name = table[code]['nets'][0]['name']
        tv = table[code]['tv']
        if (name == 'clk'):
            clktimevals = tv
        else:
            signals[name] = New(timeslot = 0, change = False, timevals = tv)
            
    calledfunc = '' 
    pcval = 0 
    for time, clkvalstr in clktimevals:
        clkval = int(clkvalstr, 2)
        if clkval:
        
            for name, signal in signals.items():
                signal.change = False
                while (len(signal.timevals)-1>signal.timeslot and \
                signal.timevals[signal.timeslot + 1][0] < time):
                    signal.timeslot += 1
                    signal.change = True
        
            lastpcval = pcval
            pcvalstr = getsigvalstr('reg_pc[31:0]')
            pcval = int(pcvalstr, 16)
        
            if(pcval!=lastpcval and checkfuncstart(pcval)):
                curvalstr = getsigvalstr('\cpuregs[12][31:0]')
                calledfunc = address_proc[pcval] 
                print("@time=%d ns, PC=%08x funktion %s(%08x) called with %s" \
                %(time//1000, lastpcval, calledfunc,  pcval, curvalstr))
            curfunc = getfunc(pcval) 
        
            if(pcval!=lastpcval and calledfunc != curfunc):
                curvalstr = getsigvalstr('\cpuregs[10][31:0]')
                print("@time=%d ns, funktion %s returned with %s" \
                %(time//1000, calledfunc, curvalstr))
                calledfunc = curfunc
                
            curvalstr = getsigvalstr('mem_addr[31:0]')
            if curvalstr != 'x':
                memadr = int(curvalstr, 16)
            else:
                memadr = 0
            if(memadr in watchadr.keys()):
                curvalstr = getsigvalstr('mem_valid')
                valid = int(curvalstr, 16)
                curvalstr = getsigvalstr('mem_ready')
                ready = int(curvalstr, 16)
                curvalstr = getsigvalstr('mem_wstrb[3:0]')
                wstrobe = int(curvalstr, 16)
                if(valid and not wstrobe and ready):
                    var = watchadr[memadr] 
                    curvalstr = getsigvalstr('mem_rdata[31:0]') 
                    print("@time=%d ns, read var %s data:%s" %(time//1000, var, \
                     curvalstr))
               
                if(valid and wstrobe and ready):
                    var = watchadr[memadr] 
                    curvalstr = getsigvalstr('mem_wdata[31:0]') 
                    print("@time=%d ns, write var %s with data:%s" %(time//1000, var, \
                     curvalstr))
        
