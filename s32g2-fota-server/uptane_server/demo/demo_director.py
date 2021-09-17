"""
<Program Name>
  demo_director_repo.py

<Purpose>
  Demonstration code handling a Director repository and services.

  Runs an Uptane-compliant demonstration Director.
  This accepts and validates ECU and Vehicle Manifests and writes and hosts
  metadata.

  Use:
    import demo.demo_director as dd
    dd.clean_slate()

  See README.md for more details.

<Demo Interfaces Provided Via XMLRPC>

  XMLRPC interface presented TO PRIMARIES:
    register_ecu_serial(ecu_serial, ecu_public_key, vin, is_primary=False)
    submit_vehicle_manifest(vin, ecu_serial, signed_ecu_manifest)

  XMLRPC interface presented TO THE DEMO WEBSITE:
    add_new_vehicle(vin)
    add_target_to_director(target_filepath, filepath_in_repo, vin, ecu_serial) <--- assign to vehicle
    write_director_repo() <--- move staged to live / add newly added targets to live repo
    get_last_vehicle_manifest(vin)
    get_last_ecu_manifest(ecu_serial)
    register_ecu_serial(ecu_serial, ecu_key, vin, is_primary=False)

"""
from __future__ import print_function
from __future__ import unicode_literals
import json
import xmlrpclib


import demo
import demo.test_log as t_log

import uptane # Import before TUF modules; may change tuf.conf values.
import uptane.services.director as director
import uptane.services.inventorydb as inventory
import uptane.services.mysql_inventorydb as sql_db
#import tuf.formats
import tuf.formats

from six.moves import getcwd
import uptane.encoding.asn1_codec as asn1_codec

import threading # for the director services interface
import os # For paths and symlink
import shutil # For copying directory trees
import sys, subprocess, time # For hosting
import tuf.repository_tool as rt
import demo.demo_image_repo as demo_image_repo # for the Image repo directory
from uptane import GREEN, RED, YELLOW, ENDCOLORS

from six.moves import xmlrpc_server # for the director services interface

import atexit # to kill server process on exit()


# Tell the reference implementation that we're in demo mode.
# (Provided for consistency.) Currently, primary.py in the reference
# implementation uses this to display banners for defenses that would otherwise
# be hard to notice. No other reference implementation code (secondary.py,
# director.py, etc.) currently uses this setting, but it could.
uptane.DEMO_MODE = True

LOG_PREFIX = uptane.TEAL_BG + 'Director:' + ENDCOLORS + ' '

LOG_TEST = t_log.Test_Logger()

KNOWN_VINS = ["S32G-VNP-RDB2",]

TARGET_RECORD = []


# Dynamic global objects
#repo = None
repo_server_process = None
director_service_instance = None
director_service_thread = None

