import sys, os, string
import sys, os, string
import cmd
import abc
from socket import *
import binascii
import select
import struct
from collections import namedtuple 
from threading import Thread, Lock
from queue import Queue
import json
import logging 

STOPPED = 0
RUNNING = 1

INTERRUPT = 1
BRKPOINT = 2

PTRSIZE = 4
PTRTYPE = 'unsigned int'

CMDLINE = 15

fmts = {
    'unsigned int': ('<I',4),
    'void': ('<I',4),
    'int': ('<i',4),
    'unsigned short': ('<H',2),
    'short':('<h',2),
    'unsigned char':('<B',1),
    'signed char':('<b',1)
    }
    
def tyname2size(name):
    return(fmts[name][1])

def tyname2fmt(name):
    return(fmts[name][0]) 

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

def clearscreen(stdout):
    stdout.write("\033[2J\033[1;1H") 

def savepos(stdout):
    stdout.write('\x1b[s')
    stdout.flush()

def restorepos(stdout):
    stdout.write('\x1b[u') 
    stdout.flush()

def pos(stdout, y, x):
    stdout.write('\x1b[%d;%dH' % (y, x))
    stdout.flush()


def print_file_line(stdout, filename, lineno):
    pos(stdout,CMDLINE-1,1) 
    pos(stdout, 1, 1) 
    lines = open(filename).read().splitlines()    
    s = "\033[37m\033[1mFile:{}\n".format(filename)    
    stdout.write(s)    
    s = "Line: [{} of {}]\n".format(lineno, len(lines))
    stdout.write(s)    
    s = "\033[0m\n"
    stdout.write(s)    
    s = "\033[0m\n"
    stdout.write(s)
 

    #Print a fragment of the file to show in context
    for i in range(lineno - 3, lineno + 3):
        if i < 1:
            stdout.write('\n')
        elif i > len(lines):
            stdout.write('\n')
        elif i == lineno:            
            s = "\033[33m\033[1m{}\033[32m->{}\033[0m\033[39m\n".format(str(i).rjust(4),lines[i - 1])
            stdout.write(s)
        else:            
            s = "\033[33m\033[1m{}\033[0m\033[39m  {}\n".format(str(i).rjust(4),lines[i - 1])
            stdout.write(s) 

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
    funcmap = {}
    funcs = dbg['functions']
    Func = namedtuple('Func',['rtype','adr'])
    for func in funcs:
        funcmap[func['funcname']] = Func(func['rtype'],func['pos'])
    
    global adrmap
    global linemap
    adrmap = {}
    linemap = {}
    lines = dbg['locations']
    Line = namedtuple('Line',['filename','row'])
    for line in lines:
        linemap[line['filename']] = {}
    for line in lines:
        adrmap[line['pos']] = Line(line['filename'], line['row'])
        linemap[line['filename']][line['row']] = line['pos']
        
    
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
    
class dbgif(metaclass=abc.ABCMeta):
    @abc.abstractmethod
    def read_mem(self, adr, len):
        raise NotImplementedError() 
    
    @abc.abstractmethod
    def write_mem(self, adr, data):
        raise NotImplementedError() 
    
    @abc.abstractmethod
    def set_breakpoint(self, adr):
        raise NotImplementedError() 
    
    @abc.abstractmethod
    def clear_breakpoint(self, adr):
        raise NotImplementedError() 
    
    @abc.abstractmethod
    def get_register(self, regnr):
        raise NotImplementedError() 
    
    @abc.abstractmethod
    def set_register(self, regnr, value):
        raise NotImplementedError() 
        
    @abc.abstractmethod
    def halt(self):
        raise NotImplementedError() 
        
    @abc.abstractmethod
    def resume(self):
        raise NotImplementedError() 
        
    @abc.abstractmethod
    def step(self):
        raise NotImplementedError() 

screenlock = Lock()
        
