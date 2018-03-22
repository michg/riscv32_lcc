import sys, os, string
import cmd
from socket import *
import binascii
import select
import struct
from collections import namedtuple 
try:
    from colorama import init
except:
    print("for ansi on win32 package colorama needed")

STOPPED = 0
RUNNING = 1

INTERRUPT = 1
BRKPOINT = 2

PTRSIZE = 4
PTRTYPE = 'unsigned int'

INDADR = 0
INDTYPENUM = 1

INDTYPENAME = 0
INDTYPEDESC = 1
INDTYPESUBTY = 2 

fmts = {
    'unsigned int': ('<I',4),
    'void': ('<I',4),
    'int': ('<i',4),
    'unsigned short': ('<H',2),
    'short':('<h',2),
    'unsigned char':('<B',1),
    'signed char':('<b',1)
    }



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

def clearscreen():
    print("\033[2J\033[1;1H") 

def print_file_line(filename, lineno):
    clearscreen()
    lines = open(filename).read().splitlines()

    #show file and line number
    print("\033[37m\033[1mFile:", filename)
    print("Line:", "[", lineno, "of", len(lines), "]")
    print("\033[0m")
    print("\033[39m")

    #Print a fragment of the file to show in context
    for i in range(lineno - 3, lineno + 3):
        if i < 1:
            print()
        elif i > len(lines):
            print()
        elif i == lineno:
            print("\033[33m\033[1m", str(i).rjust(4), "\033[32m->", lines[i-1], "\033[0m\033[39m")
        else:
            print("\033[33m\033[1m", str(i).rjust(4), "\033[0m\033[39m  ", lines[i-1])

# --- debug information
def read_debug_symbols(module):
    global maxfileline
    maxfileline = 0
    for line in open(module+'.dbg'):
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
         
    for line in open(module+'.dbg'):
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


def var2adr(var):
    return(var[INDADR])

def var2tynum(var):
    return(var[INDTYPENUM])
 
def tynum2tyname(typenum):
    return(typedef[typenum][INDTYPENAME])

def var2tyname(var, ref):
    if(ref==0):
        return PTRTYPE;
    typenr = var2tynum(var)
    typedesc = tynum2tydesc(typenr)
    if(tyispointer(typenr)):
        typerefnr = str2int(typedesc[2:])
        return(typename(typerefnr));
    if(tyisarray(typenr)):
        typerefnr = str2int(typedesc[2:typedesc.find(';')])
        return(typename(typerefnr))
    return(typedef[typenr][INDTYPENAME])

def tynum2tydesc(typenum):
    return(typedef[typenum][INDTYPEDESC]) 

def var2tydesc(var):
    return(typedef[var2tynum(var)][INDTYPEDESC]) 

def tynum2tysubty(typenum):
    return(typedef[typenum][INDTYPESUBTY])    
    
def var2tysubty(var):
    return(typedef[var2tynum(var)][INDTYPESUBTY])    

def tyispointer(typenr):
    typedesc = tynum2tydesc(typenr)
    if(typedesc[1] == '*'):
        return 1
    else:
        return 0
        
def varispointer(var):
    return(tyispointer(var2tynum(var)))

def tyisarray(typenr):
    typedesc = tynum2tydesc(typenr)
    if(typedesc[1] == 'a'):
        return 1
    else:
        return 0

def varisarray(var):
    return(tyisarray(var2tynum(var)))

def tyname2size(name):
    return(fmts[name][1])

def tyname2fmt(name):
    return(fmts[name][0])

def typename(typenr):
    typedesc = tynum2tydesc(typenr)
    if(tyispointer(typenr)):
        typerefnr = str2int(typedesc[2:])
        return(typename(typerefnr) + '*');
    if(tyisarray(typenr)):
        typerefnr = str2int(typedesc[2:typedesc.find(';')])
        return(typename(typerefnr) + '[]')
    return(typedef[typenr][INDTYPENAME])

def varloctype(var):
    return(var[2]) 

