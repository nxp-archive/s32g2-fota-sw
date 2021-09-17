ROOT_MATADATA={
    "signed": {
        "type": "root",
        "expires": "2037-09-28T12:46:29Z",
        "version": 1,
        "body": {
            "numberOfKeys": 4,
            "keys": [
                {
                    "publicKeyid": "630cf584f392430b2119a4395e39624e86f5e5c5374507a789be5cf35bf090d6",
                    "publicKeyType": "ed25519",
                    "publicKeyValue": "99ef8790687ca252c4677a80a34e401efb7e17ccdf9b0fcb5f1bc3260c432cb9"
                },
                {
                    "publicKeyid": "da9c65c96c5c4072f6984f7aa81216d776aca6664d49cb4dfafbc7119320d9cc",
                    "publicKeyType": "ed25519",
                    "publicKeyValue": "d1ab5126fd6f0e30944910e81c0448044dfe9d5a39f478212b2afa913bb7ca7c"
                },
                {
                    "publicKeyid": "da9c65c96c5c4072f6984f7aa81216d776aca6664d49cb4dfafbc7119320d9cc",
                    "publicKeyType": "ed25519",
                    "publicKeyValue": "d1ab5126fd6f0e30944910e81c0448044dfe9d5a39f478212b2afa913bb7ca7c"
                },
                {
                    "publicKeyid": "da9c65c96c5c4072f6984f7aa81216d776aca6664d49cb4dfafbc7119320d9cc",
                    "publicKeyType": "ed25519",
                    "publicKeyValue": "d1ab5126fd6f0e30944910e81c0448044dfe9d5a39f478212b2afa913bb7ca7c"
                },
            ],
            
            "numberOfRoles":4,
            "roles": [
                {
                    "role": "root",
                    "numberOfURLs": 0,
                    "urls": [],
                    "numberOfKeyids": 1,
                    "keyids": ["630cf584f392430b2119a4395e39624e86f5e5c5374507a789be5cf35bf090d6"],
                    "threshold": 1
                },
                {
                    "role": "snapshot",
                    "numberOfURLs": 0,
                    "urls": [],
                    "numberOfKeyids": 1,
                    "keyids": ["630cf584f392430b2119a4395e39624e86f5e5c5374507a789be5cf35bf090d6"],
                    "threshold": 1
                },
                {
                    "role": "targets",
                    "numberOfURLs": 0,
                    "urls": [],
                    "numberOfKeyids": 1,
                    "keyids": ["630cf584f392430b2119a4395e39624e86f5e5c5374507a789be5cf35bf090d6"],
                    "threshold": 1
                },
                {
                    "role": "timestamp",
                    "numberOfURLs": 0,
                    "urls": [],
                    "numberOfKeyids": 1,
                    "keyids": ["630cf584f392430b2119a4395e39624e86f5e5c5374507a789be5cf35bf090d6"],
                    "threshold": 1
                }
            ]
        }
    },
 
    "numberOfSignatures": 1,
    "signatures": [
    {
        "keyid": "fdba7eaa358fa5a8113a789f60c4a6ce29c4478d8d8eff3e27d1d77416696ab2",
        "method": "ed25519",
        "hash": {
            "function": "sha512",
            "digest": "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx..."
        },
        "value": "e9855e5171934d56a78033cead3dc217d6df3730f9c668742346a4e66f0d1141fe7283a21964e0c35163e76b6103e36a04d44f1b0799fe34af45c65f32f38b09"
    }
    ]
}


Target_MATADATA={
    "signed": {
        "type": "timestamp",
        "expires": "2037-09-28T12:46:29Z",
        "version": 2,
        "body": {
            "filename": "snapshot.json", 
            "version": 2,
            "length": 1024,
            "numberOfHashes": 1,
            "hashes": [
            {
                "function": "sha-256",
                "digest": "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
            }            
            ]
        }
    },
    
    "numberOfSignatures": 1,
    "signatures": [
    {
        "keyid": "da9c65c96c5c4072f6984f7aa81216d776aca6664d49cb4dfafbc7119320d9cc",
        "method": "ed25519",
        "hash": {
            "function": "sha512",
            "digest": "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx..."
        },
        "value": "3500dd4af7002c2a7d36c1a4b546b87979805a5c31f6038b39ca19d5d61868b5d3cc8c96c93eb44c3ed862859f45739e926626b73b7beb2d37ceeb83b32c8b0c"
    }
    ]
}

