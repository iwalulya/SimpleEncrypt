#!/usr/bin/env python
import base64
import string
import json

class index:
    def Decode(self, encoded):
        key="my secure key"
        result = '';
        decoded = base64.b64decode(encoded)
        i=0;
        for char in decoded:
            if ((i % len(key))-1) >= 0:
                keychar = key[(i % len(key))-1]
                charl = chr(ord(char) - ord(keychar));
            else:
                charl = chr(ord(char));

            result += charl
            i+=1
        return result;