def clean_slate(use_new_keys=False):

  global director_service_instance


  director_dir = os.path.join(uptane.WORKING_DIR, 'director')

  # Create a directory for the Director's files.
  if os.path.exists(director_dir):
    shutil.rmtree(director_dir)
  os.makedirs(director_dir)


  # Create keys and/or load keys into memory.

  print(LOG_PREFIX + 'Loading all keys')


  if use_new_keys:
    demo.generate_key('directorroot')
    demo.generate_key('directortimestamp')
    demo.generate_key('directorsnapshot')
    demo.generate_key('director') # targets

  key_dirroot_pub = demo.import_public_key('directorroot')
  key_dirroot_pri = demo.import_private_key('directorroot')
  key_dirtime_pub = demo.import_public_key('directortimestamp')
  key_dirtime_pri = demo.import_private_key('directortimestamp')
  key_dirsnap_pub = demo.import_public_key('directorsnapshot')
  key_dirsnap_pri = demo.import_private_key('directorsnapshot')
  key_dirtarg_pub = demo.import_public_key('director')
  key_dirtarg_pri = demo.import_private_key('director')
  

  print(LOG_PREFIX + 'Initializing vehicle repositories')
  # Create the demo Director instance.
  director_service_instance = director.Director(
      director_repos_dir=director_dir,
      key_root_pri=key_dirroot_pri,
      key_root_pub=key_dirroot_pub,
      key_timestamp_pri=key_dirtime_pri,
      key_timestamp_pub=key_dirtime_pub,
      key_snapshot_pri=key_dirsnap_pri,
      key_snapshot_pub=key_dirsnap_pub,
      key_targets_pri=key_dirtarg_pri,
      key_targets_pub=key_dirtarg_pub)
  
  #for vin in KNOWN_VINS:
  #  director_service_instance.add_new_vehicle(vin)
  '''
  test_vin = 'S32G-RDB2'
  test_ecu_public_key = demo.import_public_key('primary')
  test_ecu_serial = '/GATEWAY'
  director_service_instance.add_new_vehicle(test_vin)
  #add gateway
  
  director_service_instance.register_ecu_serial(
  test_ecu_serial, test_ecu_public_key, vin=test_vin,is_primary=True)
  add_target_to_director(
    os.path.join(demo.IMAGE_REPO_TARGETS_DIR, 'GATEWAY_Flash_driver.bin'),
      'GATEWAY_Flash_driver.bin',
      test_vin,
      test_ecu_serial)
  add_target_to_director(
      os.path.join(demo.IMAGE_REPO_TARGETS_DIR, 'GATEWAY_APP_0.bin'),
      'GATEWAY_APP_0.bin',
      test_vin,
      test_ecu_serial)
  
  #add BCM
  bcm_serial = '/BCM'
  director_service_instance.register_ecu_serial(
  bcm_serial, test_ecu_public_key, vin=test_vin,is_primary=False)
  add_target_to_director(
    os.path.join(demo.IMAGE_REPO_TARGETS_DIR, 'BCM_APP_V1.0.bin'),
      'BCM_APP_V1.0.bin',
      test_vin,
      bcm_serial)
  add_target_to_director(
      os.path.join(demo.IMAGE_REPO_TARGETS_DIR, 'BCM_Flash_driver.bin'),
      'BCM_Flash_driver.bin',
      test_vin,
      bcm_serial)


  vds_serial = '/PDC'
  #add VDS
  director_service_instance.register_ecu_serial(
  vds_serial, test_ecu_public_key, vin=test_vin,is_primary=False)
  add_target_to_director(
    os.path.join(demo.IMAGE_REPO_TARGETS_DIR, 'PDC_firmware.bin'),
      'PDC_firmware.bin',
      test_vin,
      vds_serial)  
  
 

  write_to_live()
  
  director_service_instance.vehicle_repositories[test_vin].targets.remove_target(  "S32G-RDB2/targets/GATEWAY_Flash_driver.bin")
  director_service_instance.vehicle_repositories[test_vin].targets.remove_target(  "S32G-RDB2/targets/GATEWAY_APP_0.bin")
  director_service_instance.vehicle_repositories[test_vin].targets.remove_target(  "S32G-RDB2/targets/PDC_firmware.bin")
  director_service_instance.vehicle_repositories[test_vin].targets.remove_target(  "S32G-RDB2/targets/BCM_APP_V1.0.bin")
  director_service_instance.vehicle_repositories[test_vin].targets.remove_target(  "S32G-RDB2/targets/BCM_Flash_driver.bin")
  
  print("================================================")
  
  add_target_to_director(
    os.path.join(demo.IMAGE_REPO_TARGETS_DIR, 'GATEWAY_Flash_driver_2.bin'),
      'GATEWAY_Flash_driver_2.bin',
      test_vin,
      test_ecu_serial)
 
  add_target_to_director(
      os.path.join(demo.IMAGE_REPO_TARGETS_DIR, 'GATEWAY_APP_2.bin'),
      'GATEWAY_APP_2.bin',
      test_vin,
      test_ecu_serial)
  
  write_to_live() 
  '''

  '''

  
  if demo.FOTA_S32G_SWITCH:
      # You can tell the Director about ECUs this way:
 
    test_vin = 'CS_Veh_013'
    test_ecu_public_key = demo.import_public_key('primary')
    test_ecu_serial = 'nxp-s32g274a-rdb'
    director_service_instance.register_ecu_serial(
        test_ecu_serial, test_ecu_public_key, vin='CS_Veh_013',is_primary=True)

    add_target_to_director(
        os.path.join(demo.IMAGE_REPO_TARGETS_DIR, 'S32v_test_1.0.txt'),
        'S32v_test_1.0.txt',
        test_vin,
        test_ecu_serial)

    test_secondary_ecu_public_key = demo.import_public_key('secondary')
    test_secondary_ecu_serial = 'nxp-mpac5748g-evb'
    director_service_instance.register_ecu_serial(
      test_secondary_ecu_serial, test_secondary_ecu_public_key, vin='CS_Veh_013',is_primary=False)

    add_target_to_director(
          os.path.join(demo.IMAGE_REPO_TARGETS_DIR, 'fota_sec_mpc5748g.srec'),
          'fota_sec_mpc5748g.srec',
          test_vin,
          test_secondary_ecu_serial)

  # Add a first target file, for use by every ECU in every vehicle in that the
  # Director starts off with. (Currently 3)
  # This copies the file to each vehicle repository's targets directory from
  # the Image Repository.

  for vin in inventory.ecus_by_vin:
      for ecu in inventory.ecus_by_vin[vin]:
        add_target_to_director(
            os.path.join(demo.IMAGE_REPO_TARGETS_DIR, 'INFO1.0.txt'),
            'INFO1.0.txt',
            vin,
            ecu)
 
  print(LOG_PREFIX + 'Signing and hosting initial repository metadata')


  write_to_live()

  add_target_to_director(
          os.path.join(demo.IMAGE_REPO_TARGETS_DIR, 'TCU1.0.txt'),
          'TCU1.0.txt',
          '111',
          'gat')
  write_to_live(vin_to_update='111')
 '''  


  host()

  
  listen()





