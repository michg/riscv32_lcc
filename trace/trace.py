import sys, os, string
import binascii
import struct
import json
from collections import namedtuple 
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

def load_debug():
    global typemap
    typemap = {}
    f = open('dbg.json','r')
    dbg = json.load(f)
    
    typedefs = dbg['typedefs']
    Type = namedtuple('Type',['name','desc'])
    for type in typedefs: 
        typemap[type['number']] = Type(type['name'],type['desc'])
    
    global globalmap
    globalmap = {}
    globals = dbg['globals']
    Global = namedtuple('Global',['typenr','adr'])
    for glob in globals:
        globalmap[glob['name']] = Global(glob['type'],glob['pos'])
        
    global funcmap
    global funcadr
    funcadr = []
    funcmap = {}
    funcs = dbg['functions']
    Func = namedtuple('Func',['rtype','adr'])
    for func in funcs:
        funcmap[func['funcname']] = Func(func['rtype'],func['pos'])
        funcadr.append(func['pos'])
    
    global adrmap
    adrmap = {}
    lines = dbg['locations']
    Line = namedtuple('Line',['filename','row'])
    for line in lines:
        adrmap[line['pos']] = Line(line['filename'], line['row'])
    
    global stacklocalmap
    stacklocalmap = {}
    slocals = dbg['stacklocals']
    Slocal = namedtuple('Slocal',['typenr','ofs'])
    for func in funcmap:
        stacklocalmap[func] = {}
    for slocal in slocals:
        stacklocalmap[slocal['funcname']][slocal['name']] = Slocal(slocal['type'], slocal['pos'])
    
    global reglocalmap
    reglocalmap = {}
    rlocals = dbg['reglocals']
    Rlocal = namedtuple('Rlocal',['typenr','regnr'])
    for func in funcmap:
        reglocalmap[func] = {}
    for rlocal in rlocals:
        reglocalmap[rlocal['funcname']][rlocal['name']] = Rlocal(rlocal['type'], rlocal['pos'])
    f.close() 

def getfunc(adr):
    proc = ""
    min = -1;
    for key in funcmap:
        funcadr = funcmap[key].adr
        if min<0 or adr>=funcadr and adr-funcadr<min:
            min = adr-funcadr
            func = key 
    return(func) 

def checkfuncstart(adr):    
    if adr in funcadr:
        return(True)
    else:
        return(False)


def getsigvalstr(name):
    global signals
    signal = signals[name]
    curvalstr = signal.timevals[signal.timeslot][1]
    if  curvalstr != 'x':
        curvalstr = '{0:08x}'.format(int(curvalstr, 2))
    return(curvalstr)

if __name__ == '__main__':
   # --- load debug information
    load_debug()
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
        watchadr[globalmap[var].adr] = var
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
                calledfunc = getfunc(pcval) 
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
        