class rsp:
    def __init__(self, port):
        self.s = socket(AF_INET, SOCK_STREAM)
        self.s.connect(("localhost", port))
        self.state = RUNNING
        self.stopreason = BRKPOINT
        self.pc = 0

    
    def pack(self, data):
        """ formats data into a RSP packet """
        for a, b in [(x, chr(ord(x) ^ 0x20)) for x in ['}','*','#','$']]:
            data = data.replace(a,'}%s' % b)
            return bytes("$%s#%02X" % (data, (sum(ord(c) for c in data) % 256)),'ascii')
    
    def unpack(self, pkt):
        """ unpacks an RSP packet, returns the data"""
        if pkt[0:1]!=b'$' or pkt[-3:-2]!=b'#':
            raise ValueError('bad packet')
        if (sum(c for c in pkt[1:-3]) % 256) != int(pkt[-2:],16):
            raise ValueError('bad checksum')
        pkt = pkt[1:-3]
        return pkt
    
    def sendpkt(self, data ,retries=50):
        """ sends data via the RSP protocol to the device """
        self.s.send(self.pack(data))
        res = None
        while not res:
            res = self.s.recv(1)
        discards = []
        while res!=b'+' and retries>0:
            discards.append(res)
            self.s.send(self.pack(data))
            retries-=1
            res = self.s.recv(1)
        if len(discards)>0: print('send discards', discards)
        if retries==0:
            raise ValueError("retry fail")
        
    def sendbrk(self):
        """ sends data via the RSP protocol to the device """
        self.s.send(chr(0x03).encode())
        
    
    def readpkt(self):
        """ blocks until it reads an RSP packet, and returns it's
           data"""
        c=None
        discards=[]
        while(c!=b'$'):
            if c: discards.append(c)
            c=self.s.recv(1)
        if len(discards)>0: print('discards', discards)
        res=[c]

        while True:
            res.append(self.s.recv(1))
            if res[-1]==b'#' and res[-2]!=b"'":
                    res.append(self.s.recv(1))
                    res.append(self.s.recv(1))
                    try:
                        res=self.unpack(b''.join(res))
                    except:
                        self.s.send(b'-')
                        res=[]
                        continue
                    self.s.send(b'+')
                    return res.decode('ascii')
                    
    def _get_register(self, idx):
        """ Get a single register """
        self.sendpkt("p %x" % idx)
        data = self.readpkt()
        data = bytes.fromhex(data)
        res, = struct.unpack('<I', data)
        return res
    
    def _set_register(self, idx, value):
        """ Get a single register """
        value = struct.pack('<I', value)
        value = binascii.b2a_hex(value).decode('ascii')
        self.sendpkt("P %x=%s" % (idx, value))
        res = self.readpkt()
    
    def _get_line(self, address):
        maxline = 0
        for key in address_line:
            if key<=address and address_line[key]>maxline:
                maxline = address_line[key]
        return(maxline)
    
    def _get_proc(self, address):
        proc = ""
        min = 10000;
        for key in proc_address:
            procadr = proc_address[key]
            if address>=procadr and address-procadr<min:
                min = address-procadr
                proc = key
        return(proc)
    
    def set_breakpoint(self, address):
        """ Set a breakpoint """
        Rsp.sendpkt("Z0,%x,4" % address)
        res = Rsp.readpkt()
    
    def clear_breakpoint(self, address):
        """ Clear a breakpoint """
        Rsp.sendpkt("z0,%x,4" % address)
        Rsp.readpkt()

    def process_stop_status(self):
        res = self.readpkt()
        if res.startswith('T'):
            code = int(res[1:3], 16)
            self.reason = code
            self.state = STOPPED
            pcpos = res.rfind(':') + 1
            data = bytes.fromhex(res[pcpos:-1])
            self.pc, = struct.unpack('<I', data)
            self.curline = self._get_line(self.pc)
            print_file_line(srcname, self.curline)

    def read_type(self, adr, tyname):
        """ Read from type from addres"""
        Rsp.sendpkt("m %x,%x" %(adr, tyname2size(tyname)),3)
        data = Rsp.readpkt() 
        data = bytes.fromhex(data)
        res, = struct.unpack(tyname2fmt(tyname), data)
        return(res)
        
    def close(self):
        self.s.close()