class Proxy(object):
    logger = logging.getLogger('dbg') 
    def __init__(self):
        self.buffer = []
        self.stdout = sys.stdout
        
    def write(self, data):
        if len(data)>0:
            self.buffer.append(data)
            if data[-1] == '\n':
                self.flush()
    def flush(self):                
        text = ''.join(self.buffer)
        self.buffer = []
        with screenlock:
            self.logger.debug('stdout flushing: %s, [%s] ', text, text.encode('utf-8').hex()) 
            self.stdout.write(text)
            self.stdout.flush()
            
         
        
class DebugCli(cmd.Cmd):
    """ Implement a console-based debugger interface. """
    prompt = '(dbg)> '
    intro = "interactive debugger"
    

    def __init__(self):
        self.Proxy = Proxy() 
        self.stdout_ori = sys.stdout                  
        sys.stdout = self.Proxy
        self.use_rawinput = False
        super().__init__(stdout=self.Proxy)             
        self.output = "" 
        clearscreen(self.stdout_ori)
        pos(self.stdout_ori, 1, 1) 
        
        
    
    def do_exit(self, line): 
        """exit the interpreter session"""
        Rsp.close()
        sys.exit(-1)
    
    def do_rd(self, line): 
        """rd adr,len: read len bytes from memory at adr"""
        if(Rsp.state != STOPPED):
            self.output = "Command only possible during STOPPED-state."
            return
        x = line.split(',')
        adr = str2int(x[0])
        count = str2int(x[1])
        self.output = Rsp.read_mem(adr, count)
        #self.output = binascii.a2b_hex(data)        
      
    def do_wr(self, line): 
        """wr adr, hexdata: write hexdata to adr in memory"""
        if(Rsp.state != STOPPED):
            self.output = "Command only possible during STOPPED-state."
            return
        x = line.split(',')
        adr = int(x[0])
        data = x[1]
        Rsp.write_mem(adr, data)
    
    def do_break(self, line):
        """break: send break to the processor"""
        if(Rsp.state != RUNNING):
            self.output = "Command only possible during RUNNING-state."
            return
        Rsp.halt()
        while Rsp.state == RUNNING:
            pass
    
    def do_restart(self, line):
        """ restart the device """
        if(Rsp.state != STOPPED):
            self.output = "Command only possible during STOPPED-state."
            return
        Rsp.set_register(32, 0) 
        Rsp.resume()
        Rsp.state = RUNNING 
    
    def do_showpc(self,line):
        """Read the pc"""
        if(Rsp.state != STOPPED):
            self.output = "Command only possible during STOPPED-state."
            return
        self.output = "PC = %08X\n"%Rsp.pc
    
    def do_setpc(self,line):
        """Set the pc"""
        if(Rsp.state != STOPPED):
            self.output = "Command only possible during STOPPED-state."
            return
        val = str2int(line)
        Rsp.set_register(32, val)
    
    
            
    def do_getregs(self,line):
        """Read all registers"""
        if(Rsp.state != STOPPED):
            self.output = "Command only possible during STOPPED-state."
            return
        Rsp.sendpkt("g")
        data = self.rxqueue.get()
        data = bytes.fromhex(data)
        offset = 0
        for i in range(0, 33):
            regdata = data[offset:offset+4]
            regval, = struct.unpack('<I', regdata)
            self.output +="Register %02X:%08X\n"%(i,regval)
            offset+=4
    
    def do_setbrk(self, line):
        """ Set a breakpoint: setbrk linenr """
        if(Rsp.state != STOPPED):
            self.output = "Command only possible during STOPPED-state."
            return
        x = line.split(':')
        filename = x[0]
        linenum = str2int(x[1])
        adr = linemap[filename][linenum]
        Rsp.set_breakpoint(adr)

    def do_clrbrk(self, line):
        """ Clear a breakpoint: clrbrk linenr
        """
        if(Rsp.state != STOPPED):
            self.output = "Command only possible during STOPPED-state."
            return
        x = line.split(':')
        filename = x[0]
        linenum = str2int(x[1])
        adr = linemap[filename][linenum]
        Rsp.clear_breakpoint(adr)

    def do_stepi(self, line):
        """ step one instruction """
        if(Rsp.state != STOPPED):
            self.output = "Command only possible during STOPPED-state."
            return
        Rsp.step()
        Rsp.state = RUNNING 
        while Rsp.state == RUNNING:
            pass
    
    do_si = do_stepi
    
    def do_stepl(self, line):
        """ step one line """
        if(Rsp.state != STOPPED):
            self.output = "Command only possible during STOPPED-state."
            return
        Rsp.clear_breakpoint(Rsp.pc)
        lastfunc = get_func(Rsp.pc)
        curfunc = lastfunc
        lastline = Rsp.curline
        while Rsp.curline==lastline or curfunc!=lastfunc:
            self.do_si("")
            curfunc = get_func(Rsp.pc)
    
    
    do_sl = do_stepl
    

    def do_print(self, line):
        """ print variable """
        if(Rsp.state != STOPPED):
            self.output = "Command only possible during STOPPED-state."
            return
        if line.startswith('*'):
             deref=True
             line=line[1:]
        else:
             deref=False
        if(line.find('[')!=-1):
            index = str2int(line[line.find('[')+1:line.find(']')])
            var = line[:line.find('[')]
        else:
            index = 0
            var = line
        if(line.find('.')!=-1):
            subvar = line[line.find('.')+1:]
            var = line[:line.find('.')]
        else:
            subvar = ''
        
        curfunc = get_func(Rsp.pc) 
        if var in stacklocalmap[curfunc]:
            ofs = stacklocalmap[curfunc][var].ofs
            curbase = Rsp.get_register(8); 
            adr = curbase + ofs         
            typenr = stacklocalmap[curfunc][var].typenr
            adr, typename = ptr_array_struct(typenr, deref, adr, index, subvar)
            val = read_type(adr, typename)
            self.output += "%s:%s@%08X=%08X\n"%(line, typename, adr, val)
        elif var in reglocalmap[curfunc]:
            regnum = reglocalmap[curfunc][var].regnr
            val = Rsp.get_register(regnum);
            typenr = reglocalmap[curfunc][var].typenr
            adr = 0
            adr, typename = ptr_array_struct(typenr, deref, adr, index, subvar)
            val = read_type(adr, typename)
            self.output += "%s = %08X\n"%(line,val)
        elif var in globalmap:
            typenr = globalmap[var].typenr
            adr = globalmap[var].adr
            adr, typename = ptr_array_struct(typenr, deref, adr, index, subvar)
            val = read_type(adr, typename)
            self.output += "%s:%s@%08X=%08X\n"%(var, typename, adr, val)
        else:
            self.output += "%s not in context of %s"%(line, curfunc)
    
    do_p = do_print
    
    def do_showlocals(self, line):
        """ show function and variable in current context"""
        if(Rsp.state != STOPPED):
            self.output = "Command only possible during STOPPED-state."
            return
        curfunc = get_func(Rsp.pc) 
        self.output = "Funktion:%s"%curfunc
        for var in stacklocalmap[curfunc]:
            self.output += "%s:%s\n"%(var, typemap[stacklocalmap[curfunc][var].typenr].name) 
        for var in reglocalmap[curfunc]:
            self.output += "%s:%s\n"%(var, typemap[reglocalmap[curfunc][var].typenr].name)
    
    do_sloc = do_showlocals
    
    def do_showglobals(self, line):
        """ show function and variable in current context"""
        if(Rsp.state != STOPPED):
            self.output = "Command only possible during STOPPED-state."
            return
        for var in globalmap:
            self.output += "%s:%s\n"%(var, typemap[globalmap[var].typenr].name)
    
    do_sglo = do_showglobals
    
    def do_info(self, line):
        """ get stop status """
        if(Rsp.state != STOPPED):
            self.output = "Command only possible during STOPPED-state."
            return
        Rsp.sendpkt("?")
        self.output = self.rxqueue.get()
    
    def do_bye(self, args):
        return True
    
    def precmd(self, line):                    
        with screenlock:
            clearscreen(self.stdout_ori)            
            pos(self.stdout_ori, CMDLINE + 1, 1)
        return cmd.Cmd.precmd(self, line)
    
    def postcmd(self, stop, line):
        with screenlock:            
            line = get_line(Rsp.pc)
            print_file_line(self.stdout_ori, line.filename, line.row)
            pos(self.stdout_ori, CMDLINE - 1, 1)            
            self.stdout_ori.write(self.output)
            self.stdout_ori.flush()
            self.output = ""            
            Rsp.logger.debug('Postcmd!')      
            pos(self.stdout_ori, CMDLINE - 2, 1)                                  
            
        return cmd.Cmd.postcmd(self, stop, line)

