"""
start_servers.py

<Purpose>
  A simple script to start the three cloud-side Uptane servers:
    the Director (including its per-vehicle repositories)
    the Image Repository
    the Timeserver

  To run the demo services, run the following from the main uptane
  directory (which contains, for example, setup.py).
    python -i demo/start_servers.py

  That starts the services in an interactive mode (with a prompt from which
  you can manipulate them for the demonstrations).

"""
from __future__ import print_function
from __future__ import unicode_literals

import uptane # Import before TUF modules; may change tuf.conf values.
import tuf.formats
import tuf.keys
import tuf.repository_tool as repo_tool
import uptane.formats
import uptane.common

import uptane.encoding.asn1_codec as asn1_codec
import uptane.encoding.timeserver_asn1_coder as timeserver_asn1_coder
import uptane.encoding.ecu_manifest_asn1_coder as ecu_manifest_asn1_coder
import uptane.encoding.asn1_definitions as asn1_spec
import pyasn1.codec.der.encoder as p_der_encoder
import pyasn1.codec.der.decoder as p_der_decoder
import pyasn1.error
from pyasn1.type import tag, univ

import sys # to test Python version 2 vs 3, for byte string behavior
import hashlib
import os
import time
import copy
import shutil
import director_metedata

from uptane.encoding.asn1_codec import DATATYPE_TIME_ATTESTATION
from uptane.encoding.asn1_codec import DATATYPE_ECU_MANIFEST
from uptane.encoding.asn1_codec import DATATYPE_VEHICLE_MANIFEST



def main():
    uptane.formats.TIMESERVER_ATTESTATION_SCHEMA.check_match(
        director_metedata.timestamp_MATADATA)
    conversion_tester(
        director_metedata.timestamp_MATADATA, DATATYPE_TIME_ATTESTATION)

  

def conversion_tester(signable_pydict, datatype): # cls: clunky
  """
  Tests each of the different kinds of conversions into ASN.1/DER, and tests
  converting back. In one type of conversion, compares to make sure the data
  has not changed.

  This function takes as a third parameter the unittest.TestCase object whose
  functions (assertTrue etc) it can use. This is awkward and inappropriate. :P
  Find a different means of providing modularity instead of this one.
  (Can't just have this method in the class above because it would be run as
  a test. Could have default parameters and do that, but that's clunky, too.)
  Does unittest allow/test private functions in UnitTest classes?
  """


  # Test type 1: only-signed
  # Convert and return only the 'signed' portion, the metadata payload itself,
  # without including any signatures.
  signed_der = asn1_codec.convert_signed_metadata_to_der(
      signable_pydict, datatype, only_signed=True)

  is_valid_nonempty_der(signed_der)

  # TODO: Add function to asn1_codec that will convert signed-only DER back to
  # Python dictionary. Might be useful, and is useful for testing only_signed
  # in any case.


  # Test type 2: full conversion
  # Convert the full signable ('signed' and 'signatures'), maintaining the
  # existing signature in a new format and encoding.
  signable_der = asn1_codec.convert_signed_metadata_to_der(
      signable_pydict, datatype)
  is_valid_nonempty_der(signable_der)

  # Convert it back.
  signable_reverted = asn1_codec.convert_signed_der_to_dersigned_json(
      signable_der, datatype)

  # Ensure the original is equal to what is converted back.



  # Test type 3: full conversion with re-signing
  # Convert the full signable ('signed' and 'signatures'), but discarding the
  # original signatures and re-signing over, instead, the hash of the converted,
  # ASN.1/DER 'signed' element.
  resigned_der = asn1_codec.convert_signed_metadata_to_der(
      signable_pydict, datatype, resign=True, private_key=test_signing_key)
  is_valid_nonempty_der(resigned_der)

  # Convert the re-signed DER manifest back in order to split it up.
  resigned_reverted = asn1_codec.convert_signed_der_to_dersigned_json(
      resigned_der, datatype)
  resigned_signature = resigned_reverted['signatures'][0]

  # Check the signature on the re-signed DER manifest:
  uptane.common.verify_signature_over_metadata(
      test_signing_key,
      resigned_signature,
      resigned_reverted['signed'],
      datatype,
      metadata_format='der')

  # The signatures will not match, because a new signature was made, but the
  # 'signed' elements should match when converted back.
  #cls.assertEqual(
  #    signable_pydict['signed'], resigned_reverted['signed'])

if __name__ == '__main__':
  #readline.parse_and_bind('tab: complete')
  main()