def write_to_live(vin_to_update=None):
  # Release updated metadata.

  # For each vehicle repository:
  #   - write metadata.staged
  #   - copy metadata.staged to the live metadata directory
  global TARGET_RECORD

  for vin in director_service_instance.vehicle_repositories:
    if vin_to_update is not None and vin != vin_to_update:
      continue
    repo = director_service_instance.vehicle_repositories[vin]    
    repo_dir = repo._repository_directory
    repo.mark_dirty(['timestamp', 'snapshot'])
    repo.write() # will be writeall() in most recent TUF branch

    # clean privous targe
    if len(TARGET_RECORD) is not 0 and vin_to_update is not None :
      for target_file in TARGET_RECORD:
        print(vin_to_update+"/targets/"+ target_file)
        repo.targets.remove_target( vin_to_update+"/targets/"+ target_file)
    elif  len(TARGET_RECORD) is not 0 and vin_to_update is None: 
      for target_file in TARGET_RECORD:
        print("S32G-RDB2/targets/"+ target_file)
        repo.targets.remove_target( "S32G-RDB2/targets/"+ target_file)

    TARGET_RECORD = []



    assert(os.path.exists(os.path.join(repo_dir, 'metadata.staged'))), \
        'Programming error: a repository write just occurred; why is ' + \
        'there no metadata.staged directory where it is expected?'

    # This shouldn't exist, but just in case something was interrupted,
    # warn and remove it.
    if os.path.exists(os.path.join(repo_dir, 'metadata.livetemp')):
      print(LOG_PREFIX + YELLOW + 'Warning: metadata.livetemp existed already. '
          'Some previous process was interrupted, or there is a programming '
          'error.' + ENDCOLORS)
      shutil.rmtree(os.path.join(repo_dir, 'metadata.livetemp'))

    # Copy the staged metadata to a temp directory we'll move into place
    # atomically in a moment.
    shutil.copytree(
        os.path.join(repo_dir, 'metadata.staged'),
        os.path.join(repo_dir, 'metadata.livetemp'))

    # Empty the existing (old) live metadata directory (relatively fast).
    if os.path.exists(os.path.join(repo_dir, 'metadata')):
      shutil.rmtree(os.path.join(repo_dir, 'metadata'))

    # Atomically move the new metadata into place.
    os.rename(
        os.path.join(repo_dir, 'metadata.livetemp'),
        os.path.join(repo_dir, 'metadata'))

    

    if demo.FOTA_S32G_SWITCH:
      root_metadata_verison = repo.get_roledb_version('root')
      targrt_metadata_verison = repo.get_roledb_version('targets')
      snapshot_metadata_verison = repo.get_roledb_version('snapshot')
      timestamp_metadata_verison = repo.get_roledb_version('timestamp')
      #add version of metadata at the filename
      director_metada_path = os.path.join(repo_dir, 'metadata')
      #shutil.move(os.path.join(director_metada_path,'root.json'), os.path.join(director_metada_path,str(root_metadata_verison)+'.root.json'))
      if demo.METADATA_EXTENSION == '.json':
        shutil.move(os.path.join(director_metada_path,'targets.json'), os.path.join(director_metada_path,str(targrt_metadata_verison)+'.targets.json'))
        shutil.move(os.path.join(director_metada_path,'snapshot.json'), os.path.join(director_metada_path,str(snapshot_metadata_verison)+'.snapshot.json'))
      elif demo.METADATA_EXTENSION == '.der':
        shutil.move(os.path.join(director_metada_path,'targets.der'), os.path.join(director_metada_path,str(targrt_metadata_verison)+'.targets.der'))
        shutil.move(os.path.join(director_metada_path,'snapshot.der'), os.path.join(director_metada_path,str(snapshot_metadata_verison)+'.snapshot.der'))





def backup_repositories(vin=None):
  """
  <Purpose>
    Back up the last-written state (contents of the 'metadata.staged'
    directories in each repository).

    Metadata is copied from '{repo_dir}/metadata.staged' to
    '{repo_dir}/metadata.backup'.

  <Arguments>
    vin (optional)
      If not provided, all known vehicle repositories will be backed up.
      You may also provide a single VIN (string) indicating one vehicle
      repository to back up.

  <Exceptions>
    uptane.Error if backup already exists

  <Side Effecs>
    None.

  <Returns>
    None.
  """
  if vin is None:
    repos_to_backup = director_service_instance.vehicle_repositories.keys()
  else:
    repos_to_backup = [vin]

  for vin in repos_to_backup:
    repo = director_service_instance.vehicle_repositories[vin]
    repo_dir = repo._repository_directory

    if os.path.exists(os.path.join(repo_dir, 'metadata.backup')):
      raise uptane.Error('Backup already exists for repository ' +
          repr(repo_dir) + '; please delete or restore this backup before '
          'trying to backup again.')

    print(LOG_PREFIX + ' Backing up ' +
        os.path.join(repo_dir, 'metadata.staged'))
    shutil.copytree(os.path.join(repo_dir, 'metadata.staged'),
        os.path.join(repo_dir, 'metadata.backup'))