Cli = DebugCli()        
    
class gdbrsp(dbgif):
    def __init__(self, port):
        self.s = socket(AF_INET, SOCK_STREAM)
        self.s.connect(("localhost", port))
        self.state = RUNNING
        self.stopreason = BRKPOINT
        self.pc = 0
        self.curline = 0
        self.curfile = ""
        self.rxqueue = Queue()
        self.rxthread = Thread(target=self.recv_thread)
        self.running = True
        self.rxthread.start()
        self.logger = logging.getLogger('dbg') 

    
    def pack(self, data):
        """ formats data into a RSP packet """
        for a, b in [(x, chr(ord(x) ^ 0x20)) for x in ['}','*','#','$']]:
            data = data.replace(a,'}%s' % b)
            crc = (sum(ord(c) for c in data) % 256) 
            return "$%s#%02X" %(data, crc)
    
    def unpack(self, pkt):
        """ unpacks an RSP packet, returns the data"""
        if pkt[0]!='$' or pkt[-3]!='#':
            raise ValueError('bad packet')
        if (sum(ord(c) for c in pkt[1:-3]) % 256) != int(pkt[-2:],16):
            raise ValueError('bad checksum')
        pkt = pkt[1:-3]
        return pkt
    
    def decodepkt(self, pkt):
        """ blocks until it reads an RSP packet, and returns it's data"""
        res = ""
        if pkt.startswith('$'):
            try:
                self.logger.debug('unpack< %s', pkt) 
                res = self.unpack(pkt)
            except ValueError as ex:
                self.logger.debug('GDB-< %s', res)
                self.logger.warning('Bad packet %s', ex)  
                self.s.send(b'-')
            else:
                self.s.send(b'+')
                self.logger.debug('GDB+< %s', res) 
                return res
        else:
            self.logger.warning('discards %s', pkt) 
            
    
    def sendpkt(self, data, retries=10):
        """ sends data via the RSP protocol to the device """        
        wire_data = self.pack(data).encode()
        self.logger.debug('sending> %s', data) 
        self.s.send(wire_data)
        res = self.rxqueue.get()
        while res != '+':
            self.s.send(wire_data)
            res = self.rxqueue.get()
            retries -= 1
            if retries == 0:
                raise ValueError("retry fail") 
        
    def halt(self):
        """ sends data via the RSP protocol to the device """
        self.s.send(chr(0x03).encode())
        
    def get_register(self, regnr):
        """ Get a single register """
        self.sendpkt("p %x" % regnr)
        data = self.rxqueue.get()
        data = bytes.fromhex(data)
        res, = struct.unpack('<I', data)
        return res
    
    def set_register(self, regnr, value):
        """ Get a single register """
        value = struct.pack('<I', value)
        value = binascii.b2a_hex(value).decode('ascii')
        self.sendpkt("P %x=%s" % (regnr, value))
        res = self.rxqueue.get()
    
    def set_breakpoint(self, adr):
        """ Set a breakpoint """
        self.sendpkt("Z0,%x,4" % adr)
        res = self.rxqueue.get()
    
    def clear_breakpoint(self, adr):
        """ Clear a breakpoint """
        self.sendpkt("z0,%x,4" % adr)
        res = self.rxqueue.get()
        
    def read_mem(self, adr, len):
        self.sendpkt("m %x,%x"%(adr, len),3)
        data  = self.rxqueue.get() 
        return data
    
    def write_mem(self, adr, data):
        self.sendpkt("M %x,%x:%s"%(adr, len(data)//2 , data),3)
        data = self.rxqueue.get()
        
    def resume(self):      
        self.sendpkt("c")
    
    def step(self): 
        self.set_register(32, self.pc)
        self.sendpkt("s")
    
    def process_stop_status(self, pkt):
        code = int(pkt[1:3], 16)
        self.reason = code
        res = pkt[3:]
        if pkt.startswith('T'):
            self.state = STOPPED
            pcpos = res.rfind(':') + 1
            data = bytes.fromhex(res[pcpos:-1])
            self.pc, = struct.unpack('<I', data)
            line = get_line(self.pc)
            self.curline = line.row
            self.curfile = line.filename
            self.logger.debug('stopping at %08x!'%self.pc)
            if self.reason == BRKPOINT:
                self.logger.debug('running update!')               
                self.pc -= 4
                #self.set_register(32, self.pc)                
                with screenlock:
                    savepos(Cli.stdout_ori)                    
                    line = get_line(Rsp.pc)
                    print_file_line(Cli.stdout_ori, line.filename, line.row)                    
                    pos(Cli.stdout_ori, CMDLINE + 1, 1)                    
                    restorepos(Cli.stdout_ori)
                    
                
        
    
    def rx_avail(self):
        readable, _, _ = select.select([self.s], [], [], 0)
        return readable 
    
    def recv_thread(self):
        while self.running:
            readable = self.rx_avail()
            if readable:
                data = self.s.recv(1)
                if data == b'$':
                    res = bytearray()
                    res.extend(data)
                    while True:
                        res.extend(self.s.recv(1))
                        if res[-1] == ord('#') and res[-2] != ord("'"):
                            res.extend(self.s.recv(1))
                            res.extend(self.s.recv(1))
                            pkt = self.decodepkt(res.decode('ascii'))                          
                            self.logger.debug('decoded:%s',pkt)
                            if pkt.startswith('S') or pkt.startswith('T'):
                                self.process_stop_status(pkt)
                            else:
                                self.rxqueue.put(pkt)
                            break
                elif data == b'+':
                    self.rxqueue.put(data.decode('ascii')) 
    
    
    def close(self):
        self.s.close()
        self.running = False

     
        
Rsp = gdbrsp(4567)


def get_func(adr):
    proc = ""
    min = -1;
    for key in funcmap:
        funcadr = funcmap[key].adr
        if min<0 or adr>=funcadr and adr-funcadr<min:
            min = adr-funcadr
            func = key 
    return(func)
    

def get_line(adr):
    minkey = min(adrmap.keys(), key=lambda k: abs(k - adr))
    return(adrmap[minkey])

def read_type(adr, tyname):
    """ Read from type from addres"""
    data = Rsp.read_mem(adr,tyname2size(tyname))
    data = bytes.fromhex(data)
    res, = struct.unpack(tyname2fmt(tyname), data)
    return(res)         
    
def ptr_array_struct(typenr, deref, adr, index, subvar):
    typedesc =  typemap[typenr].desc
    if typedesc[1] == "*":
        typename = PTRTYPE
    else:
        typename = typemap[typenr].name
    if deref and typedesc[1] == "*":
        adr = read_type(adr, PTRTYPE);
        typedesc = typemap[typenr].desc
        typenr = str2int(typedesc[2:])
        typename = typemap[typenr].name
        typedesc = typemap[typenr].desc
    if(index != 0) and typedesc[1] == "a":
        typenr = str2int(typedesc[2:].split(';')[0])
        typename = typemap[typenr].name
        size = tyname2size(typename)
        adr += index*size
    if(subvar) and typedesc[1]== "s":
        subtypes = typedesc.split(';')[1:]
        for subtype in subtypes:
         if  subtype[0:subtype.find(',')] == subvar:
            break
        str = subtype.split(',')
        typenr = str2int(str[1])
        typename = typemap[typenr].name
        adr += str2int(str[2])//8        
    return adr, typename




        
if __name__ == '__main__':
   # --- load debug information
    logging.basicConfig(level=logging.DEBUG,filename="log.txt" )
    load_debug()
    Cli.cmdloop()