Rsp = rsp(4567)

class DebugCli(cmd.Cmd):
    """ Implement a console-based debugger interface. """
    prompt = '(dbg)> '
    intro = "interactive debugger"

    def __init__(self):
        super().__init__()
    
    def do_exit(self, line): 
        """exit the interpreter session"""
        Rsp.close()
        sys.exit(-1)
    
    def do_rd(self, line): 
        """rd adr,len: read len bytes from memory at adr"""
        if(Rsp.state != STOPPED):
            print("Command only possible during STOPPED-state.")
            return
        x = line.split(',')
        adr = str2int(x[0])
        count = str2int(x[1])
        Rsp.sendpkt("m %x,%x"%(adr,count),3)
        data = Rsp.readpkt() 
        print(binascii.a2b_hex(data))
      
    def do_wr(self, line): 
        """wr adr,len,hexdata: write len bytes with hexdata to adr in memory"""
        if(Rsp.state != STOPPED):
            print("Command only possible during STOPPED-state.")
            return
        x = line.split(',')
        adr = int(x[0])
        count = int(x[1])
        data = x[2]
        Rsp.sendpkt("M %x,%x:%s"%(adr,count,data),3)
        print(Rsp.readpkt())
      
    
    def do_run(self, line):
        """start the device"""
        if(Rsp.state != STOPPED):
            print("Command only possible during STOPPED-state.")
            return
        if(Rsp.reason == BRKPOINT):
            pc = Rsp._get_register(32)
            Rsp._set_register(32,pc - 4)
            print("BRK")
        Rsp.sendpkt("c")
        Rsp.state = RUNNING
    
    
    def do_break(self, line):
        """break: send break to the processor"""
        if(Rsp.state != RUNNING):
            print("Command only possible during RUNNING-state.")
            return
        Rsp.sendbrk()
        Rsp.process_stop_status()
    
    def do_restart(self, line):
        """ restart the device """
        if(Rsp.state != STOPPED):
            print("Command only possible during STOPPED-state.")
            return
        Rsp._set_register(32, 0) 
        self.do_run("") 
        Rsp.state = RUNNING 
    
    def do_showpc(self,line):
        """Read the pc"""
        if(Rsp.state != STOPPED):
            print("Command only possible during STOPPED-state.")
            return
        print("PC = %08X"%Rsp.pc)
    
    def do_setpc(self,line):
        """Set the pc"""
        if(Rsp.state != STOPPED):
            print("Command only possible during STOPPED-state.")
            return
        val = str2int(line)
        Rsp._set_register(32, val)
            
    def do_getregs(self,line):
        """Read all registers"""
        if(Rsp.state != STOPPED):
            print("Command only possible during STOPPED-state.")
            return
        Rsp.sendpkt("g")
        data = Rsp.readpkt()
        data = bytes.fromhex(data)
        #data = binascii.a2b_hex(data)
        offset = 0
        for i in range(0, 33):
            regdata = data[offset:offset+4]
            regval, = struct.unpack('<I', regdata)
            print("Register %02X:%08X"%(i,regval))
            offset+=4
    
    def do_setbrk(self, line):
        """ Set a breakpoint: setbrk linenr """
        if(Rsp.state != STOPPED):
            print("Command only possible during STOPPED-state.")
            return
        linenum = str2int(line)
        adr = line_address[linenum]
        Rsp.set_breakpoint(adr)

    def do_clrbrk(self, line):
        """ Clear a breakpoint: clrbrk linenr
        """
        if(Rsp.state != STOPPED):
            print("Command only possible during STOPPED-state.")
            return
        linenum = str2int(line)
        adr = line_address[linenum]
        Rsp.clear_breakpoint(adr)

    def do_stepi(self, line):
        """ step one instruction """
        if(Rsp.state != STOPPED):
            print("Command only possible during STOPPED-state.")
            return
        if(Rsp.reason == BRKPOINT):
            Rsp._set_register(Rsp.pc - 4, 0)
        Rsp.sendpkt("s")
        Rsp.process_stop_status()
    
    do_si = do_stepi
    
    def do_stepl(self, line):
        """ step one line """
        if(Rsp.state != STOPPED):
            print("Command only possible during STOPPED-state.")
            return
        Rsp.clear_breakpoint(Rsp.pc)
        lastfunc = Rsp._get_proc(Rsp.pc)
        curfunc = lastfunc
        lastline = Rsp.curline
        while Rsp.curline==lastline or curfunc!=lastfunc:
            self.do_si("")
            curfunc = Rsp._get_proc(Rsp.pc)
    
    
    do_sl = do_stepl
    

    def do_print(self, line):
        """ print variable """
        if(Rsp.state != STOPPED):
            print("Command only possible during STOPPED-state.")
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
        if var in global_var:
            adr = var2adr(global_var[var])
            vartypename = var2tyname(global_var[var], deref)
            if deref:
                adr = Rsp.read_type(adr, PTRTYPE) 
            if(index != 0):
                size = tyname2size(vartypename)
                adr += index*size
            val = Rsp.read_type(adr, vartypename)
            print("%s:%s@%08X=%08X"%(var,vartypename,adr,val))
        else:
            curfunc = Rsp._get_proc(Rsp.pc) 
            if var in local_var[curfunc]:
                if(varloctype(local_var[curfunc][var]).find('stack') != -1):
                    ofs = var2adr(local_var[curfunc][var])
                    curbase = Rsp._get_register(8); 
                    adr = curbase + ofs         
                    vartypename = var2tyname(local_var[curfunc][var], deref)
                    if deref:
                        adr = Rsp.read_type(adr, PTRTYPE);
                    if(index != 0):
                        size = tyname2size(vartypename)
                        adr += index*size
                    val = Rsp.read_type(adr, vartypename)
                    print("%s:%s@%08X=%08X"%(line,vartypename,adr,val))
                elif(varloctype(local_var[curfunc][var]).find('reg') != -1):
                    regnum = var2adr(local_var[curfunc][var])
                    val = Rsp._get_register(regnum);
                    vartypename = var2tyname(local_var[curfunc][var], deref)
                    if deref:
                        val = Rsp.read_type(val, vartypename)
                    print("%s = %08X"%(line,val))
            else:
                print("%s not in context of %s"%(line, curfunc))
    
    do_p = do_print
    
    def do_showlocals(self, line):
        """ show function and variable in current context"""
        if(Rsp.state != STOPPED):
            print("Command only possible during STOPPED-state.")
            return
        curfunc = Rsp._get_proc(Rsp.pc) 
        print("Funktion:%s"%curfunc)
        for var in local_var[curfunc]:
            typenr = var2tynum(local_var[curfunc][var])
            print("%s:%s"%(var, typename(typenr)))
    
    do_sloc = do_showlocals
    
    def do_showglobals(self, line):
        """ show function and variable in current context"""
        if(Rsp.state != STOPPED):
            print("Command only possible during STOPPED-state.")
            return
        for var in global_var:
            typenr = var2tynum(global_var[var])
            print("%s:%s"%(var, typename(typenr)))
    
    do_sglo = do_showglobals
    
    def do_info(self, line):
        """ get stop status """
        if(Rsp.state != STOPPED):
            print("Command only possible during STOPPED-state.")
            return
        Rsp.sendpkt("?")
        print(Rsp.readpkt())
        
    
    def precmd(self, line):
        readable, writable, exceptional = select.select([Rsp.s], [], [], 0)
        if readable:
             Rsp.process_stop_status()
        return cmd.Cmd.precmd(self, line)
    
    def do_bye(self, args):
        return True

if __name__ == '__main__':
   # --- load debug information
    init()
    load_debug_information(sys.argv[1])
    DebugCli().cmdloop()