def restore_repositories(vin=None):
  """
  <Purpose>
    Restore the last backup of each Director repository.

    Metadata is copied from '{repo_dir}/metadata.backup' to
    '{repo_dir}/metadata.staged' and '{repo_dir}/metadata'

  <Arguments>
    vin (optional)
      If not provided, all known vehicle repositories will be restored to their
      backed-up state. You may also provide a single VIN (string) indicating
      one vehicle to restore from backup.

  <Exceptions>
    uptane.Error if backup does not exist

  <Side Effecs>
    None.

  <Returns>
    None.
  """
  if vin is None:
    repos_to_restore = director_service_instance.vehicle_repositories.keys()
  else:
    repos_to_restore = [vin]

  for vin in repos_to_restore:

    repo_dir = director_service_instance.vehicle_repositories[
        vin]._repository_directory

    # Copy the backup metadata to the metada.staged and live directories.  The
    # backup metadata should already exist if
    # sign_with_compromised_keys_attack() was called.

    if not os.path.exists(os.path.join(repo_dir, 'metadata.backup')):
      raise uptane.Error('Unable to restore backup of ' + repr(repo_dir) +
          '; no backup exists.')

    # Empty the existing (old) live metadata directory (relatively fast).
    print(LOG_PREFIX + 'Deleting ' + os.path.join(repo_dir, 'metadata.staged'))
    if os.path.exists(os.path.join(repo_dir, 'metadata.staged')):
      shutil.rmtree(os.path.join(repo_dir, 'metadata.staged'))

    # Atomically move the new metadata into place.
    print(LOG_PREFIX + 'Moving backup to ' +
        os.path.join(repo_dir, 'metadata.staged'))
    os.rename(os.path.join(repo_dir, 'metadata.backup'),
        os.path.join(repo_dir, 'metadata.staged'))

    # Re-load the repository from the restored metadata.stated directory.
    # (We're using a temp variable here, so we have to assign the new reference
    # to both the temp and the source variable.)
    print(LOG_PREFIX + 'Reloading repository from backup ' + repo_dir)
    director_service_instance.vehicle_repositories[vin] = rt.load_repository(
        repo_dir)

    # Load the new signing keys to write metadata. The root key is unchanged,
    # but must be reloaded because load_repository() was called.
    valid_root_private_key = demo.import_private_key('directorroot')
    director_service_instance.vehicle_repositories[vin].root.load_signing_key(
        valid_root_private_key)

    # Copy the staged metadata to a temp directory, which we'll move into place
    # atomically in a moment.
    shutil.copytree(os.path.join(repo_dir, 'metadata.staged'),
        os.path.join(repo_dir, 'metadata.livetemp'))

    # Empty the existing (old) live metadata directory (relatively fast).
    print(LOG_PREFIX + 'Deleting live hosted dir:' +
        os.path.join(repo_dir, 'metadata'))
    if os.path.exists(os.path.join(repo_dir, 'metadata')):
      shutil.rmtree(os.path.join(repo_dir, 'metadata'))

    # Atomically move the new metadata into place in the hosted directory.
    os.rename(os.path.join(repo_dir, 'metadata.livetemp'),
        os.path.join(repo_dir, 'metadata'))
    print(LOG_PREFIX + 'Repository ' + repo_dir + ' restored and hosted.')




def revoke_compromised_keys():
  """
  <Purpose>
    Revoke the current Timestamp, Snapshot, and Targets keys for all vehicles,
    and generate a new key for each role.  This is a high-level version of the
    common function to update a role key. The director service instance is also
    updated with the key changes.

  <Arguments>
    None.

  <Exceptions>
    None.

  <Side Effecs>
    None.

  <Returns>
    None.
  """

  global director_service_instance

  # Generate news keys for the Targets, Snapshot, and Timestamp roles.  Make
  # sure that the director service instance is updated to use the new keys.
  # The 'director' name actually references the targets role.
  # TODO: Change Director's targets key to 'directortargets' from 'director'.
  new_targets_keyname = 'new_director'
  new_timestamp_keyname = 'new_directortimestamp'
  new_snapshot_keyname = 'new_directorsnapshot'

  # References are needed for the old and new keys later below when we modify
  # the repository.  Generate new keys for the Targets role...
  demo.generate_key(new_targets_keyname)
  new_targets_public_key = demo.import_public_key(new_targets_keyname)
  new_targets_private_key = demo.import_private_key(new_targets_keyname)
  old_targets_public_key = director_service_instance.key_dirtarg_pub

  # Timestamp...
  demo.generate_key(new_timestamp_keyname)
  new_timestamp_public_key = demo.import_public_key(new_timestamp_keyname)
  new_timestamp_private_key = demo.import_private_key(new_timestamp_keyname)
  old_timestamp_public_key = director_service_instance.key_dirtime_pub

  # And Snapshot.
  demo.generate_key(new_snapshot_keyname)
  new_snapshot_public_key = demo.import_public_key(new_snapshot_keyname)
  new_snapshot_private_key = demo.import_private_key(new_snapshot_keyname)
  old_snapshot_public_key = director_service_instance.key_dirsnap_pub

  # Set the new public and private Targets keys in the director service.
  # These keys are shared between all vehicle repositories.
  director_service_instance.key_dirtarg_pub = new_targets_public_key
  director_service_instance.key_dirtarg_pri = new_targets_private_key
  director_service_instance.key_dirtime_pub = new_timestamp_public_key
  director_service_instance.key_dirtime_pri = new_timestamp_private_key
  director_service_instance.key_dirsnap_pub = new_snapshot_public_key
  director_service_instance.key_dirsnap_pri = new_snapshot_private_key

  for vin in director_service_instance.vehicle_repositories:
    repository = director_service_instance.vehicle_repositories[vin]
    repo_dir = repository._repository_directory

    # Swap verification keys for the three roles.
    repository.targets.remove_verification_key(old_targets_public_key)
    repository.targets.add_verification_key(new_targets_public_key)

    repository.timestamp.remove_verification_key(old_timestamp_public_key)
    repository.timestamp.add_verification_key(new_timestamp_public_key)

    repository.snapshot.remove_verification_key(old_snapshot_public_key)
    repository.snapshot.add_verification_key(new_snapshot_public_key)

    # Unload the old signing keys so that the new metadata only contains
    # signatures produced by the new signing keys. Since this is based on
    # keyid, the public key can be used.
    repository.targets.unload_signing_key(old_targets_public_key)
    repository.snapshot.unload_signing_key(old_snapshot_public_key)
    repository.timestamp.unload_signing_key(old_timestamp_public_key)

    # Load the new signing keys to write metadata. The root key is unchanged,
    # and in the demo it is already loaded.
    repository.targets.load_signing_key(new_targets_private_key)
    repository.snapshot.load_signing_key(new_snapshot_private_key)
    repository.timestamp.load_signing_key(new_timestamp_private_key)

    # The root role is not automatically marked as dirty when the verification
    # keys are updated via repository.<non-root-role>.add_verification_key().
    # TODO: Verify this behavior with the latest version of the TUF codebase.
    repository.mark_dirty(['root'])


  # Push the changes to "live".
  write_to_live()





