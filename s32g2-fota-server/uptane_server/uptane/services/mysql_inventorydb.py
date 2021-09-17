from __future__ import print_function
from __future__ import unicode_literals
from io import open

import uptane # Import before TUF modules; may change tuf.conf values.
import uptane.formats
import tuf
import uptane.services.mysql_manager as sm  


VEHICLE_MANIFEST_TABLE = 'vehicle_manifests'
ECU_MANIFEST_TABLE = 'ecu_manifests'

VEHICLE_TABLE = 'vehicle_table'
ECU_TABLE = 'ecu_table'
PRIMARY_ECU_TABLE = 'primary_ecu_table'
ECU_KEY_TABLE = 'ecu_key_table'


_VIN = 'vin'

#the ecu table 
_ECU_ID = 'id'
_ECU_SERIAL = 'ecu_serial'
_ECU_VIN = 'vin'
_ECU_IS_PRIMARY = 'isprimary'
_ECU_PUBLIC_KEY = 'public_key'
_ECU_HW_ID = 'hardwareIdentifier'
_ECU_IDENTIFIER = 'ecuIdentifier'
_ECU_HW_VERSION = 'hardwareVersion'
_ECU_INSTALL_METHOD =  'installMethod'
_ECU_IMAGE_FORMAT = 'imageFormat'
_ECU_IS_COMPRESSED = 'isCompressed'
_ECU_DEPENDENCY = 'isCompressed'
_ECU_CRYPTOGRAPH = 'cryptography_method'

_PRIMARY_ECU_SUB = 'subecus '


_KEY_ID = 'key_id'
_KEY_TYPE = 'key_type'
_KEY_HASH_1 = 'keyid_hash_algorithm_1'
_KEY_HASH_2 = 'keyid_hash_algorithm_2'
_KEY_PUBLIC = 'public_key'
_KEY_OF_ECU = 'ecu_serial'
_KEY_OF_ECU_OF_VEH = 'vin'


_VEH_MANIFEST = 'vehicle_manifest'
_VEH_MANIFEST_COUNT = 'count'
_VEH_VIN = 'vin'

_VECU_MANIFEST = 'ecu_manifest'
_VECU_MANIFEST_COUNT = 'count'
_VECU_SERIAL = 'ecu_serial'



# MYSQL instrion
VEH_MAX_COUNT = "max(count)"
VECU_MAX_COUNT = "max(count)"



'''
def mysql_connnect_test():
    
    sql_manager = sm.MysqlManager("DIRECTOR_DB","root","asdfg12345")
    insert_data_1 = [{"identifier": "'uptane_cs_test'",}]
    sql_manager.insert("vehicle", insert_data_1)
    print('============================')
    insert_data_2 = [{
        "identifier":"'s32v234'",
        "public_key":"'saddsadasdasfeas'",
        "cryptography_method":"'RSA'",
        "isprimary":"'1'",
        "vehicle_id":"'vuptane_cs_test'",
        }]
    sql_manager.insert("ecu",insert_data_2)
    show_list_1= ['identifier']
    show_list_2= ['public_key']
    t1 = sql_manager.get("vehicle",show_list_1)  
    t2 = sql_manager.get("ecu",show_list_2)  

    print('the vehicle table = ')
    print(t1)
    print('the ecu table = ')
    print(t2)
    #register_vehicle("dsad","wvw")
    print(get_vehicel_ID())
    print(get_ecu_ID('dsad'))
'''
def register_vehicle(vin, primary_ecu_serial, overwrite=True):
 

  vin_in_bd = _check_registration_is_sane(vin)

  if primary_ecu_serial is not None:
    uptane.formats.ECU_SERIAL_SCHEMA.check_match(primary_ecu_serial)

  tuf.formats.BOOLEAN_SCHEMA.check_match(overwrite)

  if not overwrite and vin in vin_in_bd:
    raise uptane.Spoofing('The given VIN, ' + repr(vin) + ', is already '
        'registered.')

  sql_manager = sm.MysqlManager("DIRECTOR_DB","CQ_OTA","123456")
  insert_vehicle_data = [{_VIN:str(vin) }]
  sql_manager.insert(VEHICLE_TABLE,insert_vehicle_data)

  if primary_ecu_serial is not None:
    sql_manager.insert(ECU_TABLE,insert_vehicle_data)
  else:
    sql_manager.insert(ECU_TABLE,insert_vehicle_data)
  insert_init_vehicle_manifest = [{_VIN:str(vin),_VEH_MANIFEST_COUNT:0}]
  sql_manager.insert(VEHICLE_MANIFEST_TABLE,insert_init_vehicle_manifest)


