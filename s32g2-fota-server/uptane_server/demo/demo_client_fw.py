from __future__ import print_function
from __future__ import unicode_literals




import os # For paths and makedirs
import shutil # For copyfile
import demo

from six.moves import xmlrpc_client
from six.moves import xmlrpc_server
import readline, rlcompleter # for tab completion in interactive Python shell

import xmlrpclib
import json

import socket

server = xmlrpc_client.ServerProxy(
    'http://' + str(demo.DIRECTOR_SERVER_HOST) + ':' +
    str(demo.DIRECTOR_SERVER_PORT))

#recieve
print("connect to ServerProxy")
Test_vin = 'veh1'
Test_ecu_serial = 'ecu1'
manifest_address= '2dsa'
file_info =  server.push_vehicle_manifest(Test_vin,Test_ecu_serial,manifest_address)

print(file_info)
'''
def main():
    client_obj = xmlrpc_client.ServerProxy(
        'http://' + str(demo.DIRECTOR_SERVER_HOST) + ':' +
        str(demo.DIRECTOR_SERVER_PORT))
    #recieve
    print("connect to Director ServerProxy %s:%s"%(str(demo.DIRECTOR_SERVER_HOST),str(demo.DIRECTOR_SERVER_PORT)))
    #set the mainfest 
    mainfest = {'Test_vin':'veh1','Test_ecu_serial':'ecu1','version':'ecu001'}
    Test_vehicle_manifest = json.dumps(mainfest)
    
    Test_vin = 'veh1'
    Test_ecu_serial = 'ecu1'

    
    try:

        answer = client_obj.vehicle_manifest(Test_vin,Test_ecu_serial,str(Test_vehicle_manifest)).data
        config_path = '/home/songyu/workplace/cs-s32g-gw/uptane_test/'
        with open(config_path + 'config.txt') as fd:
            ctr_case = fd.readline()
            fd.close()
        
        mate_filepath = '/tftpboot/xuewei/s32v234/newroot/Fota_test'
        metadata_local = mate_filepath +'/'+ ctr_case + '/metadata.json'
        
        fp = open("metadata_local", "w+")
        fp.write(answer)
        fp.close()
    except xmlrpclib.Fault as e:
        print(e)

   

 

if __name__ == '__main__':
    readline.parse_and_bind('tab: complete')
    main()
   
'''