def sign_with_compromised_keys_attack(vin=None):
  """
  <Purpose>
    Re-generate Timestamp, Snapshot, and Targets metadata for all vehicles and
    sign each of these roles with its previously revoked key.  The default key
    names (director, directorsnapshot, directortimestamp, etc.) of the key
    files are used if prefix_of_previous_keys is None, otherwise
    'prefix_of_previous_keys' is prepended to them.  This is a high-level
    version of the common function to update a role key. The director service
    instance is also updated with the key changes.

  <Arguments>
    vin (optional)
      If not provided, all known vehicles will be attacked. You may also provide
      a single VIN (string) indicating one vehicle to attack.

  <Side Effects>
    None.

  <Exceptions>
    None.

  <Returns>
    None.
  """

  global director_service_instance

  print(LOG_PREFIX + 'ATTACK: arbitrary metadata, old key, all vehicles')

  # Start by backing up the repository before the attack occurs so that we
  # can restore it afterwards in undo_sign_with_compromised_keys_attack.
  backup_repositories(vin)

  # Load the now-revoked keys.
  old_targets_private_key = demo.import_private_key('director')
  old_timestamp_private_key = demo.import_private_key('directortimestamp')
  old_snapshot_private_key = demo.import_private_key('directorsnapshot')

  current_targets_private_key = director_service_instance.key_dirtarg_pri
  current_timestamp_private_key = director_service_instance.key_dirtime_pri
  current_snapshot_private_key = director_service_instance.key_dirsnap_pri

  # Ensure the director service uses the old (now-revoked) keys.
  director_service_instance.key_dirtarg_pri = old_targets_private_key
  director_service_instance.key_dirtime_pri = old_timestamp_private_key
  director_service_instance.key_dirsnap_pri = old_snapshot_private_key

  repo_dir = None

  if vin is None:
    vehicles_to_attack = director_service_instance.vehicle_repositories.keys()
  else:
    vehicles_to_attack = [vin]

  for vin in vehicles_to_attack:

    repository = director_service_instance.vehicle_repositories[vin]
    repo_dir = repository._repository_directory

    repository.targets.unload_signing_key(current_targets_private_key)
    repository.snapshot.unload_signing_key(current_snapshot_private_key)
    repository.timestamp.unload_signing_key(current_timestamp_private_key)

    # Load the old signing keys to generate the malicious metadata. The root
    # key is unchanged, and in the demo it is already loaded.
    repository.targets.load_signing_key(old_targets_private_key)
    repository.snapshot.load_signing_key(old_snapshot_private_key)
    repository.timestamp.load_signing_key(old_timestamp_private_key)

    repository.timestamp.version = repository.targets.version + 1
    repository.timestamp.version = repository.snapshot.version + 1
    repository.timestamp.version = repository.timestamp.version + 1

    # Metadata must be partially written, otherwise write() will throw
    # a UnsignedMetadata exception due to the invalid signing keys (i.e.,
    # we are using the old signing keys, which have since been revoked.
    repository.write(write_partial=True)

    # Copy the staged metadata to a temp directory we'll move into place
    # atomically in a moment.
    shutil.copytree(os.path.join(repo_dir, 'metadata.staged'),
        os.path.join(repo_dir, 'metadata.livetemp'))

    # Empty the existing (old) live metadata directory (relatively fast).
    if os.path.exists(os.path.join(repo_dir, 'metadata')):
      shutil.rmtree(os.path.join(repo_dir, 'metadata'))

    # Atomically move the new metadata into place.
    os.rename(os.path.join(repo_dir, 'metadata.livetemp'),
        os.path.join(repo_dir, 'metadata'))

  print(LOG_PREFIX + 'COMPLETED ATTACK')





def undo_sign_with_compromised_keys_attack(vin=None):
  """
  <Purpose>
    Undo the actions executed by sign_with_compromised_keys_attack().  Namely,
    move the valid metadata into the live and metadata.staged directories, and
    reload the valid keys for each repository.

  <Arguments>
    vin (optional)
      If not provided, all known vehicles will be reverted to normal state from
      attacked state. You may also provide a single VIN (string) indicating
      one vehicle to undo the attack for.

  <Side Effects>
    None.

  <Exceptions>
    None.

  <Returns>
    None.
  """
  # Re-load the valid keys, so that the repository objects can be updated to
  # reference them and replace the compromised keys set.
  valid_targets_private_key = demo.import_private_key('new_director')
  valid_timestamp_private_key = demo.import_private_key('new_directortimestamp')
  valid_snapshot_private_key = demo.import_private_key('new_directorsnapshot')

  current_targets_private_key = director_service_instance.key_dirtarg_pri
  current_timestamp_private_key = director_service_instance.key_dirtime_pri
  current_snapshot_private_key = director_service_instance.key_dirsnap_pri

  # Set the new private keys in the director service.  These keys are shared
  # between all vehicle repositories.
  director_service_instance.key_dirtarg_pri = valid_targets_private_key
  director_service_instance.key_dirtime_pri = valid_timestamp_private_key
  director_service_instance.key_dirsnap_pri = valid_snapshot_private_key

  # Revert to the last backup for all metadata in the Director repositories.
  restore_repositories(vin)

  if vin is None:
    vehicles_to_attack = director_service_instance.vehicle_repositories.keys()
  else:
    vehicles_to_attack = [vin]

  for vin in vehicles_to_attack:

    repository = director_service_instance.vehicle_repositories[vin]
    repo_dir = repository._repository_directory

    # Load the new signing keys to write metadata.
    repository.targets.load_signing_key(valid_targets_private_key)
    repository.snapshot.load_signing_key(valid_snapshot_private_key)
    repository.timestamp.load_signing_key(valid_timestamp_private_key)

  print(LOG_PREFIX + 'COMPLETED UNDO ATTACK')





