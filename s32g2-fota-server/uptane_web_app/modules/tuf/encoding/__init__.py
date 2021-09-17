"""
<Program Name>
  __init__.py  (for tuf/encoding)

<Copyright>
  See LICENSE for licensing information.

<Purpose>
  Provide common functions and constants for ASN.1 encoding code.
"""

# Help with Python 3 compatibility, where the print statement is a function, an
# implicit relative import is invalid, and the '/' operator performs true
# division.  Example:  print 'hello world' raises a 'SyntaxError' exception.
from __future__ import print_function
from __future__ import unicode_literals

import tuf
import tuf.formats


def hex_from_octetstring(octetstring):
  """
  Convert a pyasn1 OctetString object into a hex string.
  Example return:   '4b394ae2'
  Raises Error() if an individual octet's supposed integer value is out of
  range (0 <= x <= 255).
  """
  octets = octetstring.asNumbers()
  hex_string = ''

  for x in octets:
    if x < 0 or x > 255:
      raise tuf.Error('Unable to generate hex string from OctetString: integer '
          'value of octet provided is not in range: ' + str(x))
    hex_string += '%.2x' % x

  # Make sure that the resulting value is a valid hex string.
  tuf.formats.HEX_SCHEMA.check_match(hex_string)
  if '\\x' in str(hex_string):
    print(hex_string)
    import pdb; pdb.set_trace()
    print()

  return hex_string
