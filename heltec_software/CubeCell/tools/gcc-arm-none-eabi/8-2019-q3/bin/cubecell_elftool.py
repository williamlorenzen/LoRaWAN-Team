import codecs
import os
import six
import sys


def replace(start,lenth,str,repstr):
    start=start+11
    str=str[0:start]+repstr+str[start+lenth:]
    return(str)
   
hex_decoder = codecs.getdecoder('hex')

objcopy=sys.argv[1]
file_elf=sys.argv[2] 
file_hex=sys.argv[3] 
file_cyacd=sys.argv[4]   

cmd=objcopy+' -O ihex '+file_elf+' '+file_hex
#print(cmd)
os.system(cmd)

f = open(file_hex)
list_cyacd=[]
list_cyacd.append('256A11B50000')

startline=0x26 #end of bootloader
row_cyacd = 0
flash = 0
zero512=''
for i in range(0,512):
    zero512=zero512+'0'


#creat empty data list with index same as output.cyacd 
for line in f:
    data_hex=line.strip()
    index_hex=hex_decoder(data_hex[1:9])[0]
    index_cyacd=hex_decoder(list_cyacd[row_cyacd][3:9])[0]
#    print(index_hex[0],index_hex[1],index_hex[2],index_hex[3])
#    print(index_hex[0],index_hex[1],index_hex[2],index_hex[3])
    if data_hex == ':020000021000EC':
        flash = 1
    elif flash == 0 and index_hex[1] >= startline and index_cyacd[1] != index_hex[1]:
        list_cyacd.append(':000'+str(flash)+str.upper(hex(index_hex[1])[2:])+'0100'+zero512)
        row_cyacd=row_cyacd+1
    elif flash == 1 and index_cyacd[1] != index_hex[1]:
        if index_hex[1] < 0x10:
            list_cyacd.append(':000'+str(flash)+'0'+str.upper(hex(index_hex[1])[2:])+'0100'+zero512)
        else:
            list_cyacd.append(':000'+str(flash)+str.upper(hex(index_hex[1])[2:])+'0100'+zero512)
        row_cyacd=row_cyacd+1
        if index_hex[1] == 0xFF:
            flash = 2   
f.close()   

f = open(file_hex)
flash = 0  
max_row =  row_cyacd
row_cyacd = 0   
last_index = 0 
def getrow(index):
    for i in range(1,max_row+1):
        index_cyacd=hex_decoder(list_cyacd[i][3:7])[0]
        if index == index_cyacd[0] * 0x100 + index_cyacd[1]:
            return i

      
for line in f:
    data_hex=line.strip()
    index_hex=hex_decoder(data_hex[1:9])[0]
    
    if data_hex == ':020000021000EC':
        flash = 1    
    elif flash == 0 and index_hex[1] >= startline :
        index = index_hex[1] + flash * 0x100
        row_cyacd=getrow(index)
        start = index_hex[2] * 2        
        len = index_hex[0]*2
        
        if start+len<=512:
            list_cyacd[row_cyacd]=replace(start,len,list_cyacd[row_cyacd],data_hex[9:9+len])
        else:
            len1 = 512 - start
            list_cyacd[row_cyacd]=replace(start,len1,list_cyacd[row_cyacd],data_hex[9:9+len1])
            row_cyacd=row_cyacd+1
            start = 0
            list_cyacd[row_cyacd]=replace(start,len-len1,list_cyacd[row_cyacd],data_hex[9+len1:9+len])
            
    elif flash == 1:
        if last_index == 0xFF and index_hex[1]!=0xFF:
            flash = 2
        else:            
            index = index_hex[1] + flash * 0x100
            row_cyacd=getrow(index)
            start = index_hex[2] * 2        
            len = index_hex[0]*2      

            if start+len<=512:
                list_cyacd[row_cyacd]=replace(start,len,list_cyacd[row_cyacd],data_hex[9:9+len])
            else:                
                len1 = 512 - start
                list_cyacd[row_cyacd]=replace(start,len1,list_cyacd[row_cyacd],data_hex[9:9+len1])
                row_cyacd=row_cyacd+1
                start = 0
                list_cyacd[row_cyacd]=replace(start,len-len1,list_cyacd[row_cyacd],data_hex[9+len1:9+len])
        last_index=index_hex[1]  
        
appsum=0
for i in range(1,max_row+1):
    data=hex_decoder(list_cyacd[i][11:])[0] 
    if i != (max_row+1):
        if (six.PY2):
            appsum=appsum+sum(ord(x) for x in data[:])
        if (six.PY3):
            appsum=appsum+sum(data[:])
    else:
        if (six.PY2):
            appsum=appsum+sum(ord(x) for x in data[:-64])
        if (six.PY3):
            appsum=appsum+sum(data[:-64])        
        
app_checksum = 0x100 - (appsum & 0xFF)    
if app_checksum == 0x100:
    app_checksum = 0x00

if app_checksum < 0x10:
    Metadata='0'+str.upper(hex(app_checksum)[2:])+'112600002500000000'
else:
    Metadata=str.upper(hex(app_checksum)[2:])+'112600002500000000'

if (max_row-1) < 0x10:
    Metadata=Metadata+'0'+str.upper(hex(max_row-1)[2:])
else:
    Metadata=Metadata+str.upper(hex(max_row-1)[2:])

list_cyacd[max_row]=replace(384,22,list_cyacd[max_row],Metadata)


for i in range(1,max_row+1):
    data = hex_decoder(list_cyacd[i][1:])[0]
    if (six.PY2):
        data_checksum = 0x100 - (sum(ord(x) for x in data[:]) & 0xFF)
    elif (six.PY3):
        data_checksum = 0x100 - (sum(data[:]) & 0xFF)
        
    if data_checksum == 0x100:
        data_checksum = 0x00
        
    if data_checksum < 0x10:
        list_cyacd[i]=list_cyacd[i]+'0'+str.upper(hex(data_checksum)[2:])
    else:
        list_cyacd[i]=list_cyacd[i]+str.upper(hex(data_checksum)[2:])
        
if os.path.exists(file_cyacd):
    os.remove(file_cyacd)
row_cyacd = 0
f = open(file_cyacd,'a')
for line in list_cyacd:
    f.write(list_cyacd[row_cyacd])
    f.write('\n')
    row_cyacd=row_cyacd+1
f.close()