def add_target_to_director(target_fname, filepath_in_repo, vin, ecu_serial):
  """
  For use in attacks and more specific demonstration.

  Given the filename of the file to add, the path relative to the repository
  root to which to copy it, the VIN of the vehicle whose repository it should
  be added to, and the ECU's serial directory, adds that file
  as a target file (calculating its cryptographic hash and length) to the
  appropriate repository for the given VIN.

  <Arguments>
    target_fname
      The full filename of the file to be added as a target to the Director's
      targets role metadata. This file doesn't have to be in any particular
      place; it will be copied into the repository directory structure.

    filepath_in_repo
      The path relative to the root of the repository's targets directory
      where this file will be kept and accessed by clients. (e.g. 'file1.txt'
      or 'brakes/firmware.tar.gz')

    ecu_serial
      The ECU to assign this target to in the targets metadata.
      Complies with uptane.formats.ECU_SERIAL_SCHEMA

  """
  uptane.formats.VIN_SCHEMA.check_match(vin)
  uptane.formats.ECU_SERIAL_SCHEMA.check_match(ecu_serial)
  tuf.formats.RELPATH_SCHEMA.check_match(target_fname)
  tuf.formats.RELPATH_SCHEMA.check_match(filepath_in_repo)
  global TARGET_RECORD
  if vin not in director_service_instance.vehicle_repositories:
    raise uptane.UnknownVehicle('The VIN provided, ' + repr(vin) + ' is not '
        'that of a vehicle known to this Director.')

  repo = director_service_instance.vehicle_repositories[vin]
  repo_dir = repo._repository_directory

  print(LOG_PREFIX + 'Copying target file into place.')
  destination_filepath = os.path.join(repo_dir, 'targets', filepath_in_repo)

  # TODO: This should probably place the file into a common targets directory
  # that is then softlinked to all repositories.
  shutil.copy(target_fname, destination_filepath)

  print(LOG_PREFIX + 'Adding target' + repr(target_fname) + ' for ECU ' + repr(ecu_serial))


  custom = tuf.roledb.get_role_paths('targets', 'default')['/'+ filepath_in_repo.encode('gbk')]
  print("custom:"+str(custom))
  
  # This calls the appropriate vehicle repository.
  #director_service_instance.add_target_for_ecu(
  #    vin, ecu_serial, destination_filepath)
  director_service_instance.add_target_for_ecu(vin, ecu_serial, destination_filepath,custom)
  TARGET_RECORD.append(filepath_in_repo)





def host():
  """
  Hosts the Director repository (http serving metadata files) as a separate
  process. Should be stopped with kill_server().

  Note that you must also run listen() to start the Director services (run on
  xmlrpc).

  If this module already started a server process to host the repo, nothing will
  be done.
  """


  global repo_server_process

  if repo_server_process is not None:
    print(LOG_PREFIX + 'Sorry: there is already a server process running.')
    return

  # Prepare to host the director repo contents.

  os.chdir(demo.DIRECTOR_REPO_DIR)

  command = []
  #if sys.version_info.major < 3: # Python 2 compatibility
    #command = ['python', '-m', 'SimpleHTTPServer', str(demo.DIRECTOR_REPO_PORT)]
  #else:
  #command = ['python3', '-m', 'http.server', str(demo.DIRECTOR_REPO_PORT)]
  command = ['python3','../M_http_server_30401.py']


  # Begin hosting the director's repository.

  repo_server_process = subprocess.Popen(command, stderr=subprocess.PIPE)

  os.chdir(uptane.WORKING_DIR)

  print(LOG_PREFIX + 'Director repo server process started, with pid ' +
      str(repo_server_process.pid) + ', serving on port ' +
      str(demo.DIRECTOR_REPO_PORT) + '. Director repo URL is: ' +
      demo.DIRECTOR_REPO_HOST + ':' + str(demo.DIRECTOR_REPO_PORT) + '/')

  # Kill server process after calling exit().
  atexit.register(kill_server)

  # Wait / allow any exceptions to kill the server.
  #try:
    #time.sleep(1000000) # Stop hosting after a while.
  #except:
    #print('Exception caught')
  #   pass
  #finally:
     #if repo_server_process.returncode is None:
      #print('Terminating Director repo server process ' + str(repo_server_process.pid))
      #repo_server_process.kill()


# Restrict director requests to a particular path.
# Must specify RPC2 here for the XML-RPC interface to work.
class RequestHandler(xmlrpc_server.SimpleXMLRPCRequestHandler):
  rpc_paths = ('/RPC2',)


