"""
__init__.py for the Uptane demo package
"""
from __future__ import unicode_literals
import os
import tuf
import tuf.keys
import json

from six.moves import getcwd
from six.moves import range

# FOR FOTA Project
FOTA_S32G_SWITCH = False

# Values to plug in below as needed.
LOCAL = 'localhost'
HOSTING = '0.0.0.0'
#tuf.conf.METADATA_FORMAT = 'json'
METADATA_EXTENSION = '.der' 
#METADATA_EXTENSION = '.json'
WORKING_DIR = getcwd()
DEMO_DIR = os.path.join(WORKING_DIR, 'demo')
DEMO_KEYS_DIR = '/home/gccs-cq2/workplace/cs-s32g-gw/uptane_web_app/applications/UPTANE/modules/demo/keys'


IMAGE_REPO_HOST = HOSTING
IMAGE_REPO_PORT = 30301
IMAGE_REPO_NAME = 'imagerepo'


DIRECTOR_REPO_HOST = HOSTING
DIRECTOR_REPO_PORT = 30401
DIRECTOR_REPO_NAME = 'director'

DIRECTOR_SERVER_HOST = HOSTING
DIRECTOR_SERVER_PORT = 30501

# These two are are being added solely to provide an interface to the demo web
# frontend.
IMAGE_REPO_SERVICE_HOST = HOSTING
IMAGE_REPO_SERVICE_PORT = 30309

TIMESERVER_HOST = HOSTING
TIMESERVER_PORT = 30601

PRIMARY_SERVER_HOST = HOSTING
PRIMARY_SERVER_DEFAULT_PORT = 30701
PRIMARY_SERVER_AVAILABLE_PORTS = [
    30701, 30702, 30703, 30704, 30705, 30706, 30707, 30708, 30709, 30710, 30711]




def import_public_key(keyname):
  """
  Import a public key according to the demo's current default key config.
  The keyname does not include '.pub'; it matches that used for the other
  functions here.

    Key type: ed25519
    Key location: DEMO_KEYS_DIR
  """
  filepath = os.path.join(DEMO_KEYS_DIR, keyname + '.pub')
  print('\n\nOPEN KEY:' + filepath)

  fileobject = open(filepath)

  try:
    deserialized_object = json.load(fileobject)
  
  except (ValueError, TypeError):
    raise tuf.Error('Cannot deserialize to a Python object: ' + repr(filepath))
  
  fileobject.close() 

  ed25519_key, junk = tuf.keys.format_metadata_to_key(deserialized_object)

  # Raise an exception if an unexpected key type is imported.
  # Redundant validation of 'keytype'.  'tuf.keys.format_metadata_to_key()'
  # should have fully validated 'ed25519_key_metadata'.
  if ed25519_key['keytype'] != 'ed25519': # pragma: no cover
    message = 'Invalid key type loaded: ' + repr(ed25519_key['keytype'])
    raise tuf.FormatError(message)

  return ed25519_key