def _check_registration_is_sane(vin):
  sql_manager = sm.MysqlManager("DIRECTOR_DB","CQ_OTA","123456")
  uptane.formats.VIN_SCHEMA.check_match(vin)

  # A VIN may be in either none or all three of these dictionaries, and nowhere
  # in between, or ther(e is a bug.
  vehicle_manifests = sql_manager.get(VEHICLE_TABLE,[_VIN], condition = {_VIN:vin}) 
  ecus_by_vin = sql_manager.get(ECU_TABLE,[_VIN], condition = {_VIN:vin}) 
  primary_ecus_by_vin = sql_manager.get(PRIMARY_ECU_TABLE,[_VIN], condition = {_VIN:vin})  

  assert (vin in vehicle_manifests) == (vin in ecus_by_vin) == (
      vin in primary_ecus_by_vin), 'Programming error.'
  
def get_ecu_public_key(ecu_serial):

  uptane.formats.ECU_SERIAL_SCHEMA.check_match(ecu_serial)

  sql_manager = sm.MysqlManager("DIRECTOR_DB","CQ_OTA","123456")
  ecu_public_key_in_bd = sql_manager.get(ECU_KEY_TABLE,[_ECU_PUBLIC_KEY] ,condition = {_KEY_OF_ECU:ecu_serial}) 
  if ecu_public_key_in_bd is None:
      raise uptane.UnknownECU('The given ECU Serial, ' + repr(ecu_serial) +
        ' is not known. It must be registered.')

def get_vehicle_manifests(vin):
  vehicle_manifests = {}
  check_vin_registered(vin)
  sql_manager = sm.MysqlManager("DIRECTOR_DB","CQ_OTA","123456")
  vehicle_manifests_in_db = sql_manager.get(VEHICLE_MANIFEST_TABLE,[_VEH_MANIFEST] ,condition = {_VEH_VIN:vin}) 
  for manifest in vehicle_manifests_in_db:
    if manifest is not None:
      vehicle_manifests[vin].append(manifest)
  return vehicle_manifests[vin]

def get_last_vehicle_manifest(vin):
  check_vin_registered(vin)
  sql_manager = sm.MysqlManager("DIRECTOR_DB","CQ_OTA","123456")
  max_count = sql_manager.get(VEHICLE_MANIFEST_TABLE,[VEH_MAX_COUNT],condition = {_VEH_VIN:vin},get_one=True) 
  last_manifest = sql_manager.get(VEHICLE_MANIFEST_TABLE,[_VEH_MANIFEST],condition = {_VEH_MANIFEST_COUNT:max_count[0]},get_one=True)
  return last_manifest[0]

def save_vehicle_manifest(vin, signed_vehicle_manifest):
  check_vin_registered(vin) # check arg format and registration
  sql_manager = sm.MysqlManager("DIRECTOR_DB","CQ_OTA","123456")
  max_count = sql_manager.get(VEHICLE_MANIFEST_TABLE,[VEH_MAX_COUNT],condition = {_VEH_VIN:vin},get_one=True) 
  count = max_count[len(max_count)-1]+1
  insert_manifest = [{_VIN:str(vin),
                      _VEH_MANIFEST_COUNT:count,
                      _VEH_MANIFEST:str(signed_vehicle_manifest)}]

  sql_manager.insert(VEHICLE_MANIFEST_TABLE,insert_manifest)

def get_ecu_manifests(ecu_serial):
  ecu_manifests = {}
  check_ecu_registered(ecu_serial)

  sql_manager = sm.MysqlManager("DIRECTOR_DB","CQ_OTA","123456")
  ecu_manifests_in_db = sql_manager.get(ECU_MANIFEST_TABLE,[_ECU_MANIFEST] ,condition = {_VECU_SERIAL:ecu_serial}) 
  for manifest in ecu_manifests_in_db:
    if manifest is not None:
      ecu_manifests[vin].append(manifest)
  return ecu_manifests[ecu_serial]

def get_last_ecu_manifest(ecu_serial):
  check_ecu_registered(ecu_serial)

  sql_manager = sm.MysqlManager("DIRECTOR_DB","CQ_OTA","123456")
  max_count = sql_manager.get(ECU_MANIFEST_TABLE,[VECU_MAX_COUNT],condition = {_VECU_SERIAL:ecu_serial},get_one=True) 
  last_manifest = sql_manager.get(ECU_MANIFEST_TABLE,[_VECU_MANIFEST],condition = {_VECU_MANIFEST_COUNT:max_count.len()-1},get_one=True)
  return last_manifest[ast_manifest.len()-1]