def register_vehicle_manifest_wrapper(
    vin, primary_ecu_serial, signed_vehicle_manifest):
  """
  This function is a wrapper for director.Director::register_vehicle_manifest().

  The purpose of this wrapper is to make sure that the data that goes to
  director.register_vehicle_manifest is what is expected.

  In the demo, there are two scenarios:

    - If we're using ASN.1/DER, then the vehicle manifest is a binary object
      and signed_vehicle_manifest had to be wrapped in an XMLRPC Binary()
      object. The reference implementation has no notion of XMLRPC (and should
      not), so the vehicle manifest has to be extracted from the XMLRPC Binary()
      object that is signed_vehicle_manifest in this case.

    - If we're using any other data format / encoding (e.g. JSON), then the
      vehicle manifest was transfered as an object that the reference
      implementation can already understand, and we just pass the argument
      along to the director module.

  """
  print(LOG_PREFIX + 'the register_vehicle_manifest_wrapper start')
  global director_service_instance
  
  if tuf.conf.METADATA_FORMAT == 'der':
    #print( " %x" % signed_vehicle_manifest.data)
    director_service_instance.register_vehicle_manifest(
        vin, primary_ecu_serial, signed_vehicle_manifest.data)

  else:
    director_service_instance.register_vehicle_manifest(
        vin, primary_ecu_serial, signed_vehicle_manifest)
  print(LOG_PREFIX + 'the register_vehicle_manifest_wrapper done')



def Test_get_case():
    #Get case  
    config_path = '/home/songyu/workplace/cs-s32g-gw/uptane_test/'
    with open(config_path + 'config.txt') as fd:
        ctr_case = fd.readline()
        fd.close()
    return ctr_case

def copymetadata(srcfile,dstfile):
    if not os.path.isfile(srcfile):
        print("%s not exist!"% (srcfile))
    else:
        fpath,fname=os.path.split(dstfile)    
        if not os.path.exists(fpath):
            os.makedirs(fpath)               
        shutil.copyfile(srcfile,dstfile)   
        print("copy %s -> %s"%( srcfile,dstfile))


      
def listen():
  """
  Listens on DIRECTOR_SERVER_PORT for xml-rpc calls to functions:
    - submit_vehicle_manifest
    - register_ecu_serial

  Note that you must also run host() in order to serve the metadata files via
  http.
  """

  global director_service_thread

  if director_service_thread is not None:
    print(LOG_PREFIX + 'Sorry: there is already a Director service thread '
        'listening.')
    return

  # Create server
  server = xmlrpc_server.SimpleXMLRPCServer(
      (demo.DIRECTOR_SERVER_HOST, demo.DIRECTOR_SERVER_PORT),
      requestHandler=RequestHandler, allow_none=True)



  # Register function that can be called via XML-RPC, allowing a Primary to
  # submit a vehicle version manifest.
  server.register_function(
      #director_service_instance.register_vehicle_manifest,
      register_vehicle_manifest_wrapper, # due to XMLRPC.Binary() for DER
      'submit_vehicle_manifest')                                           

  server.register_function(
      director_service_instance.register_ecu_serial, 'register_ecu_serial')


  # Interface available for the demo website frontend.
  server.register_function(
      director_service_instance.add_new_vehicle, 'add_new_vehicle')
  # Have decided that a function to add an ecu is unnecessary.
  # Just add targets for it. It'll be registered when that ecu registers itself.
  # Eventually, we'll want there to be an add ecu function here that takes
  # an ECU's public key, but that's not reasonable right now.

  # Provide absolute path for this, or path relative to the Director's repo
  # directory.
  server.register_function(add_target_to_director, 'add_target_to_director')
  server.register_function(write_to_live, 'write_director_repo')

 
  server.register_function(
      inventory.get_last_vehicle_manifest, 'get_last_vehicle_manifest')
      
  server.register_function(
      inventory.get_last_ecu_manifest, 'get_last_ecu_manifest')
  '''
  server.register_function(
      sql_db.get_last_vehicle_manifest, 'get_last_vehicle_manifest')
      
  server.register_function(
      sql_db.get_last_ecu_manifest, 'get_last_ecu_manifest')
  '''

  server.register_function(
      director_service_instance.register_ecu_serial, 'register_ecu_serial')

  server.register_function(clear_vehicle_targets, 'clear_vehicle_targets')


  print(LOG_PREFIX + 'Starting Director Services Thread: will now listen on '
      'port ' + str(demo.DIRECTOR_SERVER_PORT))
    #for Test

  director_service_thread = threading.Thread(target=server.serve_forever)
  director_service_thread.setDaemon(True)
  director_service_thread.start()






"""
Simulating a replay attack can be done with instructions in README.md,
using the functions below.
"""

def backup_timestamp(vin):
  """
  Copy timestamp.der to backup_timestamp.der

  Example:
  >>> import demo.demo_director as dd
  >>> dd.clean_slate()
  >>> dd.backup_timestamp('111')
  """

  timestamp_filename = 'timestamp.' + tuf.conf.METADATA_FORMAT
  timestamp_path = os.path.join(demo.DIRECTOR_REPO_DIR, vin, 'metadata',
      timestamp_filename)

  backup_timestamp_path = os.path.join(demo.DIRECTOR_REPO_DIR, vin,
      'backup_' + timestamp_filename)

  shutil.copyfile(timestamp_path, backup_timestamp_path)





def replay_timestamp(vin):
  """
  Move 'backup_timestamp.der' to 'timestamp.der', effectively rolling back
  timestamp to a previous version.  'backup_timestamp.der' must already exist
  at the expected path (can be created via backup_timestamp(vin)).
  Prior to rolling back timestamp.der, the current timestamp is saved to
  'current_timestamp.der'.

  Example:
  >>> import demo.demo_director as dd
  >>> dd.clean_slate()
  >>> dd.backup_timestamp('111')
  >>> dd.replay_timestamp()
  """

  timestamp_filename = 'timestamp.' + tuf.conf.METADATA_FORMAT
  backup_timestamp_path = os.path.join(demo.DIRECTOR_REPO_DIR, vin,
      'backup_' + timestamp_filename)

  if not os.path.exists(backup_timestamp_path):
    raise Exception('Cannot replay the Timestamp'
        ' file.  ' + repr(backup_timestamp_path) + ' must already exist.'
        '  It can be created by calling backup_timestamp(vin).')

  else:
    timestamp_path = os.path.join(demo.DIRECTOR_REPO_DIR, vin, 'metadata',
        timestamp_filename)
    current_timestamp_backup = os.path.join(demo.DIRECTOR_REPO_DIR, vin,
        'current_' + timestamp_filename)

    # First backup the current timestamp.
    shutil.move(timestamp_path, current_timestamp_backup)
    shutil.move(backup_timestamp_path, timestamp_path)






def restore_timestamp(vin):
  """
  # restore timestamp.der (first move current_timestamp.der to timestamp.der).

  Example:
  >>> import demo.demo_director as dd
  >>> dd.clean_slate()
  >>> dd.backup_timestamp('111')
  >>> dd.replay_timestamp()
  >>> dd.restore_timestamp()
  """

  timestamp_filename = 'timestamp.' + tuf.conf.METADATA_FORMAT
  current_timestamp_backup = os.path.join(demo.DIRECTOR_REPO_DIR, vin,
      'current_' + timestamp_filename)

  if not os.path.exists(current_timestamp_backup):
    raise Exception('A backup copy of the timestamp file'
        ' could not be found.  Missing: ' + repr(current_timestamp_backup))

  else:
    timestamp_path = os.path.join(demo.DIRECTOR_REPO_DIR, vin, 'metadata',
        timestamp_filename)
    shutil.move(current_timestamp_backup, timestamp_path)








def undo_keyed_arbitrary_package_attack(vin, ecu_serial, target_filepath):
  """
  Recover from keyed_arbitrary_package_attack.

  1. Revoke existing timestamp, snapshot, and targets keys, and issue new
     keys to replace them. This uses the root key for the Director, which
     should be an offline key.
  2. Replace the malicious target the attacker added with a clean version of
     the target, as it was before the attack.

  This attack recovery is described in README.md, section 3.6.
  """

  print(LOG_PREFIX + 'UNDO ATTACK: keyed arbitrary package attack with '
      'parameters: vin ' + repr(vin) + '; ecu_serial ' + repr(ecu_serial) +
      '; target_filepath ' + repr(target_filepath))

  # Revoke potentially compromised keys, replacing them with new keys.
  revoke_compromised_keys()

  # Replace malicious target with original.
  add_target_and_write_to_live(filename=target_filepath,
      file_content='Fresh firmware image', vin=vin, ecu_serial=ecu_serial)

  print(LOG_PREFIX + 'COMPLETED UNDO ATTACK')





def clear_vehicle_targets(vin):
  """
  Remove all instructions to the given vehicle from the current Director
  metadata.

  This does not execute write_to_live. After changes are complete, you should
  call that to write new metadata.

  This can be called to clear an existing instruction for an ECU so that a new
  instruction for different firmware can be given to that ECU.

  TODO: In the future, adding a target assignment to the Director for a given
  ECU should replace any other target assignment for that ECU.
  """
  print(LOG_PREFIX + 'CLEARING VEHICLE TARGETS for VIN ' + repr(vin))
  director_service_instance.vehicle_repositories[vin].targets.clear_targets()





def add_target_and_write_to_live(filename, file_content, vin, ecu_serial):
  """
  High-level version of add_target_to_director() that creates 'filename'
  and writes the changes to the live directory repository.
  """

  # Create 'filename' in the current working directory, but it should
  # ideally be to a temporary destination.  The demo code will eventually
  # be modified to use temporary directories (which will cleaned up after
  # running the demo code).
  with open(filename, 'w') as file_object:
    file_object.write(file_content)

  # The path that will identify the file in the repository.
  filepath_in_repo = filename

  add_target_to_director(filename, filepath_in_repo, vin, ecu_serial)
  write_to_live(vin_to_update=vin)





def kill_server():
  """
  Kills the forked process that is hosting the Director repositories via
  Python's simple HTTP server. This does not affect the Director service
  (which handles manifests and responds to requests from Primaries), nor does
  it affect the metadata in the repositories or the state of the repositories
  at all. host() can be run afterwards to begin hosting again.
  """

  global repo_server_process

  if repo_server_process is None:
    print(LOG_PREFIX + 'No repository hosting process to stop.')
    return

  else:
    print(LOG_PREFIX + 'Killing repository hosting process with pid: ' +
        str(repo_server_process.pid))
    
    
    #sql_db.clean_all_table()
    repo_server_process.kill()
    repo_server_process = None