def save_ecu_manifest(vin, ecu_serial, signed_ecu_manifest):
  check_ecu_registered(ecu_serial) 

  sql_manager = sm.MysqlManager("DIRECTOR_DB","CQ_OTA","123456")
  max_count = sql_manager.get(ECU_MANIFEST_TABLE,[VECU_MAX_COUNT],condition = {_VECU_SERIAL:ecu_serial},get_one=True) 
  count = max_count[max_count.len-1]+1
  insert_manifest = [{_VECU_SERIAL:str(vin),
                      _VECU_MANIFEST_COUNT:count,
                      _VECU_MANIFEST_:str(signed_ecu_manifest)}]

  sql_manager.insert(ECU_MANIFEST_TABLE,insert_manifest)

'''
def save_ecu_manifest(vin, ecu_serial, signed_ecu_manifest):

  check_ecu_registered(ecu_serial) # check format and registration

  uptane.formats.SIGNABLE_ECU_VERSION_MANIFEST_SCHEMA.check_match(
        signed_ecu_manifest)

  ecu_manifests[ecu_serial].append(signed_ecu_manifest)
'''

def check_ecu_registered(ecu_serial):

  uptane.formats.ECU_SERIAL_SCHEMA.check_match(ecu_serial)
  
  sql_manager = sm.MysqlManager("DIRECTOR_DB","CQ_OTA","123456")
  ecu_in_db = sql_manager.get(ECU_TABLE,[_ECU_SERIAL] ,condition = {_ECU_SERIAL:ecu_serial}) 
  

  if ecu_in_db is None:
    raise uptane.UnknownECU('The given ECU serial, ' + repr(ecu_serial) +
        ', is not known.')



def register_ecu(is_primary, vin, ecu_serial, public_key, overwrite=True):
  tuf.formats.BOOLEAN_SCHEMA.check_match(is_primary)
  uptane.formats.VIN_SCHEMA.check_match(vin)
  uptane.formats.ECU_SERIAL_SCHEMA.check_match(ecu_serial)
  tuf.formats.ANYKEY_SCHEMA.check_match(public_key)
  tuf.formats.BOOLEAN_SCHEMA.check_match(overwrite)

 
  sql_manager = sm.MysqlManager("DIRECTOR_DB","CQ_OTA","123456")

  if is_primary is not None:
    insert_ecu_data = [{
      _ECU_SERIAL: str(ecu_serial),
      _ECU_VIN: str(vin),
      _ECU_PUBLIC_KEY: str(public_key['keyid']),
      _ECU_CRYPTOGRAPH: str(public_key['keytype']),
      _ECU_IS_PRIMARY: True,
      }]
    sql_manager.insert(PRIMARY_ECU_TABLE,insert_ecu_data)
  else:
    insert_ecu_data = [{
      _ECU_SERIAL: str(ecu_serial),
      _ECU_VIN: str(vin),
      _ECU_PUBLIC_KEY: str(public_key['keyid']),
      _ECU_CRYPTOGRAPH: str(public_key['keytype']),
      _ECU_IS_PRIMARY: False,
      }]
    sql_manager.insert(ECU_TABLE,insert_ecu_data)

  insert_ecu_key_data = [{
  _KEY_OF_ECU: str(ecu_serial),
  _KEY_OF_ECU_OF_VEH: str(vin),
  _KEY_ID: str(public_key['keyid']),
  _KEY_TYPE: str(public_key['keytype']),
  _KEY_PUBLIC: str(public_key['keyval']['public']),
  _KEY_HASH_1:str(public_key['keyid_hash_algorithms'][0]),
  _KEY_HASH_2:str(public_key['keyid_hash_algorithms'][1]),
  }]

  sql_manager.insert(ECU_KEY_TABLE,insert_ecu_key_data)
  # Create an entry in the ecu_manifests dictionary for future manifests from
  # the ECU.


def check_vin_registered(vin):

  _check_registration_is_sane(vin)

  sql_manager = sm.MysqlManager("DIRECTOR_DB","CQ_OTA","123456")
  vin_in_db = sql_manager.get(VEHICLE_MANIFEST_TABLE,[_VIN] ,condition = {_VIN:vin}) 

  if vin_in_db is None:
    raise uptane.UnknownVehicle('The given VIN, ' + repr(vin) + ', is not '
        'known.') 

def clean_all_table():
  sql_manager = sm.MysqlManager("DIRECTOR_DB","CQ_OTA","123456")
  sql_manager.delete_table(VEHICLE_TABLE)
  sql_manager.delete_table(ECU_TABLE)
  sql_manager.delete_table(ECU_KEY_TABLE)

if __name__ == '__main__':
   mysql_connnect_test()